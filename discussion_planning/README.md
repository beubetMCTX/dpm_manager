# DPM Manager Discussion Planning

## Purpose

Record product discussions, implementation priorities, unresolved issues, and future ideas for `dpm_manager`.

## Current Baseline

- Qt desktop application with OpenCASCADE visualization.
- DPM file import/export has been reorganized around the newer field-table-driven approach.
- Chemkin import reads the `SPECIES` section and provides species choices.
- Materials are managed independently from Chemkin species.
- Species colors and material data can be edited in dedicated table dialogs and saved in configuration files.
- `unit_edit_dialog` supports synchronized editing of injector data and geometry updates.
- Multiple auxiliary dialogs have close/lifetime handling intended to avoid shutdown crashes.
- Injector geometry preview supports several injector types, with complex models selectively disabled where implementation is incomplete.
- A Release build has been generated with Qt/OpenCASCADE/runtime DLL deployment for testing.

## Candidate Next Work

### High Priority

- Verify the deployed Release package on a clean machine or environment.
- Confirm whether `unit_edit_dialog` opening latency is acceptable in Release mode.
- Audit remaining shutdown, dialog lifetime, and OpenCASCADE selection/dragging edge cases.
- Define a stable configuration format and versioning strategy for Chemkin paths, materials, and species colors.
- Improve validation and user feedback for malformed or unsupported input files.

### Medium Priority

- Complete or refine injector geometry generators that are currently placeholders or disabled.
- Improve synchronization between UI data, injector model data, and displayed OCCT geometry.
- Add undo/cancel semantics for unit editing if direct live modification becomes inconvenient.
- Add a clearer unit-system abstraction instead of storing units only as display strings.
- Add basic automated tests for DPM parsing, Chemkin species parsing, configuration persistence, and numeric expression input.

### Future Ideas

- Add search/filtering to species and materials tables.
- Add import/export for material libraries and species color presets.
- Add geometry preview tests or screenshot-based regression checks.
- Organize source files more strictly by application, model, I/O, visualization, and UI features.
- Add a project/session file that records loaded geometry, DPM units, Chemkin file, materials, and color settings together.

## Design Decisions To Confirm

- Whether unit editing should remain live-synchronized or gain explicit Apply/Cancel behavior.
- Whether configuration files should remain JSON-based or move to a project-oriented format.
- Whether unsupported injector models should be hidden, marked as experimental, or rendered with simplified geometry.
- Whether the Release deployment directory should be packaged with only matching Release DLLs rather than the broader Debug DLL set.
- Which injector types and DPM features are most important for the next development phase.

## Discussion Log

### 2026-08-31

- User proposed creating a dedicated planning folder in the project and recording future discussions there.
- Initial planning document created.
- Next discussion should prioritize the remaining work according to actual usage rather than implementing every currently declared injector feature at once.

### 2026-08-31 Unclosed Items Review

#### Priority 1: Stability and Release Verification

- Run a complete smoke test on the deployed Release executable, especially `unit_edit_dialog` opening time and OpenCASCADE interaction.
- Verify that the Release package uses compatible Qt/OpenCASCADE/runtime DLLs. The current package also contains DLLs copied from Debug, which remains a packaging risk.
- Test shutdown with multiple `unit_edit_dialog`, species/colors, and materials dialogs open, including closing child dialogs after the main window starts closing.
- Test OpenCASCADE selection, dragging, deselection on dialog close, and geometry refresh after editing position, direction, type, and geometric parameters.

#### Priority 2: Input and Persistence

- Test invalid DPM, Chemkin, and unsupported file selections and ensure they report errors instead of terminating the program.
- Add regression coverage for multiple injectors per DPM file, Chemkin `SPECIES` parsing, numeric expressions, and deprecated DPM API routing.
- Verify restoration of the last Chemkin path, species colors, and materials after switching files and switching back.
- Decide whether configuration files need versioning and migration support.
- Confirm duplicate species-color detection, rollback, and save/load consistency.

#### Priority 3: Editor Behavior

- Confirm particle-type-dependent controls for `massless`, `inert`, `droplet`, `combusting`, and `multicomponent`.
- Confirm UI-to-model-to-geometry synchronization while the model is moved externally as well as while fields are edited.
- Decide whether editing remains live or gains explicit Apply/Cancel and undo semantics.
- Formalize the unit system; the interface is prepared, but conversion and validation are not yet complete.

#### Priority 4: Geometry and Product Completeness

