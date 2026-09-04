# DPM Manager Project Memory

## Current Scope

- Qt/C++ DPM manager with OpenCASCADE geometry view.
- Leaf `Unit` owns one injector; composite behavior is represented by runtime
  array/fill child relations while the full nested Assembly hierarchy remains
  a planned extension.
- Saving-format changes are deferred unless explicitly requested.

## Completed Areas

- Field-table-driven DPM import, including multiple injectors per file.
- Chemkin species import from the `SPECIES` section and material/config UI.
- Species color configuration and material table dialogs with config storage.
- Unit editor synchronization with live injector data and geometry updates.
- Fluent-style conditional visibility, enablement, and fallback rules for:
  particle/injection types, diameter distributions, cone parameters, surface
  options, DDPM, parcel release, drag laws, breakup, staggering, rotation,
  Brownian motion, wet combustion, chemistry, and case context.
- Invalid legacy values are normalized before UI/model use. All editor paths
  normalize state before emitting data or geometry updates.
- Child dialogs use guarded lifetime handling to avoid shutdown crashes.
- Release build and 12 focused CTest regressions currently pass.

## Planned Injector Arrays

```text
Unit
  master Injector
  InjectorArraySpec
  generated child instances
```

### Unit-Based Composite Hierarchy

- Keep `Unit` as the common upper-level object instead of introducing a
  separate top-level array object.
- Leaf `Unit` uses Injector mode and owns one injector.
- Composite `Unit` uses Array or Assembly mode and owns child `Unit`s; it
  should not simultaneously contain an active injector and child units.
- Store child transforms in the parent local coordinate system. Resolve
  geometry recursively: child local transform -> parent array transform ->
  higher-level transform -> world coordinates.
- A multi-injector seed, such as two alternating injectors in a rotational or
  planar array, is represented by one composite `Unit` containing both leaf
  units. The composite is then transformable and nestable as one unit.
- Parent/child signals should propagate upward and be consolidated by the
  root unit. Selection, locking, visibility, and edit scope should follow the
  same hierarchy.

- Support linear, mirror, and rotational arrays.
- Store array rules separately from generated child state.
- Treat children as rebuildable derived instances, not unrelated copies.
- Prefer explicit ownership (`unique_ptr` or a QObject-owned controller) over
  unmanaged raw-pointer callbacks.
- Use Qt signals/slots through an `InjectorArrayController` or `Unit` layer;
  child changes should be semantic, validated by `Unit`, and consolidated into
  one data/geometry update.
- Guard against master/child feedback loops with update-source guards or
  revision IDs.

## Array Editing Policy

- Right-click may choose `Follow Array`, `Edit Current Child Only`, or
  `Restore Inheritance`.
- Object-panel context menus now provide `Restore Array Inheritance` for an
  independent child; it removes the override instance and rebuilds it from the
  parent array or fill rule.
- Prefer independent inheritance scopes for physical properties and geometric
  properties. A child may inherit material/flow while overriding position.
- Local overrides must survive array regeneration until explicitly discarded.
- Decide later whether master flow is total flow or per-child flow, and whether
  array expansion is display-only or exported as multiple DPM injections.

## Injector Local Direction Design

- Treat each leaf injector as having a local coordinate frame with origin,
  forward, up, and right axes. Store geometry and direction parameters in the
  local frame, then transform them to world coordinates.
- Single injection direction should be selectable in the editor through a
  combo box such as `Vector`, `Pitch/Yaw`, or `Target Hitpoint`.
- `Vector` keeps the existing direct velocity-vector workflow.
- `Pitch/Yaw` uses local `+X` as forward, with yaw about local `Z` and pitch
  about local `Y`; velocity magnitude is independent from direction.
- `Target Hitpoint` derives direction from `normalize(hitpoint - origin)`.
  Zero-length targets must be rejected or fall back safely.
- Target points should support an explicit coordinate scope: local-array,
  parent-local, or world-fixed. A local target follows array transforms; a
  world-fixed target makes every instance aim at the same world point.
