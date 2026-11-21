# HCFR

## Welcome to the HCFR project

This repository is a continuation of the mostly abandoned repository which can be found [here](https://sourceforge.net/p/hcfr/code/), based on the latest zoyd branch.

## Windows Users

### Requirements

- Visual Studio 2022 (Community is fine) with the **Desktop development with C++** workload
  and the **C++ MFC for latest v143 build tools** component.
- Windows 10/11 SDK (installed with the workload).
- Optional: the *Microsoft Visual Studio Installer Projects* extension, only needed for the
  installer project (`ColorHCFR_VS2019.vdproj`); the application builds without it.

### Building

Open `ColorHCFR.sln`, select **ColorHCFR** as the startup project and build the
**Release** configuration for the platform you want:

| Platform | Output | Notes |
|----------|--------|-------|
| **x64**  | `x64\Release\ColorHCFR.exe` | 64-bit build |
| **Win32** | `Release\ColorHCFR.exe` | 32-bit build; copy the `CHCFRDI*.dll` files from `devlib` next to the exe |

From a command line:

```
msbuild ColorHCFR.sln /t:ColorHCFR /p:Configuration=Release /p:Platform=x64
```

Both builds read and write the same `.chc` file format.

### HDR10 measurements

With **HDR10** enabled in the GDI generator settings, the pattern window draws the measurement
patterns through Direct3D 11: a 10-bit swap chain in BT.2020/PQ (`DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`),
so pattern levels go to the display as exact PQ code values. This works with any GPU vendor, but
**Windows HDR must be on** for the display showing the patterns (Win+Alt+B). If it is off, HCFR
says so once and draws the patterns in SDR. HDR10 static metadata (mastering display, MaxCLL,
MaxFALL) is taken from the preferences. Special test images, grids and animations stay in SDR.

Typical settings: colour space *UHDTV Rec2020* (or *P3 in Rec2020*), EOTF *ST 2084*, and an Argyll
meter with Reading Type *Display*.

### Limitations of the 64-bit build

- The legacy native drivers for **Spyder II, Eye-One, Mazet MTCS-C2 and Spyder 3**
  (`devlib\CHCFRDI1-4.dll`) are closed-source 32-bit binaries and are not available.
  Use the Argyll driver instead (it supports these meters except the MTCS-C2), or the 32-bit build.
- The **Raspberry Pi pattern generator** needs `RB8PGenerator.dll`, which only exists as a
  32-bit binary.
