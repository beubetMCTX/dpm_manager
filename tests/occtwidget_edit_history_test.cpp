#include "occtwidget.h"

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

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
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
    if (!check(widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .use_reference_geometry,
               "Reference-bound Array should retain its dependency before clear") ||
        !check(widget.clear_reference_geometry(),
               "Clearing reference geometry should succeed") ||
        !check(!widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .use_reference_geometry &&
                   !widget.unit_hash.value(reference_bound_uuid)->array_spec
                   .conform_to_reference_normal,
               "Clearing reference geometry should downgrade Array dependencies"))
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
    return 0;
}