- Cone injections retain `axis + cone_angle`. Do not add Single-style
  Pitch/Yaw/Hitpoint controls to Cone when its axis is constrained by the model
  plane or reference geometry.
- Direction sources for other types remain specialized: Surface may use its
  velocity or selected-face normal, Volume uses its configured direction,
  Atomizers use `atomizer_axis`, Flat Fan uses `ff_normal`, File uses per-file
  velocities, and Condensate is solver/surface driven.
- Array transforms apply after the injector-local direction is resolved:
  injector local direction -> child transform -> array transform -> world.

## Manipulator And Numeric Inspector

- Quick transform editing should combine an OpenCASCADE-style view gizmo with
  precise numeric fields.
- Translation exposes live `Position X/Y/Z`; rotation exposes the relevant
  `vel`, `axis`, `atomizer_axis`, `ff_normal`, Pitch/Yaw, or Target Hitpoint
  values according to the injector direction mode.
- Support World, Local, Parent, and Reference Geometry transform spaces.
- Support selectable pivots: current Unit, parent Unit, array origin, or a
  selected reference axis/point.
- Dragging previews continuously but commits one data/geometry/history update
  on release; manual numeric edits support Apply and Cancel.
- Keep position, direction, and speed magnitude conceptually separate. For
  Single Pitch/Yaw mode, edit direction angles and magnitude independently.
- The same manipulator and numeric-inspector mechanism should work on leaf
  injectors, composite/array Units, and reference geometry.

## Velocity Field Semantics

- `vel` and `vel2` are Group-only fields. They represent the two endpoints of
  a pending A-to-B range/gradient, not two ordinary directions.
- Group gradient behavior is not implemented yet and remains deferred.
- Other injection types must not use `vel2` for direction logic. Use the
  actual type-specific field, such as `vel`, `vel_mag`, `axis`,
  `atomizer_axis`, or `ff_normal`, as appropriate.

## Array Feature Decisions

- Mirror array: approved as an independent useful mode.
- Helical array: consider as an option of rotational array, combining axial
  translation with rotation rather than creating a separate top-level mode.
- Radial array: no dedicated mode; compose a rotational array from a linear
  seed array.
- Concentric and staggered rings: no dedicated mode; compose multiple
  rotational arrays when needed.
- Hexagonal/honeycomb fill: high priority. Support filling a specified
  rectangle or circle with a selectable ratio of two or three injector types.
  This targets common rocket-engine injector layouts.
- Square fill: important common layout, to be discussed and designed
  separately after the hexagonal fill model.
- Elliptical array: optional experimental feature, low priority.
- Curve-following array: do not implement as a dedicated mode because it
  requires a separate curve-authoring system.
- Surface conforming: make it an option on other array modes. Linear and other
  arrays may orient instances to the selected surface normal without a
  standalone surface-array type.
- Custom point array: do not expose as a primary user mode. Keep a generic
  master/template representation internally so other array modes can expand
  into explicit points when necessary.

## Assembly And Reference Geometry

- Existing `Unit_Type::Assebly` is reserved for a composite structure. The
  spelling is legacy and should be handled consistently until a deliberate
  rename is planned.
- Assembly is different from Array: Assembly groups multiple different child
  Units into one reusable object, while Array applies a repeat/transform rule
  to a seed object. An Assembly may itself be used as an Array seed, enabling
  nested structures.
- Reference geometry should remain separate from injector/array physics. It
  provides construction and orientation aids such as symmetry planes, axes of
  rotation, origin points, section planes, and alignment frames.
- Reference geometry should support selection, visibility, locking, face
  selection, view alignment, and local-coordinate display without replacing
  the world coordinate system.
- Array rules may optionally bind to reference geometry: a symmetry array can
  use a reference plane, a rotational array can use a reference axis, and
  surface-conforming placement can use selected face normals.
- Reference geometry is a coordinate/design aid, not automatically a DPM
  boundary or an injector. Any later physical meaning must be explicitly
  assigned.

### Reference Geometry Subsystem

- Give reference geometry an independent model, renderer, interaction layer,
  and IO layer rather than embedding it in injector geometry generation.
