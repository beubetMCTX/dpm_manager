# DPM Manager Discussion Planning

### 2026-09-04 Per-Property Array Follow/Override Editing

- Right-click actions may choose whether the selected child injector follows
  array edits or receives an independent override.
- Prefer separate scopes for physical properties (material, diameter, flow,
  temperature, models) and geometric properties (position, direction,
  rotation, local offset); one child may inherit one scope and override the
  other.
- Suggested actions: follow array edits, edit current child only, and restore
  inheritance while discarding local overrides.
- Child edits should emit semantic change signals to the owning `Unit`. The
  `Unit` validates the change, updates the master or selected override,
  regenerates derived instances when needed, and emits one consolidated data
  or geometry update.
- Store override state explicitly. Rebuilding an array must not erase local
  overrides unless the user restores inheritance.

### 2026-09-04 Reserve Master/Child Injector Array Architecture

- A `Unit` may keep one master injector while linear, mirror, or rotational
  array instances are represented as dynamically generated child injectors.
- The array rule should be stored separately from generated child state. Child
  injectors should be rebuildable derived instances rather than unrelated
  copies of the master data.
- Future synchronization may use Qt signals and slots: a child emits a
  semantic change signal, the owning `Unit` validates and synchronizes the
  master/array state, then emits one consolidated update signal for the view
  and geometry.
- Signal ownership and lifetime must be explicit. Prefer QObject-owned
  controllers or guarded pointers over unmanaged raw-pointer callbacks, and
  use update-source guards or revision IDs to prevent master/child feedback
  loops.
- The flow-rate policy (master total versus per-child flow) and whether array
  expansion is display-only or exported to DPM remain design decisions.

### 2026-09-03 Normalize State At Edit Commit

- All Unit Editor edit paths now normalize the live Injector before emitting
  data or geometry updates, so range ordering, mutually exclusive options, and
  case-dependent fallbacks also apply to direct edits instead of only external
  refreshes.
- No persistence format or project-level fields were added.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Add Shared Unit-Editor Case Context Bridge

- Added `MainWindow -> OCCTWidget -> unit_edit_dialog` case-context propagation.
- New and already-open editors now receive one shared `Unit_Edit_Case_Context`.
- Default `Unknown` values remain permissive until real solver/case metadata is connected.
- Unified Condensate and Brownian Motion gating with the shared heat-transfer prerequisite.
- Restored the Fluent-documented Volume + Tabulated distribution.
- Did not gate File injections by unsteady tracking because Fluent supports both
  steady and unsteady file formats, while Injector currently stores no file-format
  discriminator.
- Ordinary atomizer branches now hide particle initial-position rows; Massless
  atomizer branches retain position-only controls.
- Surface, Volume, and Condensate now disable the Parcel page and normalize
  stale parcel-release settings to Standard; DDPM and steady-case precedence remains unchanged.
- Save-format work remains deferred.

### 2026-09-03 Lock Unsupported Top-Level Stagger Control

- The top-level `Stagger Options` checkbox is now disabled for Surface, Volume, File, and Condensate injections, matching their separate or absent placement mechanisms.
- Unsupported stale stagger flags remain cleared during refresh; supported Single, Group, Cone, and Atomizer controls remain editable.
- Added regression coverage confirming unsupported types cannot re-enable generic staggering.

### 2026-09-03 Restrict Generic Staggering to Supported Injections

- Generic `Stagger Positions` is now exposed only for Single, Group, and Cone injections; Atomizer types retain their atomizer-specific branch.
- Surface, Volume, File, and Condensate injections clear stale generic staggering flags and keep `Stagger Radius` disabled; Surface uses `Random Surface` instead.
- The top-level Stagger panel remains visible for consistent UI navigation, but unsupported controls are locked.
- Added regression coverage for Surface and Volume state cleanup.

### 2026-09-03 Include Random-Eddy State in Model Layout Key

- External refreshes now detect `Random Eddy` changes and rebuild its dependent model rows.
- `Time Scale Constant` therefore relocks correctly when the value changes outside the dialog.
- Added regression coverage for the external-refresh path.

### 2026-09-03 Synchronize Stagger Controls Across Model Pages

- The top-level `Stagger Options` checkbox and the Physical Models stagger checkbox now stay synchronized in both directions.
- Toggling either control immediately refreshes the dependent `Stagger Radius` enablement state.
- Added regression coverage for both control-entry paths; Release build and all 11 CTest regressions passed.

### 2026-09-03 Refresh Random-Eddy Dependency Immediately

- Toggling `Random Eddy` now rebuilds the Physical Models rows immediately.
- `Time Scale Constant` is disabled when Random Eddy is off and becomes editable as soon as it is enabled.
- Added regression coverage for both states.

### 2026-09-03 Normalize Diameter Distribution Flags

- `tabulated`, Rosin-Rammler, and logarithmic Rosin-Rammler flags are now mutually exclusive during external refreshes and legacy-data normalization.
- A logarithmic RR selection automatically restores the parent RR flag; tabulated selection clears both RR flags.
- Added regression coverage for both stale-flag combinations.

### 2026-09-03 Normalize Parcel State for Steady Tracking

- When the case explicitly disables Unsteady Particle Tracking, alternate Parcel Release Methods are reset to `Standard` before the editor rebuilds its controls.
- The Parcel page remains disabled in that context, preventing stale constant-number, constant-mass, or constant-diameter settings from surviving a steady-case switch.
- Added regression coverage for parcel state reset, Dynamic Drag fallback, and page enablement; save-format work remains deferred.

### 2026-09-03 Harden Remaining Numeric Normalization

- RR distribution parameters, cone radii, Stagger radius, Swirl Fraction, cloud diameters, unsteady-file times, and Shape Factor now reject non-finite legacy values before controls or geometry consume them.
- Added regression coverage for NaN and infinity values in these remaining editor-exposed fields.
- Save-format work remains deferred.

### 2026-09-03 Align Volume Input Fields With Fluent

- Volume injections now show exactly one amount field: `Total Flow Rate`, `Total Mass`, or `Volume Fraction`.
- The displayed field follows the mutually exclusive input-mode flags; Massless Volume injections continue to show none.
- DDPM still adds `Packing Limit` while calculating starting points automatically.
- Added regression coverage for all three input modes; Release build and all 11 CTest regressions passed.

### 2026-09-03 Normalize Additional Model Scalars

- Editor-exposed Volume fractions, packing limits, liquid quality, and liquid fraction now stay within `0~1`.
- Time-scale, spray-model, and SECO parameter values now reject non-finite or negative legacy values before controls are rebuilt.
- Atomizer and effervescent half-angle values are clamped to `0~pi/2`.
- Added regression coverage for NaN, infinity, negative, and oversized values; Release build and all 11 CTest regressions passed.
- Project save-format work remains deferred.

### 2026-09-03 Lock File Parcel Release Method

- File injections now normalize to and expose only the Standard Parcel Release Method, matching Fluent's documented automatic behavior.
- DDPM's existing constant-diameter override remains higher priority when a dense discrete-phase domain is active.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Hide Massless Volume Amount Inputs

- Massless Volume injections now hide volume fraction, packing limit, mass input, and volume-fraction input rows.
- Volume geometry and stream-location controls remain available because they define where particles are released.
- This follows Fluent v242's statement that Massless Volume injections require no particle amount input.

### 2026-09-03 Restrict Bounding-Geometry Parcel Specification

- Bounding-geometry Volume injections now expose only `Total Parcel Count`; `Parcel Per Cell` remains available for Zone-based Volume injections.
- Volume stream counts are normalized to at least one before the editor builds the controls.

### 2026-09-03 Align v242 Chemistry Threshold

- Droplet and Combusting particle types both require at least two active chemistry species unless non-premixed or partially premixed combustion is active.
- The threshold follows the current Fluent v242 documentation rather than the older 12.0 guide.

### 2026-09-03 Refine Massless and Two-Dimensional Injection Controls

- Massless Surface injections now expose only surface and boundary selection; their solver-defined positions remain hidden.
- Massless Cone injections retain the documented geometric inputs (position, axis, cone angle, and radius) while hiding velocity and flow inputs.
- Axis inputs for ordinary atomizer injections are hidden in explicitly two-dimensional cases.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Separate Volume Stream Controls

- Volume injections now hide the generic `Number of Streams` control because they use the dedicated `Total Streams` or `Streams Per Cell` settings.
- Surface injections retain the generic stream count, as required by the current Fluent v242 documentation; switching types preserves its value.
- Invalid `numpts` values are normalized to at least one stream before the editor consumes them.
- Release build and all 11 CTest regressions passed.
- Project save-format work remains deferred.

### 2026-09-03 Audit Fluent Injection Dependencies

- Rechecked Fluent v242 conditions for injection types, particle types, diameter distributions, stagger options, particle rotation, Parcel Release Method, Brownian Motion, breakup, rough-wall, and surface-placement options.
- All restrictions that can be determined from the current `Injector` fields or `Unit_Edit_Case_Context` are now represented by visibility, enablement, or normalization rules.
- Remaining documented conditions require project-level data not currently exposed to the Unit Editor, including gravity availability, dynamic-mesh surface eligibility, detailed heat-transfer models, and DEM collision regime.
- Primary reference: Fluent v242 [Setting Initial Conditions for the Discrete Phase](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/flu_ug/flu_ug_sec_discrete_initial.html).
- Dependency overview reference: Fluent v242 [Steps for Using the Discrete Phase Models](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/flu_ug/flu_ug_sec_discrete_use_oview.html).
- No project save-format changes were made.

### 2026-09-03 Normalize External Injector State

- External or legacy data with out-of-range enum values now falls back to safe defaults before UI layout construction.
- Position, velocity, angular-velocity, bounding-geometry, and other editable vectors now clear non-finite components before display or geometry use.
- Added regression coverage for invalid enum and vector data.
- Project save-format work remains deferred.

### 2026-09-03 Use Chemkin Species Count For Case Constraints

- When no explicit chemistry count is supplied, a non-empty Chemkin species list now supplies the count used by Droplet and Combusting prerequisites.
- Replacing the Chemkin list immediately re-evaluates particle-type availability and dependent editor pages.
- Clearing the Chemkin list removes the previous automatic count instead of reusing stale chemistry metadata.
- Explicit case-context counts retain priority over the imported list.

