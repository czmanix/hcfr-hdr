# AGENTS.md

Guidance for coding agents working in this repository (`czmanix/hcfr-hdr`).

## What this is

HCFR Colorimeter: a Windows MFC (C++) application for display calibration measurements (colour
meters via ArgyllCMS, test-pattern generators, grayscale/gamut/HDR analysis). This repo is a fork of
`arthur-liberman/hcfr-code` (3.5.4.4), which continues the abandoned SourceForge project. User-facing
text and the code are in English.

## Build

Visual Studio 2022 (Community or Build Tools) with the MFC component (`v143` toolset, latest Windows
SDK). Solution: `ColorHCFR.sln`.

```powershell
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=x64 /m
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=Win32 /m
```

- Outputs: x64 -> `x64\Release\ColorHCFR.exe`, Win32 -> `Release\ColorHCFR.exe` (both git-ignored).
  The `ColorHCFR` target also builds the libraries and the `CHCFR21_*` resource DLLs.
- Win32 runtime needs `devlib\CHCFRDI*.dll` copied next to the exe (closed-source 32-bit sensor
  drivers). x64 does not import them.
- A running `ColorHCFR.exe` locks the output; stop it before rebuilding.
- For symbols when profiling Release: set env `_CL_=/Zi` and `_LINK_=/DEBUG` before msbuild
  (do not pass a fixed `/PDB:` – the resource DLL projects would share it).

### Installer (Inno Setup 6)

Build the Argyll helper tools (`oeminst` and `spotread`) for the target platform, then compile
`Install\hcfr.iss` with Inno Setup 6 (`ISCC.exe`):

```powershell
msbuild ColorHCFR.sln /t:oeminst;spotread /p:Configuration=Release /p:Platform=x64 /m
iscc /DPlatform=x64 Install\hcfr.iss
```

- Outputs: `x64\Release\HCFRSetup-x64.exe` (default) or `Release\HCFRSetup-Win32.exe` (`/DPlatform=Win32`).
- `ColorHCFR_VS2019.vdproj` is legacy and skipped by msbuild.

## Tests

`UnitTests` (cppunit, references `libHCFR`, `ArgyllMeterWrapper`, `spectro`) is in the solution but
not part of the `ColorHCFR` target and has not been built/verified in this fork:

```powershell
msbuild ColorHCFR.sln /t:UnitTests /p:Configuration=Release /p:Platform=Win32
UnitTests.exe [cppunit test path, e.g. a suite name]   # empty = all suites; writes tests.xml
```

Otherwise verification is done by building both `Release|x64` and `Release|Win32` and running the
app: `ColorHCFR.exe file.chc` opens a document. Opening a `.chc` whose saved preferences differ from
the active ones shows a Yes/No prompt. Preferences live in `ColorHCFR.ini` next to the exe
(`GetConfig()->GetProfileInt/WriteProfileInt`), so Win32 and x64 builds have separate ini files.

## Source encoding (important)

Most existing `.cpp/.h/.rc` files are **Windows-1252 with CRLF**; UTF-8 editors/tools may refuse them.
Modify them with a script that reads/writes bytes as code page 1252 and preserves CRLF. Plain ASCII
files (e.g. `Tools\spectro`, `Tools\ArgyllMeterWrapper`, `Install\hcfr.iss`, `README.md`) can be
edited normally. Keep new files ASCII. `.vcxproj` files are UTF-8 with BOM.

## Architecture

- **MFC doc/view**: `CDataSetDoc` (`datasetdoc.cpp`) owns one sensor (`CSensor`), one generator
  (`CGenerator`) and the measurements (`CMeasure`, `Measure.cpp`). `CMainView` (`Views\MainView.cpp`)
  is the measures grid plus the "Information" pane hosting graph views (`Views\*HistoView`,
  `CIEChartView`); other graphs open as tabs in `CMultiFrame`. `CMainView::OnSize` uses a
  lightweight `ResizeGrid()` under `SetRedraw(FALSE/TRUE)` (do not call `InitGrid`/`UpdateGrid` on
  resize). `CCIEChartView::OnSize` debounces `MakeBgBitmap` (50 ms) with a fast `StretchBlt` preview.
