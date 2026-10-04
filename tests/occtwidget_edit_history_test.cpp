#include "occtwidget.h"
#include "transform_snap.h"

#include <algorithm>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <QApplication>
#include <cmath>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        qCritical() << message;
        return false;
    }
    return true;
}

bool vectors_close(const QVector3D &left, const QVector3D &right,
                   float tolerance = 1.0e-4f)
{
    return (left - right).lengthSquared() <= tolerance * tolerance;
}

Unit make_valid_unit()
{
    Unit unit;
    unit.inj.injector_data.name = "edit-history-test";
    unit.inj.injector_data.pos = QVector3D(1.0f, 2.0f, 3.0f);
    unit.inj.injector_data.vel = QVector3D(1.0f, 0.0f, 0.0f);
    unit.inj.injector_data.atomizer_axis = QVector3D(1.0f, 0.0f, 0.0f);
    unit.inj.injector_data.total_flow_rate = 1.0;
    return unit;
}

double shape_center_x(const TopoDS_Shape &shape)
{
    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds);
    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return 0.5 * (xmin + xmax);
}
}

class OCCTWidgetEditHistoryTestAccess
{
public:
    static bool begin_preview(OCCTWidget &widget, const QUuid &uuid,
                              AIS_ManipulatorMode mode,
                              const gp_Trsf &transformation)
    {
        if (!widget.attach_transform_gizmo(uuid, mode))
        {
            return false;
        }
        widget.update_transform_gizmo_preview(transformation);
        return true;
    }

    static void cancel_preview(OCCTWidget &widget)
    {
        widget.restore_transform_gizmo_preview();
        widget.m_transform_gizmo_snapshot_valid = false;
        widget.clear_transform_gizmo();
    }

    static bool commit_preview(OCCTWidget &widget)
    {
        const bool committed = widget.commit_transform_gizmo_preview();
        widget.m_transform_gizmo_snapshot_valid = false;
        widget.clear_transform_gizmo();
        return committed;
    }
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    const gp_Pnt rotation_pivot(0.012, -0.023, 0.005);
    gp_Trsf unsnapped_rotation;
    unsnapped_rotation.SetRotation(
        gp_Ax1(rotation_pivot, gp_Dir(0.0, 0.0, 1.0)),
        qDegreesToRadians(43.0));
    const gp_Trsf snapped_rotation = transform_snap::snap_rotation_about_pivot(
        unsnapped_rotation, rotation_pivot, qDegreesToRadians(15.0));
    gp_Pnt transformed_pivot = rotation_pivot;
    transformed_pivot.Transform(snapped_rotation);
    gp_Pnt rotated_probe(rotation_pivot.X() + 1.0,
                         rotation_pivot.Y(), rotation_pivot.Z());
    rotated_probe.Transform(snapped_rotation);
    if (!check(transformed_pivot.Distance(rotation_pivot) < 1.0e-10,
               "Snapped gizmo rotation should preserve a non-origin pivot") ||
        !check(std::abs((rotated_probe.X() - rotation_pivot.X()) -
                        std::sqrt(0.5)) < 1.0e-10 &&
                   std::abs((rotated_probe.Y() - rotation_pivot.Y()) -
                            std::sqrt(0.5)) < 1.0e-10,
               "Snapped gizmo rotation should apply the nearest 45 degree step"))
    {
        return 1;
    }
    OCCTWidget widget(nullptr);
    widget.resize(640, 480);
    widget.show();
    application.processEvents();

    QList<QUuid> last_selected_units;
    QObject::connect(&widget, &OCCTWidget::unit_selection_changed,
                     [&last_selected_units](const QList<QUuid> &uuids)
    {
        last_selected_units = uuids;
    });

