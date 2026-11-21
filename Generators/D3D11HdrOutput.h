/////////////////////////////////////////////////////////////////////////////
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.
/////////////////////////////////////////////////////////////////////////////
//
// D3D11HdrOutput.h : HDR10 pattern output through a Direct3D 11 swap chain
//
// Renders solid rectangles into a 10-bit R10G10B10A2 flip-model swap chain in the
// DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 colour space. Values 0..1 are PQ (ST 2084)
// code values in BT.2020, i.e. absolute luminance, passed to the display unchanged by
// Windows. Requires Windows HDR to be enabled on the monitor showing the window.
//
#pragma once

#include <windows.h>
#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11DeviceContext1;
struct ID3D11RenderTargetView;
struct IDXGISwapChain3;

// Same layout as DXGI_HDR_METADATA_HDR10 (chromaticities in 0.00002 units,
// mastering luminance in 0.0001 cd/m2, MaxCLL / MaxFALL in cd/m2)
struct HdrOutputMetadata
{
	UINT16	RedPrimary[2];
	UINT16	GreenPrimary[2];
	UINT16	BluePrimary[2];
	UINT16	WhitePoint[2];
	UINT	MaxMasteringLuminance;
	UINT	MinMasteringLuminance;
	UINT16	MaxContentLightLevel;
	UINT16	MaxFrameAverageLightLevel;
};

class CD3D11HdrOutput
{
public:
	CD3D11HdrOutput();
	~CD3D11HdrOutput();

	// Is Windows HDR (BT.2020/PQ output) active on the monitor showing hWnd?
	static bool IsHdrActiveForWindow(HWND hWnd);

	// Creates (or re-creates after a size change) the swap chain for hWnd.
	// Returns false with a message in error when HDR10 output is not possible;
	// in that case nothing stays attached to the window.
	bool Ensure(HWND hWnd, std::string& error);

	// HDR10 static metadata sent with the swap chain; NULL = none
	void SetMetadata(const HdrOutputMetadata* pMetadata);

	bool IsActive() const { return m_pSwapChain != NULL; }
	int Width() const { return m_Width; }
	int Height() const { return m_Height; }

	// Drawing: values are PQ code values 0..1 (clamped)
	void Clear(double r, double g, double b);
	void Fill(const RECT& rc, double r, double g, double b);
	void Present();

	void Release();

private:
	void ApplyMetadata();

	HWND					m_hWnd;
	HMONITOR				m_hMonitor;
	int						m_Width, m_Height;
	ID3D11Device *			m_pDevice;
	ID3D11DeviceContext *	m_pContext;
	ID3D11DeviceContext1 *	m_pContext1;
	IDXGISwapChain3 *		m_pSwapChain;
	ID3D11RenderTargetView *m_pRtv;
	bool					m_bHasMetadata;
	HdrOutputMetadata		m_Metadata;
};