### 2026-09-03 Separate Heat-Transfer Prerequisite

- Added an explicit Heat Transfer case-context state instead of relying only on Energy Equation as a proxy.
- Droplet, Combusting, and Multicomponent become unavailable when either Energy Equation or Heat Transfer is explicitly disabled.
- Unknown Heat Transfer context remains permissive for older callers; added regression coverage for explicit Heat Transfer disablement.
- Added regression coverage for insufficient and sufficient Chemkin species counts.

### 2026-09-03 Lock DDPM-Only Physical Model Fields

- Collision Partner and Continuous Phase Domain now remain visible but are read-only when DPM Domain is none.
- Both fields become editable when a discrete-phase domain is selected; legacy values are preserved while locked.
- Added regression coverage for both domain states.

### 2026-09-03 Gate Advanced Drag Laws By Case Context

- Added Fluent drag-law entries for Grace, Ishii-Zuber, Wen-Yu, Gidaspow, Syamlal-O'Brien, Huilin-Gidaspow, Gibilaro, EMMS, and Filtered.
- Grace and Ishii-Zuber require gravity; dense gas-solid drag laws require a non-none DPM Domain and a non-disabled dense gas-solid context.
- Dense gas-solid drag laws are also restricted to Inert particles in the current model.
- Unsupported loaded laws fall back to Spherical; Unknown context remains permissive for compatibility.
- Existing drag-law enum values remain numerically stable, and DPM parser/output mappings were extended.
- Added regression coverage for gravity and dense-flow gating.


### 2026-09-03 Lock Stagger Radius for Atomizers

- `Stagger Options` remains available for atomizer and solid-cone injections, but `Stagger Radius` is disabled because Fluent derives that region from atomizer/orifice geometry.
- Standard injections such as Group continue to allow editing `Stagger Radius` when spatial staggering is enabled.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Gate Parcel Settings by Unsteady Tracking

- The Parcel page is now disabled when the case context explicitly disables Unsteady Particle Tracking, matching Fluent's documented dependency.
- `Unknown` tracking context remains permissive; DDPM restrictions continue to take precedence.
- Case-context changes now refresh auxiliary tab enablement immediately, including Parcel and Wet Combustion visibility.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Apply 2D Vector Visibility Rules

- In an explicitly two-dimensional case, ordinary position, velocity, axis, and normal vectors now omit their Z component.
- Two-dimensional angular velocity now exposes only the Z component, which is normal to the 2D plane.
- Three-dimensional and `Unknown` contexts retain the existing vector fields.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Lock DDPM Volume Injection Option

- `Volume` remains available when DPM Domain is not `none`; Fluent restricts the Parcel page and Parcel Specification instead.
- DDPM Volume injections now hide `Stream Specification`, `Total Streams`, and `Streams Per Cell`, while exposing `Packing Limit` for non-Massless particles.
- Ordinary Bounding Geometry still exposes a fixed `Total Parcel Count` option, and Zone-based Volume injections retain both ordinary stream specifications.
- Updated regression coverage against Fluent v242; Release compilation and all 11 regression executables passed.

### 2026-09-03 Finalize Condensate, Reference Frame, and DDPM Locks

- Condensate particle type is restricted to Droplet and Multicomponent; unsupported loaded values normalize to Droplet and their radio buttons are disabled.
- Local Reference Frame is omitted for Surface, Volume, and Condensate injections, matching Fluent's unavailable-control rule.
- In a non-`none` DPM Domain, stochastic Eddy Attempts is fixed at `1` and its editor row is locked.
- Added regression coverage for these dependencies; Release compilation and all 11 regression executables passed.

### 2026-09-03 Restrict Condensate and DDPM Controls

- Condensate injections now normalize unsupported particle types to Droplet and disable Massless, Inert, and Combusting choices; Multicomponent remains available.
- Local Reference Frame is no longer created for Surface, Volume, or Condensate injections, matching Fluent's unavailable-control rule.
- Under a non-`none` DPM Domain, stochastic Eddy Attempts is forced to `1` and its row is locked.
- Added Unit Editor regression coverage; Release compilation and all 11 regression executables passed.
- Energy, unsteady-tracking, mesh-dimension, DEM, wall-boundary, and material-reaction prerequisites remain pending because no reliable project-level context exists in the current editor API.

### 2026-09-03 Normalize Injection Ranges and Directions

- Numeric injection ranges for diameter, temperature, and flow rate now reject non-finite values and keep lower/upper ordering.
- Flat-fan phi limits and unsteady atomizer limits now keep start no later than stop.
- Atomizer, cone, and flat-fan normal vectors receive a safe non-zero fallback when legacy data contains invalid vectors.
- Air-blast outer diameter is kept no smaller than inner diameter; atomizer dispersion angle uses the same strict upper bound as cone angle.
- No persistence fields or file formats changed.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Normalize Volume Bounding Geometry

- Volume bounding-geometry radius and cone angle now reject non-finite values and are clamped to non-negative ranges.
- Bounding-box, cylinder, and cone min/max coordinates are normalized component-wise so the lower bound is not greater than the upper bound.
- The existing data/session format is unchanged; this only normalizes in-memory editor state before rebuilding the UI or geometry.
- Added Unit Editor regression coverage; Release compilation and all 11 regression executables passed.

### 2026-09-03 Align Injection Limits With Output Validation

- Cone Angle is clamped below `180` degrees, and hollow/solid/ring cone outer radii are kept positive.
- Curved Volume bounding geometries keep a positive radius, matching DPM output validation.
- Parcel number, parcel mass/diameter, SECO table count, shape factor, and Cunningham correction receive basic valid-range normalization.
- Added regression coverage for the Cone Angle limit; Release compilation and all 11 regression executables passed.

### 2026-09-03 Align Surface, Condensate, and Atomizer Dependencies

- Surface `Scale By Area` and `Random Surface` now follow Fluent's explicit mutual-exclusion rule.
- Surface `Use Face Normal` now switches Point Properties between velocity components and velocity magnitude, with immediate UI rebuild.
- Condensate injections now expose only Start Time and Stop Time; unsupported diameter distributions are cleared automatically.
- Atomizer point-property panels were aligned with Fluent's documented common and model-specific parameters, including appropriate numeric ranges.
- Direct changes to DPM Domain, Drag Law, Parcel Model, Rotation, and Face Normal now rebuild dependent controls immediately.
- Added regression coverage; Release compilation and the focused Unit Editor and injector-geometry tests passed.

### 2026-09-03 Validate File Injection Time Range

- File injection start/stop and flow-time fields now reject negative values.
- Unsteady Stop is normalized to be no earlier than Unsteady Start.
- Added regression coverage for invalid file-injection timing values.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Clear Unsupported Particle Fields

- Switching particle type now clears Material when the type does not use a material.
- Evaporating Species is cleared outside Droplet; devolatilizing, oxidizing, and product species are cleared outside Combusting.
- This keeps disabled controls from leaving stale values in the injector data.
- Added regression coverage for unsupported-field cleanup.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Validate Ring Radius and Swirl Sign

- Ring-cone inner radius is normalized to the valid range below the outer radius.
- Hollow-cone Swirl Fraction now accepts the Fluent-supported `-1~1` range, including negative values for reverse swirl direction.
- Added regression coverage for invalid ring radii and negative swirl fraction.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Validate Diameter Distribution Parameters

- Rosin-Rammler limits are normalized to non-negative values with `min <= mean <= max`.
- Spread is kept positive and diameter count is at least one.
- Tabulated diameter, number-fraction, and mass-fraction column indices now require one-based positive values.
- Added regression coverage for invalid Rosin-Rammler input.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Normalize Cloud Diameter Bounds

- Cloud Tracking now clamps both diameter limits to non-negative values and ensures the maximum is not below the minimum.
- Added regression coverage for externally loaded invalid cloud limits.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Validate Stagger Radius

- Stagger Radius now accepts only non-negative numeric values.
- Invalid negative values loaded from old data are normalized to zero.
- The radius editor remains locked until Stagger Options is enabled.
- Added regression coverage for normalization and disabled-state behavior.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Restrict Hollow-Cone Swirl Fraction

- Swirl Fraction is now shown only for non-Massless hollow-cone injections, matching Fluent's cone-specific parameter definition.
- Switching to another injection or cone type clears stale swirl-fraction data.
- Added regression coverage for hollow versus ring cone behavior.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Match Cone Point-Property Requirements

- Point-cone injections now hide radius fields because their release plane is a point.
- Hollow-cone injections expose only Outer Radius; Inner Radius is reserved for ring-cone injections.
- Cone Type changes now rebuild both point properties and dependent physical-model rows.
- Added regression coverage for point, hollow, and ring cone field availability.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Limit Massless Point Properties

- Fluent obtains Massless particle velocity from the continuous phase, so direct velocity, diameter, temperature, and flow-rate inputs are omitted where they are not meaningful.
- Single pages keep only position fields; Cone pages retain the documented position, axis, cone angle, and radius geometry inputs.
- Group pages keep first/last position fields, while Surface pages keep surface selection only.
- Uniform Massflow Distribution is now cleared and hidden for Massless cone injections.
- Added regression coverage for Massless Single, Group, and Surface layouts.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Enable Diameter Distributions for Physical Particles

- Fluent's Rosin-Rammler and tabulated distributions depend on injection type; they are not limited to Droplet particles.
- Inert, Droplet, Combusting, and Multicomponent particles now retain diameter-distribution controls when their injection type supports them.
- Massless particles continue to clear and disable diameter distributions because they have no particle diameter.
- Updated particle-type regression coverage.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Link Wet Combustion to Evaporating Species

- Fluent exposes Evaporating Species for a Combusting particle only when Wet Combustion is enabled.
- The editor now clears and disables that species selection when the liquid option is off, and restores it when enabled.
- Added regression coverage for both states.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Make Volume Input Modes Mutually Exclusive

- Fluent volume injections use one input basis: flow rate, mass, or volume fraction.
- Mass Input and Volume Fraction Input now normalize conflicts, lock the alternative while selected, and unlock it when disabled.
- Massless volume injections clear both input-mode flags because no volume input is required from the user.
- Added regression coverage for normalization and both interaction directions.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Superseded DDPM Volume Assumption