    Unit source = make_valid_unit();
    if (!check(source.inj.create_injector(),
               "Initial injector geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({source});
    application.processEvents();

    if (!check(widget.unit_hash.size() == 1,
               "Test widget should contain one injector"))
    {
        return 1;
    }

    const QUuid uuid = widget.unit_hash.constBegin().key();
    std::shared_ptr<Unit> stored_unit = widget.unit_hash.value(uuid);
    if (!check(stored_unit != nullptr,
               "Stored injector should be available"))
    {
        return 1;
    }

    const QVector3D original_position =
        stored_unit->inj.injector_data.pos;
    const QVector3D unified_original_direction =
        stored_unit->inj.injector_data.vel;
    if (!check(widget.translate_units_by_uuid(
                   {uuid}, QVector3D(4.0f, 0.0f, 0.0f)) == 1,
               "Unified history move should succeed") ||
        !check(widget.set_unit_direction_by_uuid(
                   uuid, QVector3D(0.0f, 1.0f, 0.0f)),
               "Unified history edit should succeed") ||
        !check(widget.undo_last_operation(),
               "Unified undo should first restore the latest edit") ||
        !check(widget.unit_hash.value(uuid)->inj.injector_data.vel ==
                   unified_original_direction &&
                   widget.unit_hash.value(uuid)->inj.injector_data.pos !=
                       original_position,
               "Unified undo should preserve the earlier move") ||
        !check(widget.undo_last_operation(),
               "Unified undo should then restore the earlier move") ||
        !check(widget.unit_hash.value(uuid)->inj.injector_data.pos ==
                   original_position,
               "Unified undo should restore the original position") ||
        !check(widget.redo_operation(),
               "Unified redo should reapply the move") ||
        !check(widget.redo_operation(),
               "Unified redo should reapply the edit") ||
        !check(widget.unit_hash.value(uuid)->inj.injector_data.vel ==
                   QVector3D(0.0f, 1.0f, 0.0f),
               "Unified redo should restore the edited direction"))
    {
        return 1;
    }

    widget.display_units({source}, true);
    application.processEvents();
    stored_unit = widget.unit_hash.value(uuid);
    if (!check(stored_unit != nullptr,
               "History reset should restore the source injector"))
    {
        return 1;
    }

    UnitArraySpec leaf_array;
    leaf_array.type = UnitArrayType::Linear;
    leaf_array.count = 2;
    leaf_array.direction = QVector3D(1.0f, 0.0f, 0.0f);
    leaf_array.origin = source.inj.injector_data.pos;
    leaf_array.spacing = 10.0f;
    if (!check(widget.create_unit_array(uuid, leaf_array) == 2,
               "Leaf array should create two children") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 2,
               "Leaf array source should own two children"))
    {
        return 1;
    }
    const QVector<std::shared_ptr<Unit>> visibility_children =
        widget.unit_hash.value(uuid)->child_units;
    if (!check(widget.set_unit_visible(uuid, false),
               "Hiding an array source should succeed") ||
        !check(std::all_of(
                   visibility_children.cbegin(), visibility_children.cend(),
                   [&widget](const std::shared_ptr<Unit> &child)
                   {
                       return child != nullptr &&
                              !widget.unit_visible(child->inj.uuid);
                   }),
               "Hiding an array source should hide generated children") ||
        !check(widget.set_unit_visible(uuid, true),
               "Showing an array source should succeed") ||
        !check(std::all_of(
                   visibility_children.cbegin(), visibility_children.cend(),
                   [&widget](const std::shared_ptr<Unit> &child)
                   {
                       return child != nullptr &&
                              widget.unit_visible(child->inj.uuid);
                   }),
               "Showing an array source should show generated children") ||
        !check(widget.set_unit_locked(uuid, true),
               "Locking an array source should succeed") ||
        !check(std::all_of(
                   visibility_children.cbegin(), visibility_children.cend(),
                   [&widget](const std::shared_ptr<Unit> &child)
                   {
                       return child != nullptr &&
                              widget.unit_locked(child->inj.uuid);
                   }),
               "Locking an array source should lock generated children") ||
        !check(widget.set_unit_locked(uuid, false),
               "Unlocking an array source should succeed") ||
        !check(std::all_of(
                   visibility_children.cbegin(), visibility_children.cend(),
                   [&widget](const std::shared_ptr<Unit> &child)
                   {
                       return child != nullptr &&
                              !widget.unit_locked(child->inj.uuid);
                   }),
               "Unlocking an array source should unlock generated children"))
    {
        return 1;
    }
    const QList<Unit> exported_leaf_array = widget.dpm_export_units();
    if (!check(exported_leaf_array.size() == 2,
               "DPM export should expand a leaf array into concrete injectors") ||
        !check(std::all_of(exported_leaf_array.cbegin(), exported_leaf_array.cend(),
                           [](const Unit &exported)
                           {
                               return exported.type == injector &&
                                      !exported.inj.injector_data.name.isEmpty() &&
                                      !exported.has_array_spec &&
                                      exported.child_units.isEmpty();
                           }),
               "DPM export should omit array control metadata") ||
        !check(exported_leaf_array.at(0).inj.injector_data.name !=
                   exported_leaf_array.at(1).inj.injector_data.name,
               "DPM export should assign unique concrete injector names"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> state_child_before_rebuild =
        widget.unit_hash.value(uuid)->child_units.first();
    const QList<DerivedUnitDisplayState> saved_derived_states =
        [&widget, &state_child_before_rebuild]()
        {
            widget.set_unit_visible(state_child_before_rebuild->inj.uuid, false);
            widget.set_unit_locked(state_child_before_rebuild->inj.uuid, true);
            return widget.derived_unit_display_states();
        }();
    if (!check(saved_derived_states.size() == 2,
               "Derived array display states should include generated children") ||
        !check(widget.set_unit_direction_by_uuid(
                   uuid, QVector3D(0.0f, 1.0f, 0.0f)),
               "Array source edit should rebuild before display-state restore"))
    {
        return 1;
    }
    widget.restore_derived_unit_display_states(saved_derived_states);
    const auto same_stable_identity =
        [&state_child_before_rebuild](const DerivedUnitDisplayState &state)
        {
            return state.array_parent_uuid ==
                       state_child_before_rebuild->array_parent_uuid &&
                   state.prototype_chain ==
                       state_child_before_rebuild->prototype_chain &&
                   state.array_instance_path ==
                       state_child_before_rebuild->array_instance_path &&
                   state.array_instance_key ==
                       state_child_before_rebuild->array_instance_key &&
                   state.array_layer_uuid ==
                       state_child_before_rebuild->array_layer_uuid;
        };
    const auto saved_state_it = std::find_if(
        saved_derived_states.cbegin(), saved_derived_states.cend(),
        same_stable_identity);
    if (!check(saved_state_it != saved_derived_states.cend(),
               "Hidden derived child should have a stable display-state record"))
    {
        return 1;
    }
    const DerivedUnitDisplayState &saved_first_state = *saved_state_it;
    std::shared_ptr<Unit> restored_state_child;
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(uuid)->child_units)
    {
        if (child != nullptr &&
            child->array_parent_uuid == saved_first_state.array_parent_uuid &&
            child->prototype_chain == saved_first_state.prototype_chain &&
            child->array_instance_path == saved_first_state.array_instance_path &&
            child->array_instance_key == saved_first_state.array_instance_key &&
            child->array_layer_uuid == saved_first_state.array_layer_uuid)
        {
            restored_state_child = child;
            break;
        }
    }
    if (!check(restored_state_child != nullptr &&
                   !widget.unit_visible(restored_state_child->inj.uuid) &&
                   widget.unit_locked(restored_state_child->inj.uuid),
               "Derived array display state should restore after rebuild"))
    {
        return 1;
    }
    widget.set_unit_visible(restored_state_child->inj.uuid, true);
    widget.set_unit_locked(restored_state_child->inj.uuid, false);
    const QVector<std::shared_ptr<Unit>> leaf_children =
        widget.unit_hash.value(uuid)->child_units;
    QUuid leaf_child_uuid = leaf_children.first()->inj.uuid;
    if (!check(leaf_children.at(0) != nullptr && leaf_children.at(1) != nullptr,
               "Leaf array children should be available") ||
        !check(std::abs(shape_center_x(leaf_children.at(1)->inj.shape) -
                        shape_center_x(leaf_children.at(0)->inj.shape)) > 9.0,
               "Leaf array child shapes must move with their data") ||
        !check(std::abs(leaf_children.at(1)->inj.injector_data.pos.x() -
                        leaf_children.at(0)->inj.injector_data.pos.x() - 10.0f) <
                   1.0e-4f,
               "Leaf array child data positions must preserve spacing"))
    {
        return 1;
    }
    widget.set_chemkin_species_names({"O2", "N2"});
    const int species_edit_count =
        widget.set_species_for_units_by_uuid({leaf_child_uuid}, "O2");
    if (!check(species_edit_count == 1,
               "Editing a following child species should edit its prototype") ||
        !check(widget.unit_hash.value(uuid)->inj.injector_data.evaporating_species == "O2",
               "Prototype evaporating species should receive a following-child edit") ||
        !check(std::all_of(widget.unit_hash.value(uuid)->child_units.cbegin(),
                           widget.unit_hash.value(uuid)->child_units.cend(),
                           [](const std::shared_ptr<Unit> &child)
                           {
                               return child != nullptr &&
                                      child->inj.injector_data.evaporating_species == "O2";
                           }),
               "Following children should rebuild from the edited prototype"))
    {
        return 1;
    }
    if (!check(widget.set_unit_direction_by_uuid(
                   uuid, QVector3D(0.0f, 1.0f, 0.0f)),
               "Editing an array source direction should succeed") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 2 &&
                   widget.unit_hash.value(uuid)->child_units.first()
                           ->inj.injector_data.vel ==
                       QVector3D(0.0f, 1.0f, 0.0f),
               "Editing an array source should rebuild its own children"))
    {
        return 1;
    }
    if (!check(widget.unit_hash.value(uuid) != nullptr &&
                   !widget.unit_hash.value(uuid)->child_units.isEmpty(),
               "Rebuilt leaf array should expose a current child instance"))
    {
        return 1;
    }
    // Array rebuilds intentionally regenerate runtime UUIDs. Continue nested
    // editing through the current instance rather than a stale UUID.
    leaf_child_uuid = widget.unit_hash.value(uuid)->child_units.first()->inj.uuid;
    const int nested_array_count =
        widget.create_unit_array(leaf_child_uuid, leaf_array);
    if (!check(nested_array_count == 2,
               "A following array child should be promoted for nested arrays") ||
        !check(widget.unit_hash.value(leaf_child_uuid) != nullptr &&
                   !widget.unit_hash.value(leaf_child_uuid)->follows_array &&
                   widget.unit_hash.value(leaf_child_uuid)->has_array_spec &&
                   widget.unit_hash.value(leaf_child_uuid)->child_units.size() == 2,
               "Promoted array child should become a persistent nested source"))
    {
        return 1;
    }
    if (!check(!widget.set_unit_follow_array(leaf_child_uuid, true),
               "A nested array source must not be discarded by re-enabling inheritance") ||
        !check(widget.unit_hash.value(leaf_child_uuid) != nullptr &&
                   !widget.unit_hash.value(leaf_child_uuid)->follows_array &&
                   widget.unit_hash.value(leaf_child_uuid)->has_array_spec,
               "Rejected inheritance change must preserve nested source state"))
    {
        return 1;
    }

    if (!check(widget.set_unit_direction_by_uuid(
                   leaf_child_uuid, QVector3D(0.0f, 0.0f, 1.0f)),
               "Nested array source direction edit should succeed") ||
        !check(std::all_of(
                   widget.unit_hash.value(leaf_child_uuid)->child_units.cbegin(),
                   widget.unit_hash.value(leaf_child_uuid)->child_units.cend(),
                   [](const std::shared_ptr<Unit> &child)
                   {
                       return child != nullptr &&
                              child->inj.injector_data.vel ==
                                  QVector3D(0.0f, 0.0f, 1.0f);
                   }),
               "Nested array source edits should reach every nested child"))
    {
        return 1;
    }

    const QVector3D nested_source_position_before =
        widget.unit_hash.value(leaf_child_uuid)->inj.injector_data.pos;
    const QVector3D nested_source_direction_before =
        widget.unit_hash.value(leaf_child_uuid)->inj.injector_data.vel;
    if (!check(widget.set_unit_parent_transform_follow(leaf_child_uuid, true),
               "Nested array source should be able to follow parent transform") ||
        !check(widget.unit_hash.value(leaf_child_uuid)->follows_parent_transform,
               "Parent-transform follow flag should be enabled"))
    {
        return 1;
    }
    if (!check(widget.set_unit_position_by_uuid(
                   uuid, QVector3D(20.0f, 0.0f, 0.0f)),
               "Moving the outer array source should succeed") ||
        !check(widget.unit_hash.value(leaf_child_uuid) != nullptr &&
                   widget.unit_hash.value(leaf_child_uuid)->inj.injector_data.pos !=
                       nested_source_position_before,
               "Following nested source should receive parent placement updates") ||
        !check(widget.unit_hash.value(leaf_child_uuid)->inj.injector_data.vel ==
                   nested_source_direction_before,
               "Parent transform follow should preserve nested source direction"))
    {
        return 1;
    }
    if (!check(widget.set_unit_parent_transform_follow(leaf_child_uuid, false),
               "Nested array source should be detachable from parent transform"))
    {
        return 1;
    }

    UnitArraySpec second_leaf_array;
    second_leaf_array.type = UnitArrayType::Rotational;
    second_leaf_array.count = 3;
    second_leaf_array.direction = QVector3D(0.0f, 0.0f, 1.0f);
    second_leaf_array.origin = source.inj.injector_data.pos;
    second_leaf_array.angle_degrees = 180.0f;
    if (!check(widget.create_unit_array(uuid, second_leaf_array) == 4,
               "A second array layer should expand the complete first array") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 2,
               "Array source should retain both array layers") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 4,
               "Second array layer should retain first-layer instances"))
    {
        return 1;
    }
    if (!check(widget.undo_last_edit(),
               "Array creation should be undoable") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 1 &&
                   widget.unit_hash.value(uuid)->child_units.size() == 2,
               "Undo should restore the previous array layer") ||
        !check(widget.redo_edit(),
               "Array creation should be redoable") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 2 &&
                   widget.unit_hash.value(uuid)->child_units.size() == 4,
               "Redo should restore the second array layer"))
    {
        return 1;
    }

    if (!check(widget.set_unit_direction_by_uuid(
                   uuid, QVector3D(0.0f, 0.0f, 1.0f)),
               "Editing a two-layer array source should succeed"))
    {
        return 1;
    }

    QList<std::shared_ptr<Unit>> nested_leaf_units;
    std::function<void(const std::shared_ptr<Unit> &)> collect_leaf_units;
    collect_leaf_units = [&](const std::shared_ptr<Unit> &node)
    {
        if (node == nullptr)
        {
            return;
        }
        if (node->child_units.isEmpty())
        {
            nested_leaf_units.append(node);
            return;
        }
        for (const std::shared_ptr<Unit> &child : node->child_units)
        {
            collect_leaf_units(child);
        }
    };
    for (const std::shared_ptr<Unit> &child : widget.unit_hash.value(uuid)->child_units)
    {
        collect_leaf_units(child);
    }
    if (!check(nested_leaf_units.size() == 6,
               "Two-layer array should expose all first and second layer leaves") ||
        !check(std::all_of(nested_leaf_units.cbegin(), nested_leaf_units.cend(),
                           [](const std::shared_ptr<Unit> &leaf)
                           {
                               const QVector3D direction =
                                   leaf->inj.injector_data.vel;
                               return std::abs(direction.x()) < 1.0e-4f &&
                                      std::abs(direction.y()) < 1.0e-4f &&
                                      direction.z() > 0.99f;
                           }),
               "Two-layer array leaves should rebuild from the edited source"))
    {
        return 1;
    }

    UnitArraySpec edited_first_layer = leaf_array;
    edited_first_layer.count = 3;
    if (!check(widget.update_unit_array_layer(uuid, 0, edited_first_layer),
               "Existing array layer should be editable") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 2 &&
                   widget.unit_hash.value(uuid)->array_specs.first().count == 3,
               "Edited array layer should persist its new parameters"))
    {
        return 1;
    }
    if (!check(widget.move_unit_array_layer(uuid, 0, 1),
               "Array layers should be reorderable") ||
        !check(widget.unit_hash.value(uuid)->array_specs.first().type ==
                   UnitArrayType::Rotational,
               "Reordered array layers should change evaluation order") ||
        !check(widget.remove_unit_array_layer(uuid, 1),
               "Existing array layer should be removable") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 1 &&
                   widget.unit_hash.value(uuid)->array_specs.first().type ==
                       UnitArrayType::Rotational,
               "Removing a layer should retain remaining layers"))
    {
        return 1;
    }

    // A three-layer chain must propagate source edits through every derived
    // level and rebuild to the same leaf count without accumulating outputs.
    widget.display_units({source}, true);
    application.processEvents();
    UnitArraySpec third_layer_linear = leaf_array;
    third_layer_linear.count = 2;
    if (!check(widget.create_unit_array(uuid, leaf_array) == 2,
               "Three-layer setup should create the first layer") ||
        !check(widget.create_unit_array(uuid, second_leaf_array) > 0,
               "Three-layer setup should create the second layer") ||
        !check(widget.create_unit_array(uuid, third_layer_linear) > 0,
               "Three-layer setup should create the third layer") ||
        !check(widget.unit_hash.value(uuid)->array_specs.size() == 3,
               "Three-layer array metadata should be retained"))
    {
        return 1;
    }
    if (!check(widget.set_unit_direction_by_uuid(
                   uuid, QVector3D(1.0f, 0.0f, 0.0f)),
               "Three-layer source direction edit should succeed"))
    {
        return 1;
    }
    nested_leaf_units.clear();
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(uuid)->child_units)
    {
        collect_leaf_units(child);
    }
    if (!check(nested_leaf_units.size() == 8,
               "Three-layer array should expose eight leaf injectors") ||
        !check(std::all_of(nested_leaf_units.cbegin(), nested_leaf_units.cend(),
                           [](const std::shared_ptr<Unit> &leaf)
                           {
                               return leaf != nullptr &&
                                      qAbs(leaf->inj.injector_data.vel.length() -
                                           1.0f) < 1.0e-4f;
                           }),
               "Three-layer leaves should retain transformed direction lengths") ||
        !check(std::count_if(
                   nested_leaf_units.cbegin(), nested_leaf_units.cend(),
                   [](const std::shared_ptr<Unit> &leaf)
                   {
                       return leaf->inj.injector_data.vel ==
                              QVector3D(1.0f, 0.0f, 0.0f);
                   }) == 4,
               "Three-layer source direction should reach linear placements") ||
        !check(std::count_if(
                   nested_leaf_units.cbegin(), nested_leaf_units.cend(),
                   [](const std::shared_ptr<Unit> &leaf)
                   {
                       const QVector3D direction = leaf->inj.injector_data.vel;
                       return qAbs(direction.x() - 0.5f) < 1.0e-4f &&
                              qAbs(direction.y() - 0.8660254f) < 1.0e-4f;
                   }) == 2,
               "Three-layer rotation should transform the first direction") ||
        !check(std::count_if(
                   nested_leaf_units.cbegin(), nested_leaf_units.cend(),
                   [](const std::shared_ptr<Unit> &leaf)
                   {
                       const QVector3D direction = leaf->inj.injector_data.vel;
                       return qAbs(direction.x() + 0.5f) < 1.0e-4f &&
                              qAbs(direction.y() - 0.8660254f) < 1.0e-4f;
                   }) == 2,
               "Three-layer rotation should transform the second direction"))
    {
        return 1;
    }
    const int three_layer_hash_size = widget.unit_hash.size();
    if (!check(widget.rebuild_unit_array(uuid) > 0,
               "Three-layer array should rebuild successfully"))
    {
        return 1;
    }
    nested_leaf_units.clear();
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(uuid)->child_units)
    {
        collect_leaf_units(child);
    }
    if (!check(nested_leaf_units.size() == 8 &&
                   widget.unit_hash.size() == three_layer_hash_size,
               "Three-layer rebuild should not accumulate stale outputs"))
    {
        return 1;
    }

    // Reset to the original single injector before testing Assembly paths.
    widget.display_units({source}, true);
    application.processEvents();
    stored_unit = widget.unit_hash.value(uuid);
    if (!check(widget.unit_hash.size() == 1,
               "Leaf array reset should restore the single source"))
    {
        return 1;
    }

    // Selecting an object must not implicitly leave the persistent toolbar
    // mode. The mouse release path is covered by the same mode contract.
    widget.set_interaction_mode(OCCTWidget::Interaction_Mode::Translation);
    if (!check(widget.select_unit_by_uuid(uuid) &&
                   widget.interaction_mode() ==
                       OCCTWidget::Interaction_Mode::Translation,
               "Translation mode should persist after selecting an injector"))
    {
        return 1;
    }
    widget.set_interaction_mode(OCCTWidget::Interaction_Mode::Rotation);
    if (!check(widget.select_unit_by_uuid(uuid) &&
                   widget.interaction_mode() ==
                       OCCTWidget::Interaction_Mode::Rotation,
               "Rotation mode should persist after selecting an injector"))
    {
        return 1;
    }
    widget.set_interaction_mode(OCCTWidget::Interaction_Mode::Selection);

    Unit second_source = make_valid_unit();
    second_source.inj.injector_data.name = "assembly-seed";
    second_source.inj.injector_data.pos = QVector3D(5.0f, 2.0f, 3.0f);
    if (!check(second_source.inj.create_injector(),
               "Assembly member geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({second_source}, false);
    application.processEvents();
    const QUuid member_uuid = second_source.inj.uuid;
    if (!check(widget.select_units_by_uuid({uuid, member_uuid}, member_uuid),
               "Selecting multiple tree units should succeed") ||
        !check(last_selected_units.size() == 2 &&
                   last_selected_units.contains(uuid) &&
                   last_selected_units.contains(member_uuid),
               "Multiple unit selection should remain synchronized with OCCT"))
    {
        return 1;
    }
    if (!check(widget.create_assembly({uuid, member_uuid}),
               "Assembly creation should succeed"))
    {
        return 1;
    }
    const QVector3D assembly_local_before =
        widget.unit_hash.value(member_uuid)->assembly_local_position;
    if (!check(widget.translate_units_by_uuid({uuid}, QVector3D(2.0f, 0.0f, 0.0f)) > 0,
               "Assembly translation should succeed") ||
        !check(widget.unit_hash.value(member_uuid)->assembly_local_position ==
                   assembly_local_before,
               "Parent translation must preserve child local position"))
    {
        return 1;
    }
    if (!check(widget.translate_units_by_uuid({member_uuid},
                                              QVector3D(0.0f, 1.0f, 0.0f)) > 0,
               "Independent child translation should succeed") ||
        !check(widget.unit_hash.value(member_uuid)->assembly_local_position !=
                   assembly_local_before,
               "Independent child translation must update local position"))
    {
        return 1;
    }
    const QVector3D child_local_before_parent_rotation =
        widget.unit_hash.value(member_uuid)->assembly_local_position;
    if (!check(widget.rotate_units_by_uuid(
                   {uuid}, QVector3D(0.0f, 0.0f, 1.0f), 90.0f,
                   widget.unit_hash.value(uuid)->inj.injector_data.pos, true) > 0,
               "Assembly rotation should succeed") ||
        !check(widget.unit_hash.value(member_uuid)->assembly_local_position !=
                   child_local_before_parent_rotation,
               "Parent rotation must refresh child local position"))
    {
        return 1;
    }
    const QVector3D local_rotation_before_child_edit =
        widget.unit_hash.value(member_uuid)->assembly_local_rotation;
    if (!check(widget.rotate_units_by_uuid(
                   {member_uuid}, QVector3D(0.0f, 0.0f, 1.0f), 30.0f) > 0,
               "Independent child rotation should succeed") ||
        !check(widget.unit_hash.value(member_uuid)->assembly_local_rotation !=
                   local_rotation_before_child_edit,
               "Independent child rotation must update local orientation"))
    {
        return 1;
    }
    if (!check(!widget.copy_unit_by_uuid(uuid),
               "Leaf paste-copy should not replace an Assembly"))
    {
        return 1;
    }

    UnitArraySpec assembly_array;
    assembly_array.type = UnitArrayType::Linear;
    assembly_array.count = 2;
    assembly_array.direction = QVector3D(0.0f, 1.0f, 0.0f);
    assembly_array.spacing = 10.0f;
    const int generated_count = widget.create_unit_array(uuid, assembly_array);
    if (!check(generated_count == 2,
               "Assembly array should create one composite instance per placement") ||
        !check(widget.unit_hash.value(uuid)->type == Assebly,
               "Assembly array source must remain an Assembly") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 3,
               "Assembly source should retain members and own composite children") )
    {
        return 1;
    }
    int generated_children = 0;
    for (const std::shared_ptr<Unit> &child : widget.unit_hash.value(uuid)->child_units)
    {
        if (child != nullptr && child->is_array_child)
        {
            ++generated_children;
            if (!check(child->array_parent_uuid == uuid,
                       "Assembly array child should reference its source Assembly"))
            {
                return 1;
            }
        }
    }
    if (!check(generated_children == 2,
               "Assembly source should own all generated composite instances"))
    {
        return 1;
    }

    const QVector3D gizmo_source_position =
        widget.unit_hash.value(uuid)->inj.injector_data.pos;
    const QVector3D gizmo_member_position =
        widget.unit_hash.value(member_uuid)->inj.injector_data.pos;
    const QVector3D gizmo_array_origin =
        widget.unit_hash.value(uuid)->array_specs.first().origin;
    std::shared_ptr<Unit> gizmo_array_instance;
    for (const std::shared_ptr<Unit> &child : widget.unit_hash.value(uuid)->child_units)
    {
        if (child != nullptr && child->is_array_child)
        {
            gizmo_array_instance = child;
            break;
        }
    }
    if (!check(gizmo_array_instance != nullptr,
               "Gizmo regression should find a generated Assembly instance"))
    {
        return 1;
    }
    const QVector3D gizmo_array_instance_position =
        gizmo_array_instance->inj.injector_data.pos;
    const QVector3D gizmo_delta(0.008f, 0.0f, 0.0f);
    gp_Trsf gizmo_translation;
    gizmo_translation.SetTranslation(
        gp_Vec(gizmo_delta.x(), gizmo_delta.y(), gizmo_delta.z()));
    if (!check(OCCTWidgetEditHistoryTestAccess::begin_preview(
                   widget, uuid, AIS_MM_Translation, gizmo_translation),
               "Assembly translation gizmo should attach") ||
        !check(vectors_close(
                   widget.unit_hash.value(uuid)->inj.injector_data.pos,
                   gizmo_source_position + gizmo_delta) &&
                   vectors_close(
                       widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                       gizmo_member_position + gizmo_delta) &&
                   vectors_close(
                       gizmo_array_instance->inj.injector_data.pos,
                       gizmo_array_instance_position + gizmo_delta),
               "Gizmo preview should transform Assembly members and array instances"))
    {
        return 1;
    }
    OCCTWidgetEditHistoryTestAccess::cancel_preview(widget);
    if (!check(vectors_close(
                   widget.unit_hash.value(uuid)->inj.injector_data.pos,
                   gizmo_source_position) &&
                   vectors_close(
                       widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                       gizmo_member_position) &&
                   vectors_close(
                       widget.unit_hash.value(uuid)->array_specs.first().origin,
                       gizmo_array_origin),
               "Cancel should restore the full Assembly and array preview state"))
    {
        return 1;
    }

    if (!check(OCCTWidgetEditHistoryTestAccess::begin_preview(
                   widget, uuid, AIS_MM_Translation, gizmo_translation) &&
                   OCCTWidgetEditHistoryTestAccess::commit_preview(widget),
               "Assembly translation gizmo should commit") ||
        !check(vectors_close(
                   widget.unit_hash.value(uuid)->inj.injector_data.pos,
                   gizmo_source_position + gizmo_delta) &&
                   vectors_close(
                       widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                       gizmo_member_position + gizmo_delta),
               "Gizmo commit should apply one shared translation to persistent members"))
    {
        return 1;
    }
    if (!check(widget.undo_last_operation(),
               "Assembly gizmo translation should undo as one operation") ||
        !check(vectors_close(widget.unit_hash.value(uuid)->inj.injector_data.pos,
                             gizmo_source_position) &&
                   vectors_close(
                       widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                       gizmo_member_position) &&
                   vectors_close(
                       widget.unit_hash.value(uuid)->array_specs.first().origin,
                       gizmo_array_origin),
               "Undo should restore all Assembly and array coordinates") ||
        !check(widget.redo_operation(),
               "Assembly gizmo translation should redo as one operation") ||
        !check(vectors_close(
                   widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                   gizmo_member_position + gizmo_delta),
               "Redo should restore the Assembly member transform") ||
        !check(widget.undo_last_operation(),
               "Assembly gizmo translation should return to baseline"))
    {
        return 1;
    }

    gp_Trsf gizmo_rotation;
    const QVector3D gizmo_rotation_pivot =
        widget.unit_hash.value(uuid)->inj.injector_data.pos;
    gizmo_rotation.SetRotation(
        gp_Ax1(gp_Pnt(gizmo_rotation_pivot.x(), gizmo_rotation_pivot.y(),
                      gizmo_rotation_pivot.z()),
               gp_Dir(0.0, 0.0, 1.0)),
        qDegreesToRadians(90.0));
    gp_Pnt expected_gizmo_member_position(
        gizmo_member_position.x(), gizmo_member_position.y(),
        gizmo_member_position.z());
    expected_gizmo_member_position.Transform(gizmo_rotation);
    const QVector3D expected_gizmo_member(
        static_cast<float>(expected_gizmo_member_position.X()),
        static_cast<float>(expected_gizmo_member_position.Y()),
        static_cast<float>(expected_gizmo_member_position.Z()));
    const QVector3D array_direction_before_rotation =
        widget.unit_hash.value(uuid)->array_specs.first().direction;
    if (!check(OCCTWidgetEditHistoryTestAccess::begin_preview(
                   widget, uuid, AIS_MM_Rotation, gizmo_rotation) &&
                   vectors_close(
                       widget.unit_hash.value(member_uuid)->inj.injector_data.pos,
                       expected_gizmo_member) &&
                   OCCTWidgetEditHistoryTestAccess::commit_preview(widget),
               "Assembly rotation gizmo should preview and commit its member tree") ||
        !check(!vectors_close(
                   widget.unit_hash.value(uuid)->array_specs.first().direction,
                   array_direction_before_rotation),
               "Rotating an Assembly should rotate its array frame") ||
        !check(widget.undo_last_operation() &&
                   vectors_close(
                       widget.unit_hash.value(uuid)->array_specs.first().direction,
                       array_direction_before_rotation),
               "Undo should restore Assembly array-frame orientation") ||
        !check(widget.redo_operation() &&
                   !vectors_close(
                       widget.unit_hash.value(uuid)->array_specs.first().direction,
                       array_direction_before_rotation),
               "Redo should restore Assembly array-frame orientation") ||
        !check(widget.undo_last_operation(),
               "Assembly gizmo rotation should return to baseline"))
    {
        return 1;
    }

    const QVector3D member_direction(0.0f, 1.0f, 0.0f);
    if (!check(widget.set_unit_direction_by_uuid(member_uuid,
                                                 member_direction),
               "Editing an Assembly member should succeed"))
    {
        return 1;
    }
    std::shared_ptr<Unit> regenerated_member;
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(uuid)->child_units)
    {
        if (child == nullptr || !child->is_array_child)
        {
            continue;
        }
        for (const std::shared_ptr<Unit> &member : child->child_units)
        {
            if (member != nullptr && member->prototype_uuid == member_uuid)
            {
                regenerated_member = member;
                break;
            }
        }
        if (regenerated_member != nullptr)
        {
            break;
        }
    }
    if (!check(regenerated_member != nullptr &&
                   regenerated_member->inj.injector_data.vel == member_direction,
               "Assembly array should rebuild when a member changes"))
    {
        return 1;
    }

    if (!check(widget.unit_hash.value(member_uuid)->assembly_parent_uuid == uuid,
               "Assembly array expansion must preserve the original member link") ||
        !check(std::all_of(widget.unit_hash.value(uuid)->child_units.cbegin(),
                           widget.unit_hash.value(uuid)->child_units.cend(),
                           [](const std::shared_ptr<Unit> &child)
                           {
                               return child == nullptr ||
                                      !child->is_array_child ||
                                      child->assembly_child_uuids.size() == 1;
                           }),
               "Composite Assembly array children must retain their child link"))
    {
        return 1;
    }
    if (!check(widget.rebuild_unit_array(uuid) == 2 &&
                   widget.unit_hash.value(uuid)->child_units.size() == 3 &&
                   widget.unit_hash.size() == 6,
               "Rebuilding an Assembly array should replace, not accumulate, children"))
    {
        return 1;
    }

    UnitFillSpec composite_fill;
    composite_fill.rows = 1;
    composite_fill.columns = 2;
    composite_fill.spacing_x = 8.0f;
    if (!check(widget.create_unit_fill({uuid}, composite_fill) == 2,
               "Assembly fill should create one composite instance per placement") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 3,
               "Assembly fill should retain the source member and own instances") ||
        !check(std::count_if(widget.unit_hash.value(uuid)->child_units.cbegin(),
                             widget.unit_hash.value(uuid)->child_units.cend(),
                             [](const std::shared_ptr<Unit> &child)
                             {
                                 return child != nullptr && child->is_array_child &&
                                        child->child_units.size() == 1;
                             }) == 2,
               "Assembly fill instances should retain their child trees"))
    {
        return 1;
    }
    std::shared_ptr<Unit> override_child;
    for (const std::shared_ptr<Unit> &instance :
         widget.unit_hash.value(uuid)->child_units)
    {
        if (instance != nullptr && instance->is_array_child &&
            !instance->child_units.isEmpty())
        {
            override_child = instance->child_units.first();
            break;
        }
    }
    if (!check(override_child != nullptr,
               "Composite fill should expose a child for override testing") ||
        !check(widget.unit_position_by_uuid(override_child->inj.uuid) ==
                   widget.unit_position_by_uuid(member_uuid),
               "Following composite child should resolve inspector position to its prototype") ||
        !check(widget.set_unit_follow_array(override_child->inj.uuid, false),
               "Composite child should support independent override") ||
        !check(widget.unit_position_by_uuid(override_child->inj.uuid) ==
                   override_child->inj.injector_data.pos,
               "Independent composite child should expose its own inspector position") ||
        !check(widget.rebuild_unit_fill(uuid) > 0,
               "Composite fill rebuild should succeed with an override") ||
        !check(widget.unit_hash.contains(override_child->inj.uuid),
               "Independent composite override must survive rebuild"))
    {
        return 1;
    }

    if (!check(widget.dissolve_assembly(uuid),
               "Assembly with generated children should dissolve cleanly") ||
        !check(widget.unit_hash.size() == 3 &&
                   widget.unit_hash.value(uuid)->child_units.isEmpty() &&
                   widget.unit_hash.value(uuid)->type == injector &&
                   !widget.unit_hash.value(uuid)->has_array_spec &&
                   widget.unit_hash.contains(override_child->inj.uuid),
               "Dissolving an Assembly should preserve independent overrides"))
    {
        return 1;
    }
    if (!check(!widget.create_assembly({uuid, uuid}),
               "Assembly creation with no valid child should fail") ||
        !check(widget.unit_hash.value(uuid)->type == injector &&
                   widget.unit_hash.value(uuid)->assembly_child_uuids.isEmpty() &&
                   widget.unit_hash.size() == 3,
               "Failed Assembly creation must not mutate the source"))
    {
        return 1;
    }

    const QVector3D original_direction = widget.unit_direction_by_uuid(uuid);
    if (!check(widget.set_unit_direction_by_uuid(uuid, QVector3D(0.0f, 1.0f, 0.0f)),
               "Direction inspector edit should succeed") ||
        !check(widget.unit_direction_by_uuid(uuid) == QVector3D(0.0f, 1.0f, 0.0f),
               "Direction inspector edit should update the Unit"))
    {
        return 1;
    }
    if (!check(widget.undo_last_edit(),
               "Direction inspector edit should be undoable") ||
        !check(widget.unit_direction_by_uuid(uuid) == original_direction,
               "Undo should restore the previous Unit direction"))
    {
        return 1;
    }

    stored_unit->inj.injector_data.single_direction_mode =
        Single_Direction_Mode::Target_Hitpoint;
    stored_unit->inj.injector_data.single_target_hitpoint =
        stored_unit->inj.injector_data.pos + QVector3D(0.0f, 0.0f, 4.0f);
    if (!check(stored_unit->inj.create_injector(),
               "Target Hitpoint test geometry should be valid"))
    {
        return 1;
    }
    const Single_Target_Scope original_scope =
        stored_unit->inj.injector_data.single_target_scope;
    if (!check(widget.set_unit_single_target_scope_by_uuid(
                   uuid, Single_Target_Scope::Reference_Local),
               "Target scope inspector edit should succeed") ||
        !check(widget.unit_single_target_scope_by_uuid(uuid) ==
                   Single_Target_Scope::Reference_Local,
               "Target scope inspector edit should update the Unit") ||
        !check(widget.undo_last_edit(),
               "Target scope inspector edit should be undoable") ||
        !check(widget.unit_single_target_scope_by_uuid(uuid) == original_scope,
               "Undo should restore the previous target scope"))
    {
        return 1;
    }

    // Create a history entry whose before-state cannot rebuild. Undo must
    // reject it without damaging the currently valid state.
    stored_unit->inj.injector_data.vel = QVector3D();
    widget.begin_unit_edit_transaction(stored_unit.get());
    stored_unit->inj.injector_data.vel = QVector3D(2.0f, 0.0f, 0.0f);
    widget.finish_unit_edit_transaction(stored_unit.get(), true);

    const QVector3D valid_velocity = stored_unit->inj.injector_data.vel;
    if (!check(!widget.undo_last_edit(),
               "Undo should reject an invalid snapshot"))
    {
        return 1;
    }
    if (!check(stored_unit->inj.injector_data.vel == valid_velocity,
               "Failed undo should preserve the valid current injector data") ||
        !check(widget.create_reference_section_plane(10.0, 0.01,
                                                    QVector3D(0.0f, 0.0f, 1.0f)),
               "Section Plane creation should succeed") ||
        !check(widget.set_section_plane_clipping(true),
               "Section Plane clipping should succeed") ||
        !check(widget.section_plane_clipping_enabled(),
               "Section Plane clipping should be enabled") ||
        !check(widget.set_section_plane_clipping(false),
               "Section Plane clipping should be reversible") ||
        !check(!widget.section_plane_clipping_enabled(),
               "Section Plane clipping should be disabled after restore") ||
        !check(widget.select_reference_face_by_index(0),
               "Reference face selection should succeed") )
    {
        return 1;
    }

    const QVector3D saved_face_origin =
        widget.reference_selected_face_origin();
    const QVector3D saved_face_normal =
        widget.reference_selected_face_normal();
    const QVector3D saved_face_x =
        widget.reference_selected_face_x_direction();
    if (!check(widget.select_reference_face_by_descriptor(
                   saved_face_origin, saved_face_normal, saved_face_x, -1),
               "Reference face descriptor should restore the same face"))
    {
        return 1;
    }

    Unit droplet = make_valid_unit();
    droplet.inj.injector_data.type = Droplet;
    droplet.inj.injector_data.injection_type = single;
    droplet.inj.injector_data.material = "liquid-material";
    droplet.inj.injector_data.evaporating_species.clear();
    Unit inert = make_valid_unit();
    inert.inj.injector_data.type = Inert;
    inert.inj.injector_data.injection_type = single;
    inert.inj.injector_data.material.clear();
    if (!check(droplet.inj.create_injector() && inert.inj.create_injector(),
               "Species assignment test geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({droplet, inert}, true);
    widget.set_chemkin_species_names({"H2O", "O2"});
    widget.set_species_colors({{"H2O", QColor(255, 0, 0)},
                               {"O2", QColor(0, 255, 0)}});
    const QUuid droplet_uuid = droplet.inj.uuid;
    const QUuid inert_uuid = inert.inj.uuid;
    if (!check(widget.set_species_for_units_by_uuid({droplet_uuid}, "H2O") == 1 &&
                   widget.set_species_for_units_by_uuid({inert_uuid}, "O2") == 1,
               "Species assignment should update both particle types") ||
        !check(widget.unit_hash.value(droplet_uuid)->inj.injector_data.material ==
                   "liquid-material" &&
                   widget.unit_hash.value(droplet_uuid)->inj.injector_data.evaporating_species ==
                   "H2O",
               "Droplet Species assignment must not overwrite Material") ||
        !check(widget.unit_hash.value(inert_uuid)->inj.injector_data.material == "O2",
               "Non-droplet Species assignment should update Material"))
    {
        return 1;
    }
    Quantity_Color droplet_color;
    Quantity_Color inert_color;
    widget.unit_hash.value(droplet_uuid)->ais_display->Color(droplet_color);
    widget.unit_hash.value(inert_uuid)->ais_display->Color(inert_color);
    if (!check(std::abs(droplet_color.Red() - 1.0) < 1.0e-6 &&
                   droplet_color.Green() < 1.0e-6 &&
                   inert_color.Green() > 0.99 && inert_color.Red() < 1.0e-6,
               "Injector colors should follow Droplet evaporating Species and other Material"))
    {
        return 1;
    }

    if (!check(widget.create_reference_section_plane(10.0, 0.01,
                                                    QVector3D(0.0f, 0.0f, 1.0f)),
               "Section Plane creation should succeed") ||
        !check(widget.set_section_plane_clipping(true),
               "Section Plane clipping should succeed") ||
        !check(widget.section_plane_clipping_enabled(),
               "Section Plane clipping should be enabled") ||
        !check(widget.set_section_plane_clipping(false),
               "Section Plane clipping should be reversible") ||
        !check(!widget.section_plane_clipping_enabled(),
               "Section Plane clipping should be disabled after restore"))
    {
        return 1;
    }

    Unit reference_bound_source = make_valid_unit();
    reference_bound_source.inj.injector_data.name = "reference-bound-source";
    reference_bound_source.has_array_spec = true;
    reference_bound_source.array_spec.type = UnitArrayType::Linear;
    reference_bound_source.array_spec.count = 2;
    reference_bound_source.array_spec.direction = QVector3D(1.0f, 0.0f, 0.0f);
    reference_bound_source.array_spec.spacing = 2.0f;
    reference_bound_source.array_spec.origin =
        reference_bound_source.inj.injector_data.pos;
    reference_bound_source.array_spec.use_reference_geometry = true;
    reference_bound_source.array_spec.conform_to_reference_normal = true;
    reference_bound_source.array_specs = {reference_bound_source.array_spec};
    if (!check(reference_bound_source.inj.create_injector(),
               "Reference-bound source geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({reference_bound_source}, true);
    if (!check(widget.create_reference_datum_plane(10.0, 0.01,
                                                   QVector3D(0.0f, 0.0f, 1.0f)),
               "Reference datum plane should be available for dependency test"))
    {
        return 1;
    }
    const QUuid reference_bound_uuid = reference_bound_source.inj.uuid;
    widget.set_reference_transform(QVector3D(2.0f, 0.0f, 0.0f),
                                   QVector3D(0.0f, 0.0f, 90.0f), false);
    if (!check(widget.unit_hash.value(reference_bound_uuid)->array_spec.direction ==
                   QVector3D(1.0f, 0.0f, 0.0f),
               "Restoring a saved reference transform should not rebase specs"))
    {
        return 1;
    }
    widget.set_reference_transform(QVector3D(), QVector3D(), false);
    widget.begin_reference_transform_transaction();
    widget.set_reference_transform(QVector3D(2.0f, 0.0f, 0.0f),
                                   QVector3D(0.0f, 0.0f, 90.0f));
    widget.finish_reference_transform_transaction();
    const std::shared_ptr<Unit> active_reference_array_source =
        widget.unit_hash.value(reference_bound_uuid);
    if (!check(widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .use_reference_geometry,
               "Reference-bound Array should retain its dependency before clear") ||
        !check(vectors_close(
                   active_reference_array_source->array_spec.direction,
                   QVector3D(0.0f, 1.0f, 0.0f)) &&
                   vectors_close(
                       active_reference_array_source->child_units.at(1)
                               ->inj.injector_data.pos -
                           active_reference_array_source->inj.injector_data.pos,
                       QVector3D(0.0f, 2.0f, 0.0f)),
               "Active reference rotation should update bound Array geometry") ||
        !check(widget.undo_reference_transform() &&
                   vectors_close(
                       active_reference_array_source->array_spec.direction,
                       QVector3D(1.0f, 0.0f, 0.0f)) &&
                   vectors_close(
                       active_reference_array_source->child_units.at(1)
                               ->inj.injector_data.pos -
                           active_reference_array_source->inj.injector_data.pos,
                       QVector3D(2.0f, 0.0f, 0.0f)),
               "Undo should restore reference-bound Array coordinates") ||
        !check(widget.redo_reference_transform() &&
                   vectors_close(
                       active_reference_array_source->array_spec.direction,
                       QVector3D(0.0f, 1.0f, 0.0f)),
               "Redo should reapply reference-bound Array coordinates") ||
        !check([&widget]()
               {
                   QVector3D origin;
                   QVector3D x_axis;
                   QVector3D z_axis;
                   return widget.reference_frame(&origin, &x_axis, &z_axis) &&
                          vectors_close(x_axis, QVector3D(0.0f, 1.0f, 0.0f));
               }(),
               "Reference axis direction should not be skewed by translation"))
    {
        return 1;
    }
    widget.set_reference_transform(QVector3D(3.0f, 0.0f, 0.0f),
                                   QVector3D());
    if (!check(widget.can_undo_reference_transform() &&
                   widget.undo_reference_transform() &&
                   vectors_close(
                       active_reference_array_source->array_spec.direction,
                       QVector3D(0.0f, 1.0f, 0.0f)) &&
                   widget.redo_reference_transform() &&
                   vectors_close(
                       active_reference_array_source->array_spec.direction,
                       QVector3D(1.0f, 0.0f, 0.0f)),
               "Direct active-reference transforms should be undoable"))
    {
        return 1;
    }
    widget.set_reference_transform(QVector3D(2.0f, 0.0f, 0.0f),
                                   QVector3D(0.0f, 0.0f, 90.0f));
    if (!check(widget.clear_reference_geometry(),
               "Clearing reference geometry should succeed") ||
        !check(!widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .use_reference_geometry &&
                   !widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .conform_to_reference_normal,
               "Clearing reference geometry should downgrade Array dependencies"))
    {
        return 1;
    }

    ReferenceGeometryConfig secondary_reference;
    secondary_reference.uuid = QUuid::createUuid();
    secondary_reference.kind = QStringLiteral("datum_plane");
    secondary_reference.construction_size = 1.0;
    secondary_reference.construction_thickness = 0.01;
    secondary_reference.position = QVector3D(4.0f, 5.0f, 6.0f);
    if (!check(widget.add_reference_geometry_visual(secondary_reference),
               "A secondary reference geometry should display") ||
        !check(widget.has_reference_geometry_visual(secondary_reference.uuid) &&
                   widget.reference_geometry_visual_visible(
                       secondary_reference.uuid),
               "Secondary reference geometry should have independent visibility") ||
        !check(widget.select_reference_geometry_visual(secondary_reference.uuid) &&
                   widget.selected_reference_geometry_uuid() ==
                       secondary_reference.uuid,
               "Secondary reference geometry should be selectable by UUID") ||
        !check(widget.select_reference_geometry_visual_face_by_index(
                   secondary_reference.uuid, 0) &&
                   widget.reference_geometry_visual_selected_face_index(
                       secondary_reference.uuid) == 0,
               "Secondary reference faces should be individually selectable") ||
        !check(widget.select_reference_geometry_visual(
                   secondary_reference.uuid) &&
                   widget.reference_geometry_visual_selected_face_index(
                       secondary_reference.uuid) == -1,
               "Changing selection should clear stale per-reference face state") ||
        !check(widget.set_reference_geometry_visual_transform(
                   secondary_reference.uuid,
                   QVector3D(7.0f, 8.0f, 9.0f),
                   QVector3D(0.0f, 0.0f, 45.0f)) &&
                   vectors_close(widget.reference_geometry_visual_position(
                                     secondary_reference.uuid),
                                 QVector3D(7.0f, 8.0f, 9.0f)),
               "Secondary reference transform should be independently editable") ||
        !check([&widget, &secondary_reference]()
               {
                   QVector3D origin;
                   QVector3D x_axis;
                   QVector3D z_axis;
                   QUuid reference_uuid;
                   return widget.reference_frame(&origin, &x_axis, &z_axis,
                                                 &reference_uuid) &&
                          reference_uuid == secondary_reference.uuid &&
                          vectors_close(origin, QVector3D(7.0f, 8.0f, 9.0f));
               }(),
               "Reference frame should retain the UUID of the selected visual") ||
        !check(widget.undo_reference_transform() &&
                   vectors_close(
                       widget.reference_geometry_visual_position(
                           secondary_reference.uuid),
                       secondary_reference.position) &&
                   widget.redo_reference_transform() &&
                   vectors_close(
                       widget.reference_geometry_visual_position(
                           secondary_reference.uuid),
                       QVector3D(7.0f, 8.0f, 9.0f)),
               "Direct secondary-reference transforms should be undoable") ||
        !check(widget.set_reference_geometry_visual_locked(
                   secondary_reference.uuid, true) &&
                   !widget.set_reference_geometry_visual_transform(
                       secondary_reference.uuid, QVector3D(), QVector3D()),
               "Locked secondary reference geometry should reject transforms") ||
        !check(widget.set_reference_geometry_visual_locked(
                   secondary_reference.uuid, false),
               "Secondary reference geometry should be unlockable"))
    {
        return 1;
    }

    ReferenceGeometryConfig unselected_reference;
    unselected_reference.uuid = QUuid::createUuid();
    unselected_reference.kind = QStringLiteral("datum_axis");
    unselected_reference.position = QVector3D(-2.0f, 3.0f, 4.0f);
    unselected_reference.rotation = QVector3D(0.0f, 0.0f, 90.0f);
    unselected_reference.construction_direction = QVector3D(1.0f, 0.0f, 0.0f);
    const QUuid selected_reference_before_frame_query =
        widget.selected_reference_geometry_uuid();
    QVector3D unselected_origin;
    QVector3D unselected_x;
    QVector3D unselected_z;
    if (!check(widget.add_reference_geometry_visual(unselected_reference) &&
                   widget.reference_frame_uuids().contains(
                       unselected_reference.uuid) &&
                   !widget.reference_frame_label(
                        unselected_reference.uuid).isEmpty() &&
                   widget.reference_frame_for_uuid(
                       unselected_reference.uuid, &unselected_origin,
                       &unselected_x, &unselected_z) &&
                   vectors_close(unselected_origin,
                                 unselected_reference.position) &&
                   vectors_close(unselected_x, QVector3D(-1.0f, 0.0f, 0.0f)) &&
                   vectors_close(unselected_z, QVector3D(0.0f, 1.0f, 0.0f)) &&
                   std::abs(QVector3D::dotProduct(unselected_x,
                                                  unselected_z)) < 1.0e-6f &&
                   widget.selected_reference_geometry_uuid() ==
                       selected_reference_before_frame_query,
               "A reference frame should be addressable by UUID without changing selection"))
    {
        return 1;
    }

    Unit secondary_reference_source = make_valid_unit();
    secondary_reference_source.inj.injector_data.name =
        "secondary-reference-bound-source";
    secondary_reference_source.has_array_spec = true;
    secondary_reference_source.array_spec.type = UnitArrayType::Linear;
    secondary_reference_source.array_spec.count = 2;
    secondary_reference_source.array_spec.direction =
        QVector3D(1.0f, 0.0f, 0.0f);
    secondary_reference_source.array_spec.spacing = 1.0f;
    secondary_reference_source.array_spec.use_reference_geometry = true;
    secondary_reference_source.array_spec.reference_geometry_uuid =
        secondary_reference.uuid;
    QVector3D initial_array_origin;
    QVector3D initial_array_direction;
    QVector3D initial_array_normal;
    QUuid initial_array_reference_uuid;
    if (!check(widget.reference_frame(
                   &initial_array_origin, &initial_array_direction,
                   &initial_array_normal, &initial_array_reference_uuid),
               "Selected reference should provide the initial Array frame"))
    {
        return 1;
    }
    secondary_reference_source.array_spec.origin = initial_array_origin;
    secondary_reference_source.array_spec.direction = initial_array_direction;
    secondary_reference_source.array_spec.plane_normal = initial_array_normal;
    secondary_reference_source.array_spec.reference_geometry_uuid =
        initial_array_reference_uuid;
    secondary_reference_source.array_specs = {
        secondary_reference_source.array_spec};
    if (!check(secondary_reference_source.inj.create_injector(),
               "Secondary reference dependency fixture should be valid"))
    {
        return 1;
    }
    widget.display_units({secondary_reference_source}, true);
    const std::shared_ptr<Unit> stored_secondary_array_source =
        widget.unit_hash.value(secondary_reference_source.inj.uuid);
    if (!check(stored_secondary_array_source != nullptr &&
                   stored_secondary_array_source->child_units.size() == 2,
               "Reference-bound Array should initially generate two children") ||
        !check(vectors_close(
                   stored_secondary_array_source->child_units.at(1)
                           ->inj.injector_data.pos -
                       stored_secondary_array_source->inj.injector_data.pos,
                   initial_array_direction),
               "Initial Array spacing should follow its saved direction") ||
        !check(widget.set_reference_geometry_visual_transform(
                   secondary_reference.uuid,
                   QVector3D(7.0f, 8.0f, 9.0f),
                   QVector3D(0.0f, 0.0f, 90.0f)),
               "Secondary reference rotation should update bound Arrays"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> rotated_array_source =
        widget.unit_hash.value(secondary_reference_source.inj.uuid);
    if (!check(vectors_close(
                   rotated_array_source->array_specs.last().direction,
                   QVector3D(0.0f, 1.0f, 0.0f)) &&
                   vectors_close(
                       rotated_array_source->child_units.at(1)
                               ->inj.injector_data.pos -
                           rotated_array_source->inj.injector_data.pos,
                       QVector3D(0.0f, 1.0f, 0.0f)),
               "Array rules and generated children should follow reference rotation"))
    {
        return 1;
    }

    Unit reference_fill_source = make_valid_unit();
    reference_fill_source.inj.injector_data.name =
        "secondary-reference-bound-fill-source";
    if (!check(reference_fill_source.inj.create_injector(),
               "Secondary reference Fill fixture should be valid"))
    {
        return 1;
    }
    widget.display_units({reference_fill_source}, false, false);
    if (!check(widget.select_reference_geometry_visual(
                   secondary_reference.uuid),
               "Secondary reference should be selectable for Fill setup"))
    {
        return 1;
    }
    QVector3D fill_origin;
    QVector3D fill_x;
    QVector3D fill_z;
    QUuid fill_reference_uuid;
    if (!check(widget.reference_frame(&fill_origin, &fill_x, &fill_z,
                                     &fill_reference_uuid),
               "Selected secondary reference should provide a Fill frame"))
    {
        return 1;
    }
    UnitFillSpec reference_fill;
    reference_fill.rows = 1;
    reference_fill.columns = 2;
    reference_fill.spacing_x = 2.0f;
    reference_fill.origin = fill_origin;
    reference_fill.direction = fill_x;
    reference_fill.plane_normal = fill_z;
    reference_fill.use_reference_geometry = true;
    reference_fill.reference_geometry_uuid = fill_reference_uuid;
    if (!check(widget.create_unit_fill(
                   {reference_fill_source.inj.uuid}, reference_fill) == 2,
               "Reference-bound Fill should generate two placements"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> stored_fill_source =
        widget.unit_hash.value(reference_fill_source.inj.uuid);
    const QVector3D fill_first_before =
        stored_fill_source->child_units.at(0)->inj.injector_data.pos;
    const QVector3D fill_second_before =
        stored_fill_source->child_units.at(1)->inj.injector_data.pos;
    if (!check(widget.set_reference_geometry_visual_transform(
                   secondary_reference.uuid,
                   QVector3D(9.0f, 10.0f, 11.0f),
                   QVector3D(0.0f, 0.0f, 90.0f)),
               "Secondary reference translation should update bound Fills") ||
        !check(vectors_close(
                   stored_fill_source->fill_spec.origin,
                   fill_origin + QVector3D(2.0f, 2.0f, 2.0f)) &&
                   vectors_close(
                       stored_fill_source->child_units.at(0)
                               ->inj.injector_data.pos,
                       fill_first_before + QVector3D(2.0f, 2.0f, 2.0f)) &&
                   vectors_close(
                       stored_fill_source->child_units.at(1)
                               ->inj.injector_data.pos,
                       fill_second_before + QVector3D(2.0f, 2.0f, 2.0f)),
               "Fill frame and generated placements should follow translation"))
    {
        return 1;
    }

    widget.display_units({}, true);
    Unit nested_reference_source = make_valid_unit();
    nested_reference_source.inj.injector_data.name =
        "nested-reference-array-source";
    if (!check(nested_reference_source.inj.create_injector(),
               "Nested reference-bound Array fixture should be valid"))
    {
        return 1;
    }
    widget.display_units({nested_reference_source});
    const QUuid nested_reference_root_uuid =
        nested_reference_source.inj.uuid;
    QVector3D nested_reference_origin;
    QVector3D nested_reference_direction;
    QVector3D nested_reference_normal;
    if (!check(widget.reference_frame_for_uuid(
                   secondary_reference.uuid, &nested_reference_origin,
                   &nested_reference_direction, &nested_reference_normal),
               "Nested Array setup should resolve its reference by UUID"))
    {
        return 1;
    }
    UnitArraySpec outer_reference_array;
    outer_reference_array.type = UnitArrayType::Linear;
    outer_reference_array.count = 2;
    outer_reference_array.spacing = 2.0f;
    outer_reference_array.origin = nested_reference_origin;
    outer_reference_array.direction = nested_reference_direction;
    outer_reference_array.plane_normal = nested_reference_normal;
    outer_reference_array.use_reference_geometry = true;
    outer_reference_array.reference_geometry_uuid = secondary_reference.uuid;
    if (!check(widget.create_unit_array(nested_reference_root_uuid,
                                        outer_reference_array) == 2,
               "Reference-bound outer Array should create two instances"))
    {
        return 1;
    }
    const auto nested_reference_root =
        widget.unit_hash.value(nested_reference_root_uuid);
    if (!check(nested_reference_root != nullptr &&
                   !nested_reference_root->child_units.isEmpty(),
               "Reference-bound outer Array should expose a child source"))
    {
        return 1;
    }
    const QUuid nested_reference_child_uuid =
        nested_reference_root->child_units.first()->inj.uuid;
    UnitArraySpec inner_reference_array = outer_reference_array;
    inner_reference_array.spacing = 0.5f;
    if (!check(widget.create_unit_array(nested_reference_child_uuid,
                                        inner_reference_array) > 0,
               "Reference-bound nested Array should be created from its child") ||
        !check(widget.unit_hash.value(nested_reference_child_uuid) != nullptr &&
                   widget.unit_hash.value(nested_reference_child_uuid)
                       ->has_array_spec,
               "Nested source should retain its own bound Array rule"))
    {
        return 1;
    }
    if (!check(widget.set_reference_geometry_visual_transform(
                   secondary_reference.uuid,
                   QVector3D(10.0f, 11.0f, 12.0f),
                   QVector3D(0.0f, 0.0f, 180.0f)),
               "Moving shared reference should rebuild nested Arrays"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> moved_nested_reference_root =
        widget.unit_hash.value(nested_reference_root_uuid);
    const std::shared_ptr<Unit> moved_nested_reference_source =
        widget.unit_hash.value(nested_reference_child_uuid);
    if (!check(moved_nested_reference_root != nullptr &&
                   moved_nested_reference_source != nullptr &&
                   moved_nested_reference_source->child_units.size() == 2 &&
                   vectors_close(
                       moved_nested_reference_root->array_specs.last().direction,
                       QVector3D(-1.0f, 0.0f, 0.0f)) &&
                   vectors_close(
                       moved_nested_reference_source->array_specs.last().direction,
                       QVector3D(-1.0f, 0.0f, 0.0f)) &&
                   vectors_close(
                       moved_nested_reference_source->child_units.at(1)
                               ->inj.injector_data.pos -
                           moved_nested_reference_source->inj.injector_data.pos,
                       QVector3D(-0.5f, 0.0f, 0.0f)),
               "Reference motion should update both nested rules and leaf placements"))
    {
        return 1;
    }
    widget.display_units({secondary_reference_source}, true);
    application.processEvents();

    if (!check(widget.select_reference_geometry_visual(
                   secondary_reference.uuid) &&
                   widget.select_reference_geometry_visual_face_by_index(
                       secondary_reference.uuid, 0) &&
                   widget.select_unit_by_uuid(
                       secondary_reference_source.inj.uuid),
               "Selecting an injector should preserve the chosen reference face") ||
        !check([&widget, &secondary_reference]()
               {
                   QVector3D origin;
                   QVector3D x_axis;
                   QVector3D z_axis;
                   QUuid reference_uuid;
                   return widget.reference_frame(&origin, &x_axis, &z_axis,
                                                 &reference_uuid) &&
                          reference_uuid == secondary_reference.uuid;
               }(),
               "Reference frame should retain its face owner while selecting an injector"))
    {
        return 1;
    }
    if (!check(widget.remove_reference_geometry_visual(
                   secondary_reference.uuid),
               "Secondary reference geometry should be removable") ||
        !check(widget.selected_reference_geometry_uuid().isNull(),
               "Removing a selected visual should clear its reference-frame UUID") ||
        !check(!widget.has_reference_geometry_visual(secondary_reference.uuid) &&
                   !widget.reference_frame_uuids().contains(
                       secondary_reference.uuid) &&
                   !widget.reference_frame_for_uuid(
                       secondary_reference.uuid, &unselected_origin,
                       &unselected_x, &unselected_z) &&
                   !widget.unit_hash.value(secondary_reference_source.inj.uuid)
                        ->array_spec.use_reference_geometry &&
                   widget.unit_hash.value(secondary_reference_source.inj.uuid)
                        ->array_spec.reference_geometry_uuid.isNull(),
               "Removing secondary reference should detach only its Array dependency"))
    {
        return 1;
    }

    // Structural additions and deletes must preserve the same persistent
    // hierarchy that was visible before the operation.
    widget.display_units({source}, true);
    application.processEvents();
    if (!check(widget.clone_unit_tree_by_uuid(uuid),
               "Cloning a Unit should create a structural history entry") ||
        !check(widget.unit_hash.size() == 2,
               "Clone should add one persistent Unit") ||
        !check(widget.undo_last_edit() && widget.unit_hash.size() == 1,
               "Undo should remove a cloned Unit") ||
        !check(widget.redo_edit() && widget.unit_hash.size() == 2,
               "Redo should restore a cloned Unit"))
    {
        return 1;
    }

    widget.display_units({source}, true);
    Unit delete_member = make_valid_unit();
    delete_member.inj.injector_data.name = "delete-member";
    delete_member.inj.injector_data.pos = QVector3D(5.0f, 2.0f, 3.0f);
    if (!check(delete_member.inj.create_injector(),
               "Delete hierarchy member geometry should be valid"))
    {
        return 1;
    }
    const QUuid delete_member_uuid = delete_member.inj.uuid;
    widget.display_units({delete_member}, false);
    if (!check(widget.create_assembly({uuid, delete_member_uuid}),
               "Delete hierarchy Assembly should be created") ||
        !check(widget.remove_unit_by_uuid(uuid),
               "Assembly delete should succeed"))
    {
        return 1;
    }
    if (!check(widget.undo_last_delete(),
               "Undo delete should restore the full Assembly hierarchy") ||
        !check(widget.unit_hash.contains(uuid) &&
                   widget.unit_hash.value(uuid)->type == Assebly &&
                   widget.unit_hash.value(delete_member_uuid)->assembly_parent_uuid == uuid,
               "Undo delete should restore parent and child relationships") ||
        !check(widget.redo_delete() && !widget.unit_hash.contains(uuid),
               "Redo delete should remove the complete hierarchy again"))
    {
        return 1;
    }

    // Property-level array overrides must survive parent edits without
    // turning the generated child into an unrelated persistent Unit.
    Unit override_source = make_valid_unit();
    override_source.inj.injector_data.name = "override-source";
    override_source.inj.injector_data.type = Inert;
    override_source.inj.injector_data.material = "O2";
    override_source.inj.injector_data.pos = QVector3D(20.0f, 0.0f, 0.0f);
    if (!check(override_source.inj.create_injector(),
               "Override source geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({override_source}, true);
    widget.set_chemkin_species_names({"O2", "N2"});
    const QUuid override_source_uuid = widget.unit_hash.constBegin().key();
    UnitArraySpec override_array;
    override_array.type = UnitArrayType::Linear;
    override_array.count = 2;
    override_array.direction = QVector3D(1.0f, 0.0f, 0.0f);
    override_array.origin = override_source.inj.injector_data.pos;
    override_array.spacing = 3.0f;
    if (!check(widget.create_unit_array(override_source_uuid, override_array) == 2,
               "Override regression array should create two children"))
    {
        return 1;
    }

    const auto find_override_child = [&]() -> std::shared_ptr<Unit>
    {
        const std::shared_ptr<Unit> source_unit =
            widget.unit_hash.value(override_source_uuid);
        if (source_unit == nullptr)
        {
            return nullptr;
        }
        for (const std::shared_ptr<Unit> &child : source_unit->child_units)
        {
            if (child != nullptr && child->array_instance_path == QVector<int>{0})
            {
                return child;
            }
        }
        return nullptr;
    };

    std::shared_ptr<Unit> physical_child = find_override_child();
    if (!check(physical_child != nullptr,
               "Override regression child should be available") ||
        !check(widget.set_unit_array_override_scope(
                    physical_child->inj.uuid, true, false),
                "Physical override should be enabled"))
    {
        return 1;
    }
    const QUuid physical_child_uuid_before_rebuild = physical_child->inj.uuid;
    physical_child->inj.injector_data.material = "O2";
    if (!check(widget.set_species_for_units_by_uuid(
                    {override_source_uuid}, "N2") == 1,
                "Parent material edit should succeed"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> physical_child_after_rebuild =
        find_override_child();
    if (!check(physical_child_after_rebuild != nullptr &&
                   physical_child_after_rebuild->inj.injector_data.material == "O2",
               "Physical override should survive parent material edit") ||
        !check(physical_child_after_rebuild != nullptr &&
                   physical_child_after_rebuild->inj.uuid !=
                   physical_child_uuid_before_rebuild &&
                   widget.unit_hash.value(physical_child_after_rebuild->inj.uuid).get() ==
                       physical_child_after_rebuild.get(),
               "Physical override must not overwrite regenerated child UUID"))
    {
        return 1;
    }

    std::shared_ptr<Unit> geometry_child = find_override_child();
    const QVector3D local_override_position(77.0f, 4.0f, 0.0f);
    if (!check(geometry_child != nullptr &&
                   widget.set_unit_array_override_scope(
                       geometry_child->inj.uuid, true, true),
               "Geometry override should be enabled") ||
        !check(widget.set_unit_position_by_uuid(
                    geometry_child->inj.uuid, local_override_position),
                "Geometry override position edit should succeed"))
    {
        return 1;
    }
    if (!check(widget.set_unit_position_by_uuid(
                    override_source_uuid, QVector3D(120.0f, 0.0f, 0.0f)),
                "Parent position edit should succeed with child override"))
    {
        return 1;
    }
    geometry_child = find_override_child();
    if (!check(geometry_child != nullptr &&
                   geometry_child->inj.injector_data.pos ==
                       local_override_position,
               "Geometry override should survive parent position edit"))
    {
        return 1;
    }

    if (!check(widget.restore_unit_array_inheritance(geometry_child->inj.uuid),
               "Restoring array inheritance should rebuild the child") ||
        !check(find_override_child() != nullptr &&
                   find_override_child()->inj.injector_data.material == "N2",
               "Restored child should follow parent physical properties"))
    {
        return 1;
    }

    // Narrow override scopes must not detach unrelated properties. Material
    // and position can diverge independently while direction/other state keeps
    // following the parent source.
    std::shared_ptr<Unit> material_child = find_override_child();
    if (!check(material_child != nullptr,
               "Material-scope child should be available") ||
        !check(widget.set_unit_array_override_fields(
                    material_child->inj.uuid,
                    UnitArrayOverrideMaterialSpecies),
                "Material/species override should be enabled") ||
        !check((widget.unit_array_override_fields(
                    material_child->inj.uuid) &
                UnitArrayOverrideMaterialSpecies) != 0,
               "Material/species override mask should persist"))
    {
        return 1;
    }
    material_child = find_override_child();
    material_child->inj.injector_data.material = "N2";
    widget.capture_unit_array_override(material_child.get());
    if (!check(widget.set_species_for_units_by_uuid(
                    {override_source_uuid}, "O2") == 1,
                "Parent material should change after narrow override") ||
        !check(find_override_child() != nullptr &&
                   find_override_child()->inj.injector_data.material == "N2",
               "Narrow material override should survive parent edit"))
    {
        return 1;
    }

    std::shared_ptr<Unit> position_child = find_override_child();
    const QVector3D narrow_position(91.0f, 6.0f, 0.0f);
    if (!check(widget.set_unit_array_override_fields(
                    position_child->inj.uuid,
                    UnitArrayOverrideMaterialSpecies |
                        UnitArrayOverridePosition),
                "Position override should be enabled") ||
        !check(widget.set_unit_position_by_uuid(
                    position_child->inj.uuid, narrow_position),
                "Position override should edit only the selected child") ||
        !check(widget.set_unit_position_by_uuid(
                    override_source_uuid, QVector3D(150.0f, 0.0f, 0.0f)),
                "Parent position should change with narrow child override"))
    {
        return 1;
    }
    position_child = find_override_child();
    if (!check(position_child != nullptr &&
                   position_child->inj.injector_data.pos == narrow_position,
               "Narrow position override should survive parent movement") ||
        !check(position_child->inj.injector_data.material == "N2",
               "Position override should retain independent material scope"))
    {
        return 1;
    }

    // Fill parents must preserve the same parent-transform-follow semantics
    // as Array parents when a generated child becomes a nested source.
    widget.display_units({}, true);
    Unit fill_root = make_valid_unit();
    fill_root.inj.injector_data.name = "fill-root";
    fill_root.inj.injector_data.pos = QVector3D(100.0f, 0.0f, 0.0f);
    fill_root.inj.injector_data.vel = QVector3D(0.0f, 1.0f, 0.0f);
    if (!check(fill_root.inj.create_injector(),
               "Fill root geometry should be valid"))
    {
        return 1;
    }
    widget.display_units({fill_root});
    const QUuid fill_root_uuid = widget.unit_hash.constBegin().key();
    UnitFillSpec fill_spec;
    fill_spec.rows = 1;
    fill_spec.columns = 2;
    fill_spec.spacing_x = 5.0f;
    fill_spec.spacing_y = 5.0f;
    fill_spec.origin = fill_root.inj.injector_data.pos;
    fill_spec.direction = QVector3D(1.0f, 0.0f, 0.0f);
    fill_spec.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    fill_spec.source_weights = {1};
    if (!check(widget.create_unit_fill({fill_root_uuid}, fill_spec) == 2,
               "Fill parent should create two children"))
    {
        return 1;
    }
    std::shared_ptr<Unit> fill_child;
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(fill_root_uuid)->child_units)
    {
        if (child != nullptr && child->array_instance_path == QVector<int>{0})
        {
            fill_child = child;
            break;
        }
    }
    if (!check(fill_child != nullptr,
               "Fill child should be available for nested source promotion") ||
        !check(widget.create_unit_array(
                    fill_child->inj.uuid,
                    UnitArraySpec{QUuid(), {}, UnitArrayType::Linear, 2,
                                  QVector3D(1.0f, 0.0f, 0.0f),
                                  fill_child->inj.injector_data.pos, 2.0f,
                                  360.0f, 0.01f, 0.005f,
                                  QVector3D(0.0f, 0.0f, 1.0f), false, false,
                                  QUuid()}) == 2,
               "Fill child should become a nested array source"))
    {
        return 1;
    }
    std::shared_ptr<Unit> fill_nested_source;
    for (const std::shared_ptr<Unit> &child :
         widget.unit_hash.value(fill_root_uuid)->child_units)
    {
        if (child != nullptr && !child->follows_array &&
            child->has_array_spec)
        {
            fill_nested_source = child;
            break;
        }
    }
    if (!check(fill_nested_source != nullptr,
               "Fill nested source should remain persistent"))
    {
        return 1;
    }
    const QVector3D fill_nested_direction =
        fill_nested_source->inj.injector_data.vel;
    const QVector3D fill_nested_position =
        fill_nested_source->inj.injector_data.pos;
    const bool fill_follow_enabled = widget.set_unit_parent_transform_follow(
        fill_nested_source->inj.uuid, true);
    const bool fill_moved = widget.set_unit_position_by_uuid(
        fill_root_uuid, QVector3D(120.0f, 0.0f, 0.0f));
    if (!check(fill_follow_enabled,
               "Fill nested source should follow parent transform") ||
        !check(fill_moved,
               "Moving Fill parent should succeed") ||
        !check(widget.unit_hash.value(fill_nested_source->inj.uuid) != nullptr &&
                   widget.unit_hash.value(fill_nested_source->inj.uuid)
                           ->inj.injector_data.pos ==
                       fill_nested_position + QVector3D(20.0f, 0.0f, 0.0f),
               "Fill parent movement should preserve nested source offset") ||
        !check(widget.unit_hash.value(fill_nested_source->inj.uuid)
                       ->inj.injector_data.vel == fill_nested_direction,
               "Fill parent transform should preserve nested source direction"))
    {
        return 1;
    }

    const QVector3D nested_array_direction_before =
        fill_nested_source->array_spec.direction;
    const QVector3D nested_array_origin_before =
        fill_nested_source->array_spec.origin;
    const QVector3D fill_root_position_before_rotation =
        widget.unit_position_by_uuid(fill_root_uuid);
    if (!check(widget.rotate_units_by_uuid(
                    {fill_root_uuid}, QVector3D(0.0f, 0.0f, 1.0f), 90.0f),
               "Rotating Fill parent should succeed") ||
        !check(widget.unit_hash.value(fill_nested_source->inj.uuid) != nullptr,
               "Rotated Fill nested source should remain available"))
    {
        return 1;
    }
    const std::shared_ptr<Unit> rotated_fill_nested =
        widget.unit_hash.value(fill_nested_source->inj.uuid);
    const QVector3D expected_nested_direction(0.0f, 1.0f, 0.0f);
    const QVector3D expected_nested_origin =
        fill_root_position_before_rotation +
        QVector3D(-(nested_array_origin_before.y() -
                    fill_root_position_before_rotation.y()),
                   nested_array_origin_before.x() -
                       fill_root_position_before_rotation.x(),
                   nested_array_origin_before.z() -
                       fill_root_position_before_rotation.z());
    if (!check(vectors_close(rotated_fill_nested->array_spec.direction,
                             expected_nested_direction),
               "Rotating Fill parent should rotate nested array direction") ||
        !check(vectors_close(rotated_fill_nested->array_spec.origin,
                             expected_nested_origin),
               "Rotating Fill parent should rotate nested array origin") ||
        !check(vectors_close(nested_array_direction_before,
                             QVector3D(1.0f, 0.0f, 0.0f)),
               "Nested array test should start with world X direction"))
    {
        return 1;
    }

    UnitFillSpec edited_fill = widget.unit_fill_spec_by_uuid(fill_root_uuid);
    edited_fill.pattern = UnitFillPattern::Hexagonal;
    edited_fill.rows = 2;
    edited_fill.columns = 2;
    edited_fill.spacing_x = 3.0f;
    edited_fill.spacing_y = 4.0f;
    edited_fill.source_weights = {1};
    if (!check(widget.update_unit_fill(fill_root_uuid, edited_fill),
               "Existing Fill should be editable") ||
        !check(widget.unit_fill_spec_by_uuid(fill_root_uuid).pattern ==
                   UnitFillPattern::Hexagonal,
               "Fill editor should preserve the edited pattern") ||
        !check(widget.unit_hash.value(fill_root_uuid)->child_units.size() == 4,
               "Edited Fill should rebuild its complete placement set"))
    {
        return 1;
    }
    widget.update_fill_preview(fill_root_uuid, edited_fill);
    widget.clear_array_preview();
    return 0;
}
