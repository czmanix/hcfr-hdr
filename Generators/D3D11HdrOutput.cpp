/////////////////////////////////////////////////////////////////////////////
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.
/////////////////////////////////////////////////////////////////////////////
//
// D3D11HdrOutput.cpp : HDR10 pattern output through a Direct3D 11 swap chain
//

#include "stdafx.h"
#include "D3D11HdrOutput.h"

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <stdio.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

template <class T> static void SafeRelease(T*& p)
{
	if (p) { p->Release(); p = NULL; }
}

static std::string HrText(const char* what, HRESULT hr)
{
	char buf[128];
	sprintf_s(buf, "%s failed (HRESULT 0x%08X)", what, (unsigned)hr);
	return buf;
}

// Finds the DXGI adapter and output showing hWnd. Caller releases both.
static bool FindOutputForWindow(HWND hWnd, IDXGIAdapter1** ppAdapter, IDXGIOutput6** ppOutput)
{
	*ppAdapter = NULL;
	*ppOutput = NULL;
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	IDXGIFactory1* pFactory = NULL;
	if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
		return false;

	IDXGIAdapter1* pAdapter = NULL;
	for (UINT a = 0; !*ppOutput && pFactory->EnumAdapters1(a, &pAdapter) != DXGI_ERROR_NOT_FOUND; a++)
	{
		IDXGIOutput* pOutput = NULL;
		for (UINT o = 0; pAdapter->EnumOutputs(o, &pOutput) != DXGI_ERROR_NOT_FOUND; o++)
		{
			DXGI_OUTPUT_DESC desc;
			if (SUCCEEDED(pOutput->GetDesc(&desc)) && desc.Monitor == hMon)
			{
				IDXGIOutput6* pOutput6 = NULL;
				if (SUCCEEDED(pOutput->QueryInterface(__uuidof(IDXGIOutput6), (void**)&pOutput6)))
				{
					*ppOutput = pOutput6;
					*ppAdapter = pAdapter;
					pAdapter->AddRef();
				}
			}
			SafeRelease(pOutput);
			if (*ppOutput)
				break;
		}
		SafeRelease(pAdapter);
	}
	SafeRelease(pFactory);
	return *ppOutput != NULL;
}

CD3D11HdrOutput::CD3D11HdrOutput()
	: m_hWnd(NULL), m_hMonitor(NULL), m_Width(0), m_Height(0), m_pDevice(NULL), m_pContext(NULL), m_pContext1(NULL),
	  m_pSwapChain(NULL), m_pRtv(NULL), m_bHasMetadata(false)
{
	ZeroMemory(&m_Metadata, sizeof(m_Metadata));
}

CD3D11HdrOutput::~CD3D11HdrOutput()
{
	Release();
}

bool CD3D11HdrOutput::IsHdrActiveForWindow(HWND hWnd)
{
	IDXGIAdapter1* pAdapter = NULL;
	IDXGIOutput6* pOutput = NULL;
	bool bHdr = false;
	if (FindOutputForWindow(hWnd, &pAdapter, &pOutput))
	{
		DXGI_OUTPUT_DESC1 desc1;
		if (SUCCEEDED(pOutput->GetDesc1(&desc1)))
			bHdr = desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
	}
	SafeRelease(pOutput);
	SafeRelease(pAdapter);
	return bHdr;
}