- Earlier work incorrectly treated a non-`none` Discrete Phase Domain as a reason to reject Volume injections.
- Fluent v242 instead keeps Volume available and removes Parcel Specification controls, using the Injection Packing Limit to calculate starting points.
- The implementation and regression coverage were corrected in the later `Lock DDPM Volume Injection Option` entry above.

### 2026-09-03 Clear Inertial State for Massless Particles

- Fluent massless particles do not use the inertial physical-model group.
- Switching to Massless now clears Rotation, Rough Wall, Drag Law, and Brownian Motion state instead of leaving stale hidden settings.
- Added regression coverage for the state transition.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Restrict Dynamic Drag Availability

- Fluent exposes Dynamic Drag only when a droplet breakup model is active.
- The editor now requires Droplet + SECO Breakup + one selected SECO model before exposing Dynamic Drag.
- Unsupported loaded states normalize back to Spherical drag; the combo uses explicit enum values so filtering does not shift selections.
- The unsteady-tracking prerequisite remains unmodeled because this project has no corresponding global field.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Enforce DDPM Parcel Dependency

- Fluent disables the Parcel page when a non-`none` Discrete Phase Domain is selected.
- The editor now disables that tab and normalizes the parcel release method to `constant-diameter`.
- Leaving the dense discrete-phase domain re-enables the Parcel page without changing the saved domain value.
- Added regression coverage for both directions of the transition.
- Release compilation and all 11 regression executables passed.

### 2026-09-03 Add Numeric Model Constraints

- Added expression-aware numeric range validation to Unit Editor model and injection rows.
- Enforced non-negative physical quantities, `0~1` fractions, `0~180` degree cone angles, positive stream/count fields, and `0~1` nonspherical shape factors.
- Invalid values keep the last valid value and use the existing `QUI_LineEdit` rejection state; arithmetic expressions remain supported.
- Added regression coverage for rejecting an invalid shape factor and accepting `1/2`.
- Release compilation and all 11 regression executables passed with the MSVC2019 runtime environment.

### 2026-09-03 Constrain Surface and Cone Distribution Options

- Fluent limits Uniform Massflow Distribution to solid-cone and ring-cone injections; other cone types now clear and hide it.
- Fluent marks Randomize Starting Points and Scale Flow Rate by Face Area as incompatible for surface injections.
- The editor now normalizes conflicts, locks the alternative while one option is active, and restores it when disabled.
- Added regression coverage for both interaction groups and injection-type transitions.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Verify Tabulated Diameter Distribution Scope

- Fluent v242's dedicated tabulated-distribution section limits `tabulated` to Cone and Surface injections.
- Volume retains only its fixed/uniform and Rosin-Rammler choices; stale tabulated state is cleared on refresh.
- Updated Unit Editor regression coverage for the documented availability matrix.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Stabilize Dynamic Model Editor Callbacks

- Dynamic model combo callbacks now capture stable injector pointers instead of references to local aliases.
- This prevents delayed refresh, selection, and model-layout synchronization callbacks from using dangling stack references.
- Volume branch, rotational laws, drag law, Parcel model, and turbulent-dispersion callbacks are covered.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Normalize Model Dependencies Before Every Editor Refresh

- Model dependency normalization now runs before layout-key comparison, so externally changed flags cannot bypass validation when the visible layout key is unchanged.
- Brownian, SECO, dynamic-drag, and stochastic/cloud constraints remain synchronized during both rebuilds and lightweight refreshes.
- Added regression coverage for external Brownian state correction and Madabhushi dynamic-drag coupling.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Constrain Brownian Motion by Drag Law

- Fluent documentation requires Brownian Motion to use the Stokes-Cunningham drag law and a configured energy model.
- Because this project has no global energy-equation field in `Injector`, the editor enforces the available local dependency: Brownian Motion is cleared and disabled for other drag laws.
- Switching to Stokes-Cunningham restores the Brownian control without changing the stored format.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Make SECO Breakup Models Mutually Exclusive

- Fluent documentation defines `Breakup Model` as one selection among TAB, Wave, KHRT, SSD, Madabhushi, and Schmehl rather than parallel switches.
- The editor now normalizes legacy multi-selected SECO flags in that order and keeps only the first selected model.
- Selecting one model locks the other model rows; disabling it unlocks them, and each model exposes only its own parameters.
- Disabling SECO Breakup clears stale submodel flags.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Make Turbulent Dispersion Models Mutually Exclusive

- Stochastic Tracking and Cloud Tracking are now treated as mutually exclusive model choices.
- Loading legacy data with both flags enabled keeps Stochastic Tracking and clears Cloud Tracking deterministically.
- The active model locks the other model's row; disabling it restores the other choice.
- Dynamic parameters remain scoped to their selected model and rebuild asynchronously after a toggle to avoid deleting the signal sender during delivery.
- Release build and all 11 CTest regressions passed.

### 2026-09-02 Treat Missing Reference Geometry as Optional

- A settings file without `reference_geometry` is now treated as a normal first-run state.
- The loader preserves the caller's existing value and does not create a corrupt-config backup when the optional section is absent.
- Malformed present sections continue to be rejected and backed up.
- Release build and all 11 CTest regressions passed.

### 2026-09-02 Strict Material Configuration Parsing

- Material configuration loading now requires a `materials` array, string material names, and finite numeric densities.
- Malformed material files are rejected and backed up without clearing the caller's existing material list.
- The existing material configuration format is unchanged.
- Release build and all 11 CTest regressions passed.

### 2026-09-02 Make Configuration Reads Atomic

- Configuration readers now parse into temporary values and update caller-owned state only after validation succeeds.
- Failed or missing Chemkin-path, recent-project, window-state, unit-preference, reference-geometry, and species-color reads no longer clear or partially overwrite existing values.
- Added regression coverage for preserving pre-existing values after malformed configuration reads.
- Release build and all 11 CTest regressions passed.

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

### 2026-09-02 Reference Geometry Face Selection Isolation

- Reference geometry face picking now uses an object-level, single active selection mode.
- Switching injector selection modes can no longer leave stale modes active on the reference object.
- The reference geometry lock remains a movement lock only; it does not disable face selection.
- Release build and all 9 CTest regressions passed. Real OCCT mouse interaction still needs manual verification.

### 2026-09-02 Fit Selected Object

- Added a `Fit Selected` view command backed by OCCT `FitSelected`.
- Exposed it in the object-list toolbar and object context menus for injectors and reference geometry.
- The command changes only the camera view and does not rebuild displayed geometry.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Unit Editor Apply and Cancel

- Added explicit `Apply and Close` and `Cancel Changes` actions to the unit editor.
- Existing window-close behavior still keeps live edits; explicit cancellation restores the transaction snapshot and rebuilds the affected injector once.
- Added a lifetime regression check for the new cancel action and signal.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Track Reference Geometry Changes

- Reference geometry transformations now mark the project dirty when changed through OCCT interaction.
- Startup restoration is protected from dirty tracking, so loading saved reference geometry does not appear as a new unsaved edit.
- Configuration files are not written on every mouse-move event; the existing save paths remain responsible for persistence.

### 2026-09-02 Make Object-List Selection Deterministic

- Object-list selection now uses OCCT `SetSelected` instead of a toggle operation.
- Repeated selection cannot accidentally deselect an injector or reference geometry because of stale OCCT selection state.
- This also makes `Fit Selected` consistent when invoked from the object list.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Add Standard View Selector

- Added Top, Front, Right, Back, Left, Bottom, and Isometric camera orientations to the object panel.
- Standard view changes affect only the OCCT camera and preserve injector/reference geometry data.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Add Object Metadata Tooltips

- Object-list entries now show injection type, particle type, and UUID in their tooltips.
- Display names, sorting, visibility, and selection behavior remain unchanged.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Make Fit-Selected Toolbar Deterministic

- The object-panel `Fit Selected` button now resolves the current list row before fitting.
- Keyboard or programmatic list selection therefore works the same as mouse selection.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Reset Reused Unit Editors

- Reopening a previously closed unit editor now starts a fresh edit transaction and refreshes its controls.
- Explicit cancellation clears the editor's modified flag before closing.
- Assigned runtime editor controls now have stable object names for regression tests and UI automation.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Add Reference Geometry Tooltip

- The reference-geometry object entry now shows its source file, visibility, and lock state in a tooltip.
- The list label, sorting, and selection behavior remain unchanged.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Add Escape Selection Reset

- Pressing `Esc` in the OCCT view now clears the current face/object selection and stops active dragging state.
- The shortcut does not modify injector or reference geometry data.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Finalize Drag State on Escape

- Clearing selection now finalizes an active reference transform or injector move transaction before resetting interaction state.
- Pressing `Esc` during a drag no longer leaves a stale transaction for the next drag operation.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 DPM Export Preflight

- Added `validate_dpm_units()` to validate the complete injector list before showing the save dialog.
- The preflight reports all invalid units in one message, while `write_dpm_file()` keeps the same validation as a second safety check.
- Added regression coverage for valid lists, empty names, invalid names, invalid numeric values, and non-injector units.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Portable Project Paths

- Project sessions now store same-volume Chemkin and reference-geometry paths relative to the session file.
- Relative paths are resolved to absolute runtime paths when loading; legacy absolute paths and cross-volume paths remain supported.
- Added round-trip coverage for relative path storage and runtime resolution.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Project Reference Preflight

- Added project-session reference validation for injector materials and Chemkin species.
- Saving and loading now report all missing references before project data is applied or written.
- Case differences in material and species names are accepted consistently with the existing tables.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 DPM Geometry Semantic Validation

- Extended DPM export preflight beyond finite-value checks for cone and volume geometry.
- Zero cone axes, invalid cone angles, inverted ring-cone radii, non-positive hollow/solid radii, and invalid curved volume radii are now rejected with specific messages.
- Added regression coverage for invalid ring-cone radii.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Track Species Color Edits in Project State

- Species color edits now notify `MainWindow` and mark the project dirty.
- Closing the application after an unsaved color change can now participate in the existing save/discard prompt, and project-session saving can capture the latest colors.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Protect Unsaved Projects Before Replacement

