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

bool reject_invalid_config(const QString &file_path,
                           const QString &detail,
                           QString *error_message)
{
    const QString backup_path = backup_invalid_config(file_path);
    if (error_message != nullptr)
    {
        *error_message = detail;
        if (!backup_path.isEmpty())
        {
            *error_message += QString("; backup: %1").arg(backup_path);
        }
    }
    return false;
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
    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonValue saved_path_value = root_object.value("last_chemkin_file_path");
    if (!saved_path_value.isUndefined() && !saved_path_value.isString())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Last Chemkin file path must be a string.",
            error_message);
    }

    if (file_path != nullptr)
    {
        *file_path = saved_path_value.toString();
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

bool load_recent_project_paths(QStringList *file_paths, QString *error_message)
{
    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    if (file_paths == nullptr)
    {
        return true;
    }

    const QJsonValue recent_value = root_object.value("recent_project_paths");
    if (!recent_value.isUndefined() && !recent_value.isArray())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Recent project paths must be an array.",
            error_message);
    }

    if (!recent_value.isArray())
    {
        return true;
    }

    QStringList loaded_paths;
    for (const QJsonValue &value : recent_value.toArray())
    {
        if (!value.isString())
        {
            return reject_invalid_config(
                app_settings_file_path(),
                "Recent project paths must contain only strings.",
                error_message);
        }

        const QString path = value.toString().trimmed();
        if (path.isEmpty())
        {
            continue;
        }

        const QFileInfo file_info(path);
        const QString absolute_path = file_info.absoluteFilePath();
        if (file_info.exists() && file_info.isFile() &&
            !loaded_paths.contains(absolute_path, Qt::CaseInsensitive))
        {
            loaded_paths.append(absolute_path);
        }

        if (loaded_paths.size() >= 10)
        {
            break;
        }
    }

    *file_paths = std::move(loaded_paths);
    return true;
}

bool save_recent_project_paths(const QStringList &file_paths,
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

    QJsonArray recent_array;
    QStringList normalized_paths;
    for (const QString &path : file_paths)
    {
        const QFileInfo file_info(path.trimmed());
        if (!file_info.exists() || !file_info.isFile())
        {
            continue;
        }

        const QString absolute_path = file_info.absoluteFilePath();
        if (normalized_paths.contains(absolute_path, Qt::CaseInsensitive))
        {
            continue;
        }

        normalized_paths.append(absolute_path);
        recent_array.append(absolute_path);
        if (normalized_paths.size() >= 10)
        {
            break;
        }
    }

    root_object.insert("recent_project_paths", recent_array);
    root_object.insert("schema_version", kConfigSchemaVersion);
    return write_json_object(app_settings_file_path(), root_object, error_message);
}

