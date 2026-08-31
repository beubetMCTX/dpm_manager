#include "mainwindow.h"
#include "species_color_dialog.h"
#include "species_material_dialog.h"
#include "unit_edit_dialog.h"

#include <QApplication>
#include <QDebug>
#include <QMetaObject>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto *window = new MainWindow();
    window->show();
    application.processEvents();

    if (!QMetaObject::invokeMethod(window, "on_actionSpecies_Colors_triggered") ||
        !QMetaObject::invokeMethod(window, "on_actionSpecies_Materials_triggered"))
    {
        qCritical() << "Unable to open auxiliary dialogs through MainWindow actions.";
        delete window;
        return 1;
    }

    application.processEvents();
    if (window->findChildren<SpeciesColorDialog *>().isEmpty() ||
        window->findChildren<SpeciesMaterialDialog *>().isEmpty())
    {
        qCritical() << "Auxiliary dialogs were not created.";
        delete window;
        return 1;
    }

    Unit unit;
    unit.inj.injector_data.name = QStringLiteral("shutdown-smoke");
    auto *editor = new unit_edit_dialog(&unit, {}, {}, window);
    editor->show();
    application.processEvents();

    window->close();
    delete window;
    application.processEvents();

    qInfo() << "Shutdown smoke test passed";
    return 0;
}