- DPM import and project opening now use the same Save/Discard/Cancel confirmation as application shutdown.
- The prompt appears only after the selected DPM file has parsed successfully, so invalid files and canceled file dialogs do not interrupt the current project.
- The close path now shares one confirmation implementation, reducing divergence between replacement and shutdown behavior.

### 2026-09-02 Make Replacement Discard Explicit

- When replacing a project, choosing `Discard` now reloads the last saved project before continuing if a session file exists.
- Unsaved temporary projects are explicitly described as being abandoned; the dialog no longer silently implies that every auxiliary state has been reverted.
- If restoring the saved project fails, the replacement is canceled to avoid a partially restored state.

### 2026-09-02 Require Unique DPM Injector Names

- DPM export preflight now rejects duplicate injector names case-insensitively.
- The later duplicate is reported with its list position, while the first occurrence remains the reference for diagnostics.
- Added regression coverage for duplicate names.

### 2026-09-02 Reject Duplicate Names During DPM Import

- DPM parsing now rejects duplicate injector names case-insensitively before exposing any partial unit list.
- Import and export now enforce the same name-uniqueness invariant.
- Added regression coverage for a duplicate-name input file.

### 2026-09-02 Validate DPM Semantics During Import

- Parsed DPM unit lists now pass the same semantic validation used by export before they are returned.
- Invalid cone and volume geometry is rejected without exposing partial units.
- Added regression coverage for an invalid ring-cone import.

### 2026-09-02 Fix Preflight Error Formatting

- Corrected project and DPM preflight summaries to use real line breaks instead of displaying literal `\\n` text.
- Validation behavior is unchanged; only diagnostic readability was corrected.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Back Up Semantically Invalid Configurations

- Material and species-color configuration files that contain valid JSON but invalid content are now backed up as `.corrupt-*.bak` before rejection.
- Invalid material entries and invalid color values no longer disappear silently during recovery.
- Added regression coverage for material and species-color config backup behavior.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Report Unsupported DPM Fields

- The field-table-driven DPM reader now optionally reports unknown Fluent fields while continuing to import supported data.
- The main window logs these warnings and shows a status-bar summary instead of silently discarding them.
- Added regression coverage confirming unsupported fields do not abort a multi-injector import.

### 2026-09-02 Enforce Unique Names in Project Sessions

- Project-session validation now rejects duplicate injector names case-insensitively, matching DPM import/export behavior.
- Added regression coverage using distinct UUIDs with the same injector name.

### 2026-09-02 Reject Malformed Project Structure

- Project-session loading now requires a valid `units` array and validates the JSON types of optional top-level arrays/objects and the Chemkin path.
- Missing or incorrectly typed structural fields are rejected before any session data is exposed.
- Added regression coverage for a missing units array.

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

### 2026-09-02 Numeric Input Regression Coverage

- Added `qui_line_edit_regression` for expression evaluation, invalid-input rollback, integer-result validation, and display/storage unit conversion.
- Covered `5+6`, degree-based `sin(30)`, incompatible unit rejection, and protection of bound values after rejected input.
- Release build and the focused regression test passed.

### 2026-09-02 Project Session Save Validation

- Added `project_session::validate` and invoked it before writing `.dpmproj` files.
- Rejected empty or duplicate unit UUIDs, invalid unit types, empty or duplicate material names, non-positive/non-finite material densities, and invalid Species color entries.
- Extended `project_session_regression`; Release build and focused session/input tests passed.

### 2026-09-02 Strict Project Session Loading

- Project loading now rejects malformed material and Species color entries instead of silently skipping them.
- Parsed session data is validated before replacing the caller's current data.
- A failed load therefore preserves the existing session and avoids exposing partial state.
- Release build and focused project-session/input regressions passed.

### 2026-09-02 DPM Writer Round-Trip

- Added `write_dpm_file(...)` using the same field names and S-expression structure consumed by the current DPM reader.
- The writer supports multiple injector blocks, atomic file replacement, enum serialization, vectors, and integer lists.
- Added round-trip coverage: write two injectors, read them back, and verify names, positions, and enum values.
- Release DPM I/O regression passed after correcting the existing `madahushi` member spelling.

### 2026-09-02 DPM Export Menu

- Added `Save DPM File` to the main File menu.
- The action exports the current edited injector list through `write_dpm_file(...)` and reports cancel, empty-list, and write failures without replacing the current scene.
- Release application build and all 8 regression tests passed.

### 2026-09-02 DPM Export Preflight

- DPM export now rejects invalid enum values, non-finite scalar/vector values, empty injector lists, and invalid injector names before opening the output file.
- Added regression coverage for `NaN` and empty-list rejection.
- Focused DPM I/O regression passed.

### 2026-09-02 DPM Export Unit-Type Guard

- DPM export now rejects non-injector `Unit` entries instead of serializing them as injectors.
- Added regression coverage for spacer-unit rejection.

### 2026-09-02 DPM Export Atomicity Coverage

- Extended DPM I/O regression coverage to verify that a failed export leaves an existing output file unchanged.

### 2026-09-02 Species and Material Filters

- Added case-insensitive filter fields to Species/Colors and Materials dialogs.
- Filtering hides non-matching rows without changing stored Species colors or material entries.
- Extended auxiliary-dialog regression coverage for filter controls and row visibility.

### 2026-09-02 Numeric Range Validation

- Added optional numeric ranges to `QUI_LineEdit`.
- Materials density editors now reject non-positive and non-finite values with the existing invalid-input rollback behavior.
- Added range acceptance/rejection regression coverage.

### 2026-09-02 Filtered Selection Safety

- Species/Colors now clears the active row when filtering hides it, preventing the color picker from editing an invisible Species entry.
- Added regression coverage for hidden-selection clearing.

### 2026-09-02 Material Configuration Validation

- Added shared material-entry validation for the standalone `material_cfg` file.
- Material configuration now rejects empty/duplicate names and non-positive/non-finite densities instead of silently skipping invalid rows.
- Failed loads preserve the caller's existing material list.
- Added `app_config_regression` coverage.

### 2026-09-02 Shared Material Validation

- Project sessions now reuse the same material validation routine as `material_cfg`.
- This prevents project and standalone material files from accepting different data rules.

### 2026-09-02 Shared Validation Linkage

- Project-session validation now reuses the public Materials validation routine instead of duplicating rules.
- Added the required `UnitSystem` implementation to the project-session regression target after the shared configuration dependency was introduced.
- Focused project-session and configuration regressions passed.

### 2026-09-02 Repeatable Release Shutdown Probe

- Added `scripts/verify_release_shutdown.ps1` to launch the deployed Release executable without an external DLL `PATH`, wait for its main window, send a normal `WM_CLOSE`, and require exit code `0`.
- The script fails explicitly on missing executables, startup timeout, shutdown timeout, or non-zero exit, making release lifecycle checks repeatable in future regressions.

### 2026-09-02 Reference Face Selection Mode Recovery

- Injector selection can change the active OpenCASCADE selection mode globally, leaving reference-geometry faces unpickable afterward.
- Added an explicit face-selection-mode restoration before left-click and context-menu detection.
- Reference-geometry locking still only prevents dragging and transform edits; face selection and view alignment remain available.
- Release build passed and all 9 CTest regressions passed. Desktop verification of lock/unlock, face picking, and alignment remains required because the current automated tests run offscreen.

### 2026-09-02 Remove Duplicate Geometry Refresh Synchronization

- Geometry edits already publish `unit_data_updated` before the deferred OCCT rebuild.
- Removed the second identical signal from `refresh_unit_visual()` to avoid redundant editor synchronization and main-window updates.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Coalesce Burst Geometry Rebuilds

- Kept injector data synchronization immediate while adding a 30 ms coalescing window to the expensive OCCT geometry rebuild.
- Multiple geometry-field commits in a short burst now rebuild from the latest data once instead of rebuilding for each commit.
- Release build and all 9 CTest regressions passed. Desktop verification should confirm that the short delay remains visually imperceptible.

### 2026-09-02 Guard OCCT Widget Events During Teardown

- Added null-handle guards to paint, mouse, and wheel event handlers.
- Late Qt events during OCCT view/context teardown are now ignored instead of dereferencing released native handles.
- Release build and all 9 CTest regressions passed. A real desktop close-while-interacting test remains desirable.

### 2026-09-02 Preserve DPM File Names During Import

- The field-table-driven parser no longer replaces `dpm-fname` with the default blank value.
- Non-empty DPM file names now survive writer/import round trips.
- The deprecated legacy reader path was left unchanged.
- Added DPM round-trip coverage; Release build and all 9 CTest regressions passed.

### 2026-09-02 Strict DPM Enum Validation

- Replaced substring-based enum parsing with exact matching plus explicit compatibility aliases.
- Invalid values such as `not-a-cone` are rejected instead of being silently interpreted as valid enums.
- Propagated the `show_error_message_box` option through the new parser so headless callers do not create widgets.
- Preserved detailed field-level parse errors instead of replacing them with a generic block error.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Case-Insensitive Chemkin Species Deduplication

- Chemkin species collection now treats names differing only by case as duplicates.
- The first spelling is preserved for display and later duplicates are omitted.
- Added regression coverage for duplicate species across multiple lines.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Reject Non-Finite DPM Values on Import

- The field-table-driven DPM parser now rejects `NaN` and infinite values at numeric conversion time.
- This prevents invalid coordinates, velocities, temperatures, and geometry parameters from reaching model or OCCT code.
- Added regression coverage for a non-finite imported temperature.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Accept Quoted Fluent Enum Values

- DPM enum parsing now accepts optional double quotes used by Fluent output files.
- Strict matching remains enabled after quote removal, so malformed substrings are still rejected.
- Added round-trip coverage for quoted drag-law values.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Reject Unbalanced DPM Imports

- Added a structural validation pass before DPM block parsing.
- Parentheses inside quoted strings and escaped characters are ignored correctly.
- Truncated or malformed DPM files now fail safely with an explicit error instead of entering partial parsing.
- Added regression coverage for an unbalanced DPM file; the Release build and all 9 CTest regressions passed.

### 2026-09-02 Clear Stale OCCT Reference Selection Modes