- Support imported reference geometry and constructed aids through one common
  interface. Imported objects may later include STEP, IGES, BREP, or STL;
  constructed objects include symmetry planes, rotation axes, datum planes,
  origins, section planes, and alignment frames.
- Keep file reading separate from AIS/display creation so failed imports do
  not damage the current scene.
- Reference geometry belongs to the project/scene and may be shared by many
  Units or arrays; a Unit or Array references it instead of owning it.
- Array, mirror, rotational, helical, and surface-conforming operations must
  offer an optional reference-geometry basis. Examples: use a reference plane
  for symmetry, a reference axis for rotation, and a selected face normal for
  orientation.
- Preserve the world coordinate system. Reference-local coordinate systems
  are additional aids and must be independently visible, selectable,
  lockable, and editable.

## Known Limits

- Dynamic-mesh surface eligibility and Wall-Film-specific fields are not
  represented in the current `Injector`/case-context model, so they are not
  hard-locked in the editor.
- Multicomponent component lists and some detailed Fluent material/heat-model
  fields remain outside the current data model.
- Exact Fluent labels/defaults may vary by release; restrictions are applied
  only where current fields provide reliable evidence.

## Verification

- Build directory: `D:\Git\dpm_manager\build\codex_msvc142_release`
- Compiler: `E:\Program Files\Microsoft Visual Studio\18\Community`
- Release build command uses `vcvars64.bat`, CMake, and CTest.
- Last verified result: `12/12` focused tests passed.

## Current Checkpoint

- The legacy application-wide smoke-test sources and CMake targets are
  removed. Current automated coverage uses focused regression executables;
  do not restore the old smoke-test archive or add icon-only button tests.
- Buttons currently use text labels. The remaining icon resources are limited
  to the application icon and combo-box arrow indicators.
- Release startup displays the stable default injector preview in both Debug
  and Release builds. Experimental advanced atomizer previews remain gated by
  `DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW`.
- The 3D view preserves the world coordinate system and displays independent
  local coordinate trihedrons for each injector and every imported reference
  geometry face. These trihedrons are display-only and follow visibility,
  editing, dragging, undo/redo, paste, delete/restore, and reference transforms.
- Checkpoint commit `9b84d9d` was pushed to `origin/main` after Release build
  and all 11 focused regressions passed.

## Single Direction Mode Progress

- Single injectors now support `Vector`, `Pitch/Yaw`, and `Target Hitpoint`
  direction modes in the model and local-frame display direction.
- The Unit Editor exposes the selected mode and rebuilds its point-property
  rows so only the relevant velocity, angle, or target-point fields are shown.
- Pitch is constrained to `-90..90` degrees and yaw to `-360..360` degrees;
  all values use the existing numeric validation and expression input path.
- Direction mode, pitch, yaw, and target point are persisted in project
  sessions, with older sessions remaining compatible through defaults.
- Focused Unit Editor regression coverage verifies all three mode layouts.
- Array expansion now transforms `single_target_hitpoint` together with the
  injector origin for linear, rotational, and mirror children.
- Single target points now have explicit `World` and `Array Local` scopes;
  only the latter follows array transforms.
- Rotational array creation now exposes axial spacing per child, providing the
  planned helical-array behavior without introducing a separate top-level mode.
- Added model-level square and hexagonal fill expansion for one or more seed
  Units, with deterministic round-robin seed assignment for mixed layouts.
- Object-panel context menus now expose text-based `Create Fill...` actions
  for selected Units, including square/hexagonal pattern, rows, columns, and
  spacing controls; generated children are displayed with local frames.
- Fill metadata is now stored on the first fill parent with its seed UUID list
  and restored from project sessions; derived fill children remain runtime
  objects and are rebuilt after loading.
- Fill layouts can optionally clip generated square or hexagonal points to a
  configured circular boundary radius; the setting is available in the menu
  and project session data.
- Fill-derived children are now registered in the parent `child_units` list so
  rebuilds remove stale runtime objects before creating replacements.
- Deleting a parent now removes following derived children, while deleting a
  child detaches it from the parent's runtime ownership list.
