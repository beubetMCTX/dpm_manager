#include "app_config.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <cmath>

namespace
{
constexpr int kConfigSchemaVersion = 1;

QString normalize_chemkin_path(const QString &file_path)
{
    const QFileInfo file_info(file_path);
    const QString canonical_path = file_info.canonicalFilePath();
    if (!canonical_path.isEmpty())
    {
        return QDir::toNativeSeparators(canonical_path);
    }

    return QDir::toNativeSeparators(file_info.absoluteFilePath());
}

QString color_config_file_path(const QString &chemkin_file_path,
                               const QStringList &species_names)
{
    const QString signature_source =
        normalize_chemkin_path(chemkin_file_path) + "\n" + species_names.join('\n');
    const QByteArray digest = QCryptographicHash::hash(signature_source.toUtf8(),
                                                       QCryptographicHash::Sha1);
    return QDir(app_color_config_directory_path()).filePath(QString::fromLatin1(digest.toHex()) + ".json");
}

QString backup_invalid_config(const QString &file_path);

bool migrate_config_schema(QJsonObject *json_object,
                           const QString &file_path,
                           QString *error_message)
{
    if (json_object == nullptr)
    {
        return true;
    }

    const QJsonValue version_value = json_object->value("schema_version");
    if (version_value.isUndefined())
    {
        // Version 0 was written before schema_version existed. All keys used
        // by that format remain compatible, so only normalize its in-memory
        // version marker.
        json_object->insert("schema_version", kConfigSchemaVersion);
        return true;
    }

    if (!version_value.isDouble())
    {
        const QString backup_path = backup_invalid_config(file_path);
        if (error_message != nullptr)
        {
            *error_message = QString("Invalid schema_version in config file: %1%2")
                                 .arg(file_path,
                                      backup_path.isEmpty()
                                          ? QString()
                                          : QString("; backup: %1").arg(backup_path));
        }
        return false;
    }

    const double version_number = version_value.toDouble();
    const int version = static_cast<int>(version_number);
    if (!std::isfinite(version_number) || version_number < 0.0 || version_number != version)
    {
        const QString backup_path = backup_invalid_config(file_path);
        if (error_message != nullptr)
        {
            *error_message = QString("Invalid schema_version in config file: %1%2")
                                 .arg(file_path,
                                      backup_path.isEmpty()
                                          ? QString()
                                          : QString("; backup: %1").arg(backup_path));
        }
        return false;
    }

    if (version > kConfigSchemaVersion)
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Config file %1 requires newer schema version %2.")
                                 .arg(file_path)
                                 .arg(version);
        }
        return false;
    }

    if (version < kConfigSchemaVersion)
    {
        // Reserved migration point for future key renames. Current version 1
        // only needs the version marker because its key layout is unchanged.
        json_object->insert("schema_version", kConfigSchemaVersion);
    }
    return true;
}

bool read_json_object(const QString &file_path,
                      QJsonObject *json_object,
                      QString *error_message)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to open config file: %1").arg(file_path);
        }
        return false;
    }

    const QByteArray contents = file.readAll();
    file.close();

    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(contents, &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject())
    {
        const QString backup_path = backup_invalid_config(file_path);
        if (error_message != nullptr)
        {
            *error_message = QString("Invalid JSON object in config file: %1 (%2)%3")
                                 .arg(file_path,
                                      parse_error.error != QJsonParseError::NoError
                                          ? parse_error.errorString()
                                          : QString("root value is not an object"),
                                      backup_path.isEmpty()
                                          ? QString()
                                          : QString("; backup: %1").arg(backup_path));
        }
        return false;
    }

    if (json_object != nullptr)
    {
        *json_object = document.object();
        if (!migrate_config_schema(json_object, file_path, error_message))
        {
            return false;
        }
    }
    return true;
}

QString backup_invalid_config(const QString &file_path)
{
    const QFileInfo file_info(file_path);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString backup_path = file_path + ".corrupt-" + timestamp + ".bak";
    int suffix = 1;
    while (QFileInfo::exists(backup_path))
    {
        backup_path = file_path + ".corrupt-" + timestamp + "-" + QString::number(suffix++) + ".bak";
    }

    if (QFile::rename(file_info.absoluteFilePath(), backup_path))
    {
        qWarning() << "Backed up invalid config file to" << backup_path;
        return backup_path;
    }

    qWarning() << "Unable to back up invalid config file" << file_path;
    return {};
}

