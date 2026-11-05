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
4. **Git with Git LFS** for the full-resolution map assets.

## 1. Clone the repository

```bat
git lfs install
git clone https://github.com/Pal266/lord-simulator.git
cd lord-simulator
git lfs pull
```

The one hundred Southern Coast Range `.umap` files use Git LFS and total approximately 11.6 GB. Retrieve their full contents before opening, building or cooking the project.

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

The suite includes `LordSimulator.Settings.StartupDisplay`, localization regressions, `LordSimulator.UI.Framework`, and `LordSimulator.UI.MainMenu`.

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
| `MainMenu.LoadGame` | `LOAD GAME` | `NAČÍST` | `ЗАВАНТАЖИТИ` |
| `MainMenu.Settings` | `SETTINGS` | `NASTAVENÍ` | `ПАРАМЕТРИ` |
| `MainMenu.Quit` | `QUIT` | `UKONČIT` | `ВИЙТИ` |
| `NewGame.RegionSelection.Title` | `Select a region` | `Vyberte oblast` | `Виберіть регіон` |
| `NewGame.RegionSelection.Back` | `Back` | `Zpět` | `Назад` |
| `NewGame.RegionSelection.LoadFailed` | `Unable to load the selected region.` | `Vybranou oblast se nepodařilo načíst.` | `Не вдалося завантажити вибраний регіон.` |

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

The menu uses the canonical Lord Simulator String Table entries above. `NEW GAME` is enabled and opens region selection. `LOAD GAME` and `SETTINGS` remain disabled/non-focusable. `QUIT` is enabled, receives initial focus, and performs normal Unreal quit behavior. In PIE it ends PIE without closing the editor; in a packaged build it closes the application.

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
3. `NEW GAME` opens region selection; `LOAD GAME` and `SETTINGS` remain disabled;
4. `QUIT` responds to normal/hover/pressed states and closes the packaged process normally;
5. `Documents\Lord Simulator\log.log` contains the expected startup localization sequence and `Application shutdown started.` after normal Quit.

### Packaged display verification

Before each launch, close the game and edit the three display entries under `[Settings]` in `Documents\Lord Simulator\settings.ini`. Use a numeric pair supported by the startup monitor.

| Width and height | Fullscreen | Expected startup |
|---|---|---|
| `auto` and `auto` | `1` | Current screen resolution in fullscreen |
| Supported numeric pair | `1` | Exact requested resolution in fullscreen |
| Supported numeric pair | `0` | Exact requested resolution in a window |
| `auto` and a numeric height | `0` | Current screen resolution in a window |
| `0` and a numeric height | `1` | Current screen resolution in fullscreen |

Confirm that the main menu remains operational. Check `log.log` for one resolution rejection in each of the last two cases and zero in the first three. The rejection message includes the configured pair. Quit normally between cases so the settings file can be updated through the normal shutdown lifecycle.

### Packaged localization verification

Check every supported culture (`en`, `cs`, `uk`) in the packaged build. Before each run, close the game and set `Language=<culture>` in `Documents\Lord Simulator\settings.ini` under `[Settings]`. For each culture, set `Fullscreen=0` and set `Width` and `Height` to each supported test size in turn (`1280x720`, `1920x1080`, `2560x1440`, `3440x1440`, and `3840x2160`). Use a monitor large enough for the requested sizes. Launch the packaged game after each edit.

At each size verify that:

1. `log.log` contains `Language '<culture>' was applied.` (no fallback or unavailable message);
2. the main menu shows the exact labels from the table in section 5, with Czech diacritics and Ukrainian Cyrillic rendered correctly;
3. every label is readable, aligned as in English, and fully contained by its button;
4. button states and `QUIT` behavior match the packaged smoke verification.

