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
2. The application installs to `C:\Program Files\Lord Simulator\`. The installer also installs the Unreal Engine runtime prerequisites silently, and creates the `Lord Simulator` folder in your Documents folder.
3. Launch `C:\Program Files\Lord Simulator\LordSimulator.exe`.

It can be uninstalled from *Settings → Apps → Installed apps → Lord Simulator*.

## Localization (Spec 006)

The single target is **LordSimulator**, native culture **en**, loading policy
**Game**. The empty asset-based String Table is
`Content/Localization/ST_LordSimulator.uasset`, object path and Unreal-assigned
table ID `/Game/Localization/ST_LordSimulator.ST_LordSimulator`. Use that ID
with `FText::FromStringTable`, or select the asset in Unreal's text picker.
Keep `FText` values alive in UI; do not cache their `ToString()` results.
There are deliberately no placeholder entries or production test translations.

`Config/DefaultGame.ini`, section `[LordSimulator.Localization]`, is the
authoritative runtime-supported culture configuration. Initially it contains
only `en`. The dashboard target settings are in `Config/DefaultEditor.ini`;
standard gather/compile configurations are in `Config/Localization/` and
resources in `Content/Localization/LordSimulator/`. Commit the String Table,
target configuration, manifest, culture archives, `.locmeta` and `.locres`
files together. Resource metadata is checked against the authoritative culture
configuration at initialization, never used to infer supported cultures.

### Gather, translate and compile

1. Open **Tools → Localization Dashboard → LordSimulator**. Keep the native
   culture `en` and loading policy **Game**. Open the canonical String Table in
   the Content Browser to manage entries required by future specifications.
2. Gather using Unreal's standard `GatherText` commandlet and the checked-in
   target gather configuration:

   ```bat
   "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -run=GatherText -config=Config/Localization/LordSimulator_Gather.ini -unattended
   ```

   Unreal 5.8's dashboard regenerates an unconditional
   `ExcludePathFilters=Content/Localization/*` line when its **Gather Text**
   button is used. That excludes this specification's canonical String Table
   location. Remove that exclusion from the generated gather configuration
   before running the command above; retain the `Content/L10N/*` exclusion.
   The checked-in gather configuration intentionally includes the canonical
   table. Do not move the asset or introduce a parallel source table.
3. Use the dashboard's translation editor or standard PO export/import for
   translations. English source wording and semantic keys must come from a
   specification. Missing non-native translations use Unreal's native fallback.
4. Click **Compile Text** on the LordSimulator target, or run:

   ```bat
   "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -run=GatherText -config=Config/Localization/LordSimulator_Compile.ini -unattended
   ```

   Resolve structural/configuration errors and inspect the conflict report.
   The empty initial target legitimately has zero words.

To add a supported culture, add its exact Unreal identifier to
`SupportedCultures` in `[LordSimulator.Localization]`, add the same culture to
the dashboard target, regenerate the standard configurations (checking the
gather exclusion above), gather and compile. Keep `en` native and supported.
Update **Project Settings → Packaging → Localizations to Package**
(`CulturesToStage`) to the same set and select internationalization data
sufficient for every supported culture. `English` is sufficient only for the
initial `en` configuration. Rebuild and validate all supported cultures.
Culture matching is case-sensitive: `pt`, `pt-BR` and `pt-br` are distinct.

### Runtime and startup

`FLordSimulatorLocalization` exposes immutable language metadata, configured
language, optional active language, validation, startup completion, and
`ChangeLanguage`. Runtime feature code uses this operation; only it may update
configured `Language` and game culture together. There are no Blueprint APIs
or custom text lookup/refresh events. Language changes update in-memory settings
only; the existing complete-state shutdown persistence saves them later.

Game configuration establishes the `en` language/locale baseline independently
of Windows language. The module starts at `PostEngineInit`, when the asset
loading bridge and localization resources are available. Logging initializes
first, then localization, complete settings loading/validation, LogMode,
configured language application, settings completion and localization startup
completion. Any future localization-dependent UI initialization must wait for
`IsStartupComplete()`. Failure also completes startup and remains non-fatal.

Editor/PIE uses Unreal's game-localization preview; it never changes the editor
application's global language or locale. Use standalone game execution or the
packaged game to verify both real language and locale changes and culture-aware
number/date formatting. Editor preview validates game text resources.

### Automated and packaged validation

After building the editor, run these built-in Unreal C++ tests from the editor
console, or select **LordSimulator → Localization** in **Session Frontend →
Automation**:

```text
Automation RunTests LordSimulator.Localization
```

For unattended execution:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\LordSimulator.uproject" -unattended -nullrhi -ExecCmds="Automation RunTests LordSimulator.Localization" -TestExit="Automation Test Queue Empty" -ReportExportPath="%CD%\Saved\LocalizationTests"
```

The tests cover deterministic engine-failure seams, initialization precedence,
fallback/recovery, exact messages, settings parsing/serialization and complete
state comparison, actual target/resources/asset configuration, and transient
localization-aware FText. No test cultures or strings are packaged.

Gather and compile before the Shipping packaging command in section 4. The
packaging configuration stages native localization resources and English ICU
data. `/Game/Localization` is explicitly cooked because the empty String Table
is loaded by a runtime path and has no map/content reference for the cooker's
dependency graph. Keep the always-cook directory scoped to this canonical asset
location; there is no second localization resource structure.

Packaging verification is separate from automation:

1. Verify the staged manifest/container includes the canonical String Table,
   `LordSimulator.locmeta`, every configured culture's `LordSimulator.locres`,
   and required internationalization data, with no extra game cultures.
2. Launch `Packaged\Windows\LordSimulator.exe`. Check the Lord Simulator log
   in the Documents known folder for localization initialization completion and
   `Language 'en' was applied.`, in the required settings-startup order.
3. For every configured culture, verify exact support recognition, Unreal
   native display name, game language and locale activation, packaged resource
   access, existing FText changing live, native fallback for untranslated
   existing entries, and return to `en`. Use the framework from a debugger or
   existing feature call site; do not add test UI, production strings or cultures.
   The initially empty table has no translated entries to exercise; the
   transient automated fixture covers FText resolution without adding content.
4. Verify successful changes persist only through normal shutdown, and editor
   preview leaves editor language/locale unchanged. Repeat after installing the
   installer from section 5.