- The source of truth for every injector display color is now the existing
  Species/Color table queried by `injector.material`; ordinary units, arrays,
  fills, restores, and geometry refreshes no longer copy palette or parent
  colors. Missing entries use a fixed neutral fallback.
- Mixed fill children now retain the round-robin source color, making different
  injector seeds visually distinguishable in square and hexagonal layouts.
- Added a text-based `Rotate Selected` object-panel action. It rotates each
  unlocked selected Unit around its own origin using a numeric world-axis and
  angle, updates position-derived points and direction vectors, rebuilds the
  geometry, refreshes the local frame, and resolves the display color again
  from the Species/Color table.
- Rotation actions are now recorded in the existing Unit edit history, so the
  current Undo/Redo edit commands can restore the complete injector snapshot;
  edit-snapshot restoration also refreshes the local frame and species color.
- Array specifications now persist a `use_reference_geometry` flag. When
  enabled from the text-based creation flow, linear arrays use the reference
  X axis, rotational arrays use its origin and X axis, and mirror arrays use
  its Z normal.
- If a reference face is selected, the array frame now prefers that face's
  transformed origin, X direction, and normal; otherwise it falls back to the
  reference geometry frame.
- Single target points now support a `Parent Local` scope in the editor and
  model. Current array expansion transforms this scope like local targets;
  nested Assembly resolution remains responsible for the future parent-frame
  distinction.
- Quick Unit rotation now preserves `World` target hitpoints; only local and
  parent-local targets rotate with the injector.
- Single target scopes now include `Reference Local`; array expansion resolves
  such targets through the stored reference frame origin, X axis, and normal.
- Unit-array regression coverage verifies reference-local conversion and its
  invariant across linear array offsets.
- Assembly phase one now stores a composite parent's child UUIDs and each
  child's assembly parent UUID. Text-based object-panel creation supports
  grouping multiple existing Units, and loading/deletion clean up the runtime
  relationships without duplicating leaf geometry.

## Array Core Progress

- Added `src/model/unit_array.h/.cpp` with a standalone expansion API for
  linear, rotational, and mirror arrays.
- Expansion keeps the source Unit unchanged, creates fresh child UUIDs, and
  transforms injector positions, direction vectors, flat-fan points, and
  volume bounds consistently.
- The module is connected to object-panel controls, array inheritance actions,
  fill metadata persistence, and runtime child rebuilding.
- Release build and all 12 focused regressions passed after adding the module.
- Array metadata is now included in project sessions. Loading a session
  restores the mother Unit's array rule and rebuilds its following children;
  older sessions without these optional fields remain readable.
- Child runtime instances stay out of the main project injector list, avoiding
  duplicate DPM/project entries.
- `Unit` copy and move construction now preserve array metadata, which is
  required before composing nested arrays or assemblies from copied Units.
- Numeric translation and rotation now recursively include an Assembly
  parent's descendants, with UUID de-duplication when both parent and child
  are selected.
- Assembly creation now permits an existing Assembly as a child, removes that
  child from its former parent, and rejects descendant cycles.
- Rebuilding an Assembly now clears stale parent UUIDs from its previous
  members before assigning the new membership set.
- Object-panel management now exposes text-based detachment, and labels
  Assembly parents and members explicitly with parent UUID information.
- Assembly parent visibility and locking now recursively propagate to nested
  members; editing a child alone does not change its parent.
- Recursive Assembly rotation records one edit-history batch, so a single
  Undo/Redo restores all affected members together rather than one at a time.
- Recursive Assembly translation now uses the same batch history behavior.
- Numeric rotation now offers text-based `Reference X/Y/Z` axis presets when
  a usable reference frame exists; without one, only `Custom` is exposed.
- Numeric rotation now also offers a `Reference Origin` pivot when available;
  otherwise it preserves the per-Unit pivot behavior.
- Numeric rotation also offers an `Assembly Parent` pivot when a selected
  member has a resolvable parent Unit.
- Project reference validation now checks Assembly parent/child UUID
  consistency and rejects cyclic Assembly graphs before loading.
