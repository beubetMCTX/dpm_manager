#include "project_session.h"

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QDebug>

#include <limits>

namespace
{
bool check(bool condition, const QString &message)
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
    QCoreApplication application(argc, argv);
    QTemporaryDir temporary_directory;
    if (!check(temporary_directory.isValid(), "Unable to create temporary test directory"))
    {
        return 1;
    }

    Unit unit;
    unit.type = injector;
    unit.inj.injector_data.name = "session-test";
    unit.inj.injector_data.injection_type = single;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.pos = QVector3D(1.0f, 2.0f, 3.0f);
    unit.inj.injector_data.vel = QVector3D(4.0f, 5.0f, 6.0f);
    unit.inj.injector_data.total_flow_rate = 0.25;
    unit.inj.injector_data.cone_angle = 37.0;
    unit.inj.injector_data.rr_disturb = true;
    unit.inj.injector_data.rr_uniform_ln_d = true;
    unit.inj.injector_data.rr_mean = 0.00042;
    unit.inj.injector_data.volume_zones = {3, 7, 11};
    if (!check(unit.inj.create_injector(), "Unable to create source injector geometry"))
    {
        return 1;
    }

    project_session::Data source;
    source.units.append(unit);
    source.chemkin_file_path = "C:/chemistry/example.inp";
    source.species_colors.insert("O2", QColor("#123456"));
    source.materials.append({"water", 998.2});
    source.reference_geometry.file_path = "C:/geometry/example.step";
    source.reference_geometry.position = QVector3D(9.0f, 8.0f, 7.0f);
    source.reference_geometry.rotation = QVector3D(10.0f, 20.0f, 30.0f);
    source.reference_geometry.locked = true;
    source.reference_geometry.visible = false;

    QString validation_error;
    if (!check(project_session::validate(source, &validation_error), validation_error))
    {
        return 1;
    }

    project_session::Data invalid = source;
    invalid.materials.first().density = 0.0;
    if (!check(!project_session::validate(invalid, &validation_error) &&
                   validation_error.contains("density"),
               "non-positive material density should fail validation"))
    {
        return 1;
    }

    invalid = source;
    invalid.units.append(source.units.first());
    if (!check(!project_session::validate(invalid, &validation_error) &&
                   validation_error.contains("duplicate unit UUIDs"),
               "duplicate unit UUID should fail validation"))
    {
        return 1;
    }

    invalid = source;
    invalid.reference_geometry.position.setX(std::numeric_limits<float>::quiet_NaN());
    if (!check(!project_session::validate(invalid, &validation_error) &&
                   validation_error.contains("non-finite"),
               "non-finite reference transform should fail validation"))
    {
        return 1;
    }

    const QString session_path = temporary_directory.filePath("round_trip.dpmproj");
    QString error_message;
    if (!check(project_session::save(session_path, source, &error_message), error_message))
    {
        return 1;
    }

    project_session::Data restored;
    if (!check(project_session::load(session_path, &restored, &error_message), error_message))
    {
        return 1;
    }

    if (!check(restored.units.size() == 1, "Unexpected restored unit count") ||
        !check(restored.units.first().inj.uuid == unit.inj.uuid, "Unit UUID did not round-trip") ||
        !check(restored.units.first().inj.injector_data.name == "session-test",
               "Unit name did not round-trip") ||
        !check(restored.units.first().inj.injector_data.pos == QVector3D(1.0f, 2.0f, 3.0f),
               "Unit position did not round-trip") ||
        !check(restored.units.first().inj.injector_data.cone_angle == 37.0,
               "Unit cone angle did not round-trip") ||
        !check(restored.units.first().inj.injector_data.rr_uniform_ln_d,
               "RR logarithmic flag did not round-trip") ||
        !check(restored.units.first().inj.injector_data.volume_zones == QVector<int>({3, 7, 11}),
               "Volume zones did not round-trip") ||
        !check(!restored.units.first().inj.shape.IsNull(),
               "Restored injector geometry was not rebuilt") ||
        !check(restored.species_colors.value("O2") == QColor("#123456"),
               "Species color did not round-trip") ||
        !check(restored.materials.size() == 1 && restored.materials.first().density == 998.2,
               "Material did not round-trip") ||
        !check(restored.reference_geometry.position == QVector3D(9.0f, 8.0f, 7.0f),
               "Reference position did not round-trip") ||
        !check(restored.reference_geometry.locked && !restored.reference_geometry.visible,
               "Reference visibility/lock state did not round-trip"))
    {
        return 1;
    }

