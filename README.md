# Lord Simulator

Unreal Engine 5.8.4 C++ project (`LordSimulator`) targeting 64-bit Windows.
This README describes how to go from a clone of this repository to the installer `LordSimulatorSetup.exe`.

## Repository layout

| Path | Purpose |
|---|---|
| `LordSimulator.uproject` | Unreal project file (project `LordSimulator`) |
| `Source/LordSimulator.Target.cs` | Game target `LordSimulator` (C++20) |
| `Source/LordSimulatorEditor.Target.cs` | Editor target `LordSimulatorEditor` (C++20) |
| `Source/LordSimulator/` | Primary C++ module `LordSimulator` |
| `Config/` | Project configuration, localization configuration and packaging settings |
| `Content/Localization/` | Canonical String Table and compiled Lord Simulator localization resources |
| `Content/UI/` | Production and framework UI assets |
| `Installer/LordSimulator.iss` | Inno Setup script that produces `LordSimulatorSetup.exe` |

## Prerequisites

All steps run on 64-bit Windows 10 or Windows 11.

1. **Unreal Engine 5.8.4**
   - Install it from the Epic Games Launcher.
   - The default install location is `C:\Program Files\Epic Games\UE_5.8`. The commands below refer to this folder as `%UE_ROOT%`.
2. **Visual Studio 2022 (17.14 or later) or Visual Studio 2026 (18.0 or later)** with:
   - Workloads: *Game development with C++*, *Desktop development with C++*, *.NET desktop development*.
   - Under *Game development with C++*: *Unreal Engine installer* and a Windows 10/11 SDK.
   - MSVC toolset 14.38 or newer and Windows SDK 10.0.22621.0 or newer. Unreal Engine 5.8 currently prefers MSVC 14.50; newer installed toolchains may produce a warning even when the build succeeds.
3. **Inno Setup 6.3 or later**. It provides `ISCC.exe`, by default in `C:\Program Files (x86)\Inno Setup 6\`.
4. **Git**.

## 1. Clone the repository

```bat
git clone https://github.com/Pal266/lord-simulator.git
cd lord-simulator
```

All commands below run from the repository root in `cmd.exe`.

```bat
set UE_ROOT=C:\Program Files\Epic Games\UE_5.8
```

## 2. Build the Development Editor target

Close any running Unreal Editor/game instance before command-line builds if Live Coding is active.

```bat
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" LordSimulatorEditor Win64 Development -Project="%CD%\LordSimulator.uproject" -WaitMutex
```

Alternatively, generate Visual Studio project files, open `LordSimulator.sln`, select `Development Editor` / `Win64`, and build.

## 3. Launch the compiled project

Editor:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\LordSimulator.uproject"
```

Game mode:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%CD%\LordSimulator.uproject" -game
```

## 4. Run automated tests

Open an idle Unreal Editor session and use **Tools → Session Frontend → Automation**. Run the top-level `LordSimulator` test group.

The suite includes localization regressions, `LordSimulator.UI.Framework`, and `LordSimulator.UI.MainMenu`.

The UI tests start PIE and require Slate/viewport rendering; do not use `-nullrhi` for the complete UI suite.

Localization-only unattended execution is supported with:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -unattended -nullrhi -ExecCmds="Automation RunTests LordSimulator.Localization" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\LocalizationTests"
```

## 5. Localization workflow

The canonical Unreal Localization Target is **LordSimulator**, with native culture **en** and loading policy **Game**.

The canonical asset-based String Table is:

```text
Content/Localization/ST_LordSimulator.uasset
/Game/Localization/ST_LordSimulator.ST_LordSimulator
```

Use the Unreal-assigned table ID with `FText::FromStringTable`, or select the String Table asset in Unreal's text picker. Keep player-facing text as `FText` throughout its UI lifetime.

The production table currently contains these main-menu entries, with their production translations:

| Key | Native English source `en` | Czech `cs` | Ukrainian `uk` |
|---|---|---|---|
| `MainMenu.NewGame` | `NEW GAME` | `NOVÁ HRA` | `НОВА ГРА` |
| `MainMenu.LoadGame` | `LOAD GAME` | `NAČÍST HRU` | `ЗАВАНТАЖИТИ ГРУ` |
| `MainMenu.Settings` | `SETTINGS` | `NASTAVENÍ` | `НАЛАШТУВАННЯ` |
| `MainMenu.Quit` | `QUIT` | `UKONČIT` | `ВИЙТИ` |

Do not add placeholder or test-only production entries.

`Config/DefaultGame.ini`, section `[LordSimulator.Localization]`, is authoritative for runtime-supported cultures. The production-supported set is exactly `en`, `cs`, and `uk`. Localization Dashboard target settings are in `Config/DefaultEditor.ini`; gather/compile configurations are in `Config/Localization/`; generated resources are in `Content/Localization/LordSimulator/`.

### Gather and compile

Open **Tools → Localization Dashboard → LordSimulator** to manage the target. Keep native culture `en` and loading policy **Game**.

