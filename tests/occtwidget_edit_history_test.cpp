#include "occtwidget.h"

#include <algorithm>
#include <QApplication>

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
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    OCCTWidget widget(nullptr);
    widget.resize(640, 480);
    widget.show();
    application.processEvents();

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
    const std::shared_ptr<Unit> stored_unit = widget.unit_hash.value(uuid);
    if (!check(stored_unit != nullptr,
               "Stored injector should be available"))
    {
        return 1;
    }

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
    if (!check(widget.create_assembly({uuid, member_uuid}),
               "Assembly creation should succeed"))
    {
        return 1;
    }

    UnitArraySpec assembly_array;
    assembly_array.type = UnitArrayType::Linear;
    assembly_array.count = 2;
    assembly_array.direction = QVector3D(0.0f, 1.0f, 0.0f);
    assembly_array.spacing = 10.0f;
    const int generated_count = widget.create_unit_array(uuid, assembly_array);
    if (!check(generated_count == 4,
               "Assembly array should expand every non-derived member") ||
        !check(widget.unit_hash.value(uuid)->type == Assebly,
               "Assembly array source must remain an Assembly") ||
        !check(widget.unit_hash.value(uuid)->child_units.size() == 5,
               "Assembly source should retain members and own generated children") )
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
    if (!check(generated_children == 4,
               "Assembly source should own all generated array instances"))
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
                                      child->assembly_child_uuids.isEmpty();
                           }),
               "Flattened Assembly array children must not retain Assembly links"))
    {
        return 1;
    }
    if (!check(widget.rebuild_unit_array(uuid) == 4 &&
                   widget.unit_hash.value(uuid)->child_units.size() == 5 &&
                   widget.unit_hash.size() == 6,
               "Rebuilding an Assembly array should replace, not accumulate, children"))
    {
        return 1;
    }

    if (!check(widget.dissolve_assembly(uuid),
               "Assembly with generated children should dissolve cleanly") ||
        !check(widget.unit_hash.size() == 2 &&
                   widget.unit_hash.value(uuid)->child_units.isEmpty() &&
                   widget.unit_hash.value(uuid)->type == injector &&
                   !widget.unit_hash.value(uuid)->has_array_spec,
               "Dissolving an Assembly should remove derived children and metadata"))
    {
        return 1;
    }
    if (!check(!widget.create_assembly({uuid, uuid}),
               "Assembly creation with no valid child should fail") ||
        !check(widget.unit_hash.value(uuid)->type == injector &&
                   widget.unit_hash.value(uuid)->assembly_child_uuids.isEmpty() &&
                   widget.unit_hash.size() == 2,
               "Failed Assembly creation must not mutate the source"))
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
    return check(stored_unit->inj.injector_data.vel == valid_velocity,
                 "Failed undo should preserve the valid current injector data")
               ? 0
               : 1;
}