- Review simplified or disabled injector geometry models and prioritize those needed by actual DPM workflows.
- Validate `axis + cone_angle` behavior and the ring/hollow-cone parameter relationship.
- Add geometry smoke tests or screenshot-based regression checks.

#### Later Improvements

- Add search/filtering to species and materials tables.
- Add import/export for material libraries and species color presets.
- Continue organizing source files by application, model, I/O, visualization, and UI features.
- Consider a project/session file bundling geometry, DPM units, Chemkin, materials, and colors.

#### Recommended Order

1. Release package and startup/shutdown verification.
2. Multi-dialog and OpenCASCADE interaction stability.
3. Parser and configuration regression tests.
4. Editor synchronization and validation review.
5. Injector geometry completion based on actual usage.

#### Progress In This Pass

- Added a path-based DPM reader entry point so parser tests can run without opening a file dialog.
- Added an I/O smoke-test target covering the multi-injector DPM reader and Chemkin `SPECIES` reader.
- Verified the sample `dpm_3.txt` and `chemkin-import_chem.inp` together with the smoke test.
- Changed auxiliary dialogs to remain owned by their Qt parents and close OCCT editors before main-window UI teardown.
- Changed dynamic property rows to be released synchronously during layout rebuilds.
- Expanded point-property synchronization rows for the implemented atomizer and condensate geometry models.
- Release build succeeds when the VS developer environment is loaded and vcpkg app-local deployment is disabled; the local vcpkg post-build script still references a missing `pwsh.exe` path.

#### Still Needs Manual Verification

- Close the real Release GUI with every auxiliary dialog open and with multiple unit editors open.
- Drag and edit a selected injector while an editor is open, then verify position and geometry updates visually.
- Exercise invalid file selections in the GUI and verify the error path is non-blocking and non-crashing.
- Decide whether the smoke test should later be extended to configuration round-trips and numeric-expression controls.

#### Verification Update

- The Release directory was redeployed with the matching Qt 6.7.3 MSVC2022 `windeployqt`; `platforms/qwindows.dll` is present.
- The previous Qt platform-plugin error was reproduced with `QT_QPA_PLATFORM=offscreen`; that plugin is not deployed, while the normal Windows platform works as expected.
- The Release executable passed a startup smoke test using the Windows platform plugin and remained alive for five seconds.
- Copied the matching vcpkg runtime DLL set into the Release directory and verified startup with an isolated system-only `PATH`; the executable remained alive for six seconds.
- The I/O smoke test and UI component smoke test both completed with exit code 0.
- Added and passed an isolated configuration round-trip smoke test for Chemkin path, materials, and species colors.
- Extended the editor mapping so `surface` uses range fields and the remaining implemented geometry previews expose their active axis, diameter, angle, position, and flow parameters.
- Added an explicit auxiliary-dialog discard path before replacing displayed units, preventing reused editors from retaining pointers to cleared DPM `Unit` objects.
- The remaining validation is interactive: multiple editor shutdown, OCCT dragging/selection, and visual field-to-geometry synchronization.
- Re-ran the self-contained Release checks after the dialog-discard change; I/O, UI component, configuration, and startup checks all returned 0.

### 2026-08-31 Continuation Verification

- Built the latest `unit_editor_smoke_test` target successfully with the Qt 6.7.3 MSVC2022 Release toolchain.
- Ran the four Release checks successfully: DPM/Chemkin I/O, numeric components, configuration round-trip, and unit-editor construction/synchronization all returned exit code 0.
- Added Volume property controls for specification, bounding shape, stream specification, stream counts, mass input, and volume-fraction input; selection changes rebuild the relevant rows through a queued Qt callback.
- Added a Volume editor regression check that changes specification and stream mode and verifies the corresponding `Injector` enum values.
- Replaced the MainWindow DPM import path so production code uses `read_dpm_file(...)` instead of the deprecated dialog-bound reader.
- Corrected deprecation messages for both legacy DPM reader wrappers so they point to `read_dpm_file(file_path, ...)`.
- Verified the deployed Release executable starts and remains alive for five seconds with an isolated system-only `PATH`; the latest runtime log reaches MainWindow initialization checkpoints without an error.
- The screenshot inspected in this pass reported a missing SolveSpace executable and does not correspond to code or dependencies present in this DPM Manager repository.

#### Remaining Evidence Gap