The packaged build stores localization resources in its `.pak` file. Confirm that `LordSimulator.locres` is present for `en`, `cs`, and `uk`:

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealPak.exe" "%CD%\Packaged\Windows\LordSimulator\Content\Paks\LordSimulator-Windows.pak" -List | findstr /i "LordSimulator.locres"
```

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

## 10. Southern Coast Range content preparation (spec-011)

Use the documentation repository's canonical `fictional_map/regions/southern_coast_range/region.json`, seed `7`, and its unmodified scientific scripts. Python 3.10+ and `fictional_map/requirements.txt` are offline preparation requirements only. The region must specify 100 km, a 10×10 grid of 10 km plots, and 4033 vertices per plot. From the game repository, with `DOCS` pointing to the documentation repository and `PYTHON` to that Python environment:

```bat
"%PYTHON%" -m pip install -r "%DOCS%\fictional_map\requirements.txt"
"%PYTHON%" Scripts\generate_region.py --docs-repo "%DOCS%" --output-dir Saved\RegionGeneration
"%PYTHON%" Scripts\prepare_region_inputs.py --source-region "%DOCS%\fictional_map\regions\southern_coast_range" --generation-manifest Saved\RegionGeneration\manifest.json --output-dir Saved\RegionImport
```

The generator runs `plates.py 7`, `validate.py`, `heightmap.py`, `validate.py`, `climate.py`, `validate.py`, `hydrology.py`, `validate.py` from `fictional_map/scripts`, then `extract_region.py`, `regional_model.py`, `validate_region.py`, `refine_25m.py`, `validate_landforms.py`, `detail_plots.py --tiles all`, `validate_plots.py`, `export_water.py`, `validate_water.py` from the region's `scripts` directory. Any failed command stops preparation. Retain the generated `manifest.json` and logs: they record the source commit, configuration, script hashes and output hashes. The input verifier checks these hashes and all 100 heightmap/water pairs, then writes `Saved/RegionImport/source-run.json` and `region-overview.png`. The overview bilinearly samples the north-up refined 25 m elevation grid at 1024×1024 pixel centres, normalized by the full 100 km square's minimum/maximum elevations.

Build `LordSimulatorEditor` using section 6, open the project in Unreal 5.8.4, enable the bundled Python Editor Script Plugin if necessary, and enter these application commands in the editor's **Cmd** console (replace the absolute example placeholders):

```text
LordSimulator.ImportRegion OverviewFile="<game>/Saved/RegionImport/region-overview.png"
py "<game>/Scripts/import_region_in_editor.py" "<docs>/fictional_map/regions/southern_coast_range"
```

Wait for `Saved/RegionImport/batch-status.json` to contain all 100 completed tiles with no pending/failed item. The batch replaces each generated map; do not run it against manually edited levels. Alternatively import one plot with `LordSimulator.ImportRegion SourceRegion="<region>" Tile=tile_x0_y0`. The native editor module also exposes `-run=LordSimulatorImportRegion -SourceRegion="<region>" -Tile=tile_x0_y0` for standard Unreal commandlet execution. All source paths are preparation inputs, never runtime requirements.

The native importer repairs the documented `ue_import_water.py` integration for 5.8.4: it uses `UWaterSplineComponent`, per-key river width metadata, native `OnWaterBodyChanged` notifications, and sets `bAffectsLandscape=false` before actor creation is observed by WaterEditor. River locations/elevations/widths and every closed linear lake outline/elevation are retained without filtering. Clockwise lake outlines have their traversal reversed to satisfy Water's positive fill rule; every vertex and the exact closed boundary are preserved. Depth/velocity are unused exported metadata. Native lake collision generation can report degenerate convex hulls or collision triangulation warnings for dense outlines; collision is not used by the free camera. Surface triangulation is a separate Water plugin path. The importer requires a nonempty native surface mesh for every water body before saving.

After importing, run `py "<game>/Scripts/verify_region_water_in_editor.py" "<docs>/fictional_map/regions/southern_coast_range"` in the editor Cmd console. This reloads all 100 saved levels without saving changes, regenerates each native water surface mesh, and checks body counts against the complete exports. Require all 100 entries complete with no failures in `Saved/RegionImport/WaterMeshes/batch-status.json`; per-level reports include nonempty mesh and triangle counts. For an interrupted import, append specific `tile_x{col}_y{row}` IDs to `import_region_in_editor.py` to reimport only those tiles; its subset status is separate from the full batch report.

Each 16-bit 4033×4033 heightmap becomes one 32×32-component Landscape, with two 63-quad subsections per component, scale XY `1000000/4032` cm and Z `1000`. Every encoded vertex is compared against the input before and after water placement; any difference fails import. River spline positions/widths and lake vertices are compared in centimetres, and counts must match the complete export. Per-tile success reports are written only after saving. This is stronger than the required representative ~0.078 m height tolerance. Local X is east, local Y is south; shared geographic metres are `(-50000 + col*10000 + localX/100, 50000 - row*10000 - localY/100, localZ/100)`.

Prepared assets are `/Game/Maps/SouthernCoastRange/tile_x{col}_y{row}`, `/Game/Maps/SouthernCoastRange/M_TerrainVerification`, and `/Game/UI/Screens/RegionSelection/{WBP_RegionSelection,T_RegionOverview}`. The Widget Blueprint owns the screen layout; runtime C++ owns selection, input and asynchronous streaming. Built-in Water is enabled. `DefaultGame.ini` explicitly cooks all 100 maps and the selection content. Package using section 7 after the canonical localization gather/compile. No Python, source exports or documentation repository is required by the packaged executable.

For an IoStore package, inspect its content with `"%UE_ROOT%\Engine\Binaries\Win64\UnrealPak.exe" -ListContainer="Packaged\Windows\LordSimulator\Content\Paks\LordSimulator-Windows.utoc" -Csv="cooked-content.csv"`. Require all 100 distinct `SouthernCoastRange/tile_x{col}_y{row}.umap` paths, the selection Blueprint/overview, and native Water dependencies. List the companion `.pak` with UnrealPak `-List` and require the canonical en/cs/uk `.locres` files. Neither list may contain the editor-only test fixtures.

Run `Automation RunTests LordSimulator.Region` in an idle editor for all 100 mappings/orientation, state transitions, real landscape loading, M input binding, dismissal, reselecting, failed loads and old-level unloading. Run `Automation RunTests LordSimulator` for the complete project regression suite. Verify New Game/Back, western mountain and eastern plain relief, water alignment, WASD/QE movement, right-mouse rotation, wheel speed, M/Escape, cyan current selection and yellow hover in both Editor and Win64 Shipping. Confirm all 100 tile packages and Water dependencies in cooked content before accepting the spec. The loaded tile is a rendering partition of one geographic world; it does not own simulation identity or introduce gameplay eligibility, simulation or saved state.

Spec-011 verification completed on 2026-11-05 with Unreal 5.8.4: all 15 project automation tests passed; all 100 imported levels retained every encoded terrain height and all 885 rivers/4,004 lakes; reloading the saved levels produced 4,889 nonempty native water surface meshes. Win64 Shipping cooking, cooked-content inspection, packaged navigation/rendering, Inno Setup compilation and installed launch/rendering passed. The user confirmed WASD/QE movement, right-mouse rotation and wheel speed adjustment in the installed game.