bool load_reference_geometry_config(ReferenceGeometryConfig *config,
                                    QString *error_message)
{
    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonValue geometry_value = root_object.value("reference_geometry");
    if (!geometry_value.isObject())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Reference geometry configuration must be an object.",
            error_message);
    }

    const QJsonObject geometry_object = geometry_value.toObject();
    const QString file_path = geometry_object.value("file_path").toString().trimmed();
    if (file_path.isEmpty())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Reference geometry configuration has an empty file path.",
            error_message);
    }

    const auto read_vector = [&geometry_object](const char *key, QVector3D *target)
    {
        const QJsonValue value = geometry_object.value(QString::fromLatin1(key));
        if (!value.isArray() || value.toArray().size() != 3)
        {
            return false;
        }

        const QJsonArray array = value.toArray();
        for (const QJsonValue &component : array)
        {
            if (!component.isDouble() || !std::isfinite(component.toDouble()))
            {
                return false;
            }
        }

        if (target != nullptr)
        {
            *target = QVector3D(static_cast<float>(array.at(0).toDouble()),
                                static_cast<float>(array.at(1).toDouble()),
                                static_cast<float>(array.at(2).toDouble()));
        }
        return true;
    };

    ReferenceGeometryConfig loaded_config;
    loaded_config.file_path = file_path;
    if (!read_vector("position", &loaded_config.position) ||
        !read_vector("rotation", &loaded_config.rotation))
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Reference geometry position and rotation must be finite 3D vectors.",
            error_message);
    }

    const QJsonValue locked_value = geometry_object.value("locked");
    const QJsonValue visible_value = geometry_object.value("visible");
    if (!locked_value.isBool() || !visible_value.isBool())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Reference geometry locked and visible values must be boolean.",
            error_message);
    }
    loaded_config.locked = locked_value.toBool();
    loaded_config.visible = visible_value.toBool();

    if (config != nullptr)
    {
        *config = loaded_config;
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
    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonValue geometry_value = root_object.value("window_geometry");
    const QJsonValue state_value = root_object.value("window_state");
    if ((!geometry_value.isUndefined() && !geometry_value.isString()) ||
        (!state_value.isUndefined() && !state_value.isString()))
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Window geometry and state values must be strings.",
            error_message);
    }

    const QByteArray loaded_geometry = QByteArray::fromBase64(
        geometry_value.toString().toLatin1());
    const QByteArray loaded_window_state = QByteArray::fromBase64(
        state_value.toString().toLatin1());
    if (geometry != nullptr)
    {
        *geometry = loaded_geometry;
    }
    if (window_state != nullptr)
    {
        *window_state = loaded_window_state;
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
    if (!QFileInfo::exists(app_settings_file_path()))
    {
        return false;
    }

    QJsonObject root_object;
    if (!read_json_object(app_settings_file_path(), &root_object, error_message))
    {
        return false;
    }

    const QJsonValue units_value = root_object.value("unit_preferences");
    if (!units_value.isUndefined() && !units_value.isObject())
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Unit preferences must be an object.",
            error_message);
    }

    const QJsonObject units_object = units_value.toObject();
    if (units_object.isEmpty())
    {
        return false;
    }

    Unit_Preferences loaded = UnitSystem::default_preferences();
    const auto read_unit = [&units_object](const char *key, QString *target)
    {
        const QJsonValue value = units_object.value(QString::fromLatin1(key));
        if (!value.isUndefined() && !value.isString())
        {
            return false;
        }
        if (value.isString())
        {
            *target = value.toString().trimmed();
        }
        return true;
    };
    if (!read_unit("length", &loaded.length) ||
        !read_unit("angle", &loaded.angle) ||
        !read_unit("velocity", &loaded.velocity) ||
        !read_unit("mass", &loaded.mass) ||
        !read_unit("mass_flow", &loaded.mass_flow) ||
        !read_unit("time", &loaded.time) ||
        !read_unit("pressure", &loaded.pressure) ||
        !read_unit("temperature", &loaded.temperature))
    {
        return reject_invalid_config(
            app_settings_file_path(),
            "Unit preference values must be strings.",
            error_message);
    }

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

    const QJsonValue colors_value = root_object.value("species_colors");
    if (!colors_value.isObject())
    {
        return reject_invalid_config(
            file_path,
            "Species color configuration contains an invalid species_colors object.",
            error_message);
    }

    const QJsonObject colors_object = colors_value.toObject();
    for (auto it = colors_object.constBegin(); it != colors_object.constEnd(); ++it)
    {
        if (!QColor(it.value().toString()).isValid())
        {
            return reject_invalid_config(
                file_path,
                QString("Species color configuration contains an invalid color: %1")
                    .arg(it.key()),
                error_message);
        }
    }

    QHash<QString, QString> color_owners;
    for (const QString &species_name : species_names)
    {
        const QColor color(colors_object.value(species_name).toString());
        if (!color.isValid())
        {
            continue;
        }

        const QString normalized_color = color.name(QColor::HexRgb).toUpper();
        const auto owner = color_owners.constFind(normalized_color);
        if (owner != color_owners.constEnd())
        {
            return reject_invalid_config(
                file_path,
                QString("Species color configuration assigns %1 to both %2 and %3.")
                    .arg(normalized_color, owner.value(), species_name),
                error_message);
        }
        color_owners.insert(normalized_color, species_name);
    }

    QHash<QString, QColor> loaded_colors;
    for (const QString &species_name : species_names)
    {
        const QColor color(colors_object.value(species_name).toString());
        if (color.isValid())
        {
            loaded_colors.insert(species_name, color);
        }
    }

    if (species_colors != nullptr)
    {
        *species_colors = std::move(loaded_colors);
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

    const QJsonValue materials_value = root_object.value("materials");
    if (!materials_value.isArray())
    {
        return reject_invalid_config(
            file_path,
            "Material configuration materials must be an array.",
            error_message);
    }

    QList<MaterialConfigEntry> parsed_materials;
    const QJsonArray materials_array = materials_value.toArray();
    for (const QJsonValue &value : materials_array)
    {
        if (!value.isObject())
        {
            return reject_invalid_config(
                file_path,
                "Material configuration contains an invalid entry.",
                error_message);
        }

        const QJsonObject material_object = value.toObject();
        const QJsonValue name_value = material_object.value("name");
        const QJsonValue density_value = material_object.value("density");
        if (!name_value.isString())
        {
            return reject_invalid_config(
                file_path,
                "Material configuration name must be a string.",
                error_message);
        }
        if (!density_value.isDouble() || !std::isfinite(density_value.toDouble()))
        {
            return reject_invalid_config(
                file_path,
                "Material configuration density must be a finite number.",
                error_message);
        }

        MaterialConfigEntry entry;
        entry.name = name_value.toString().trimmed();
        entry.density = density_value.toDouble();
        parsed_materials.append(entry);
    }

    if (!validate_material_entries(parsed_materials, error_message))
    {
        const QString detail = error_message != nullptr && !error_message->isEmpty()
            ? *error_message
            : QStringLiteral("Material configuration failed validation.");
        return reject_invalid_config(file_path, detail, error_message);
    }

    if (materials != nullptr)
    {
        *materials = std::move(parsed_materials);
    }

    return true;
}

bool validate_material_entries(const QList<MaterialConfigEntry> &materials,
                               QString *error_message)
{
    QStringList names;
    for (const MaterialConfigEntry &entry : materials)
    {
        const QString name = entry.name.trimmed();
        if (name.isEmpty())
        {
            if (error_message != nullptr) *error_message = "Material name cannot be empty.";
            return false;
        }

        if (names.contains(name, Qt::CaseInsensitive))
        {
            if (error_message != nullptr)
            {
                *error_message = QString("Duplicate material name: %1").arg(name);
            }
            return false;
        }
        names.append(name);

        if (!std::isfinite(entry.density) || entry.density <= 0.0)
        {
            if (error_message != nullptr)
            {
                *error_message = QString("Material density must be positive and finite: %1").arg(name);
            }
            return false;
        }
    }

    if (error_message != nullptr) error_message->clear();
    return true;
}

bool save_material_table_config(const QList<MaterialConfigEntry> &materials,
                                QString *error_message)
{
    if (!ensure_app_config_directories(error_message))
    {
        return false;
    }

    if (!validate_material_entries(materials, error_message))
    {
        return false;
    }

    QJsonArray materials_array;
    for (const MaterialConfigEntry &entry : materials)
    {
        const QString name = entry.name.trimmed();

        QJsonObject material_object;
        material_object.insert("name", name);
        material_object.insert("density", entry.density);
        materials_array.append(material_object);
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