- Reference-face recovery now deactivates every selection mode on the reference geometry before enabling `TopAbs_FACE`.
- This prevents global injector selection modes such as `COMPOUND` from remaining active and interfering with face picking after injector selection or lock/unlock operations.
- Release build and all 9 CTest regressions passed; interactive OCCT mouse verification remains required.

### 2026-09-02 Validate Project Reference Transforms

- Project-session validation now rejects non-finite reference-geometry position and rotation values before they reach OCCT.
- Added regression coverage for a `NaN` transform component.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Reject Malformed Reference Transform Arrays

- Project-session loading now checks the return value of reference position and rotation array parsing.
- A loaded reference geometry with missing or incorrectly sized transform arrays is rejected instead of silently becoming a zero transform.
- Added regression coverage for a two-component position array; Release build and all 9 CTest regressions passed.

### 2026-09-02 Avoid Redundant Combo-Box Rebuilds

- `QUI_ComboBox::set_options()` now compares the incoming labels with the existing options before clearing and repopulating the widget.
- External unit-data synchronization therefore preserves unchanged combo-box contents without rebuilding the selection or emitting extra selection changes.
- Added regression coverage for unchanged options; Release build and all 9 CTest regressions passed.

### 2026-09-02 Preserve Geometry on Failed Rebuilds

- `Injector_OCCT::create_injector()` now keeps the previous compound when a geometry generator fails.
- Invalid edits can no longer silently replace a valid displayed injector with an empty shape; callers can report the failure while retaining the last valid preview.
- Added regression coverage for a failed rebuild caused by a zero velocity; Release build and all 9 CTest regressions passed.

### 2026-09-02 Keep Debug Runtimes Out of Release Packages

- Release packaging now removes stale `*_debug.dll` files from the output directory before copying dependencies.
- Dependency copying excludes Debug-suffixed DLLs and the final package is checked for any remaining Debug runtime.
- Verified that the Release executable dependency list does not require the removed Debug TBB chain; startup/shutdown verification remains part of the packaging check.

### 2026-09-02 Rebuild Release Packages from a Clean Directory

- The packaging script now clears the generated output directory before copying the executable and dependencies.
- A filesystem-root safety check prevents an accidental broad deletion when a custom output path is supplied.
- This prevents stale logs, plugins, and removed dependencies from leaking into later Release packages.

### 2026-09-02 Make CMake Dependency Paths Overridable

- OpenCASCADE and VTK paths are now CMake Cache variables instead of fixed non-overridable assignments.
- Existing defaults preserve the current development environment, while another machine can provide its own paths during configuration with `-D...` options.

### 2026-09-02 Make Advanced Atomizer Preview Configurable

- Added the `DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW` CMake option for the experimental complex atomizer previews.
- The default remains `OFF`, preserving the stable simplified preview; developers can opt in with `-DDPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW=ON` without editing source code.

### 2026-09-02 Reject Malformed Project Injector Vectors

- Project-session loading now propagates vector parsing failures for injector fields such as `pos`, `vel`, and `axis`.
- Incorrectly sized vector arrays are rejected before injector geometry creation instead of silently retaining default values.
- Added regression coverage for a malformed injector position array; Release build and all 9 CTest regressions passed.

### 2026-09-02 Persist Unit Preferences in Project Sessions

- Project sessions can now store and restore the active display-unit preferences alongside the model data.
- Older sessions without the optional `unit_preferences` object remain compatible and keep the current global preferences.
- Unit preferences are validated on save and load; incompatible settings are rejected before partial project data is exposed.
### 2026-09-02 Project Dirty-State Fingerprint

- Added a project-session fingerprint built from editable project data rather than runtime geometry handles or save timestamps.
- MainWindow now compares the current project snapshot with the last saved/loaded snapshot, so Undo/Redo can clear the dirty marker when the saved state is restored.
- DPM imports intentionally start a new unsaved baseline and remain dirty until saved.
- Added regression coverage proving identical snapshots match and edited injector data changes the fingerprint.
- Release build and all 9 CTest regressions passed with the required runtime DLL paths.
### 2026-09-02 Project Validation Report

- Added an Edit-menu `Validate Project` action.
- The report reuses DPM unit validation, project structure validation, and material/species reference validation before export or calculation.
- Validation failures are shown in a warning dialog and the status bar; successful validation reports unit, material, and species-color counts.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Reject Species References Without Chemkin

- Project reference validation now rejects non-empty species fields when no Chemkin file is loaded.
- This prevents stale species references from surviving after the Chemkin source is cleared.
- Added regression coverage for the no-Chemkin case.
### 2026-09-02 Batch Object Visibility and Lock Controls

- Object list selection now supports selecting multiple injector rows.
- Added batch `Show Selected`, `Hide Selected`, `Lock Selected`, and `Unlock Selected` controls.
- Reference geometry is intentionally excluded from batch injector operations.
- These controls only change view/interaction state and do not alter the deferred project save format.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Export Diagnostic Report

- Added a Settings-menu `Export Diagnostics` action.
- The report records application/runtime environment, project summary, Chemkin and reference-geometry state, and the current runtime log.
- Reports use atomic file replacement so an interrupted export does not leave a partial report.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Record Rename and Paste in Edit History

- Object-list rename and paste operations now enter the existing Unit Edit undo/redo history.
- Failed paste geometry creation restores the target unit's original type and data instead of leaving a partial update.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Undo and Redo Deleted Injectors

- Deleting an injector now records its UUID, type, and complete injector data.
- Added `Undo Delete` and `Redo Delete` actions with dedicated shortcuts.
- Undo recreates the OCCT display object and synchronizes the restored unit back to MainWindow.
- Failed or replayed deletions do not recursively corrupt the delete history.
### 2026-09-02 Preserve Deleted Injector View State

- Delete history now preserves injector visibility, movement lock, and display color.
- Undo Delete restores those view states instead of resetting them to defaults.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Batch Delete Selected Injectors

- Added a `Delete Selected` action to the multi-selection object panel.
- Reference geometry is excluded from this operation.
- Each deleted injector is recorded separately, so existing Delete Undo/Redo can restore them individually.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Open Config Folder

- Added a Settings-menu `Open Config Folder` action.
- The action ensures the config, color, and material directories exist before opening them in the system file manager.
- Existing configuration formats and paths are unchanged.
### 2026-09-02 Open Logs Folder

- Added a Settings-menu `Open Logs Folder` action for quickly locating runtime diagnostics.
- The folder is created on demand and the existing log rotation and file format are unchanged.
### 2026-09-02 Open Logs Folder

- Added a Settings-menu `Open Logs Folder` action for quickly locating runtime diagnostics.
- The folder is created on demand and the existing log rotation and file format are unchanged.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Batch Paste Injector Parameters

- Added a `Paste to Selected` action to the multi-selection object panel.
- Copied injector parameters can now be applied to multiple selected injectors in one operation.
- Each successful paste remains an independent Unit Edit history entry; reference geometry is excluded.
### 2026-09-02 Validate External Project Files

- Project validation now checks that the active Chemkin and reference-geometry paths still point to existing files.
- Missing external files are reported before the deeper project validation result is shown.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Clean Stale Recent Projects

- Missing recent project files are now removed from the menu and persisted back to configuration.
- The same cleanup occurs if a file disappears after the menu was created.
- Existing project files that fail parsing remain in the list for later retry.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Prevent Duplicate Injector Rename

- Renaming an injector now rejects names already used by another injector, case-insensitively.
- The rejected rename does not enter the edit history and reports the reason in the status bar.
### 2026-09-02 Batch Translate Selected Injectors

- Added a `Translate Selected` control to the multi-selection object panel.
- Users can apply an X/Y/Z offset to multiple injectors at once.
- Locked injectors are skipped, and each successful translation gets its own Move Undo/Redo entry.
- Failed geometry rebuilds restore the original position fields.
- Release build and all 9 CTest regressions passed.
### 2026-09-02 Batch Assign Material

- Added a `Set Material Selected` control to the multi-selection object panel.
- Material choices come from the independent material table and are applied to selected injectors.
- Each changed injector gets an independent Unit Edit Undo/Redo entry; movement locks do not block material edits.
- Release build and all 9 CTest regressions passed.

### 2026-09-02 Reject Empty Chemkin Species Sections

- Chemkin files with a `SPECIES` section but no species are now rejected instead of being reported as successfully loaded.
- The parser returns a field-specific error so the UI can keep the previous valid species list.
- Added regression coverage for an empty `SPECIES ... END` block.

### 2026-09-02 Enrich Object List Filtering

- Object-list tooltips now include the assigned material for each injector.
- The object filter now matches injector type, particle type, material, and UUID in addition to the display name.
- This changes only object-list presentation and does not affect project data or save format.

### 2026-09-02 Close Auxiliary Windows on Application Quit

- MainWindow now closes OCCT unit editors, Species Colors, and Materials dialogs from `QApplication::aboutToQuit` as well as the normal close path.
- This covers application-exit paths that bypass `MainWindow::closeEvent` and keeps child dialogs from outliving the OCCT context.
- The dialogs remain parent-owned; the change avoids duplicate manual destruction.

### 2026-09-02 Finalize State Before OCCT Context Menus

- Opening a 3D-view context menu now finalizes any active move transaction and clears stale face/object selection state before hit-testing.
- This prevents a right-click after dragging from leaking old interaction state into the next mouse event.
- The context-menu actions and geometry data remain unchanged.

### 2026-09-02 Scope Injector Selection Modes Per Object

- Injector picking now enables `TopAbs_COMPOUND` or `TopAbs_SHAPE` only on injector AIS objects and disables their previous modes first.
- Removed the context-wide selection-mode activation from the injector picking path so reference-geometry face selection cannot inherit a stale injector mode.
- The context-menu actions and geometry data remain unchanged.

### 2026-09-02 Guard Injector Lock Updates During Cleanup

- `set_unit_locked()` now validates the stored `Unit` pointer before changing lock state or comparing its AIS object.
- Lock/unlock requests during object removal or teardown now fail safely instead of dereferencing an empty hash entry.

### 2026-09-02 Stabilize Chemkin Toolbar Layout Persistence

- Assigned a stable object name to the runtime Chemkin status toolbar.
- `QMainWindow::saveState()` can now serialize and restore the toolbar without the previous missing-`objectName` warning.

### 2026-09-02 Report Failed Injector Geometry Refreshes