- Automated checks still do not replace manual GUI verification of OCCT dragging, multiple auxiliary windows, and closing the main window while child windows are visible.
- The editor covers the implemented Volume fields, including editable `volume_zones`; a future dedicated zone picker could improve usability but is no longer required for basic synchronization.
- Release packaging still needs a clean-machine verification and an explicit decision about removing non-Release DLLs.

### 2026-08-31 Model Synchronization Audit

- Found and fixed a real data-lifetime gap: edits and OCCT dragging modify the stable `Unit` copy owned by `OCCTWidget`, while `MainWindow::units` could remain stale.
- `MainWindow` now listens for `unit_data_updated` and copies the changed Unit back by UUID.
- Geometry refresh now emits `unit_data_updated` after successful redisplay, so editor fields and the application-level Unit list receive the same update.
- Rebuilt the Release target and reran all four smoke tests plus a Release startup check; all completed successfully.
- Sent a normal Windows `WM_CLOSE` to the deployed Release window; it exited with code 0 and the runtime log contains the complete `MainWindow reset`, `QApplication scope`, and session-close checkpoints.
- Added synchronized dynamic layouts for the Physical Models, Turbulent Dispersion, Parcel, and Wet Combustion tabs, covering the corresponding Injector flags, enums, and numeric parameters.
- Extended the editor smoke test to submit a Parcel expression and verify that `parcel_number` is updated in the model.
- Added editable tabulated diameter-distribution fields, including table name, column indices, and accumulation flags.
- Added the missing wet-combustion `evaporating_material` selector and refresh it when the Materials table changes.
- Extended the editor smoke test to verify a tabulated string field is reflected back into the dialog.
- Added the remaining general injector controls for reference frame, collision/drag settings, angular velocity, vapor pressure, swirl, staggering, rough-wall and continuous-phase options.
- Added unsteady injection timing and cone-angle fields to the Turbulent Dispersion tab.
- Rebuilt Release after the expanded mapping; I/O, component, configuration, and unit-editor smoke tests all returned exit code 0.
- Wrapped the long Physical Models, Turbulent Dispersion, Parcel, and Wet Combustion pages in independent styled scroll areas so the Fluent-style property layout remains usable at normal window sizes.
- Rebuilt Release and reran all four smoke tests successfully after the scroll-area change.
- Added an editable `Volume Zones` integer-list field supporting comma, semicolon, or whitespace separators; invalid input restores the previous valid list.
- Extended the unit-editor regression test to verify Volume zone-list synchronization.
- Added `shutdown_smoke_test`, which creates MainWindow, Species/Materials dialogs, and a Unit Editor, then closes and destroys the main window; the Release test passed.
- The complete Release smoke-test set now includes I/O, UI components, configuration, unit editor, and auxiliary-dialog shutdown coverage, all returning exit code 0.
- Extended the editor regression test to modify Cone Angle, rebuild the OpenCASCADE geometry, and verify an external model update is reflected back into the dialog.

#### Updated Evidence Gap

- The model-tab layouts are code-level verified, but their visual spacing, tab switching, and simultaneous OCCT interaction still require manual GUI verification.
- Fixed a refresh-order edge case so model-tab fields are synchronized even when an injection-type change causes Point Properties to rebuild and return early.
- Rebuilt Release and reran `unit_editor_smoke_test` successfully after the refresh-order fix.

### 2026-08-31 Four-Priority Pass

- Registered the five smoke-test executables with CTest. The parser test is registered when `DPM_SMOKE_DPM_FILE` and `DPM_SMOKE_CHEMKIN_FILE` point to existing files; the other four tests are always registered when enabled.
- Configured the parser test with the Desktop `dpm_3.txt` and the supplied Chemkin file, then ran `ctest`: all five tests passed.
- Added editable `Surface Zone IDs` and `Boundary IDs` fields to the Surface injection page, matching the existing DPM fields and using the same validated integer-list parser as Volume Zones.
- Replaced the File injection informational label with an editable `DPM File` path field so the file source is visible and synchronized with `Injector::dpm_fname`.
- Added a regression check for Surface field write-back.

#### Remaining Evidence Gap

- The four priority areas now have code-level and automated coverage, but real GUI verification is still required for OCCT dragging, simultaneous child windows during shutdown, and visual Fluent-style layout fidelity.
- Release packaging and clean-machine verification remain separate deployment tasks; the current development package still contains a broad dependency set and should be audited before distribution.

### 2026-09-01 Dynamic DPM Editor Audit

