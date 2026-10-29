#include "unit_state.h"

#include "unit.h"

#include <memory>

bool unit_has_following_copy(const Unit &unit)
{
    if (!unit.has_array_spec)
    {
        return false;
    }

    for (const std::shared_ptr<Unit> &child : unit.child_units)
    {
        if (child != nullptr && child->is_following_array_child())
        {
            return true;
        }
    }
    return false;
}

void refresh_unit_derived_flags(Unit &unit)
{
    unit.presentation_hidden = unit_has_following_copy(unit);
}

bool unit_mesh_present(const Unit &unit)
{
    return unit.user_visible && !unit.presentation_hidden;
}