bool write_json_object(const QString &file_path,
                       const QJsonObject &json_object,
                       QString *error_message)
{
    QSaveFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to write config file: %1").arg(file_path);
        }
        return false;
    }

    file.write(QJsonDocument(json_object).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to finalize config file: %1").arg(file_path);
        }
        return false;
    }

    return true;
}
}

QString app_config_directory_path()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("config");
}

QString app_color_config_directory_path()
{
    return QDir(app_config_directory_path()).filePath("color_cfg");
}

QString app_material_config_directory_path()
{
    return QDir(app_config_directory_path()).filePath("material_cfg");
}

QString app_settings_file_path()
{
    return QDir(app_config_directory_path()).filePath("config.json");
}

bool ensure_app_config_directories(QString *error_message)
{
    QDir dir;
    if (!dir.mkpath(app_config_directory_path()))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to create config directory: %1").arg(app_config_directory_path());
        }
        return false;
    }

    if (!dir.mkpath(app_color_config_directory_path()))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to create color config directory: %1").arg(app_color_config_directory_path());
        }
        return false;
    }

    if (!dir.mkpath(app_material_config_directory_path()))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Unable to create material config directory: %1").arg(app_material_config_directory_path());
        }
        return false;
    }

    return true;
}

bool load_last_chemkin_file_path(QString *file_path, QString *error_message)
{
    if (file_path != nullptr)
    {
        file_path->clear();
    }

    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    if (file_path != nullptr)
    {
        *file_path = root_object.value("last_chemkin_file_path").toString();
    }
    return true;
}