- Audited the editor as a stateful Fluent-style form rather than a static list of fields.
- Added conditional model-page rows: stochastic-tracking parameters require stochastic tracking, cloud diameter limits require cloud tracking, rotation parameters require rotation, drag corrections follow the selected drag law, SECO parameters follow the breakup and submodel switches, Parcel parameters follow the parcel model, and wet-combustion fields require evaporating liquid.
- Added a separate model-layout key so external OCCT movement or another editor refreshes the layout when a prerequisite changes, without rebuilding the form on every ordinary coordinate update.
- Added regression coverage for conditional-field appearance/disappearance and Parcel model switching.
- Particle-type switching now clears unsupported diameter-distribution flags, rebuilds Point/Model pages, and hides Wet Combustion for non-droplet particles.
- Programmatic synchronization of reusable combo boxes, checkboxes, and radio groups is now signal-blocked so refreshes cannot masquerade as user edits or trigger recursive layout rebuilds.
- Extended component and editor smoke tests to cover silent control synchronization and non-droplet conditional fields.
- Cross-checked the grouping against Fluent's DPM organization: injection type and point properties, particle/material and diameter distribution, physical models, turbulent dispersion, parcel treatment, and wet combustion remain separate sections.

#### Remaining Evidence Gap

- The dynamic rules are now code-level and test-level verified. Manual visual comparison with the installed Fluent version is still useful for exact labels, default values, and version-specific options because the public web documentation does not expose every dialog branch consistently.

### 2026-09-01 Global Editor Applicability Pass

- Rechecked the editor as a dependency graph rather than a fixed field list: particle type, injection type, diameter distribution, volume branch, cone branch, model switches, and parcel model now each participate in the relevant layout refresh path.
- Added explicit `Massless` filtering for inertial and liquid-only physical-model fields. `Droplet` remains the only particle type exposing vapor pressure, diameter-distribution controls, SECO breakup, and Wet Combustion in this implementation; this matches the current project UI contract.
- Physical Models now show only the stagger branch matching the selected injection family: standard injection staggering for ordinary injections and atomizer staggering for atomizer injections. The separate top-level Stagger Options panel remains visible for every injection type.
- Fixed external particle-type refresh so a stale Rosin-Rammler or tabulated selection is normalized back to the base distribution and the combo box is refreshed at the same time.
- Release build passed after the pass; the historical smoke-test targets were
  subsequently removed from the project to keep CMake focused on the main
  application target.

### 2026-09-01 Smoke-Test Cleanup

- Removed the five legacy smoke-test source files and their CMake targets.
- Removed CTest registration and smoke-test-only cache options from
  `CMakeLists.txt`.
- Future regression coverage should be added as focused tests when a stable
  automated interface is available, rather than restoring the old executables.

#### Fluent Cross-Check Notes

- Public references confirm Linear, Uniform, Rosin-Rammler, Rosin-Rammler logarithmic, and Tabulated as the relevant DPM diameter-distribution families, with availability depending on injection type and particle model.
- Public references confirm stochastic tracking and cloud tracking as prerequisite-driven turbulent-dispersion options, and confirm droplet breakup as a liquid-particle feature.
- Exact field labels and some advanced atomizer/model branches vary by Fluent release. The implementation therefore hides only dependencies supported by both the project contract and the cross-check, while retaining version-sensitive generic fields.

### 2026-09-01 Reference Geometry Follow-up

- Archived the reference-geometry interaction and smoke-test cleanup changes in commit `30136ee` and pushed them to `origin/main`.
- Added selected-face coordinate feedback to the Reference Geometry panel. The panel now displays the selected face origin and normal, and clears both values when the face selection is cleared.
- Release compilation was verified with the configured Visual Studio developer environment; the coordinate feedback change was archived in commit `c23e97d` and pushed to `origin/main`.

#### Next Recommended Increment

- Add a lightweight object list for loaded injectors and reference geometry before implementing undo/redo. This provides reliable selection context and a stable place for future visibility, lock, rename, and delete operations without coupling those features to the 3D context menu.

#### Object List Progress

