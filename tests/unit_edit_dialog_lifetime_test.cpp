#include "unit_edit_dialog.h"

#include <QApplication>
#include <QPointer>
#include <QPushButton>
#include <QWidget>

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
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    auto *parent = new QWidget();
    Unit unit;
    unit.inj.injector_data.name = "lifetime-test";

    QPointer<unit_edit_dialog> dialog = new unit_edit_dialog(
        &unit, {"O2", "N2"}, {"water"}, parent);
    dialog->show();
    application.processEvents();

    if (!check(dialog != nullptr, "Unit editor should be alive after construction"))
    {
        delete parent;
        return 1;
    }

    if (!check(dialog->findChild<QPushButton*>("applyChangesButton") != nullptr &&
                   dialog->findChild<QPushButton*>("cancelChangesButton") != nullptr,
               "Unit editor should expose explicit apply and cancel actions"))
    {
        delete parent;
        return 1;
    }

    bool cancel_signal_received = false;
    QObject::connect(dialog, &unit_edit_dialog::dialog_cancelled,
                     [&cancel_signal_received](Unit *)
    {
        cancel_signal_received = true;
    });
    auto *name_editor = dialog->findChild<QUI_LineEdit*>("injectionNameEditor");
    if (!check(name_editor != nullptr,
               "Unit editor should expose its injection name editor"))
    {
        delete parent;
        return 1;
    }
    name_editor->setText("cancelled-edit");
    name_editor->commit();
    if (!check(dialog->has_unsaved_changes(),
               "Committed editor input should mark the dialog modified"))
    {
        delete parent;
        return 1;
    }
    dialog->findChild<QPushButton*>("cancelChangesButton")->click();
    application.processEvents();
    if (!check(cancel_signal_received && !dialog->isVisible() &&
                   !dialog->has_unsaved_changes(),
               "Cancel Changes should emit its signal and close the editor"))
    {
        delete parent;
        return 1;
    }
    dialog->show();
    application.processEvents();

    Injector_OCCT geometry;
    geometry.injector_data.vel = QVector3D(0.0f, 0.0f, 1.0f);
    geometry.injector_data.total_flow_rate = 1.0;
    if (!check(geometry.create_injector(),
               "A valid injector should create geometry") ||
        !check(!geometry.shape.IsNull(),
               "A valid injector should retain its geometry"))
    {
        delete parent;
        return 1;
    }

    geometry.injector_data.vel = QVector3D();
    if (!check(!geometry.create_injector(),
               "An invalid injector should fail geometry creation") ||
        !check(!geometry.shape.IsNull(),
               "A failed rebuild should preserve the previous geometry"))
    {
        delete parent;
        return 1;
    }

    dialog->close();
    application.processEvents();
    if (!check(dialog != nullptr,
               "Unit editor should remain owned after close without delete-on-close"))
    {
        delete parent;
        return 1;
    }

    delete parent;
    application.processEvents();
    return check(dialog == nullptr,
                 "Unit editor should be destroyed safely with its parent")
               ? 0
               : 1;
}
