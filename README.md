# Lord Simulator

Unreal Engine 5.8.4 C++ project (`LordSimulator`) targeting 64-bit Windows.
This README describes how to go from a clone of this repository to the
installer `LordSimulatorSetup.exe`.

## Repository layout

| Path | Purpose |
|---|---|
| `LordSimulator.uproject` | Unreal project file (project `LordSimulator`) |
| `Source/LordSimulator.Target.cs` | Game target `LordSimulator` (C++20) |
| `Source/LordSimulatorEditor.Target.cs` | Editor target `LordSimulatorEditor` (C++20) |
| `Source/LordSimulator/` | Primary C++ module `LordSimulator` |
| `Config/` | Project configuration (display name, startup map, packaging settings) |
| `Installer/LordSimulator.iss` | Inno Setup script that produces `LordSimulatorSetup.exe` |

## Prerequisites

All steps run on 64-bit Windows 10 or Windows 11.

1. **Unreal Engine 5.8.4**
   - Install it from the Epic Games Launcher (Unreal Engine → Library → add 5.8, then make sure the installed version is 5.8.4).
   - The default install location is `C:\Program Files\Epic Games\UE_5.8`. The commands below refer to this folder as `%UE_ROOT%`.
2. **Visual Studio 2022 (17.14 or later) or Visual Studio 2026 (18.0 or later)** with:
   - Workloads: *Game development with C++*, *Desktop development with C++*, *.NET desktop development*.
   - Under *Game development with C++*: *Unreal Engine installer* and a Windows 10/11 SDK.
   - MSVC toolset 14.38 or newer and Windows SDK 10.0.22621.0 or newer (Epic recommends MSVC 14.50 and Windows SDK 10.0.26100).
   - Check Epic's *Setting Up Visual Studio for Unreal Engine* page for the current 5.8 requirements.
3. **Inno Setup 6.3 or later** (https://jrsoftware.org/isinfo.php). It provides `ISCC.exe`, by default in `C:\Program Files (x86)\Inno Setup 6\`.
4. **Git**, to clone the repository.

## 1. Clone the repository

```bat
git clone https://github.com/Pal266/lord-simulator.git
cd lord-simulator
```

All commands below run from the repository root in a Windows command prompt (`cmd.exe`). First set the engine path:

```bat
set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
```

## 2. Build the C++ project

Build the editor target:

```bat
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" LordSimulatorEditor Win64 Development -Project="%CD%\LordSimulator.uproject" -WaitMutex
```

Alternatively, right-click `LordSimulator.uproject` → *Generate Visual Studio project files*, open `LordSimulator.sln`, select the `Development Editor` / `Win64` configuration and build.

## 3. Launch the compiled project

Open the project in the editor:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\LordSimulator.uproject"
```

Or run it directly as a game (window titled *Lord Simulator*):

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\LordSimulator.uproject" -game
```

## 4. Produce the Windows x64 packaged build

```bat
"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%CD%\LordSimulator.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive -archivedirectory="%CD%\Packaged" -noP4 -utf8output
```

The packaged build is written to `Packaged\Windows\`. It contains `LordSimulator.exe` and the Unreal prerequisites installer (`Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe`).

The editor's *Platforms → Windows → Package Project* menu produces the same build. If you use it, choose the `Packaged` folder in the repository root as the output folder.

## 5. Produce the installer

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\LordSimulator.iss
```

The installer is written to `Installer\Output\LordSimulatorSetup.exe`.

If the packaged build is somewhere other than `Packaged\Windows`, pass its location:

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DPackagedDir="D:\path\to\Windows" Installer\LordSimulator.iss
```

## 6. Install and launch

1. Run `LordSimulatorSetup.exe` on a 64-bit Windows system and accept the administrator prompt.
2. The application installs to `C:\Program Files\Lord Simulator\`. The installer also installs the Unreal Engine runtime prerequisites silently.
3. Launch `C:\Program Files\Lord Simulator\LordSimulator.exe`.

It can be uninstalled from *Settings → Apps → Installed apps → Lord Simulator*.