- Added OCCT-side selection-by-UUID and selection notifications in commit `34dba8a`.
- Added a dockable Objects panel in commit `818abeb`. It lists the loaded reference geometry and injectors and lets the user select them from the list.
- Added incremental injector-name refresh in commit `e6157c8`, avoiding a full list rebuild during ordinary model updates.
- All three commits were pushed to `origin/main`, and the Release target compiled successfully after each code change.
- Added per-object visibility toggles to the Objects panel in commit `9a5ef73`. Visibility changes affect only OCCT display state and safely clear selection when the selected object is hidden.
- Added injector lock and rename APIs in commit `792ef42`, then exposed them through the Objects panel context menu in commit `8bd312d`. Locking prevents 3D movement only; editing and selection remain available.
- Added direct editor opening from the Objects panel in commit `776b23e`, reusing the existing dialog lifetime and duplicate-window protection.
- Added safe injector deletion in commit `1bde00f` and exposed a confirmation action in commit `5c1cdaf`. Deletion closes the matching editor and synchronizes OCCT, MainWindow data, selection, and object-list state.
- Added object-name/UUID filtering to the Objects panel in commit `e99d26b`; list refreshes preserve the active filter.
- Fixed stale object-list highlighting after OCCT selection is cleared in commit `f7b00d0`; drag/release and context-menu cleanup now notify the list.
- Added `Fit All` and `Clear Selection` controls to the Objects panel in commit `17b3223`; these operate on the view/selection state only.
- Added a `View` menu for reopening the Objects and Reference Geometry dock panels in commit `00a01d6`; the reference panel action is disabled until geometry is loaded.
- Added reference-geometry actions to the Objects panel context menu in commit `c2a2992`: Fit All, clear selected face, align view, and lock/unlock.
- Stabilized injector ordering and selection retention during Objects-panel refreshes in commit `80ba5a4` by sorting by name and restoring the selected item when it remains visible.
- Added a first-scope move history in commit `00f70ea` and exposed `Undo Move`/`Redo Move` actions with `Ctrl+Z`/`Ctrl+Y` in commit `a3b6499`. Each complete 3D injector drag is one history entry; editor-field history remains a separate future task.
- Cleared move history on DPM reload and removed entries for deleted injectors in commit `d061b0d`, preventing undo/redo from targeting stale objects.
- Prevented hidden injectors and hidden reference geometry from being selected through the Objects panel in commit `5aa52d3`; management actions remain available.
- Added JSON-backed main-window geometry/dock-state persistence in commits `eb268c2` and `47cd6e5`; existing Chemkin settings are preserved. Release compilation passed. Full drag/dock/restart verification still requires a real interactive desktop session because the current execution environment exposes no GUI window handle.
- Added `Reset Window Layout` to the `View` menu in commit `743c965`, restoring the default size and dock positions and persisting the reset state.
- Added destructor-level layout persistence fallback in commit `c2f389a`, covering application-exit paths that bypass `closeEvent`.
- Added `schema_version: 1` to newly written application, material, and species-color configuration files in commit `69333da`; readers remain backward-compatible with existing files.
- Ensured the application creates `config`, `color_cfg`, and `material_cfg` during startup in commit `2b9b7bd`, even before any import or save operation occurs.
- Added `Show All` and `Hide All` injector visibility controls to the Objects panel in commit `322daa9`; these affect display state only and leave reference geometry unchanged.
- Optimized batch visibility updates in commit `ec99cef` so Show/Hide All redraws the OCCT view once rather than once per injector.
- Fixed `Hide All` so it does not clear an unrelated selected reference face in commit `7109a7e`; only a hidden selected injector clears selection.
- Added double-click editor opening for injector rows in commit `5c6f1a5`; the reference-geometry row is intentionally ignored.
- Implemented injector copy/paste-to-replace data handling in commit `b395c74`, connected it to the 3D context menu in `6ad3e51`, and exposed it in the Objects panel in `bfaf19f`.
- Connected the previously inactive 3D injector Delete action in commit `bcec0ea`; it now uses the same confirmation and safe-removal path as the Objects panel.
- Cleared the in-memory copied injector when replacing the displayed DPM object set in commit `1647b71`, preventing cross-file paste of stale data.
- Made the View-menu dock actions checkable and synchronized with Dock visibility in commit `7cfeaab`, so menu state and close buttons remain consistent.
- Changed reference-geometry double-click behavior in commit `3191360` to select the reference object and open its property Dock; injector double-click behavior remains editor opening.

References:

- https://www.mr-cfd.com/injection-dpm-diameter-distribution-linear-uniform-rosin-rammler-rosin-rammler-logarithmic/
- https://cfdland.com/dpm-injection-types-in-ansys-fluent-direct-methods/
- https://cfdland.com/discrete-phase-model-physical-model/

