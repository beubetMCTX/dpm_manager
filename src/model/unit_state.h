#pragma once

class Unit;

// True when this array source still has a copy that follows it.
// Fill-only sources are not hidden; their meshes stay in the scene.
bool unit_has_following_copy(const Unit &unit);

// The only semantic write of Unit::presentation_hidden.
void refresh_unit_derived_flags(Unit &unit);

// User visibility and the derived mother-mesh gate. Lock is not part of this.
bool unit_mesh_present(const Unit &unit);
