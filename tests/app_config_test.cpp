#include "app_config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUuid>

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QString error_message;

    const QList<MaterialConfigEntry> valid = {{"water", 998.2}, {"kerosene", 800.0}};
    if (!check(validate_material_entries(valid, &error_message),
               "valid material entries should pass validation"))
    {
        return 1;
    }

    if (!check(!validate_material_entries({{"water", 0.0}}, &error_message) &&
                   error_message.contains("positive"),
               "zero material density should fail validation") ||
        !check(!validate_material_entries({{"water", std::numeric_limits<double>::infinity()}},
                                          &error_message) &&
                   error_message.contains("finite"),
               "non-finite material density should fail validation") ||
        !check(!validate_material_entries({{"water", 1.0}, {"WATER", 2.0}},
                                          &error_message) &&
                   error_message.contains("Duplicate"),
               "case-insensitive duplicate material names should fail validation"))
    {
        return 1;
    }

    QString config_error;
    if (!check(ensure_app_config_directories(&config_error),
               "Unable to prepare config directory for backup test"))
    {
        return 1;
    }

    const QString materials_path =
        QDir(app_material_config_directory_path()).filePath("materials.json");
    const bool had_original_materials = QFileInfo::exists(materials_path);
    QByteArray original_materials;
    if (had_original_materials)
    {
        QFile original_file(materials_path);
        if (!check(original_file.open(QIODevice::ReadOnly),
                   "Unable to preserve existing materials config"))
        {
            return 1;
        }
        original_materials = original_file.readAll();
    }

    const QDir material_directory(app_material_config_directory_path());
    const QStringList backups_before = material_directory.entryList(
        {"materials.json.corrupt-*.bak"}, QDir::Files);
    bool backup_test_ok = save_material_table_config(valid, &config_error);
    if (backup_test_ok)
    {
        QFile corrupt_file(materials_path);
        backup_test_ok = corrupt_file.open(QIODevice::WriteOnly | QIODevice::Text);
        if (backup_test_ok)
        {
            const QJsonObject corrupt_root{
                {"schema_version", 1},
                {"materials", QJsonArray{
                    QJsonObject{{"name", 42}, {"density", 998.2}}}}};
            corrupt_file.write(QJsonDocument(corrupt_root).toJson());
            corrupt_file.close();
        }
    }

    QList<MaterialConfigEntry> ignored_materials = {{"keep-material", 321.0}};
    if (backup_test_ok)
    {
        backup_test_ok = !load_material_table_config(&ignored_materials, &config_error) &&
                         config_error.contains("name must be a string") &&
                         config_error.contains("backup:") &&
                         ignored_materials.size() == 1 &&
                         ignored_materials.first().name == "keep-material" &&
                         ignored_materials.first().density == 321.0;
    }

    const QStringList backups_after = material_directory.entryList(
        {"materials.json.corrupt-*.bak"}, QDir::Files);
    backup_test_ok = backup_test_ok && backups_after.size() > backups_before.size();

    QFile::remove(materials_path);
    for (const QString &backup_name : backups_after)
    {
        QFile::remove(material_directory.filePath(backup_name));
    }
    if (had_original_materials)
    {
        QFile restored_file(materials_path);
        backup_test_ok = backup_test_ok &&
                         restored_file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
                         restored_file.write(original_materials) == original_materials.size();
    }

    if (!check(backup_test_ok,
               "invalid material config should be backed up before rejection"))
    {
        return 1;
    }

    QTemporaryDir temporary_directory;
    if (!check(temporary_directory.isValid(),
               "Unable to create temporary directory for color backup test"))
    {
        return 1;
    }

    const QString chemkin_path = temporary_directory.filePath(
        QString("chemkin-%1.inp").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    QFile chemkin_file(chemkin_path);
    if (!check(chemkin_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create temporary Chemkin file for color backup test"))
    {
        return 1;
    }
    chemkin_file.write("SPECIES O2 N2 END\n");
    chemkin_file.close();

    const QStringList color_species = {"O2", "N2"};
    const QDir color_directory(app_color_config_directory_path());
    const QStringList color_configs_before = color_directory.entryList(
        {"*.json"}, QDir::Files);
    QHash<QString, QColor> colors;
    colors.insert("O2", QColor("#112233"));
    colors.insert("N2", QColor("#445566"));
    bool color_backup_test_ok = save_species_color_config(
        chemkin_path, color_species, colors, &config_error);
    QString color_config_name;
    if (color_backup_test_ok)
    {
        const QStringList color_configs_after = color_directory.entryList(
            {"*.json"}, QDir::Files);
        for (const QString &name : color_configs_after)
        {
            if (!color_configs_before.contains(name))
            {
                color_config_name = name;
                break;
            }
        }
        color_backup_test_ok = !color_config_name.isEmpty();
    }

    const QString color_config_path = color_directory.filePath(color_config_name);
    if (color_backup_test_ok)
    {
        QFile invalid_color_file(color_config_path);
        color_backup_test_ok = invalid_color_file.open(
            QIODevice::WriteOnly | QIODevice::Text);
        if (color_backup_test_ok)
        {
            const QJsonObject invalid_color_root{
                {"schema_version", 1},
                {"species_colors", QJsonObject{{"O2", "not-a-color"}}}};
            invalid_color_file.write(QJsonDocument(invalid_color_root).toJson());
            invalid_color_file.close();
        }
    }

    if (color_backup_test_ok)
    {
        QHash<QString, QColor> ignored_colors;
        ignored_colors.insert("keep", QColor("#abcdef"));
        color_backup_test_ok = !load_species_color_config(
            chemkin_path, color_species, &ignored_colors, &config_error) &&
            config_error.contains("backup:") &&
            ignored_colors.size() == 1 &&
            ignored_colors.value("keep") == QColor("#abcdef");
    }

    const QStringList color_backups = color_directory.entryList(
        {color_config_name + ".corrupt-*.bak"}, QDir::Files);
    color_backup_test_ok = color_backup_test_ok && !color_backups.isEmpty();
    QFile::remove(color_config_path);
    for (const QString &backup_name : color_backups)
    {
        QFile::remove(color_directory.filePath(backup_name));
    }

    if (!check(color_backup_test_ok,
               "invalid species color config should be backed up before rejection"))
    {
        return 1;
    }

    bool duplicate_color_test_ok = save_species_color_config(
        chemkin_path, color_species, colors, &config_error);
    if (duplicate_color_test_ok)
    {
        QFile duplicate_color_file(color_config_path);
        duplicate_color_test_ok = duplicate_color_file.open(
            QIODevice::WriteOnly | QIODevice::Text);
        if (duplicate_color_test_ok)
        {
            const QJsonObject duplicate_color_root{
                {"schema_version", 1},
                {"species_colors", QJsonObject{
                    {"O2", "#112233"}, {"N2", "#112233"}}}};
            duplicate_color_file.write(QJsonDocument(duplicate_color_root).toJson());
            duplicate_color_file.close();
        }
    }

    if (duplicate_color_test_ok)
    {
        QHash<QString, QColor> ignored_duplicate_colors;
        ignored_duplicate_colors.insert("keep", QColor("#123456"));
        duplicate_color_test_ok = !load_species_color_config(
            chemkin_path, color_species, &ignored_duplicate_colors, &config_error) &&
            config_error.contains("both O2 and N2") &&
            config_error.contains("backup:") &&
            ignored_duplicate_colors.size() == 1 &&
            ignored_duplicate_colors.value("keep") == QColor("#123456");
    }
    const QStringList duplicate_color_backups = color_directory.entryList(
        {color_config_name + ".corrupt-*.bak"}, QDir::Files);
    QFile::remove(color_config_path);
    for (const QString &backup_name : duplicate_color_backups)
    {
        QFile::remove(color_directory.filePath(backup_name));
    }
    if (!check(duplicate_color_test_ok,
               "duplicate species colors should be rejected and backed up"))
    {
        return 1;
    }

    const QString settings_path = app_settings_file_path();
    const bool had_original_settings = QFileInfo::exists(settings_path);
    QByteArray original_settings;
    if (had_original_settings)
    {
        QFile original_file(settings_path);
        if (!check(original_file.open(QIODevice::ReadOnly),
                   "Unable to preserve existing app settings"))
        {
            return 1;
        }
        original_settings = original_file.readAll();
    }

    bool settings_type_test_ok = true;
    const auto write_settings = [&settings_path](const QJsonObject &root)
    {
        QFile file(settings_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return false;
        }
        const QByteArray contents = QJsonDocument(root).toJson();
        return file.write(contents) == contents.size();
    };

    settings_type_test_ok = write_settings(QJsonObject{
        {"schema_version", 1},
        {"last_chemkin_file_path", 42}});
    QString ignored_path = "keep-this-chemkin-path";
    if (settings_type_test_ok)
    {
        settings_type_test_ok = !load_last_chemkin_file_path(
            &ignored_path, &config_error) &&
            config_error.contains("must be a string") &&
            config_error.contains("backup:") &&
            ignored_path == "keep-this-chemkin-path";
    }

    settings_type_test_ok = settings_type_test_ok && write_settings(QJsonObject{
        {"schema_version", 1},
        {"recent_project_paths", QJsonArray{"valid.dpm", 42}}});
    QStringList ignored_paths = {"keep-this-project"};
    if (settings_type_test_ok)
    {
        settings_type_test_ok = !load_recent_project_paths(
            &ignored_paths, &config_error) &&
            config_error.contains("only strings") &&
            config_error.contains("backup:") &&
            ignored_paths == QStringList{"keep-this-project"};
    }

    settings_type_test_ok = settings_type_test_ok && write_settings(QJsonObject{
        {"schema_version", 1},
        {"window_geometry", QJsonObject{}}});
    QByteArray ignored_geometry = "keep-geometry";
    QByteArray ignored_window_state = "keep-window-state";
    if (settings_type_test_ok)
    {
        settings_type_test_ok = !load_main_window_state(
            &ignored_geometry, &ignored_window_state, &config_error) &&
            config_error.contains("must be strings") &&
            config_error.contains("backup:") &&
            ignored_geometry == "keep-geometry" &&
            ignored_window_state == "keep-window-state";
    }

    settings_type_test_ok = settings_type_test_ok && write_settings(QJsonObject{
        {"schema_version", 1},
        {"unit_preferences", QJsonArray{}}});
    Unit_Preferences ignored_preferences;
    ignored_preferences.length = "keep-length";
    ignored_preferences.angle = "keep-angle";
    if (settings_type_test_ok)
    {
        settings_type_test_ok = !load_unit_preferences(
            &ignored_preferences, &config_error) &&
            config_error.contains("must be an object") &&
            config_error.contains("backup:") &&
            ignored_preferences.length == "keep-length" &&
            ignored_preferences.angle == "keep-angle";
    }

    bool missing_reference_geometry_test_ok = write_settings(QJsonObject{
        {"schema_version", 1}});
    ReferenceGeometryConfig preserved_reference_geometry;
    preserved_reference_geometry.file_path = "keep-geometry.step";
    preserved_reference_geometry.position = QVector3D(7.0f, 8.0f, 9.0f);
    const QDir settings_directory_for_missing_reference(app_config_directory_path());
    const QStringList reference_backups_before_missing =
        settings_directory_for_missing_reference.entryList(
            {"config.json.corrupt-*.bak"}, QDir::Files);
    if (missing_reference_geometry_test_ok)
    {
        missing_reference_geometry_test_ok =
            !load_reference_geometry_config(&preserved_reference_geometry, &config_error) &&
            preserved_reference_geometry.file_path == "keep-geometry.step" &&
            preserved_reference_geometry.position == QVector3D(7.0f, 8.0f, 9.0f);
    }
    const QStringList reference_backups_after_missing =
        settings_directory_for_missing_reference.entryList(
            {"config.json.corrupt-*.bak"}, QDir::Files);
    missing_reference_geometry_test_ok = missing_reference_geometry_test_ok &&
        reference_backups_after_missing.size() == reference_backups_before_missing.size();
    if (!check(missing_reference_geometry_test_ok,
               "missing optional reference geometry config should be ignored safely"))
    {
        return 1;
    }

    ReferenceGeometryConfig reference_geometry;
    reference_geometry.file_path = temporary_directory.filePath("geometry/example.step");
    reference_geometry.position = QVector3D(1.0f, 2.0f, 3.0f);
    reference_geometry.rotation = QVector3D(4.0f, 5.0f, 6.0f);
    reference_geometry.locked = true;
    reference_geometry.visible = false;
    bool reference_geometry_test_ok = save_reference_geometry_config(
        reference_geometry, &config_error);
    ReferenceGeometryConfig loaded_reference_geometry;
    if (reference_geometry_test_ok)
    {
        reference_geometry_test_ok = load_reference_geometry_config(
            &loaded_reference_geometry, &config_error) &&
            QFileInfo(loaded_reference_geometry.file_path).absoluteFilePath() ==
                QFileInfo(reference_geometry.file_path).absoluteFilePath() &&
            loaded_reference_geometry.position == reference_geometry.position &&
            loaded_reference_geometry.rotation == reference_geometry.rotation &&
            loaded_reference_geometry.locked && !loaded_reference_geometry.visible;
        if (!reference_geometry_test_ok)
        {
            std::cerr << "reference geometry round-trip error: "
                      << config_error.toStdString() << '\n';
        }
    }
    if (!check(reference_geometry_test_ok,
               "valid reference geometry config should round-trip"))
    {
        return 1;
    }

    reference_geometry.kind = "datum_origin";
    reference_geometry.file_path.clear();
    reference_geometry.construction_radius = 0.42;
    reference_geometry.construction_direction = QVector3D(1.0f, 2.0f, 3.0f).normalized();
    reference_geometry_test_ok = save_reference_geometry_config(
        reference_geometry, &config_error);
    loaded_reference_geometry = ReferenceGeometryConfig();
    reference_geometry_test_ok = reference_geometry_test_ok &&
        load_reference_geometry_config(&loaded_reference_geometry, &config_error) &&
        loaded_reference_geometry.kind == "datum_origin" &&
        loaded_reference_geometry.file_path.isEmpty() &&
        loaded_reference_geometry.construction_radius == 0.42 &&
        loaded_reference_geometry.construction_direction ==
            reference_geometry.construction_direction;
    if (!check(reference_geometry_test_ok,
               "constructed reference geometry config should round-trip"))
    {
        return 1;
    }

    reference_geometry.kind = "section_plane";
    reference_geometry.construction_size = 31.0;
    reference_geometry.construction_thickness = 0.35;
    reference_geometry.section_clipping = true;
    reference_geometry_test_ok = save_reference_geometry_config(
        reference_geometry, &config_error);
    loaded_reference_geometry = ReferenceGeometryConfig();
    reference_geometry_test_ok = reference_geometry_test_ok &&
        load_reference_geometry_config(&loaded_reference_geometry, &config_error) &&
        loaded_reference_geometry.kind == "section_plane" &&
        loaded_reference_geometry.construction_size == 31.0 &&
        loaded_reference_geometry.construction_thickness == 0.35 &&
        loaded_reference_geometry.section_clipping;
    if (!check(reference_geometry_test_ok,
               "section plane config should round-trip"))
    {
        return 1;
    }

    QFile invalid_reference_file(settings_path);
    reference_geometry_test_ok = invalid_reference_file.open(
        QIODevice::WriteOnly | QIODevice::Text);
    if (reference_geometry_test_ok)
    {
        const QJsonObject invalid_root{
            {"schema_version", 1},
            {"reference_geometry", QJsonObject{
                {"file_path", "geometry/example.step"},
                {"position", QJsonArray{1.0, "invalid", 3.0}},
                {"rotation", QJsonArray{0.0, 0.0, 0.0}},
                {"locked", false},
                {"visible", true}}}};
        invalid_reference_file.write(QJsonDocument(invalid_root).toJson());
        invalid_reference_file.close();
    }

    ReferenceGeometryConfig ignored_reference_geometry;
    ignored_reference_geometry.file_path = "keep-geometry.step";
    ignored_reference_geometry.position = QVector3D(7.0f, 8.0f, 9.0f);
    ignored_reference_geometry.locked = true;
    if (reference_geometry_test_ok)
    {
        reference_geometry_test_ok = !load_reference_geometry_config(
            &ignored_reference_geometry, &config_error) &&
            config_error.contains("finite 3D vectors") &&
            config_error.contains("backup:") &&
            ignored_reference_geometry.file_path == "keep-geometry.step" &&
            ignored_reference_geometry.position == QVector3D(7.0f, 8.0f, 9.0f) &&
            ignored_reference_geometry.locked;
    }
    const QDir settings_directory(app_config_directory_path());
    const QStringList settings_backups = settings_directory.entryList(
        {"config.json.corrupt-*.bak"}, QDir::Files);
    reference_geometry_test_ok = reference_geometry_test_ok && !settings_backups.isEmpty();

    QFile::remove(settings_path);
    for (const QString &backup_name : settings_backups)
    {
        QFile::remove(settings_directory.filePath(backup_name));
    }
    if (had_original_settings)
    {
        QFile restored_file(settings_path);
        reference_geometry_test_ok = reference_geometry_test_ok &&
            restored_file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
            restored_file.write(original_settings) == original_settings.size();
    }
    if (!check(reference_geometry_test_ok,
               "invalid reference geometry config should be backed up before rejection"))
    {
        return 1;
    }

    if (!check(settings_type_test_ok,
               "invalid app settings field types should be backed up before rejection"))
    {
        return 1;
    }

    return 0;
}