- Project-session regression coverage now verifies Assembly parent/child
  relationships survive save/load.
- Project-session regression coverage also rejects inconsistent and cyclic
  Assembly graphs.
- The object list now orders Assembly parents before descendants and indents
  nested members by Assembly depth.
- Object-panel Assembly controls now include dissolving a selected parent,
  which clears member parent references and restores the parent to a leaf Unit.
- Assembly Units can now act as array sources: the source Assembly remains the
  parent while its non-derived members are expanded as array seeds and the
  generated children are attached to that Assembly.
- Injector display colors must always be resolved from the existing
  Species/Color table by `injector_data.material`; the separate material table
  stores material data only and is never a color source.
- Array-child cleanup now distinguishes derived array children from ordinary
  Assembly members; rebuilding an Assembly array preserves its original
  members and replaces only the generated children.
- Regression coverage now exercises a multi-member Assembly as an array seed,
  including child ownership and rebuild replacement without accumulation.
- Array creation can optionally conform injector direction vectors to the
  selected reference frame's normal; the option is persisted and defaults off
  for backward compatibility.
- Release package verification now succeeds with the repository deployment
  script: the package contains the complete Release dependency set, no Debug
  DLLs, and the startup/WM_CLOSE probe exits cleanly with no runtime faults.
- Project-session regression coverage now verifies persistence of the
  reference-geometry and reference-normal conforming flags.
- Square and hexagonal fill arrays now support the same optional reference
  frame and normal-conforming behavior; legacy fill sessions default to the
  world XY layout with conforming disabled.
- `Reference Local` target hitpoints now resolve correctly during fill-array
  expansion, matching linear, rotational, and mirror array behavior.
- Project-session regression coverage now verifies persistence of fill-array
  reference-frame axes and normal-conforming settings.
- Flattened array and fill children no longer inherit persistent Assembly
  parent/child UUIDs; this prevents deleting a runtime derived child from
  modifying the original Assembly graph.
- Creating or dissolving an Assembly now clears its obsolete generated array
  or fill children and metadata, preventing orphaned runtime objects and stale
  rebuilds.
- Assembly creation now validates candidate children and cycles before mutating
  the parent, so an invalid request leaves existing Assembly/array state intact.
- Project-session validation now rejects invalid array and fill metadata before
  geometry restoration, including out-of-range counts, invalid enum values, and
  non-finite spacing/axes.
- Added an optional Elliptical array mode with major/minor radii, total angle,
  reference-frame support, direction conforming, and project-session
  persistence; existing array modes remain unchanged.
- Project validation now includes the Elliptical enum in its accepted array
  range, so valid ellipse sessions are not rejected before other metadata is
  checked.
- Rotational arrays with axial spacing now translate all spatial injector
  fields, including fan centers, volume bounds, and non-world target points,
  keeping helical child geometry coherent.
- Injector rendering colors are always resolved from the existing Species/Color
  table using `injector_data.material` as the species key; updates, array
  rebuilds, and restores must not copy arbitrary parent or default colors.
- Project-session validation now rejects non-finite array/fill values and
  unusable reference frames before restoration; focused regression coverage
  includes invalid counts, ellipse radii, fill spacing, and parallel axes.
- The Objects panel now exposes a numeric Position X/Y/Z inspector for the
  current Unit; edits reuse the validated translation path, synchronize model
  data and geometry, respect locks/Assembly propagation, and enter move history.
- The same panel now exposes the effective local direction vector. It edits
  `vel`, `axis`, `ff_normal`, or `atomizer_axis` according to injector type;
  parameterized Single Pitch/Yaw and Target Hitpoint modes remain read-only.
- Single Pitch/Yaw mode now exposes editable pitch and yaw fields in the same
  panel; changes rebuild the injector and synchronize the owning Unit while
  preserving the direction source semantics.
- Single Target Hitpoint mode now exposes editable target X/Y/Z fields; zero
  length targets are rejected and valid changes rebuild and synchronize the
  injector without involving `vel2`.

## History

- Full chronological decisions remain in [README.md](README.md).