### 2026-09-01 Configuration Resilience

- Invalid JSON configuration files are now detected with `QJsonParseError` details.
- Before falling back to defaults, the invalid file is renamed to a timestamped `.corrupt-*.bak` backup instead of being overwritten.
- Release compilation passed after the change; commit `dc31075` was pushed to `origin/main`.
- Startup recovery now removes a missing or unparseable saved Chemkin path, preventing the same stale import failure from recurring on every launch.
- Release compilation passed again; commit `3f3cec7` was pushed to `origin/main`.
- Added optional reference-geometry session persistence: the loaded file path, transform, lock state, and visibility are stored in `config.json` and restored on startup.
- The geometry reader now exposes the successful absolute source path and clears it after a failed read.
- Release compilation passed after each step; commits `19f5e50`, `27bb84f`, and `972acec` were pushed to `origin/main`.
- A Release startup check remained alive for six seconds when run with the existing deployed dependency DLL set; the bare build directory correctly fails because it contains no runtime DLL deployment.
- Added versioned `.dpmproj` project-session JSON serialization for all current `Injector` fields, unit UUIDs/types, Chemkin path, materials, and reference-geometry state.
- Session loading rebuilds injector geometry, validates external Chemkin/geometry files before replacement, and clears stale reference geometry when the session has none.
- Added File-menu Open/Save Project Session actions and a safe reference-geometry clear API.
- Release compilation and a six-second deployed-DLL startup check passed; commits `473fc3b`, `3bf33ab`, `85dca0a`, and `8e404cf` were pushed to `origin/main`.
- Interactive save/open verification remains to be performed in the real GUI, especially editing a field, saving, reopening, and comparing the displayed geometry.
- Added File-menu project-session actions in `mainwindow.ui`; opening validates external files before replacing the current model, while saving captures the current edited `Unit` list.
- Added `OCCTWidget::clear_reference_geometry()` so a session without reference geometry cannot leave stale geometry visible.
- Release compilation and a six-second deployed-DLL startup check passed; commits `85dca0a`, `473fc3b`, `3bf33ab`, and `8e404cf` were pushed to `origin/main`.
- Added editor transactions: a unit editor session records one before/after snapshot on close, with separate `Undo Unit Edit` and `Redo Unit Edit` actions (`Ctrl+Shift+Z` / `Ctrl+Shift+Y`).
- Editor history rebuilds the injector geometry and synchronizes the main-window model when applied; stale entries are removed when units are deleted or reloaded.
- Release compilation and a six-second deployed-DLL startup check passed; commits `df0a99f`, `2ddeb1d`, and `449678d` were pushed to `origin/main`.
- Manual GUI verification is still required for the full edit-close-undo-redo cycle.
- Project sessions now include custom Species color mappings; saving uses the live color-dialog map when available and loading refreshes an already-open color dialog while retaining the legacy `color_cfg` fallback.
- Release build and a six-second startup check with deployed dependencies passed; commits `c832cb3` and `7b4ee7b` were pushed to `origin/main`.
- Added focused `project_session_regression` coverage for injector fields/UUIDs, geometry rebuild, Species colors, materials, and reference-geometry state.
- The regression executable passes with the existing deployed dependency directory on `PATH`; a bare build directory returns `0xc0000135` because it has no runtime DLLs.
- Test target and CMake registration were pushed in commit `616634b`; interactive GUI save/open and editor undo/redo verification remains manual.
- Project-session workflow now remembers the active `.dpmproj`, uses `Ctrl+S` for direct save, provides `Save Project Session As`, and marks unsaved model changes with `*` in the window title.
- Importing a new DPM starts a new unsaved session instead of silently overwriting the previously opened project on the next save.
- Release build, project-session regression (`exit 0`), and six-second startup check passed; commit `6f72cdc` was pushed to `origin/main`.
- Closing a dirty project now asks Save/Discard/Cancel; Cancel or a canceled/failed Save keeps the application open. `Ctrl+S` is also bound to the current session save path.
- Release build, project-session regression (`exit 0`), and six-second startup check passed; commit `ba56889` was pushed to `origin/main`.
- Project-session reference geometry is now parsed into a temporary reader before the current scene is replaced, preventing a parse failure from leaving a partially loaded session.
- Release build, project-session regression (`exit 0`), and six-second startup check passed; commit `19bd1fd` was pushed to `origin/main`.
### 2026-09-02 Unit System Foundation

