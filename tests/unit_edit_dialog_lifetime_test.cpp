#include "unit_edit_dialog.h"

#include <QApplication>
#include <QPointer>
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