- **Sensors** (`Sensors\`): `CArgyllSensor` is the one that matters; it wraps
  `Tools\ArgyllMeterWrapper` over the bundled ArgyllCMS instrument code in `Tools\spectro`
  (Argyll 2.1.1 + later additions such as SpyderX2). Meter detection (`getDetectedMeters`) runs when a
  document with an Argyll sensor is opened; Bluetooth COM ports are skipped unless
  `[Argyll] ScanBluetoothPorts=1`. The sensor's Reading Type (Display/Telephoto/Ambient) is stored in
  the document. Other sensors are legacy; `devlib\CHCFRDI1-4` get x64 stubs reporting "DLL not available".
- **Generators & patterns** (`Generators\`, `PatternDisplay.cpp`): `CGDIGenerator` with display modes
  (`DISPLAY_GDI`, `DISPLAY_GDI_nBG`, `DISPLAY_GDI_Hide`, `DISPLAY_madVR`, `DISPLAY_ccast`,
  `DISPLAY_rPI`, ... in `FullScreenWindow.h`). Patterns are drawn by `CFullScreenWindow::OnPaint`.
  - With HDR10 enabled in GDI modes, `CFullScreenWindow::PaintHdr10` draws measurement patterns
    through `CD3D11HdrOutput` (10-bit `R10G10B10A2` swap chain, `G2084_NONE_P2020`, requires Windows
    HDR on); test images/animations stay in GDI. `libHDR` (D3D12/NvAPI) is only used when HDR10 is off.
  - Resizable test pattern images scale to fill the display while preserving aspect ratio using
    `HALFTONE`. Fullscreen pattern handlers pass `Init(0, TRUE)` so they show even in `DISPLAY_GDI_Hide`.
  - `CPatternDisplay` (*Additional Patterns*) uses `CGDIGenerator(int, BOOL)` which must **not** call
    `Init()` in its constructor; `BeginPatternDisplay`/`EndPatternDisplay` hide and restore the dialog
    around each fullscreen pattern.
  - Raspberry Pi/PGenerator needs `Tools\pi\RB8PGenerator.dll` (32-bit only).
- **Colour values & HDR reference white**: `ColorRGBDisplay` (`libHCFR\Color.h`) holds percent 0–100
  as doubles; in HDR mode these are PQ signal levels. `GetColorRef()` reduces them to 8 bits (GDI only).
  HDR colour patterns reference white (`HDR_REF_WHITE_DEFAULT` = 94.37844 nits; e.g. 203 nits for
  ITU-R BT.2408) is configured in *Preferences → References* (`ReferencesPropPage.cpp`, saved as
  `[References] HDRRefWhite` and in `.chc` v18 when non-default) and is distinct from `m_DiffuseL`
  (BT.2390 tone-mapping curve scaling).
- **Tooltips**: `CPPToolTip` (`Tools\CppTooltip\PPTooltip.cpp`) defaults to instant rectangular light
  tooltips (`SHOWEFFECT_NONE`, `CS_DROPSHADOW`, direct `BitBlt` fast path without screen capture).
- **Preferences**: `CColorHCFRConfig` (`ColorHCFRConfig.cpp`). HDR/PQ mode is `GammaOffsetType == 5`
  (ST 2084); colour standards are the `ColorStandard` enum (`UHDTV2` = BT.2020, `UHDTV3` = P3 in BT.2020).
- **Resources/UI text**: menus, accelerators, dialogs and strings live in each language DLL
  (`CHCFR21_ENGLISH/DEUTSCH/FRANCAIS/ITALIAN/ESPANOL.rc`); a new control/command needs an ID in
  `resource.h` and entries in all five `.rc` files. Pattern images are in `CHCFR21_PATTERNS`.

## Rules that keep Win32 and x64 compatible

- The `.chc` format is `CArchive` serialization and must stay byte-identical between platforms:
  write counts as `(int)` (not `GetSize()`/`GetCount()`, which are `INT_PTR`) and `time_t` as `LONG`.
- Pointers carried in ints must be `INT_PTR`/`DWORD_PTR` (e.g. the sensor ID carries an
  `ArgyllMeterWrapper*`); message handlers use `OnTimer(UINT_PTR)` and `LRESULT (WPARAM, LPARAM)`.
- Win32 keeps `_USE_32BIT_TIME_T`; it is removed only in the x64 configurations.