- Added standalone `UnitSystem` definitions and conversion utilities for dimensionless values, length, angle, velocity, mass, mass flow, time, pressure, and temperature.
- Added compatibility and finite-value validation so incompatible or invalid conversions fail without producing data.
- Added `unit_system_regression` coverage for length, angle, temperature, pressure, incompatible units, and non-finite input.
- Registered the module in the main target and CTest; Release build and regression test passed.
- This increment intentionally does not change existing editor bindings. The next increment can opt fields into display-unit to internal-unit conversion safely.

### 2026-09-02 Unit-Aware Line Edit

- Extended `QUI_LineEdit` with opt-in display-unit and storage-unit conversion.
- Numeric expressions are evaluated in the displayed unit, then converted before writing to the bound model value; bound values are converted back when synchronized.
- Existing fields remain unchanged until they explicitly configure a conversion pair.
- Release build passed. `unit_system_regression` and `project_session_regression` passed with the deployed Qt DLL directory on `PATH`.

### 2026-09-02 Field Row Unit Integration

- `QUI_FieldRow` now configures numeric editors from their displayed unit; `mm` fields use `m` as storage unit while existing `m`, `rad`, `deg`, `m/s`, `kg/s`, `Pa`, and `K` fields preserve their current storage semantics.
- Scalar, range, and vector property rows now synchronize and commit through the same unit-aware path.
- Release build passed. `unit_system_regression` and `project_session_regression` both passed with deployed Qt DLLs.

### 2026-09-02 Incremental Editor Refresh

- Ordinary metadata and model-option edits now emit data synchronization without scheduling OCCT geometry rebuilds.
- Injection type, cone type, and geometry-affecting property rows retain geometry refresh behavior.
- Release build passed. `unit_system_regression` and `project_session_regression` both passed with deployed Qt DLLs.
- Interactive timing comparison remains useful because the current automated tests do not expose a real OCCT window.

### 2026-09-02 Configuration Schema Handling

- Configuration readers now normalize missing and older schema markers in memory while preserving existing keys.
- Malformed schema markers are backed up with the same `.corrupt-*.bak` recovery path as malformed JSON.
- Configurations from a newer schema are rejected without being overwritten, protecting settings written by a newer application.
- Release build, `unit_system_regression`, and `project_session_regression` passed.

### 2026-09-02 Unit Preference Persistence

- Added validated `Unit_Preferences` for length, angle, velocity, mass, mass flow, time, pressure, and temperature display units.
- Added JSON persistence under `config.json` with safe defaults when the preference block is absent.
- Invalid or dimension-incompatible unit preferences are rejected without being applied.
- Added default/validation regression coverage; settings UI integration remains the next increment.

### 2026-09-02 Unit Preference Runtime Integration

- Application startup now loads valid unit preferences and activates them before editors are opened.
- `QUI_FieldRow` maps semantic dimensions to configured display units while preserving field-specific internal storage units.
- Length and temperature use base storage units; angle fields preserve their existing degree/radian storage semantics.

### 2026-09-02 Unit Preference Settings UI

- Added a reusable Display Units dialog for length, angle, velocity, mass, mass flow, time, pressure, and temperature.
- Added a Settings menu action that persists confirmed choices and refreshes all open injector editors without changing model storage units.

### 2026-09-02 Release Packaging

- Added `scripts/package_release.ps1` for repeatable Release packaging.
- The script copies the Release executable, all known runtime DLLs, Qt plugin directories, and runs matching `windeployqt` when available.
- It emits a SHA-256 deployment manifest for checking package contents.
- Generated packages under `/release/` are ignored by Git and can be tested locally without polluting the source tree.
- Generated package was started without an external DLL `PATH` override, remained alive for five seconds, then closed through the window message path with exit code `0`.

### 2026-09-02 Reference Geometry Lock-State Synchronization

- Centralized the enabled/disabled state of reference-geometry transform controls.
- Position and rotation editors plus Apply/Reset are now refreshed after project restore, external lock-state changes, and panel synchronization; they no longer depend solely on a checkbox `toggled` signal.
- Locking continues to restrict movement and transform edits only. Face selection, face clearing, and view alignment remain available while locked.
- Release compilation passed. `project_session_regression` and `unit_system_regression` both passed.
- Real mouse interaction with a locked and then unlocked reference geometry still requires manual GUI verification.

### 2026-09-02 Chemkin Import Regression Coverage

