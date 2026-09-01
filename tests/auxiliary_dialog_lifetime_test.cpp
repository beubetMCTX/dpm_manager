#include "species_color_dialog.h"
#include "species_material_dialog.h"

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

    QPointer<SpeciesColorDialog> color_dialog = new SpeciesColorDialog(parent);
    QPointer<SpeciesMaterialDialog> material_dialog = new SpeciesMaterialDialog(parent);
    color_dialog->set_chemkin_context(QString(), {});
    material_dialog->set_material_entries({});
    color_dialog->show();
    material_dialog->show();
    application.processEvents();

    if (!check(color_dialog != nullptr && material_dialog != nullptr,
               "Auxiliary dialogs should be alive before parent destruction"))
    {
        delete parent;
        return 1;
    }

    delete parent;
    application.processEvents();
    return check(color_dialog == nullptr && material_dialog == nullptr,
                 "Parent destruction should safely destroy auxiliary dialogs")
               ? 0
               : 1;
}
