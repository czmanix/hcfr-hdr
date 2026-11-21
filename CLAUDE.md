# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

HCFR Colorimeter: a Windows MFC (C++) application for display calibration measurements (colour
meters via ArgyllCMS, test-pattern generators, grayscale/gamut/HDR analysis). This repo is a fork of
`arthur-liberman/hcfr-code` (3.5.4.4), which continues the abandoned SourceForge project (git remote
`sourceforge`). User-facing text and the code are in English.

## Build

Visual Studio 2022 with the MFC component (v143 toolset, latest Windows SDK). Solution: `ColorHCFR.sln`.

```
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=x64
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=Win32
```

- Outputs: x64 -> `x64\Release\ColorHCFR.exe`, Win32 -> `Release\ColorHCFR.exe` (both git-ignored).
  The `ColorHCFR` target also builds the libraries and the `CHCFR21_*` resource DLLs.
- Win32 runtime needs `devlib\CHCFRDI*.dll` copied next to the exe (closed-source 32-bit sensor
  drivers). x64 does not import them.
- A running `ColorHCFR.exe` locks the output; stop it before rebuilding.
- `ColorHCFR_VS2019.vdproj` (installer) needs the Installer Projects extension; msbuild skips it.
- For symbols when profiling Release: set env `_CL_=/Zi` and `_LINK_=/DEBUG` before msbuild
  (do not pass a fixed `/PDB:` – the resource DLL projects would share it).

## Tests

`UnitTests` (cppunit, references libHCFR, ArgyllMeterWrapper, spectro) is in the solution but not part
of the `ColorHCFR` target and has not been built/verified in this fork:

```
msbuild ColorHCFR.sln /t:UnitTests /p:Configuration=Release /p:Platform=Win32
UnitTests.exe [cppunit test path, e.g. a suite name]   # empty = all suites; writes tests.xml
```

Otherwise verification is done by running the app: `ColorHCFR.exe file.chc` opens a document.
Opening a `.chc` whose saved preferences differ from the active ones shows a Yes/No prompt.
Preferences live in `ColorHCFR.ini` next to the exe (`GetConfig()->GetProfileInt/WriteProfileInt`),
so the Win32 and x64 builds have separate ini files.

## Source encoding (important)

Most existing `.cpp/.h/.rc` files are **Windows-1252 with CRLF**; the Edit tool refuses them. Change
them with a script that reads/writes bytes as code page 1252 and keeps CRLF. Files that are plain
ASCII (e.g. Argyll code under `Tools\spectro`, `Tools\ArgyllMeterWrapper`) can be edited normally.
New files: keep them ASCII. `.vcxproj` files are UTF-8 with BOM.

## Architecture

- **MFC doc/view**: `CDataSetDoc` (`datasetdoc.cpp`) owns one sensor (`CSensor`), one generator
  (`CGenerator`) and the measurements (`CMeasure`, `Measure.cpp`). `CMainView` (`Views\MainView.cpp`)
  is the measures grid plus the "Information" pane hosting graph views (`Views\*HistoView`,
  `CIEChartView`); other graphs open as tabs in `CMultiFrame`.
- **Sensors** (`Sensors\`): `CArgyllSensor` is the one that matters; it wraps
  `Tools\ArgyllMeterWrapper` over the bundled ArgyllCMS instrument code in `Tools\spectro`
  (Argyll 2.1.1 + later additions such as SpyderX2). Meter detection (`getDetectedMeters`) runs when a
  document with an Argyll sensor is opened; Bluetooth COM ports are skipped unless
  `[Argyll] ScanBluetoothPorts=1`. The sensor's Reading Type (Display/Telephoto/Ambient) is stored in
  the document. Other sensors are legacy; `devlib\CHCFRDI1-4` get x64 stubs reporting "DLL not available".
- **Generators** (`Generators\`): `CGDIGenerator` with display modes (`DISPLAY_GDI`, `DISPLAY_GDI_nBG`,
  `DISPLAY_madVR`, `DISPLAY_ccast`, `DISPLAY_rPI`, … in `FullScreenWindow.h`). Patterns are drawn by
  `CFullScreenWindow::OnPaint` (window size %, background level `bgStimPercent`, border, ABL flash,
  settling ramp). With HDR10 enabled in the GDI modes, `CFullScreenWindow::PaintHdr10` draws the same
  scene through `CD3D11HdrOutput` (10-bit R10G10B10A2 swap chain, `G2084_NONE_P2020`, requires Windows
  HDR on); test images/animations fall back to GDI. libHDR (D3D12/NvAPI) is only used when HDR10 is off.
  Raspberry Pi/PGenerator needs `Tools\pi\RB8PGenerator.dll` (32-bit only).
- **Colour values**: `ColorRGBDisplay` (`libHCFR\Color.h`) holds percent 0–100 as doubles; in HDR mode
  these are PQ signal levels. `GetColorRef()` reduces them to 8 bits – only for GDI.
- **Preferences**: `CColorHCFRConfig` (`ColorHCFRConfig.cpp`). HDR/PQ mode is `GammaOffsetType == 5`
  (ST 2084); colour standards are the `ColorStandard` enum (`UHDTV2` = BT.2020, `UHDTV3` = P3 in BT.2020).
- **Resources/UI text**: menus, accelerators, dialogs and strings live in each language DLL
  (`CHCFR21_ENGLISH/DEUTSCH/FRANCAIS/ITALIAN/ESPANOL.rc`); a new command needs an ID in `resource.h`
  and entries in all five `.rc` files. Patterns images are in `CHCFR21_PATTERNS`.

## Rules that keep Win32 and x64 compatible

- The `.chc` format is `CArchive` serialization and must stay byte-identical between platforms:
  write counts as `(int)` (not `GetSize()`/`GetCount()`, which are `INT_PTR`) and `time_t` as `LONG`.
- Pointers carried in ints must be `INT_PTR`/`DWORD_PTR` (e.g. the sensor ID carries an
  `ArgyllMeterWrapper*`); message handlers use `OnTimer(UINT_PTR)` and `LRESULT (WPARAM, LPARAM)`.
- Win32 keeps `_USE_32BIT_TIME_T`; it is removed only in the x64 configurations.
