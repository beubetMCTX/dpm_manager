#include "species_color_dialog.h"
#include "species_material_dialog.h"

#include <QApplication>
#include <QPointer>
#include <QLineEdit>
#include <QTableWidget>
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

    color_dialog->set_species_names({"O2", "CH4", "CO2"});
    material_dialog->set_material_entries({{"water", 998.2}, {"kerosene", 800.0}});
    auto *species_filter = color_dialog->findChild<QLineEdit*>("speciesFilterEdit");
    auto *material_filter = material_dialog->findChild<QLineEdit*>("materialFilterEdit");
    if (!check(species_filter != nullptr && material_filter != nullptr,
               "filter controls should be created") ||
        !check(color_dialog->findChild<QTableWidget*>("speciesTable") != nullptr &&
                   material_dialog->findChild<QTableWidget*>("materialsTable") != nullptr,
               "auxiliary tables should be created"))
    {
        return 1;
    }

    species_filter->setText("ch4");
    material_filter->setText("water");
    if (!check(!color_dialog->findChild<QTableWidget*>("speciesTable")->isRowHidden(1) &&
                   color_dialog->findChild<QTableWidget*>("speciesTable")->isRowHidden(0),
               "species filter should hide non-matching rows") ||
        !check(!material_dialog->findChild<QTableWidget*>("materialsTable")->isRowHidden(0) &&
                   material_dialog->findChild<QTableWidget*>("materialsTable")->isRowHidden(1),
               "material filter should hide non-matching rows"))
    {
        return 1;
    }
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