- Added `chemkin_io_regression` with temporary-file coverage for a valid multi-line `SPECIES` section, inline comments, missing `END`, and a missing file path.
- The test verifies that invalid imports return no species and a useful error instead of exposing partial data.
- CMake registers the focused test without restoring the removed legacy smoke targets.

### 2026-09-02 Safe Reference-Geometry Import Replacement

- Reference geometry selected from the main window is now parsed into a temporary `Base_Geom_Read` instance.
- The displayed geometry is replaced only after the new file is read successfully, so a canceled, missing, unsupported, or malformed file cannot clear the current scene.
- Project dirty-state and persistence updates still occur only after a successful replacement.

### 2026-09-02 DPM Import Regression Coverage

- Added `dpm_file_io_regression` for missing, empty, and malformed DPM files.
- The test verifies that failed imports return an empty unit list, set `ok` to false, and expose a useful error without showing a modal dialog.

### 2026-09-02 Injector Lock State Feedback

- Added a dedicated injector lock-state signal from `OCCTWidget`.
- The Objects panel now refreshes the affected row immediately when locking or unlocking is performed from the 3D context menu, keeping the `[Locked]` label synchronized across all entry points.

### 2026-09-02 Reference Geometry Reader Regression Coverage

- Added `base_geom_read_regression` for empty paths, missing files, empty files, and unsupported geometry extensions.
- The test confirms that these failures return `false` and preserve a descriptive reader error without requiring a GUI message box.
- The test also round-trips a generated BREP box through the reader and verifies that a non-null shape and absolute source path are returned.

### 2026-09-02 Reference Transform History

- Added independent undo/redo history for reference-geometry transforms.
- Apply/Reset actions and complete 3D reference-geometry drags are recorded as one transform operation.
- Added `Ctrl+Alt+Z` and `Ctrl+Alt+Y` actions so reference-transform history remains separate from injector movement and unit-edit history.

### 2026-09-02 Reference Selection Opens Properties

- Selecting reference geometry from the 3D view or Objects panel now opens and raises its properties Dock automatically.
- The Dock values are refreshed before selection highlighting is updated, so the displayed transform always belongs to the selected reference object.

### 2026-09-02 Clear Reference Geometry From Properties

- Added a confirmed `Clear Reference Geometry` action to the reference-geometry properties Dock.
- The action uses the existing safe clear path and persists the cleared state, including removal of the selected-face coordinate reference and Objects-panel entry.

### 2026-09-02 Reference Geometry Source Path

- Added a read-only source-file field to the reference-geometry properties Dock.
- The absolute path is refreshed after import, project restore, selection, and clear operations, with selectable text and a tooltip for long paths.

### 2026-09-02 Latest Release Package Verification

- Repackaged the current Release executable with the app-local runtime DLL set.
- The self-contained package contains 257 runtime files and passed the repeatable startup/shutdown probe with exit code `0`.

### 2026-09-02 Recent Project Sessions

- Added a `Recent Projects` submenu under File.
- Project session paths are stored in the versioned application configuration, deduplicated case-insensitively, limited to the 10 most recent existing files, and pruned when files disappear.
- A project is added only after a successful load or save; failed opens do not alter the current project or recent-project list.

### 2026-09-02 Auxiliary Dialog Lifetime Coverage

- Added `auxiliary_dialog_lifetime_regression` as a focused offscreen test for simultaneous Species/Colors and Materials dialogs.
- The test verifies that both child dialogs are owned and safely destroyed with their Qt parent, without restoring the removed legacy application-wide smoke target.

### 2026-09-02 Unit Editor Lifetime Coverage

- Added `unit_edit_dialog_lifetime_regression` for construction, close-without-delete-on-close, and parent destruction of a unit editor.
- The test runs offscreen and verifies that the editor remains valid after an ordinary close, then is released safely with its parent.

### 2026-09-02 Reference Lock Status Feedback

- The Objects panel now prefixes the reference-geometry row with `[Locked]` when movement and transform editing are locked.
- Lock and unlock operations refresh the row immediately while preserving visibility state and selection.

### 2026-09-02 Repeatable Release Shutdown Probe

- Added `scripts/verify_release_shutdown.ps1` to launch the deployed Release executable without an external DLL `PATH`, wait for its main window, send a normal `WM_CLOSE`, and require exit code `0`.
- The script fails explicitly on missing executables, startup timeout, shutdown timeout, or non-zero exit, making release lifecycle checks repeatable in future regressions.