bool CD3D11HdrOutput::Ensure(HWND hWnd, std::string& error)
{
	RECT rc;
	::GetClientRect(hWnd, &rc);
	int w = rc.right - rc.left, h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0)
	{
		error = "Pattern window has no size";
		return false;
	}
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (IsActive() && hWnd == m_hWnd && w == m_Width && h == m_Height && hMon == m_hMonitor)
		return true;

	Release();

	// Check first: a flip-model swap chain that could not be set to HDR10 would still take
	// the window over and break the GDI drawing used as fallback.
	IDXGIAdapter1* pAdapter = NULL;
	IDXGIOutput6* pOutput = NULL;
	if (!FindOutputForWindow(hWnd, &pAdapter, &pOutput))
	{
		error = "Cannot find the display showing the pattern window (DXGI 1.6 required)";
		return false;
	}
	DXGI_OUTPUT_DESC1 desc1;
	HRESULT hr = pOutput->GetDesc1(&desc1);
	SafeRelease(pOutput);
	if (FAILED(hr) || desc1.ColorSpace != DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
	{
		SafeRelease(pAdapter);
		error = "Windows HDR is not enabled on the display showing the pattern window.\n"
				"Turn on \"Use HDR\" in Windows display settings (Win+Alt+B) and try again.";
		return false;
	}

	IDXGIFactory2* pFactory = NULL;
	IDXGISwapChain1* pSwapChain1 = NULL;
	ID3D11Texture2D* pBackBuffer = NULL;
	UINT support = 0;

	hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
						   &m_pDevice, NULL, &m_pContext);
	if (FAILED(hr)) { error = HrText("D3D11CreateDevice", hr); goto fail; }

	hr = m_pContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&m_pContext1);
	if (FAILED(hr)) { error = HrText("ID3D11DeviceContext1", hr); goto fail; }

	hr = pAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pFactory);
	if (FAILED(hr)) { error = HrText("IDXGIFactory2", hr); goto fail; }

	{
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = w;
		desc.Height = h;
		desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		hr = pFactory->CreateSwapChainForHwnd(m_pDevice, hWnd, &desc, NULL, NULL, &pSwapChain1);
	}
	if (FAILED(hr)) { error = HrText("CreateSwapChainForHwnd", hr); goto fail; }
	pFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

	hr = pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_pSwapChain);
	if (FAILED(hr)) { error = HrText("IDXGISwapChain3", hr); goto fail; }

	hr = m_pSwapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &support);
	if (FAILED(hr) || !(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
	{
		error = "The graphics driver does not support HDR10 (BT.2020/PQ) output for this display";
		goto fail;
	}
	hr = m_pSwapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
	if (FAILED(hr)) { error = HrText("SetColorSpace1", hr); goto fail; }

	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr)) { error = HrText("GetBuffer", hr); goto fail; }
	hr = m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pRtv);
	if (FAILED(hr)) { error = HrText("CreateRenderTargetView", hr); goto fail; }

	SafeRelease(pBackBuffer);
	SafeRelease(pSwapChain1);
	SafeRelease(pFactory);
	SafeRelease(pAdapter);
	m_hWnd = hWnd;
	m_hMonitor = hMon;
	m_Width = w;
	m_Height = h;
	ApplyMetadata();
	return true;

fail:
	SafeRelease(pBackBuffer);
	SafeRelease(pSwapChain1);
	SafeRelease(pFactory);
	SafeRelease(pAdapter);
	Release();
	return false;
}

void CD3D11HdrOutput::SetMetadata(const HdrOutputMetadata* pMetadata)
{
	m_bHasMetadata = pMetadata != NULL;
	if (pMetadata)
		m_Metadata = *pMetadata;
	ApplyMetadata();
}

void CD3D11HdrOutput::ApplyMetadata()
{
	if (!m_pSwapChain)
		return;
	IDXGISwapChain4* pSwapChain4 = NULL;
	if (SUCCEEDED(m_pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&pSwapChain4)))
	{
		static_assert(sizeof(HdrOutputMetadata) == sizeof(DXGI_HDR_METADATA_HDR10), "metadata layout");
		if (m_bHasMetadata)
			pSwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(m_Metadata), &m_Metadata);
		else
			pSwapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, NULL);
		SafeRelease(pSwapChain4);
	}
}

static float Clamp01(double v)
{
	return (float)(v < 0. ? 0. : v > 1. ? 1. : v);
}

void CD3D11HdrOutput::Clear(double r, double g, double b)
{
	if (!m_pRtv)
		return;
	const float color[4] = { Clamp01(r), Clamp01(g), Clamp01(b), 1.f };
	m_pContext->ClearRenderTargetView(m_pRtv, color);
}

void CD3D11HdrOutput::Fill(const RECT& rc, double r, double g, double b)
{
	if (!m_pRtv || rc.right <= rc.left || rc.bottom <= rc.top)
		return;
	const float color[4] = { Clamp01(r), Clamp01(g), Clamp01(b), 1.f };
	D3D11_RECT d3dRect = rc;
	m_pContext1->ClearView(m_pRtv, color, &d3dRect, 1);
}

void CD3D11HdrOutput::Present()
{
	if (!m_pSwapChain)
		return;
	HRESULT hr = m_pSwapChain->Present(1, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		Release();	// re-created on next Ensure()
}

void CD3D11HdrOutput::Release()
{
	if (m_pContext)
	{
		m_pContext->ClearState();
		m_pContext->Flush();
	}
	SafeRelease(m_pRtv);
	SafeRelease(m_pSwapChain);
	SafeRelease(m_pContext1);
	SafeRelease(m_pContext);
	SafeRelease(m_pDevice);
	m_hWnd = NULL;
	m_hMonitor = NULL;
	m_Width = m_Height = 0;
}