- Geometry rebuild failures now emit a dedicated OCCT widget signal and are shown in the main-window status bar.
- The existing valid displayed geometry remains unchanged when a rebuild fails; the failure is no longer silent.

### 2026-09-02 Add Injector Geometry Regression Coverage

- Added a dedicated geometry regression target covering single, group, volume, and all four cone variants: point, hollow, ring, and solid.
- The test also verifies that a failed rebuild caused by zero velocity preserves the previous valid shape.

### 2026-09-02 Test Particle-Dependent Unit Editor Controls

- Added stable object names for the material, diameter-distribution, and species editors.
- The unit-editor regression now verifies the enablement matrix for all five particle types.
- When the case context explicitly disables the energy equation, Droplet, Combusting, and Multicomponent are disabled and loaded heat-dependent data falls back to Inert; condensate falls back to single injection; Unknown context remains permissive.
- Corrected diameter-distribution availability to match Fluent: `tabulated` is offered for cone and surface injections, not volume injections.
- Non-steady particle tracking now forces stochastic Number of Tries to 1 and disables the corresponding editor row; the existing DDPM restriction remains in effect.
- Added optional case-context chemistry metadata: Droplet and Combusting are restricted when fewer than two active species are explicitly reported and non-premixed combustion is explicitly disabled; Unknown metadata remains permissive.
- Added optional material-model metadata for Combusting: when the selected material uses multiple-surface-reaction, Oxidizing Species and Product Species are cleared and disabled; Unknown metadata remains permissive.
- Context validation now also runs from injection-type, particle-type, and DPM-domain change callbacks, so programmatic or externally synchronized changes cannot bypass disabled-option constraints.
- Stochastic subsettings are now cleared when Stochastic Tracking is off: Random Eddy is false and Number of Tries is reset to one.
- The same regression verifies that the cone parameter panel is hidden for non-cone injections and visible for cone injections.

### 2026-09-02 Strengthen Release Shutdown Verification

- `verify_release_shutdown.ps1` now checks the newest runtime log after the process exits.
- The probe fails if it finds `[ERROR]`, `[FATAL]`, or `QMainWindow::saveState()` warnings, instead of checking only the process exit code.

### 2026-09-02 Refresh Open Unit Editors After Display-Unit Changes

- `QUI_FieldRow` now retains its semantic/storage unit and can refresh its display unit without changing the stored model value.
- Open unit editors refresh point-property and physical-model rows when display-unit preferences change, so labels and displayed values update immediately.
- Added regression coverage for changing a length display from millimetres to centimetres while preserving the internal metre value.
- Project save-format work remains deferred as requested.
- Release compilation passed. The standalone Qt GUI regression process hung without output in this environment, so full CTest completion still needs a clean GUI-capable test environment; the non-GUI unit-system regression passed.

### 2026-09-02 Make Auxiliary Shutdown Idempotent

- Main-window auxiliary dialogs are now closed through a one-time shutdown guard.
- Repeated calls from `closeEvent`, `aboutToQuit`, and the destructor no longer re-enter child-dialog cleanup after the first pass.
- The existing parent ownership and OCCT editor cleanup order remain unchanged.

### 2026-09-02 Suppress OCCT Callbacks During Widget Destruction

- `OCCTWidget` now marks the beginning of destruction before closing its auxiliary editors.
- Selection cleanup triggered by editor-close callbacks still clears local OCCT selection, but no longer emits UI signals or queues a redraw after teardown has started.
- Normal selection, context-menu, and drag behavior is unchanged.

### 2026-09-02 Reject Truncated DPM and Chemkin Reads

- DPM and Chemkin readers now check the underlying `QFile` error after consuming file contents.
- A mid-read I/O failure returns no partial data and uses the existing error-message/UI feedback path.
- File-format and project save formats remain unchanged.

### 2026-09-02 Report Empty Input Paths

- DPM and Chemkin public readers now return explicit errors for empty paths instead of only returning an empty result.
- Chemkin regression coverage verifies the new diagnostic while preserving the existing canceled-dialog behavior in the main window.

### 2026-09-02 Lightweight Injector Drag Synchronization

- OCCT injector dragging now emits a position-only update signal while the MainWindow still receives model and dirty-state synchronization.
- Open unit editors update only point-property rows during mouse movement instead of refreshing all selectors, model pages, and dynamic layouts.
- Full data refresh remains used for property edits, paste, undo/redo, and other non-drag changes.

### 2026-09-02 Validate Duplicate Species Colors on Load

- Species color configuration loading now rejects duplicate explicit colors, matching the editor's collision rule.
- Invalid duplicate-color files are backed up through the existing corrupt-config recovery path.
- Added app-config regression coverage; the color file format itself is unchanged.

### 2026-09-02 Validate Reference Geometry Configuration Types

- Reference geometry position and rotation must now be finite 3-component arrays.
- Reference geometry `locked` and `visible` values must be booleans; malformed settings are rejected and backed up instead of silently defaulting.
- The existing settings JSON structure and valid configuration behavior remain unchanged.
- Release build, focused app-config/unit-system/Chemkin regressions, package deployment, and startup/shutdown verification passed.

### 2026-09-02 Reject Invalid App Setting Field Types

- Existing but malformed Chemkin path, recent-project list, window-state, and unit-preference fields are now rejected instead of silently becoming empty/default values.
- Invalid settings are backed up through the existing corrupt-config recovery path; missing optional fields remain compatible with older configurations.
- Added regression coverage for all four field groups; Release build, focused tests, packaging, and startup/shutdown verification passed.

### 2026-09-02 Strict Project Session Field Parsing

- Project session vectors now require three finite numeric components instead of accepting string values as zero.
- Session schema version, unit preferences, species colors, material density, and reference-geometry flags now reject incorrect JSON types.
- Added malformed-session regression coverage; failed loads still leave the caller's existing data untouched.
- Release build, project-session/configuration/unit-system/Chemkin tests, packaging, and startup/shutdown verification passed.

### 2026-09-02 Reduce Drag Synchronization Cost

- Injector dragging now updates the MainWindow model and marks the project dirty without recalculating the complete project fingerprint on every mouse move.
- A full data update is emitted once when the drag is released, preserving final dirty-state verification and editor refresh behavior.
- Release build, injector-geometry/Unit-Editor/numeric-input regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Refresh Open Editors After Chemkin Changes

- Changing the Chemkin species list now updates species selectors in every open Unit Editor.
- The refresh changes only selector options and preserves the existing editor/model synchronization path.
- Added Unit Editor regression coverage; Release build, geometry/GUI/numeric regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Propagate Unit Conversion Overflow Failure

- `UnitSystem::to_base()` and `from_base()` now reject non-finite conversion results instead of reporting overflow as success.
- `UnitSystem::convert()` now propagates intermediate conversion failure and returns `ok=false` consistently.
- Successful preference validation clears stale error text; overflow and validation regression coverage was added.
- Release build, unit-system/numeric/project-session regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Refresh Material Context Without Full Model Rebuild

- Open Unit Editors now retain references to dynamic Wet Combustion material selectors.
- Updating the Materials table refreshes those selectors directly instead of rebuilding all four model pages.
- Existing material values remain visible when no longer present in the active material list.
- Added Unit Editor regression coverage; Release build, packaging, and startup/shutdown verification passed.

### 2026-09-02 Make CTest Runtime Environment Reproducible

- Regression tests now receive Qt plugin and Qt/OpenCASCADE/vcpkg DLL paths from CMake instead of relying on the developer shell environment.
- All 10 Release CTest targets pass in a clean test invocation.
- Release packaging and startup/shutdown verification passed.

### 2026-09-02 Keep Removed Materials Visible but Unselectable

- Dynamic Wet Combustion material selectors now expose only the current Materials table entries.
- A material value removed from the table remains visible as read-only text for compatibility, but is no longer an available selection.
- Added regression coverage for stale-value display, selectable options, and read-only behavior.
- Release build, all 10 CTest regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Guard Reference Geometry Controls Without Loaded Geometry

- Reference-geometry transform, clear, and lock controls now reflect whether a reference shape is loaded.
- An empty reference view cannot retain a stale locked state or accept transform actions.
- The context-menu reset action is disabled while the reference geometry is locked.
- Release build, all 10 CTest regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Clear Face Reference When Hiding Geometry

- Hiding reference geometry now clears the independent selected-face coordinate trihedron.
- Face information and face-alignment actions can no longer remain active for an invisible reference shape.
- Release build, all 10 CTest regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Share Advanced Preview Switch With Debug Unit Builder

- MainWindow debug injector construction now follows the same `DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW` switch as `Injector_OCCT`.
- The default stable preview remains unchanged when the option is `OFF`.
- Verified both option states compile in the configured Release environment; the default-off build passed all 10 CTest regressions.

### 2026-09-02 Enforce Material Table in Batch Assignment

- Batch material assignment now rejects empty names and names absent from the current Materials table.
- This keeps the programmatic multi-selection path consistent with the Unit Editor's read-only material selectors.
- Release build, all 10 CTest regressions, packaging, and startup/shutdown verification passed.

### 2026-09-02 Roll Back Failed Edit Snapshots Safely

- Unit edit undo, redo, and cancel snapshot application now restores the previous type, data, and displayed shape when geometry rebuilding fails.
- Added an OCCTWidget edit-history regression covering rejection of an invalid historical snapshot without damaging the valid current state.
- The native OCCT test uses the Windows Qt platform instead of the incompatible offscreen platform; all 11 CTest regressions pass.

### 2026-09-03 Complete Case-Context Injection Constraints

- Injection type item availability is now applied to every item before restoring the current selection.
- Fixed an early-loop exit that left `cone`, `flat-fan`, and other restricted injection types enabled when the case context disabled them.
- Removed temporary diagnostic output; focused and complete Release CTest runs pass, with all 11 regressions successful.
### 2026-09-03 Align Fluent Injection Restrictions

- Volume injections retain only the documented Rosin-Rammler diameter distributions; `tabulated` remains limited to Cone and Surface.
- Particle rotation is limited to Single, Group, Cone, Surface, and Volume injections; Cone uses angular-velocity magnitude while the other supported types use components.
- Atomizer injections hide particle rotation and lock Parcel Release Method to `Standard`.
- Solid-cone uses the atomizer spatial-staggering flag, matching Fluent's documented default staggering family.
- Project save-format work remains deferred as requested.
- Release build and all 11 CTest regressions passed.

