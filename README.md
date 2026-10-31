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


Localization application waits for Unreal's asynchronous localization tasks before updating
active state or reporting startup completion. In the editor, PIE world initialization
restores the active game preview after Unreal's BeginPIE override, before actor initialization;
ShutdownPIE restores it after Unreal's EndPIE reset. These restores do not repeat infrastructure
initialization, alter settings, or emit duplicate success logs. If restoration fails, active
state becomes unavailable so a later request can recover rather than take a stale no-op.
Culture identifiers and target metadata are compared case-sensitively.

The localization automation suite also covers casing mismatches, repeated lifecycle restores,
restoration failure/recovery, and the production editor adapter's asynchronous completion.
The PIE lifecycle test starts and stops two real sessions with a conflicting editor preview
setting and checks restoration before actors begin play. Run the suite in an idle editor.

## UI framework (Spec 007)

UMG is the primary authoring system. The checked-in project enables Unreal's
CommonUI plugin and uses `UCommonGameViewportClient`; no additional installation
is required. CommonUI supplies activation, stacks, input routing and focus
restoration where those behaviors are needed. MVVM, Experimental UIFramework,
CommonUI Enhanced Input integration and an input-action framework are not enabled.

`ULordSimulatorUISubsystem` is the single screen-space UI owner for each local
player. It creates one `ULordSimulatorPrimaryUILayout` from
`/Game/UI/Framework/WBP_LordSimulatorPrimaryUILayout`. Root creation waits for
localization startup completion, including completion after a non-fatal failure.
Controller/viewport lifecycle callbacks initialize it; world cleanup and local
player removal release it. `InitializeUI()` is safe to repeat and returns false
when prerequisites or the root asset are unavailable. Check `IsUIAvailable()`
before feature work that requires UI.

Derive ordinary widgets from `ULordSimulatorUserWidget` (`UCommonUserWidget`).
Use `ULordSimulatorActivatableWidget` (`UCommonActivatableWidget`) for screens
needing activation and CommonUI focus/input behavior. The primary layout derives
from the activatable base. Neither base polls gameplay data.

Use the local player's subsystem in C++ or Blueprint to attach production
screen-space widgets. `AddWidget(Layer, WidgetClass)` returns the new instance
or null; for Menu/Modal it pushes a screen. `RemoveWidget(Layer, Widget)` removes
that instance and `PopWidget(Layer)` removes the active Menu/Modal screen. Invalid
classes, layers, missing roots and unrelated removals fail safely. Feature code
must not call `AddToViewport`/`AddToPlayerScreen` or manipulate root internals.
World-space `UWidgetComponent` UI is outside this ownership/input system.

The root fills its player's viewport and contains exactly these layers, in
ascending visual order:

| Layer | Behavior |
|---|---|
| `Game` | Multiple ordinary or activatable widgets; combined game/UI input baseline. |
| `Menu` | Activatable stack; only its top is displayed/active, with menu input blocking world commands. |
| `Modal` | Activatable stack above Menu; blocks lower UI and world commands. |
| `PassiveOverlay` | Multiple ordinary or activatable widgets; self and descendants are hit-test transparent, with no activation focus/input mode. |

Closing a screen restores the underlying CommonUI state without reconstructing
unrelated UI. CommonUI owns focus restoration; feature screens may provide their
desired focus target. No layer changes simulation pause or time. Layer routing
preserves widget enabled state: input blocking does not impose disabled visual
styling on passive content or on Game/Menu behind a modal. The empty root
contains no production text, art, HUD or menu. Keep screen-space content under
`/Game/UI`, with shared technical infrastructure under `/Game/UI/Framework`.
That framework directory is explicitly cooked because the root loads by path.

Gameplay/simulation systems remain authoritative. Subscribe to their delegates,
events or explicit update calls, and unbind when a widget stops observing them.
Send gameplay commands through explicit system interfaces. Widgets may retain
ephemeral presentation state such as a selected tab or scroll position. Do not
use raw per-frame UMG bindings, Tick/OnPaint polling, repeated world searches,
custom MVVM or a second event bus to synchronize gameplay values. Preserve `FText`
and localization-aware formatting throughout the UI lifetime.

Use Unreal's virtualized `UListView`, `UTileView`, `UTreeView` or suitable CommonUI
equivalents for future large collections. Do not instantiate hundreds of off-screen
entries. Apply invalidation, retainers or volatility only for a demonstrated need;
standard Widget Reflector and Slate Insights remain available for diagnosis.

### UI verification

Build the editor, open an idle editor with no PIE session, and run:

```text
Automation RunTests LordSimulator.
```

This runs localization regressions and `LordSimulator.UI.Framework`. To run only
UI, use `Automation RunTests LordSimulator.UI`. The UI test starts PIE and uses
the actual root asset, viewport and CommonUI action router. It checks ownership,
repeat initialization, two local players, cleanup, layer validation, stack
restoration, passive transparency, localization gating and routed input modes.
It arranges all four layers at 1280x720, 1920x1080, 2560x1440, 3440x1440 and
3840x2160, at DPI scales 1.0 and 1.5. Allow Slate to render; do not use `-nullrhi`
for the UI suite. Test widgets live in the editor-only `LordSimulatorTests`
module and are excluded from Shipping.

Build `LordSimulator Win64 Shipping` with the same Build.bat command used for
the editor, replacing target/configuration, then package using section 4.
Inspect the staged/container manifest for the canonical root and required
CommonUI resources, and confirm no `LordSimulatorTests` fixtures are staged.
Launch `Packaged\Windows\LordSimulator.exe`; the technical root must add no
visible placeholder text or art. With native symbols and a debugger, verify
the primary local player's `ULordSimulatorUISubsystem` has exactly one attached
`PrimaryLayout`, and repeated initialization retains that same instance.
Repeat the packaged smoke check after producing/installing section 5's installer.
