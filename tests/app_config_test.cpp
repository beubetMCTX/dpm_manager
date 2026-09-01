#include "app_config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
                    QJsonObject{{"name", "water"}, {"density", 0.0}}}}};
            corrupt_file.write(QJsonDocument(corrupt_root).toJson());
            corrupt_file.close();
        }
    }

    QList<MaterialConfigEntry> ignored_materials;
    if (backup_test_ok)
    {
        backup_test_ok = !load_material_table_config(&ignored_materials, &config_error) &&
                         config_error.contains("backup:");
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

    return 0;
}
