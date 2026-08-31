#include "app_config.h"

#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHash>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    QString error_message;
    if (!ensure_app_config_directories(&error_message))
    {
        qCritical() << "Unable to create config directories:" << error_message;
        return 1;
    }

    const QString chemkin_path = QStringLiteral("C:/test/sample.inp");
    if (!save_last_chemkin_file_path(chemkin_path, &error_message))
    {
        qCritical() << "Unable to save Chemkin path:" << error_message;
        return 1;
    }

    QString loaded_chemkin_path;
    if (!load_last_chemkin_file_path(&loaded_chemkin_path, &error_message) ||
        loaded_chemkin_path != QDir::toNativeSeparators(chemkin_path))
    {
        qCritical() << "Chemkin path round-trip failed:" << loaded_chemkin_path << error_message;
        return 1;
    }

    const QList<MaterialConfigEntry> materials = {
        {QStringLiteral("fuel"), 780.0},
        {QStringLiteral("oxidizer"), 1.2}
    };
    if (!save_material_table_config(materials, &error_message))
    {
        qCritical() << "Unable to save material config:" << error_message;
        return 1;
    }

    QList<MaterialConfigEntry> loaded_materials;
    if (!load_material_table_config(&loaded_materials, &error_message) ||
        loaded_materials.size() != materials.size() ||
        loaded_materials[0].name != materials[0].name ||
        loaded_materials[0].density != materials[0].density)
    {
        qCritical() << "Material config round-trip failed:" << error_message;
        return 1;
    }

    const QStringList species = {QStringLiteral("fuel"), QStringLiteral("oxidizer")};
    QHash<QString, QColor> colors;
    colors.insert(QStringLiteral("fuel"), QColor(QStringLiteral("#FF0000")));
    colors.insert(QStringLiteral("oxidizer"), QColor(QStringLiteral("#00FF00")));
    if (!save_species_color_config(chemkin_path, species, colors, &error_message))
    {
        qCritical() << "Unable to save color config:" << error_message;
        return 1;
    }

    QHash<QString, QColor> loaded_colors;
    if (!load_species_color_config(chemkin_path, species, &loaded_colors, &error_message) ||
        loaded_colors.value(QStringLiteral("fuel")) != colors.value(QStringLiteral("fuel")) ||
        loaded_colors.value(QStringLiteral("oxidizer")) != colors.value(QStringLiteral("oxidizer")))
    {
        qCritical() << "Color config round-trip failed:" << error_message;
        return 1;
    }

    qInfo() << "Config smoke test passed";
    return 0;
}
