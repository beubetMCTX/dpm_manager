#include "occtwidget.h"

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