### 2026-09-03 Tighten Massless Surface and Atomizer Controls

- Massless Surface injections now clear and hide `Use Face Normal`, which has no velocity input to replace in this mode.
- Massless Effervescent injections now expose position only; their atomizer-axis fields are no longer shown.
- Switching between atomizer and ordinary injection families now clears the inactive staggering flag, preventing stale hidden settings from remaining active.
- Project save-format work remains deferred as requested.
- Release build and all 11 CTest regressions passed.

### 2026-09-04 Release Preview And Coordinate-Frame Checkpoint

- Default stable injector preview units now load in Release as well as Debug,
  while advanced atomizer previews remain opt-in through the CMake switch.
- The initial OCCT view performs one deferred fit after the native widget has
  its final size, preventing the scene from appearing as a tiny block in the
  lower-left corner.
- Every injector and every imported reference-geometry face has an independent
  display-only local coordinate frame; the world coordinate system remains
  visible and existing selection/right-click behavior is preserved.
- Legacy smoke-test sources, targets, and registrations remain removed. The
  project uses focused regression tests instead; all 11 Release regressions
  passed.
- Buttons remain text-based for now. Application and combo-box indicator icon
  resources are retained because they are not button decorations.
- This state was archived in checkpoint commit `9b84d9d` and pushed to
  `origin/main`.

### 2026-09-04 Array Core Foundation

- Added an independent Unit-array expansion module for linear, rotational, and
  mirror transforms. It produces fresh-UUID child Units without mutating the
  source Unit.
- Position endpoints, direction vectors, flat-fan reference points, and volume
  bounds are transformed together so the generated child remains geometrically
  coherent.
- Editor controls, composite hierarchy, inheritance overrides, and project
  serialization remain separate follow-up layers rather than being guessed in
  this foundational increment.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Make Rotation Undoable

- Numeric Unit rotation now records a complete injector edit snapshot in the
  existing Undo/Redo history.
- Restoring an edit snapshot also refreshes the local coordinate frame and
  resolves the display color from the Species/Color table.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Use Reference Geometry For Arrays

- Added a persisted `use_reference_geometry` array-spec flag.
- Array creation now offers a text confirmation to use the visible reference
  frame; linear, rotational, and mirror modes consume its X axis, origin/X
  axis, and Z normal respectively.
- Release build and all 12 focused regressions passed.
- A selected reference face now takes precedence over the whole-geometry frame
  and supplies the transformed origin, tangent, and normal for array creation.

### 2026-09-04 Add Parent-Local Target Scope

- Added the text-visible `Parent Local` target scope to Single injector
  editing and persistence-compatible enum handling.
- Current array expansion transforms parent-local targets with the local array
  transform; nested Assembly code can later resolve the distinct parent frame.
- Added regression coverage; Release build and all 12 focused regressions
  passed.

### 2026-09-04 Preserve World Target Points During Rotation

- Fixed numeric Unit rotation so World-scoped Single target hitpoints remain
  fixed in world coordinates; local scopes continue to rotate with the Unit.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Reference-Local Target Scope

- Added `Reference Local` to Single target scope handling.
- Array expansion converts reference-local target coordinates through the
  configured reference frame and does not apply a second array transform.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Test Reference-Local Array Targets

- Added unit-array regression coverage for rotated reference frames and fixed
  reference-local targets across linear instances.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Assembly Relationship Layer

- Added text-based `Create Assembly From Selected` grouping for multiple
  existing Units.
- Assembly parent/child UUID relationships are persisted and restored, while
  leaf AIS geometry remains owned by the existing Unit objects.
- Deletion and detachment clear both sides of the runtime relationship.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Persist Array Metadata

- Project sessions now store optional array metadata on the mother Unit and
  rebuild following children when a session is loaded.
- Runtime child instances remain derived display objects and are not duplicated
  into the main project injector list.
- Sessions written before array support remain readable through optional-field
  defaults; Release build and all 12 focused regressions passed.

### 2026-09-04 Add Single Injector Direction Modes

- Single injectors now expose `Vector`, `Pitch/Yaw`, and `Target Hitpoint`
  direction modes in the Unit Editor.
- The editor dynamically shows velocity components, pitch/yaw angles, or
  target-point coordinates according to the selected mode.
- Direction settings are persisted in project sessions and used by geometry
  previews and local coordinate frames; Cone remains axis-plus-angle based.
- Added focused editor coverage; Release build and all 12 regressions passed.

### 2026-09-04 Keep Target Hitpoints Coherent In Arrays

- Linear, rotational, and mirror array expansion now transforms a Single
  injector's target hitpoint together with its position and direction data.
- Added regression checks for all three array transform types; Release build
  and all 12 focused regressions passed.

### 2026-09-04 Add Target Hitpoint Scope

- Single Target Hitpoint direction now distinguishes `World` and `Array Local`
  scopes in the editor and project session data.
- Array-local targets follow linear, rotational, and mirror transforms, while
  world targets remain fixed; Release build and all 12 regressions passed.

### 2026-09-04 Preserve Array Metadata When Copying Units

- `Unit` copy and move constructors now retain `has_array_spec` and the full
  `UnitArraySpec`, preventing array rules from disappearing during composite
  or nested-array preparation.
- Added a focused regression check; Release build and all 12 regressions
  passed.

### 2026-09-04 Expose Helical Rotational Spacing

- Rotational array creation now asks for axial spacing per child.
- Non-zero spacing combines with angular rotation to form a helical array;
  zero retains the existing planar rotational behavior.
- Added an axial-spacing regression check; Release build and all 12 tests
  passed.

### 2026-09-04 Add Square And Hexagonal Fill Core

- Added a reusable model-level fill expansion API for square and hexagonal
  layouts with configurable rows, columns, spacing, and origin.
- Multiple seed Units are assigned round-robin, providing a deterministic
  foundation for mixed injector layouts; Release build and all 12 tests pass.

### 2026-09-04 Connect Fill Layouts To Object Panel

- Added a text-based `Create Fill...` object-panel action for one or more
  selected Units.
- The action creates square or hexagonal layouts with editable row, column,
  and spacing values and displays generated children with local frames.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Persist Fill Layout Metadata

- Project sessions now store square/hexagonal fill parameters and the UUIDs of
  the participating seed Units.
- Fill children remain derived runtime objects and are rebuilt after loading;
  older sessions without fill metadata remain compatible.
- Added project-session round-trip coverage; Release build and all 12 tests
  passed.

### 2026-09-04 Track Fill-Derived Children

- Fill-generated runtime children are now registered with their fill parent,
  allowing rebuilds and repeated project loads to remove stale displays
  instead of accumulating duplicate objects.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Circular Fill Boundary

- Square and hexagonal fills can now be clipped to a user-defined circular
  boundary radius from the object-panel action.
- The circular-boundary settings are persisted with fill metadata; Release
  build and all 12 focused regressions passed.

### 2026-09-04 Restore Array Inheritance From Object Panel

- Added the text-based `Restore Array Inheritance` action for array children
  that were changed to independent mode.
- Restoring removes the overridden runtime child and rebuilds it from the
  parent array or fill rule; Release build and all 12 tests passed.

### 2026-09-04 Refresh Project Memory Baseline

- Updated the planning baseline to reflect the implemented array/fill editor,
  inheritance, persistence, and runtime rebuilding layers.
- Corrected the verification count to the current 12 focused regressions and
  documented that nested Assembly hierarchy remains future work.

### 2026-09-04 Use Species Colors For All Injector Displays

- Every injector display color is now resolved from the existing Species/Color
  table using its `material` name.
- Ordinary units, arrays, fills, restores, and geometry refreshes no longer
  copy a palette or another injector's color; missing entries use fixed neutral
  fallback color.

### 2026-09-04 Clean Up Derived Children On Delete

- Parent deletion now removes following array/fill children from the OCCT
  scene and runtime maps.
- Child deletion detaches the child from its parent's ownership list, avoiding
  stale runtime references.

### 2026-09-04 Add Numeric Unit Rotation

- Added the text-based `Rotate Selected` object-panel action with numeric axis
  and angle fields.
- Rotation applies to unlocked selected Units around each Unit's own position,
  updates injector points and direction vectors, rebuilds geometry, refreshes
  local coordinate frames, and re-resolves color from the Species/Color table.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Transform Assembly Members Together

- Numeric `Translate Selected` and `Rotate Selected` now recursively apply to
  Assembly descendants and de-duplicate overlapping selections.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Allow Nested Assemblies

- Existing Assembly nodes can now be selected as members of a new Assembly.
- Creation detaches moved nodes from their former parent and rejects cycles,
  enabling safe multi-level Assembly relationships.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Expose Assembly Membership Controls

- Added text-based `Detach Selected From Assembly` management.
- Object-panel entries now distinguish Assembly parents and members and show
  the Assembly parent UUID in their tooltip.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Keep Assembly Relationships Bidirectional

- Rebuilding an Assembly now clears stale `assembly_parent_uuid` values from
  former members before assigning new children.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Propagate Assembly Visibility And Locks

- Parent Assembly visibility and lock operations now recursively apply to
  nested members with cycle-safe visitation.
- Direct child operations remain independent and do not alter the parent.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Batch Assembly Transform History

- Recursive Assembly rotation now tags all member edit records with one batch
  UUID; Undo/Redo applies the complete transform atomically.
- Ordinary single-Unit edits retain their existing one-record behavior.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Batch Assembly Translation History

- Recursive Assembly translation now tags all member move records as one
  history batch, making one Undo/Redo restore the complete translation.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Reference Axis Presets To Rotation

- The text-based rotation dialog now offers `Reference X`, `Reference Y`, and
  `Reference Z` presets when a reference frame is available, otherwise it
  remains on `Custom` only.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Reference Origin Rotation Pivot

- The text-based rotation dialog now offers `Reference Origin` as a shared
  pivot when a reference frame is available; `Current Unit` remains the
  default per-Unit pivot.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Assembly Parent Rotation Pivot

- The text-based rotation dialog now offers `Assembly Parent` as a shared
  pivot for selected members with a valid parent Unit.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Validate Assembly Graphs Before Load