    const QString malformed_path = temporary_directory.filePath("malformed.dpmproj");
    QJsonObject malformed_root;
    malformed_root.insert("schema_version", 1);
    malformed_root.insert("units", QJsonArray());
    malformed_root.insert("materials", QJsonArray{QJsonObject{{"name", "water"},
                                                               {"density", 0.0}}});
    QFile malformed_file(malformed_path);
    if (!check(malformed_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create malformed project fixture"))
    {
        return 1;
    }
    malformed_file.write(QJsonDocument(malformed_root).toJson());
    malformed_file.close();

    project_session::Data preserved = source;
    if (!check(!project_session::load(malformed_path, &preserved, &error_message) &&
                   error_message.contains("density"),
               "malformed project should be rejected during load") ||
        !check(preserved.units.size() == source.units.size(),
               "failed project load should not expose partial data"))
    {
        return 1;
    }

    const QString malformed_transform_path =
        temporary_directory.filePath("malformed_transform.dpmproj");
    QJsonObject malformed_transform_root;
    malformed_transform_root.insert("schema_version", 1);
    malformed_transform_root.insert("units", QJsonArray());
    malformed_transform_root.insert("materials", QJsonArray());
    malformed_transform_root.insert(
        "reference_geometry",
        QJsonObject{{"file_path", "geometry.step"},
                     {"position", QJsonArray{1.0, 2.0}},
                     {"rotation", QJsonArray{0.0, 0.0, 0.0}}});
    QFile malformed_transform_file(malformed_transform_path);
    if (!check(malformed_transform_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create malformed transform fixture"))
    {
        return 1;
    }
    malformed_transform_file.write(QJsonDocument(malformed_transform_root).toJson());
    malformed_transform_file.close();

    error_message.clear();
    if (!check(!project_session::load(malformed_transform_path, &preserved, &error_message) &&
                   error_message.contains("reference geometry transform"),
               "malformed reference transform should be rejected") ||
        !check(preserved.units.size() == source.units.size(),
               "failed transform load should not expose partial data"))
    {
        return 1;
    }

    const QString malformed_unit_path = temporary_directory.filePath("malformed_unit.dpmproj");
    QJsonObject malformed_unit_root;
    malformed_unit_root.insert("schema_version", 1);
    malformed_unit_root.insert(
        "units",
        QJsonArray{QJsonObject{
            {"uuid", source.units.first().inj.uuid.toString(QUuid::WithoutBraces)},
            {"unit_type", static_cast<int>(injector)},
            {"injector", QJsonObject{{"pos", QJsonArray{1.0, 2.0}}}}}});
    malformed_unit_root.insert("materials", QJsonArray());
    QFile malformed_unit_file(malformed_unit_path);
    if (!check(malformed_unit_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create malformed unit fixture"))
    {
        return 1;
    }
    malformed_unit_file.write(QJsonDocument(malformed_unit_root).toJson());
    malformed_unit_file.close();

    error_message.clear();
    if (!check(!project_session::load(malformed_unit_path, &preserved, &error_message) &&
                   error_message.contains("invalid injector entry"),
               "malformed injector vector should be rejected") ||
        !check(preserved.units.size() == source.units.size(),
               "failed injector load should not expose partial data"))
    {
        return 1;
    }

    QFile::remove(session_path);
    QFile::remove(malformed_path);
    QFile::remove(malformed_transform_path);
    QFile::remove(malformed_unit_path);
    return 0;
}