Gather:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -run=GatherText -config=Config/Localization/LordSimulator_Gather.ini -unattended
```

Unreal 5.8's dashboard may regenerate an unconditional `ExcludePathFilters=Content/Localization/*` entry when using its **Gather Text** button. That would exclude the canonical String Table. Remove that exclusion before gathering while retaining the `Content/L10N/*` exclusion. The checked-in gather configuration intentionally includes the canonical table.

Compile:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -run=GatherText -config=Config/Localization/LordSimulator_Compile.ini -unattended
```

Translations are stored in the culture archives (`Content/Localization/LordSimulator/<culture>/LordSimulator.archive`). Edit them with **Localization Dashboard → LordSimulator → Edit Translations** for the culture, save, then compile. Never change native English source text when editing translations.

Commit the String Table, target configuration, manifest, culture archives, `.locmeta`, and `.locres` resources together whenever localization content changes.

To add a supported culture, add its exact Unreal identifier to `SupportedCultures` in `[LordSimulator.Localization]`, add the same culture to the Localization Dashboard target, gather and compile, and update **Project Settings → Packaging → Localizations to Package** (`CulturesToStage`) to the same supported set. **Project Settings → Packaging → Internationalization Support** (`InternationalizationPreset`) must include ICU data for every supported culture; Czech and Ukrainian require `All`. Keep `en` native and supported. Culture identifiers are case-sensitive.

### Runtime/startup localization

Game configuration establishes `en` as the baseline independently of Windows language. Lord Simulator initializes localization, loads/validates settings, applies the configured language, and marks localization startup complete before localization-dependent production UI is initialized. Localization failure is non-fatal and still completes the localization startup phase.

Editor/PIE uses Unreal game-localization preview behavior and must not change the Unreal Editor application's own language/locale. Use standalone or packaged execution for real application-wide language/locale verification.

## 6. UI framework

UMG is the primary authoring system. The project enables Unreal CommonUI and uses `UCommonGameViewportClient`.

`ULordSimulatorUISubsystem` is the authoritative screen-space UI owner for each local player. It creates one `ULordSimulatorPrimaryUILayout` from:

```text
/Game/UI/Framework/WBP_LordSimulatorPrimaryUILayout
```

The root exposes the canonical layers `Game`, `Menu`, `Modal`, and `PassiveOverlay`. Production screen-space widgets enter through these layers; feature code must not establish competing viewport roots.

Root creation waits for localization startup completion and for required local-player/viewport prerequisites. Repeated initialization is idempotent.

### Production main menu

After localization startup is complete and the primary local-player UI root is available, startup presents exactly one production main-menu screen on the canonical `Menu` layer:

```text
/Game/UI/Screens/MainMenu/WBP_MainMenu
```

Repeated startup/UI initialization must not create duplicate main-menu instances.

The menu uses the canonical Lord Simulator String Table entries above. `NEW GAME`, `LOAD GAME`, and `SETTINGS` are currently visible but disabled/non-focusable. `QUIT` is enabled, receives initial focus, and performs normal Unreal quit behavior. In PIE it ends PIE without closing the editor; in a packaged build it closes the application.

Main-menu production textures live under:

```text
/Game/UI/Textures/MainMenu
```

The technical primary root itself remains free of feature-specific presentation; the main menu is a production screen attached through the `Menu` layer.

## 7. Produce the Windows x64 Shipping package

Close Unreal Editor and any running Lord Simulator executable before building so Live Coding does not block UnrealBuildTool.

Optional explicit Shipping build verification:

```bat
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" LordSimulator Win64 Shipping -Project="%CD%\LordSimulator.uproject" -WaitMutex
```

Gather/compile localization before packaging when localization content has changed, then run:

```bat
"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%CD%\LordSimulator.uproject" -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -prereqs -archive -archivedirectory="%CD%\Packaged" -noP4 -utf8output
```

The packaged build is written to `Packaged\Windows\` and contains `LordSimulator.exe` plus the Unreal prerequisites installer.

Packaging must include the canonical localization resources/String Table, the primary UI root, the production main-menu Widget Blueprint, and the main-menu textures. Editor-only `LordSimulatorTests` fixtures must not be staged.

### Packaged smoke verification

Launch:

```bat
Packaged\Windows\LordSimulator.exe
```

Verify that:

1. the production main menu appears once at startup;
2. its background and button states render correctly;
3. `NEW GAME`, `LOAD GAME`, and `SETTINGS` remain disabled;
4. `QUIT` responds to normal/hover/pressed states and closes the packaged process normally;
5. `Documents\Lord Simulator\log.log` contains the expected startup localization sequence and `Application shutdown started.` after normal Quit.

### Packaged localization verification

Check every supported culture (`en`, `cs`, `uk`) in the packaged build. Before each run, close the game and set `Language=<culture>` in `Documents\Lord Simulator\settings.ini` under `[Settings]`. For each culture, launch at each viewport size `1280x720`, `1920x1080`, `2560x1440`, `3440x1440`, and `3840x2160`, for example:

```bat
Packaged\Windows\LordSimulator.exe -windowed -ResX=1280 -ResY=720
```

At each size verify that:

1. `log.log` contains `Language '<culture>' was applied.` (no fallback or unavailable message);
2. the main menu shows the exact labels from the table in section 5, with Czech diacritics and Ukrainian Cyrillic rendered correctly;
3. every label is readable, aligned as in English, and fully contained by its button;
4. button states and `QUIT` behavior match the packaged smoke verification.

Packaged localization resources must exist for each culture:

```text
Packaged\Windows\LordSimulator\Content\Localization\LordSimulator\{en,cs,uk}\LordSimulator.locres
```

When the build uses `.pak` files, confirm their presence with `UnrealPak.exe <pak> -List` instead.

## 8. Produce the installer

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" Installer\LordSimulator.iss
```

The installer is written to:

```text
Installer\Output\LordSimulatorSetup.exe
```

If the packaged build is elsewhere:

```bat
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DPackagedDir="D:\path\to\Windows" Installer\LordSimulator.iss
```

## 9. Install and launch

1. Run `LordSimulatorSetup.exe` on a supported 64-bit Windows system and accept the administrator prompt.
2. The application installs to `C:\Program Files\Lord Simulator\` and creates the `Lord Simulator` folder under the current user's Documents known folder.
3. Launch `C:\Program Files\Lord Simulator\LordSimulator.exe`.
4. Repeat the packaged smoke verification against the installed build.

The application can be uninstalled from **Settings → Apps → Installed apps → Lord Simulator**.