bool save_last_chemkin_file_path(const QString &file_path, QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonObject root_object;
    if (QFileInfo::exists(app_settings_file_path()))
    {
        QString read_error;
        if (!read_json_object(app_settings_file_path(), &root_object, &read_error))
        {
            root_object = QJsonObject();
        }
    }
    root_object.insert("last_chemkin_file_path", normalize_chemkin_path(file_path));
    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool clear_last_chemkin_file_path(QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonObject root_object;
    if (QFileInfo::exists(app_settings_file_path()))
    {
        QString read_error;
        if (!read_json_object(app_settings_file_path(), &root_object, &read_error))
        {
            root_object = QJsonObject();
        }
    }

    root_object.remove("last_chemkin_file_path");
    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool load_reference_geometry_config(ReferenceGeometryConfig *config,
                                    QString *error_message)
{
    if (config != nullptr)
    {
        *config = ReferenceGeometryConfig();
    }

    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonObject geometry_object = root_object.value("reference_geometry").toObject();
    const QString file_path = geometry_object.value("file_path").toString().trimmed();
    if (file_path.isEmpty())
    {
        return false;
    }

    if (config != nullptr)
    {
        const QJsonArray position = geometry_object.value("position").toArray();
        const QJsonArray rotation = geometry_object.value("rotation").toArray();
        if (position.size() == 3)
        {
            config->position = QVector3D(static_cast<float>(position.at(0).toDouble()),
                                         static_cast<float>(position.at(1).toDouble()),
                                         static_cast<float>(position.at(2).toDouble()));
        }
        if (rotation.size() == 3)
        {
            config->rotation = QVector3D(static_cast<float>(rotation.at(0).toDouble()),
                                         static_cast<float>(rotation.at(1).toDouble()),
                                         static_cast<float>(rotation.at(2).toDouble()));
        }
        config->file_path = file_path;
        config->locked = geometry_object.value("locked").toBool(false);
        config->visible = geometry_object.value("visible").toBool(true);
    }
    return true;
}

bool save_reference_geometry_config(const ReferenceGeometryConfig &config,
                                    QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonObject root_object;
    if (QFileInfo::exists(app_settings_file_path()))
    {
        QString read_error;
        if (!read_json_object(app_settings_file_path(), &root_object, &read_error))
        {
            root_object = QJsonObject();
        }
    }

    if (config.file_path.trimmed().isEmpty())
    {
        root_object.remove("reference_geometry");
    }
    else
    {
        const auto vector_to_json = [](const QVector3D &value)
        {
            QJsonArray result;
            result.append(value.x());
            result.append(value.y());
            result.append(value.z());
            return result;
        };

        QJsonObject geometry_object;
        geometry_object.insert("file_path", QDir::toNativeSeparators(
            QFileInfo(config.file_path).absoluteFilePath()));
        geometry_object.insert("position", vector_to_json(config.position));
        geometry_object.insert("rotation", vector_to_json(config.rotation));
        geometry_object.insert("locked", config.locked);
        geometry_object.insert("visible", config.visible);
        root_object.insert("reference_geometry", geometry_object);
    }

    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool load_main_window_state(QByteArray *geometry,
                            QByteArray *window_state,
                            QString *error_message)
{
    if (geometry != nullptr)
    {
        geometry->clear();
    }
    if (window_state != nullptr)
    {
        window_state->clear();
    }

    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    if (geometry != nullptr)
    {
        *geometry = QByteArray::fromBase64(
            root_object.value("window_geometry").toString().toLatin1());
    }
    if (window_state != nullptr)
    {
        *window_state = QByteArray::fromBase64(
            root_object.value("window_state").toString().toLatin1());
    }
    return true;
}

bool save_main_window_state(const QByteArray &geometry,
                            const QByteArray &window_state,
                            QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonObject root_object;
    if (QFileInfo::exists(app_settings_file_path()))
    {
        QString read_error;
        if (!read_json_object(app_settings_file_path(), &root_object, &read_error))
        {
            root_object = QJsonObject();
        }
    }

    root_object.insert("window_geometry",
                       QString::fromLatin1(geometry.toBase64()));
    root_object.insert("window_state",
                       QString::fromLatin1(window_state.toBase64()));
    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool load_unit_preferences(Unit_Preferences *preferences,
                           QString *error_message)
{
    if (preferences != nullptr)
    {
        *preferences = UnitSystem::default_preferences();
    }

    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonObject units_object = root_object.value("unit_preferences").toObject();
    if (units_object.isEmpty())
    {
        return false;
    }

    Unit_Preferences loaded = UnitSystem::default_preferences();
    const auto read_unit = [&units_object](const char *key, QString *target)
    {
        const QJsonValue value = units_object.value(QString::fromLatin1(key));
        if (value.isString())
        {
            *target = value.toString().trimmed();
        }
    };
    read_unit("length", &loaded.length);
    read_unit("angle", &loaded.angle);
    read_unit("velocity", &loaded.velocity);
    read_unit("mass", &loaded.mass);
    read_unit("mass_flow", &loaded.mass_flow);
    read_unit("time", &loaded.time);
    read_unit("pressure", &loaded.pressure);
    read_unit("temperature", &loaded.temperature);

    QString validation_error;
    if (!UnitSystem::validate_preferences(loaded, &validation_error))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Invalid unit preferences in %1: %2")
                                 .arg(app_settings_file_path(), validation_error);
        }
        return false;
    }

    if (preferences != nullptr)
    {
        *preferences = loaded;
    }
    return true;
}

bool save_unit_preferences(const Unit_Preferences &preferences,
                           QString *error_message)
{
    QString validation_error;
    if (!UnitSystem::validate_preferences(preferences, &validation_error))
    {
        if (error_message != nullptr)
        {
            *error_message = validation_error;
        }
        return false;
    }

    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonObject root_object;
    if (QFileInfo::exists(app_settings_file_path()))
    {
        QString read_error;
        if (!read_json_object(app_settings_file_path(), &root_object, &read_error))
        {
            root_object = QJsonObject();
        }
    }

    QJsonObject units_object;
    units_object.insert("length", preferences.length);
    units_object.insert("angle", preferences.angle);
    units_object.insert("velocity", preferences.velocity);
    units_object.insert("mass", preferences.mass);
    units_object.insert("mass_flow", preferences.mass_flow);
    units_object.insert("time", preferences.time);
    units_object.insert("pressure", preferences.pressure);
    units_object.insert("temperature", preferences.temperature);
    root_object.insert("unit_preferences", units_object);
    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool load_species_color_config(const QString &chemkin_file_path,
                               const QStringList &species_names,
                               QHash<QString, QColor> *species_colors,
                               QString *error_message)
{
    if (species_colors != nullptr)
    {
        species_colors->clear();
    }

    if (chemkin_file_path.trimmed().isEmpty() || species_names.isEmpty())
    {
        return false;
    }

    const QString file_path = color_config_file_path(chemkin_file_path, species_names);
    if (!QFileInfo::exists(file_path))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(file_path, &root_object, error_message))
    {
        return false;
    }

    const QJsonObject colors_object = root_object.value("species_colors").toObject();
    if (species_colors != nullptr)
    {
        for (const QString &species_name : species_names)
        {
            const QColor color(colors_object.value(species_name).toString());
            if (color.isValid())
            {
                species_colors->insert(species_name, color);
            }
        }
    }

    return true;
}

bool save_species_color_config(const QString &chemkin_file_path,
                               const QStringList &species_names,
                               const QHash<QString, QColor> &species_colors,
                               QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    if (chemkin_file_path.trimmed().isEmpty() || species_names.isEmpty())
    {
        if (error_message != nullptr)
        {
            *error_message = "Chemkin context is empty, color config cannot be saved yet.";
        }
        return false;
    }

    QJsonObject colors_object;
    for (const QString &species_name : species_names)
    {
        const auto it = species_colors.constFind(species_name);
        if (it != species_colors.constEnd() && it.value().isValid())
        {
            colors_object.insert(species_name, it.value().name(QColor::HexRgb).toUpper());
        }
    }

    QJsonArray species_array;
    for (const QString &species_name : species_names)
    {
        species_array.append(species_name);
    }

    QJsonObject root_object;
    root_object.insert("schema_version", kConfigSchemaVersion);
    root_object.insert("chemkin_file_path", normalize_chemkin_path(chemkin_file_path));
    root_object.insert("species", species_array);
    root_object.insert("species_colors", colors_object);

    return write_json_object(color_config_file_path(chemkin_file_path, species_names),
                             root_object,
                             error_message);
}

bool load_material_table_config(QList<MaterialConfigEntry> *materials,
                                QString *error_message)
{
    if (materials != nullptr)
    {
        materials->clear();
    }

    const QString file_path = QDir(app_material_config_directory_path()).filePath("materials.json");
    if (!QFileInfo::exists(file_path))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(file_path, &root_object, error_message))
    {
        return false;
    }

    const QJsonArray materials_array = root_object.value("materials").toArray();
    if (materials != nullptr)
    {
        for (const QJsonValue &value : materials_array)
        {
            if (!value.isObject())
            {
                continue;
            }

            const QJsonObject material_object = value.toObject();
            const QString name = material_object.value("name").toString().trimmed();
            if (name.isEmpty())
            {
                continue;
            }

            MaterialConfigEntry entry;
            entry.name = name;
            entry.density = material_object.value("density").toDouble(0.0);
            materials->append(entry);
        }
    }

    return true;
}

bool save_material_table_config(const QList<MaterialConfigEntry> &materials,
                                QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    QJsonArray materials_array;
    QStringList saved_names;
    for (const MaterialConfigEntry &entry : materials)
    {
        const QString name = entry.name.trimmed();
        if (name.isEmpty() || saved_names.contains(name))
        {
            continue;
        }

        QJsonObject material_object;
        material_object.insert("name", name);
        material_object.insert("density", entry.density);
        materials_array.append(material_object);
        saved_names.append(name);
    }

    QJsonObject root_object;
    root_object.insert("schema_version", kConfigSchemaVersion);
    root_object.insert("materials", materials_array);
    return write_json_object(QDir(app_material_config_directory_path()).filePath("materials.json"),
                             root_object,
                             error_message);
}

QStringList material_names_from_entries(const QList<MaterialConfigEntry> &materials)
{
    QStringList names;
    for (const MaterialConfigEntry &entry : materials)
    {
        const QString trimmed_name = entry.name.trimmed();
        if (!trimmed_name.isEmpty() && !names.contains(trimmed_name))
        {
            names.append(trimmed_name);
        }
    }

    return names;
}
