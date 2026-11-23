# HCFR HDR

A modern fork of **HCFR Colorimeter**, based on [`arthur-liberman/hcfr-code`](https://github.com/arthur-liberman/hcfr-code) (3.5.4.4), which continues the [SourceForge HCFR project](https://sourceforge.net/p/hcfr/code/).

## Highlights in this fork

- **Visual Studio 2022 & native 64-bit (x64) build** alongside Win32 (both platforms read and write the same `.chc` file format).
- **Working HDR10 pattern output via Direct3D 11** (10-bit BT.2020/PQ swap chain, works with any GPU vendor).
- **Configurable HDR colour patterns reference white (ST 2084)** in *Preferences → References*, with standard presets (*HCFR default 94.38*, *sRGB / Windows 80*, *SDR reference 100*, *ITU-R BT.2408 203*, *Custom*) and a *Reset* button.
- **Luminance graph axes labelled in cd/m²** in HDR mode (both logarithmic and linear axes).
- **Presentation layout** (*View* menu / `F11`) for clean article screenshots with square graph panes.
- **Fullscreen test patterns & Additional Patterns fixes** – proper aspect-preserving fullscreen scaling on 1440p/4K displays, fixed *Additional Patterns* dialog focus/lifecycle, and full keyboard/mouse navigation (`+/-`, arrows, `PageUp/PageDown`, `Home/0`, `Space`, `Esc/Enter`/right-click).
- **UI & usability improvements** – fast, clean tooltips; smoother flicker-free window resizing; skipping Bluetooth COM ports during Argyll meter detection; clearer meter position errors; startup splash-screen dialog fixes.

## HDR10 measurements

With **HDR10** enabled in the GDI generator settings, the pattern window draws measurement patterns through Direct3D 11 using a 10-bit BT.2020/PQ swap chain (`DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`), sending exact PQ code values to the display.

- **Windows HDR must be enabled** on the target display (`Win+Alt+B`). If it is off, HCFR notifies you once and falls back to SDR.
- Static HDR10 metadata (mastering display primaries/luminance, MaxCLL, MaxFALL) is read from *Preferences → References*.
- Special test images, grids, and animations stay in SDR.
- Typical settings: colour space *UHDTV Rec2020* (or *P3 in Rec2020*), EOTF *SMPTE 2084 HDR*, and an Argyll meter with Reading Type *Display*.

## Building from source

### Requirements

- **Visual Studio 2022** (Community or Build Tools) with:
  - **Desktop development with C++** (`v143` toolset)
  - **C++ MFC for latest v143 build tools (x86 & x64)**
  - **Windows 10/11 SDK**
- Optional: **Inno Setup 6** for building the Windows installer (`Install\hcfr.iss`).

### Building the application

Open `ColorHCFR.sln`, select **ColorHCFR** as the startup project, and build the **Release** configuration for the desired platform:

| Platform | Output | Notes |
|----------|--------|-------|
| **x64**  | `x64\Release\ColorHCFR.exe` | Recommended 64-bit build |
| **Win32** | `Release\ColorHCFR.exe` | 32-bit build; copy `devlib\CHCFRDI*.dll` next to the exe |

From a command line:

```powershell
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=x64 /m
```

### Building the installer

Build the Argyll helper tools (`oeminst` and `spotread`) for the same platform, then compile `Install\hcfr.iss` with Inno Setup 6:

```powershell
msbuild ColorHCFR.sln /t:oeminst;spotread /p:Configuration=Release /p:Platform=x64 /m
iscc /DPlatform=x64 Install\hcfr.iss
```

This produces `x64\Release\HCFRSetup-x64.exe` (or `Release\HCFRSetup-Win32.exe` with `/DPlatform=Win32`).

## Limitations of the 64-bit build

- Legacy native drivers for **Spyder II, Eye-One, Mazet MTCS-C2 and Spyder 3** (`devlib\CHCFRDI1-4.dll`) are closed-source 32-bit binaries and are not available in x64. Use the Argyll driver instead (supports these meters except MTCS-C2), or the 32-bit build.
- The **Raspberry Pi / PGenerator** mode requires `Tools\pi\RB8PGenerator.dll`, which is 32-bit only.
