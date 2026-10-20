# Viewport Gizmo Handoff - 2026-10-21

## User Request

- Reduce each injector's local coordinate trihedron from the current `0.45`
  scale to `0.1`.
- Make Select, Translate, and Rotate mutually exclusive radio-style modes.
- Keep the translation axes or rotation rings visible after changing modes.

## Diagnosis

`OCCTWidget::set_interaction_mode()` used `selected_shape` as its only source
for the selected Unit UUID. Ordinary mouse release clears this temporary OCCT
selection handle, so a later mode change detached the old gizmo but could not
identify its Unit for reattachment.

The toolbar already used an exclusive `QButtonGroup`, but its controls were
`QToolButton`s. The mode controls are now icon-only `QRadioButton`s in an
exclusive `QButtonGroup`, with stable checked-state styling.

## Changes

- Added persistent `m_selected_unit_uuid` alongside `selected_shape`.
- Populate it through tree/API selection and viewport injector selection.
- Reuse it when switching modes; clear it on explicit deselection, hiding or
  deleting its Unit, or selecting reference geometry.
- Reduced both local trihedron creation and update scales from `0.45` to `0.1`.
- Converted the three viewport mode controls to icon-only radio buttons.

## Verification

- Release build: `build/codex_msvc142_release` succeeded with Visual Studio
  Developer environment loaded.
- CTest: `13/13` passed.
- `git diff --check`: passed.
- Visual interaction test is still needed: select an injector, switch Select
  -> Translate -> Rotate -> Select, and verify gizmo visibility and exclusive
  checked state. Also verify reference geometry selection clears the injector
  gizmo and hidden/locked Units do not receive one.

## Files

- `src/view/occtwidget.cpp`
- `src/view/occtwidget.h`
- `src/app/mainwindow.cpp`
- `discussion_planning/README.md`
- `discussion_planning/2026-10-21-viewport-gizmo-handoff.md`

## Follow-up - gizmo lifetime

Code review found the mode-switch UUID fix already on `main`. Remaining holes:
hide, hide-all, delete, lock, and reference-face selection left
`AIS_Manipulator` attached.

- Those paths now call `clear_transform_gizmo()` before the target leaves
  the scene or becomes locked.
- Unlock plus a later mode change can attach again because the selected
  shape UUID is kept.
- `OCCTWidget::transform_gizmo_active()` / `transform_gizmo_target()` cover
  the regression test.

Release rebuild of `build/codex_msvc142_release` succeeded. CTest `13/13`
passed, including hide/lock/reattach checks. `git diff --check` passed.

Live click-through of Select -> Translate -> Rotate -> Select is still not done by a person.
Testing rule from the user: data is disposable until they say otherwise, so
old app instances may be closed and replaced. The old `release_gizmo`
process was closed and `build/codex_msvc142_release/dpm_manager.exe` was
started.