- Project preflight now validates Assembly parent/child references in both
  directions and rejects cyclic graphs before runtime restoration.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Assembly Dissolve Operation

- Added text-based `Dissolve Selected Assembly` control.
- Dissolving clears member parent references and returns the selected parent
  to a normal leaf Unit without deleting geometry.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Cover Assembly Session Round-Trip

- Added regression coverage for Assembly type, child UUIDs, and member parent
  UUIDs across project save/load.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Test Invalid Assembly Graphs

- Added preflight regression cases for one-sided parent/child references and
  cyclic Assembly relationships.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Show Assembly Hierarchy In Object List

- Object-list ordering now places Assembly ancestors before descendants and
  uses depth-based text indentation for nested members.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Expand Assemblies As Array Seeds

- Assembly Units can act as array sources while preserving the Assembly parent;
  non-derived members are expanded as seeds and generated children are attached
  beneath the source Assembly.
- Injector display colors remain sourced exclusively from the existing
  Species/Color table using `injector_data.material`; material configuration is
  not a color source.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Preserve Assembly Members During Array Rebuild

- Array-child cleanup now removes only objects marked as derived array
  children, so ordinary Assembly members remain intact during rebuilds.
- Added regression coverage for multi-member Assembly array expansion and
  replacement without stale-child accumulation.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Multi-Member Assembly Array Seeds

- Added focused OCCT regression coverage for expanding a two-member Assembly
  as a linear array source.
- The regression verifies that original members remain attached, generated
  children reference the Assembly parent, and rebuild replaces stale children.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Reference-Normal Array Conforming

- Array creation now offers an optional reference-normal conforming mode.
- When enabled, each generated injector preserves its direction magnitude and
  rotates type-specific direction vectors toward the selected reference frame
  normal; positions and array spacing remain unchanged.
- The setting is persisted in project sessions and absent fields default to
  disabled for backward compatibility.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Array Conforming Persistence

- Project-session regression coverage now verifies that reference-frame array
  settings, including normal conforming, survive save and load.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Reference Frames To Fill Arrays

- Square and hexagonal fill creation can now use the visible reference
  geometry origin, tangent axis, and normal instead of the world XY plane.
- Fill children can optionally conform their type-specific injector direction
  vectors to that reference normal, while retaining spacing and boundary rules.
- New fill fields are persisted; older sessions retain the previous world-plane
  behavior through disabled defaults.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Resolve Local Targets In Fill Arrays

- Fill-array expansion now converts `Reference Local` target hitpoints through
  the selected reference frame instead of leaving them in local coordinates.
- Added regression coverage for the transformed target point and retained
  direction-conforming behavior.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Fill Reference Settings Persistence

- Project-session regression coverage now verifies that fill-array reference
  axes and normal-conforming flags survive save/load.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Release Deployment

- Rebuilt the Release package with `scripts/package_release.ps1` and verified
  250 deployed runtime files with no `*_debug.dll` entries.
- `scripts/verify_release_shutdown.ps1` confirmed startup and WM_CLOSE exit
  code `0`; the latest runtime log contained no ERROR/FATAL or layout warning.

### 2026-09-04 Isolate Flattened Assembly Array Children

- Runtime array and fill children created from Assembly sources now clear
  persistent Assembly relationships while retaining their array ownership.
- Added regression coverage ensuring original Assembly member links survive
  expansion and flattened children cannot mutate the source graph on deletion.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Clean Assembly Lifecycle Metadata

- Reusing an array source as an Assembly now removes its previous generated
  children and array/fill metadata before rebuilding relationships.
- Dissolving an Assembly also removes generated children and clears obsolete
  array/fill state while leaving ordinary units intact.
- Added regression coverage for clean dissolve behavior.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Make Assembly Creation Transactional

- Assembly creation now pre-validates duplicate, missing, derived, and cyclic
  candidates before clearing existing parent state.
- Invalid requests return without changing the source Unit, and regression
  coverage verifies this failure behavior.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Validate Array Metadata On Project Load

- Project validation now rejects malformed array and fill specifications before
  they reach geometry rebuilding.
- Checks cover enum ranges, bounded counts, finite vectors, spacing, angles,
  and circular-boundary radii; regression coverage includes invalid count and
  non-finite spacing cases.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Elliptical Array Mode

- Added text-based Elliptical array creation with major radius, minor radius,
  and total-angle controls.
- Elliptical arrays use the configured/reference frame, rotate injector
  directions consistently, and persist their parameters in project sessions.
- Added regression coverage for ellipse positions and retained compatibility
  with existing array modes.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Validate Elliptical Array Sessions

- Fixed the project metadata enum range so persisted Elliptical arrays validate
  correctly alongside Linear, Rotational, and Mirror arrays.
- Regression coverage now distinguishes invalid ellipse radii from unrelated
  fill metadata errors.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Complete Helical Spatial Translation

- Rotational array axial spacing now moves every spatial field consistently,
  including flat-fan centers, virtual origins, volume bounds, and local target
  points.
- Added regression coverage for the translated center and volume bounds.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Enforce Species-Based Injector Colors And Metadata Validation

- Injector colors remain exclusively driven by the Species/Color table lookup
  for `injector_data.material`; no update path may copy an unrelated color.
- Project-session validation rejects non-finite array/fill metadata and
  parallel reference axes before geometry restoration.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Unit Position Inspector

- The Objects panel now displays Position X/Y/Z for the current Unit and
  accepts precise numeric edits.
- Position edits reuse the existing translation, lock, Assembly propagation,
  synchronization, and Undo/Redo paths; reference selections remain disabled.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Unit Direction Inspector

- The Objects panel now displays and edits the effective direction vector for
  supported injector types using their type-specific field.
- Single Pitch/Yaw and Target Hitpoint modes show the derived direction while
  preserving their parameterized source controls; Group `vel2` remains unused.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Single Pitch-Yaw Inspector

- Single injectors using Pitch/Yaw direction now expose editable angle fields
  in the Objects panel.
- The update path rebuilds and recolors the injector from the Species/Color
  table, then synchronizes the Unit; other direction modes remain unchanged.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Single Target Inspector

- Single Target Hitpoint mode now exposes editable target coordinates in the
  Objects panel.
- Zero-length targets are rejected; valid edits rebuild the geometry, resolve
  color from the Species/Color table, and synchronize the Unit.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Target Coordinate Scope Control

- Target Hitpoint mode now exposes World, Array Local, Parent Local, and
  Reference Local scope choices in the Objects panel.
- Scope changes rebuild and synchronize the Unit and participate in Undo/Redo;
  the control is disabled for non-target direction modes.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Direction Inspector History

- Added an OCCT regression check for direction inspector mutation and Unit Undo
  restoration.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Verify Target Scope Inspector History

- Added OCCT regression coverage for changing Target Hitpoint scope and
  restoring it through Unit Undo.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Remove Legacy OCCT Demo Hooks

- Removed unused primitive-demo declarations and the stale `create_cube`
  implementation from `OCCTWidget`; no production or focused test path used
  them.
- This keeps the old smoke/demo behavior out of the current reference geometry
  and Unit display interfaces.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Guard Composite Assembly Editing

- Assembly parent Units no longer enter the leaf injector editor or direct
  position/direction/target inspectors.
- Existing whole-Assembly translation and rotation operations remain the edit
  path for composite objects.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Guard Composite Assembly Copying

- Assembly copy/paste is rejected until composite cloning can preserve all
  child relationships and nested metadata.
- Added regression coverage for the rejection path.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Assembly Context Menu

- Right-clicking an Assembly now exposes text actions for recursive lock/unlock
  and dissolving the composite.
- The actions reuse the existing Assembly lifecycle and lock propagation code.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Audit Composite And Reference-Geometry Scope

- Recorded that Assembly copy is leaf-parameter paste only and remains
  unavailable until recursive composite cloning can remap relationships
  atomically.
- Recorded constructed reference aids and fully nested Assembly instances as
  explicit remaining implementation items; imported geometry and face frames
  remain supported.

### 2026-09-04 Add Transient Datum Reference Aids

- Added text actions to create a datum plane or datum axis in the Reference
  Geometry panel.
- Constructed aids reuse the existing selection, local-coordinate, visibility,
  locking, and transform behavior.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Persist Datum Reference Kind In App Config

- Reference configuration and project sessions distinguish imported files from
  `datum_plane` and `datum_axis` objects, allowing constructed aids and their
  dimensions/direction to be restored on startup.
- Existing file-based configurations remain backward-compatible.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Track Direction Inspector Edits

- Direction-vector, Pitch/Yaw, and Target Hitpoint edits now create normal Unit
  edit-history entries and can be undone/redone consistently with dialog edits.
- Failed geometry rebuilds remove the pending transaction and leave the prior
  injector data intact.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Keep Inspector Display State Consistent

- Direction inspector updates now refresh the Unit local coordinate frame.
- Rebuilt Volume previews retain the configured transparent rendering state.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Unify Injector Material With Species

- Injector material selection and validation now use Chemkin species names.
- Injector colors continue to be resolved only through the Species/Color table;
  the separate Materials table remains independent.
- Project sessions now read and validate constructed reference geometry
  parameters and transforms for datum planes and axes.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Apply Constructed Reference Direction

- Datum plane and datum axis creation now accepts and applies the configured
  construction direction.
- Project restore and state collection preserve that direction.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Datum Origin Reference Aid

- Added a text-based Datum Origin action using a small spherical marker.
- Datum origins use the existing reference geometry display, coordinate,
  selection, locking, transform, and persistence paths.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Add Section Plane Reference Aid

- Added a text-based Section Plane action backed by the existing oriented
  datum-plane geometry and interaction path.
- Section-plane kind, dimensions, direction, transforms, and state are
  persisted; boolean model clipping remains future work.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Cover Reference-Aid Session Round Trips

- Added project-session regression coverage for Section Plane and Datum Origin
  kinds, dimensions, direction, and radius.
- Release build and all 12 focused regressions passed.

### 2026-09-04 Cover Constructed Reference Session Validation

- Added round-trip coverage for datum-plane construction parameters and
  direction.
- Added validation coverage for zero construction directions.
- Release build and all 12 focused regressions passed.
