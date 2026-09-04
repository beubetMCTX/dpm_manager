#include "project_session.h"

#include <QColor>
#include <QCoreApplication>
#include <QDir>
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
    unit.has_fill_spec = true;
    unit.fill_spec.pattern = UnitFillPattern::Hexagonal;
    unit.fill_spec.rows = 3;
    unit.fill_spec.columns = 5;
    unit.fill_spec.spacing_x = 2.5f;
    unit.fill_spec.spacing_y = 2.0f;
    unit.fill_spec.origin = QVector3D(4.0f, 5.0f, 6.0f);
    unit.fill_source_uuids = {unit.inj.uuid};
    unit.has_array_spec = false;
    unit.array_spec.type = UnitArrayType::Elliptical;
    unit.array_spec.count = 6;
    unit.array_spec.major_radius = 12.0f;
    unit.array_spec.minor_radius = 7.0f;
    unit.array_spec.direction = QVector3D(1.0f, 0.0f, 0.0f);
    unit.array_spec.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    if (!check(unit.inj.create_injector(), "Unable to create source injector geometry"))
    {
        return 1;
    }
    unit.has_array_spec = true;

    project_session::Data source;
    source.units.append(unit);
    source.chemkin_file_path = temporary_directory.filePath("inputs/example.inp");
    source.species_colors.insert("O2", QColor("#123456"));
    source.materials.append({"water", 998.2});
    source.reference_geometry.file_path = temporary_directory.filePath("geometry/example.step");
    source.reference_geometry.position = QVector3D(9.0f, 8.0f, 7.0f);
    source.reference_geometry.rotation = QVector3D(10.0f, 20.0f, 30.0f);
    source.reference_geometry.locked = true;
    source.reference_geometry.visible = false;
    source.unit_preferences.length = "cm";
    source.unit_preferences.angle = "rad";
    source.has_unit_preferences = true;

    source.units.first().inj.injector_data.material = "water";
    source.units.first().inj.injector_data.product_species = "O2";
    QString reference_error;
    project_session::Data invalid_array_spec = source;
    invalid_array_spec.units.first().has_array_spec = true;
    invalid_array_spec.units.first().array_spec.count = 0;
    if (!check(!project_session::validate(invalid_array_spec, &reference_error) &&
                   reference_error.contains("invalid array specification"),
               "invalid array metadata should be rejected"))
    {
        return 1;
    }
    invalid_array_spec.units.first().array_spec.count = 4;
    invalid_array_spec.units.first().array_spec.type = UnitArrayType::Elliptical;
    invalid_array_spec.units.first().array_spec.major_radius = 0.0f;
    if (!check(!project_session::validate(invalid_array_spec, &reference_error) &&
                   reference_error.contains("invalid array specification"),
               "invalid elliptical radii should be rejected"))
    {
        return 1;
    }
    invalid_array_spec.units.first().array_spec.major_radius = 12.0f;
    invalid_array_spec.units.first().array_spec.direction =
        QVector3D(0.0f, 0.0f, 1.0f);
    invalid_array_spec.units.first().array_spec.plane_normal =
        QVector3D(0.0f, 0.0f, 2.0f);
    invalid_array_spec.units.first().array_spec.use_reference_geometry = true;
    if (!check(!project_session::validate(invalid_array_spec, &reference_error) &&
                   reference_error.contains("invalid array specification"),
               "parallel array reference axes should be rejected"))
    {
        return 1;
    }
    Unit fill_only_unit = unit;
    fill_only_unit.has_array_spec = false;
    fill_only_unit.has_fill_spec = true;
    fill_only_unit.fill_spec.spacing_x =
        std::numeric_limits<float>::infinity();
    project_session::Data invalid_fill_spec;
    invalid_fill_spec.units.append(fill_only_unit);
    const bool invalid_fill_valid = project_session::validate(invalid_fill_spec,
                                                              &reference_error);
    if (!check(!invalid_fill_valid &&
                   reference_error.contains("invalid fill specification"),
               "non-finite fill metadata should be rejected"))
    {
        return 1;
    }
    const bool source_references_valid =
        project_session::validate_references(source, {"O2", "N2"}, &reference_error);
    if (!check(source_references_valid, reference_error))
    {
        return 1;
    }

    project_session::Data without_chemkin = source;
    without_chemkin.chemkin_file_path.clear();
    without_chemkin.units.first().inj.injector_data.product_species = "O2";
    if (!check(!project_session::validate_references(without_chemkin,
                                                     {},
                                                     &reference_error) &&
                   reference_error.contains("no Chemkin file is loaded"),
               "species references should fail when no Chemkin file is loaded"))
    {
        return 1;
    }

    project_session::Data invalid_reference = source;
    invalid_reference.units.first().inj.injector_data.material = "missing-liquid";
    invalid_reference.units.first().inj.injector_data.product_species = "CH4";
    if (!check(!project_session::validate_references(invalid_reference,
                                                     {"O2", "N2"},
                                                     &reference_error) &&
                   reference_error.contains("2 problem(s)") &&
                   reference_error.contains("missing-liquid") &&
                   reference_error.contains("CH4"),
               "missing material and species references should fail together"))
    {
        return 1;
    }

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
    Unit duplicate_name = source.units.first();
    duplicate_name.inj.uuid = QUuid::createUuid();
    invalid.units.append(duplicate_name);
    if (!check(!project_session::validate(invalid, &validation_error) &&
                   validation_error.contains("duplicate injector name"),
               "duplicate injector names should fail project validation"))
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

    project_session::Data fingerprint_variant = source;
    if (!check(project_session::fingerprint(source) ==
                   project_session::fingerprint(fingerprint_variant),
               "Identical project data should have identical fingerprints"))
    {
        return 1;
    }
    fingerprint_variant.units.first().inj.injector_data.pos.setX(99.0f);
    if (!check(project_session::fingerprint(source) !=
                   project_session::fingerprint(fingerprint_variant),
               "Editable project changes should change the fingerprint"))
    {
        return 1;
    }

    QFile saved_session(session_path);
    if (!check(saved_session.open(QIODevice::ReadOnly | QIODevice::Text),
               "Unable to inspect saved project session"))
    {
        return 1;
    }
    const QJsonObject saved_root =
        QJsonDocument::fromJson(saved_session.readAll()).object();
    saved_session.close();
    if (!check(!QFileInfo(saved_root.value("chemkin_file_path").toString()).isAbsolute() &&
                   !QFileInfo(saved_root.value("reference_geometry")
                                  .toObject()
                                  .value("file_path")
                                  .toString())
                       .isAbsolute(),
               "Project sessions should store paths relative to the session file"))
    {
        return 1;
    }

    project_session::Data restored;
    if (!check(project_session::load(session_path, &restored, &error_message), error_message))
    {
        return 1;
    }

    if (!check(restored.units.size() == 1, "Unexpected restored unit count") ||
        !check(restored.chemkin_file_path == QFileInfo(source.chemkin_file_path).absoluteFilePath(),
               "Relative Chemkin path did not resolve during load") ||
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
        !check(restored.units.first().has_fill_spec &&
                   restored.units.first().fill_spec.pattern == UnitFillPattern::Hexagonal &&
                   restored.units.first().fill_spec.rows == 3 &&
                   restored.units.first().fill_source_uuids == QVector<QUuid>({unit.inj.uuid}),
               "Fill metadata did not round-trip") ||
        !check(restored.units.first().has_array_spec &&
                   restored.units.first().array_spec.type == UnitArrayType::Elliptical &&
                   restored.units.first().array_spec.major_radius == 12.0f &&
                   restored.units.first().array_spec.minor_radius == 7.0f,
               "Elliptical array metadata did not round-trip") ||
        !check(!restored.units.first().inj.shape.IsNull(),
               "Restored injector geometry was not rebuilt") ||
        !check(restored.species_colors.value("O2") == QColor("#123456"),
               "Species color did not round-trip") ||
        !check(restored.materials.size() == 1 && restored.materials.first().density == 998.2,
               "Material did not round-trip") ||
        !check(restored.reference_geometry.position == QVector3D(9.0f, 8.0f, 7.0f),
               "Reference position did not round-trip") ||
        !check(restored.reference_geometry.file_path ==
                   QFileInfo(source.reference_geometry.file_path).absoluteFilePath(),
               "Relative reference geometry path did not resolve during load") ||
        !check(restored.reference_geometry.locked && !restored.reference_geometry.visible,
               "Reference visibility/lock state did not round-trip") ||
        !check(restored.has_unit_preferences &&
                   restored.unit_preferences.length == "cm" &&
                   restored.unit_preferences.angle == "rad",
               "Unit preferences did not round-trip"))
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
                     {"position", QJsonArray{1.0, "invalid", 3.0}},
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

    const QString malformed_units_path = temporary_directory.filePath("malformed_units.dpmproj");
    QJsonObject malformed_units_root;
    malformed_units_root.insert("schema_version", 1);
    malformed_units_root.insert("units", QJsonArray());
    malformed_units_root.insert("materials", QJsonArray());
    malformed_units_root.insert(
        "unit_preferences",
        QJsonObject{{"length", "rad"}, {"angle", "deg"}});
    QFile malformed_units_file(malformed_units_path);
    if (!check(malformed_units_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create malformed unit preferences fixture"))
    {
        return 1;
    }
    malformed_units_file.write(QJsonDocument(malformed_units_root).toJson());
    malformed_units_file.close();

    error_message.clear();
    if (!check(!project_session::load(malformed_units_path, &preserved, &error_message) &&
                   error_message.contains("display unit"),
               "incompatible project unit preferences should be rejected") ||
        !check(preserved.units.size() == source.units.size(),
               "failed unit preference load should not expose partial data"))
    {
        return 1;
    }

    const QString malformed_structure_path =
        temporary_directory.filePath("malformed_structure.dpmproj");
    QJsonObject malformed_structure_root;
    malformed_structure_root.insert("schema_version", 1);
    malformed_structure_root.insert("materials", QJsonObject());
    QFile malformed_structure_file(malformed_structure_path);
    if (!check(malformed_structure_file.open(QIODevice::WriteOnly | QIODevice::Text),
               "Unable to create malformed structure fixture"))
    {
        return 1;
    }
    malformed_structure_file.write(QJsonDocument(malformed_structure_root).toJson());
    malformed_structure_file.close();

    error_message.clear();
    if (!check(!project_session::load(malformed_structure_path, &preserved, &error_message) &&
                   error_message.contains("units array"),
               "project session with missing units array should be rejected") ||
        !check(preserved.units.size() == source.units.size(),
               "failed structural project load should not expose partial data"))
    {
        return 1;
    }

    const auto check_invalid_session = [&](const QString &name,
                                           const QJsonObject &root,
                                           const QString &expected_message)
    {
        const QString path = temporary_directory.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return false;
        }
        const QByteArray contents = QJsonDocument(root).toJson();
        if (file.write(contents) != contents.size())
        {
            return false;
        }
        file.close();

        project_session::Data untouched = source;
        QString local_error;
        const bool rejected = !project_session::load(path, &untouched, &local_error) &&
                              local_error.contains(expected_message) &&
                              untouched.units.size() == source.units.size();
        QFile::remove(path);
        return rejected;
    };

    if (!check(check_invalid_session(
                   "invalid_schema_type.dpmproj",
                   QJsonObject{{"schema_version", "1"}, {"units", QJsonArray{}}},
                   "invalid schema version"),
               "non-numeric project schema version should be rejected") ||
        !check(check_invalid_session(
                   "invalid_color_type.dpmproj",
                   QJsonObject{{"schema_version", 1},
                               {"units", QJsonArray{}},
                               {"species_colors", QJsonArray{}}},
                   "invalid species_colors object"),
               "non-object species colors should be rejected") ||
        !check(check_invalid_session(
                   "invalid_material_type.dpmproj",
                   QJsonObject{{"schema_version", 1},
                               {"units", QJsonArray{}},
                               {"materials", QJsonArray{
                                   QJsonObject{{"name", "water"},
                                                {"density", "998.2"}}}}},
                   "invalid field types"),
               "non-numeric material density should be rejected") ||
        !check(check_invalid_session(
                   "invalid_reference_flag_type.dpmproj",
                   QJsonObject{{"schema_version", 1},
                               {"units", QJsonArray{}},
                               {"reference_geometry", QJsonObject{
                                   {"file_path", "geometry.step"},
                                   {"locked", "false"}}}},
                   "visibility flags"),
               "non-boolean reference geometry flags should be rejected"))
    {
        return 1;
    }

    project_session::Data assembly_data = source;
    Unit assembly_member(unit);
    assembly_member.inj.uuid = QUuid::createUuid();
    assembly_member.inj.injector_data.name = "assembly-member";
    assembly_data.units.first().type = Assebly;
    assembly_data.units.first().assembly_child_uuids = {assembly_member.inj.uuid};
    assembly_data.units.first().has_array_spec = true;
    assembly_data.units.first().array_spec.use_reference_geometry = true;
    assembly_data.units.first().array_spec.conform_to_reference_normal = true;
    assembly_data.units.first().has_fill_spec = true;
    assembly_data.units.first().fill_spec.use_reference_geometry = true;
    assembly_data.units.first().fill_spec.conform_to_reference_normal = true;
    assembly_data.units.first().fill_spec.direction = QVector3D(0.0f, 1.0f, 0.0f);
    assembly_data.units.first().fill_spec.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    assembly_member.assembly_parent_uuid = assembly_data.units.first().inj.uuid;
    assembly_data.units.append(assembly_member);
    if (!check(project_session::validate_references(assembly_data, {"O2", "N2"},
                                                    &reference_error),
               reference_error))
    {
        return 1;
    }
    const QString assembly_path = temporary_directory.filePath("assembly.dpmproj");
    if (!check(project_session::save(assembly_path, assembly_data, &error_message),
               error_message))
    {
        return 1;
    }
    project_session::Data restored_assembly;
    if (!check(project_session::load(assembly_path, &restored_assembly, &error_message),
               error_message) ||
        !check(restored_assembly.units.size() == 2 &&
                   restored_assembly.units.first().type == Assebly &&
                   restored_assembly.units.first().assembly_child_uuids ==
                       QList<QUuid>({assembly_member.inj.uuid}) &&
                   restored_assembly.units.first().has_array_spec &&
                   restored_assembly.units.first().array_spec.use_reference_geometry &&
                   restored_assembly.units.first().array_spec.conform_to_reference_normal &&
                   restored_assembly.units.first().has_fill_spec &&
                   restored_assembly.units.first().fill_spec.use_reference_geometry &&
                   restored_assembly.units.first().fill_spec.conform_to_reference_normal &&
                   restored_assembly.units.first().fill_spec.direction ==
                       QVector3D(0.0f, 1.0f, 0.0f) &&
                   restored_assembly.units.at(1).assembly_parent_uuid ==
                       assembly_data.units.first().inj.uuid,
               "Assembly relationships did not round-trip"))
    {
        return 1;
    }

    project_session::Data invalid_assembly = assembly_data;
    invalid_assembly.units.first().assembly_child_uuids.clear();
    if (!check(!project_session::validate_references(invalid_assembly, {"O2", "N2"},
                                                     &reference_error) &&
                   reference_error.contains("invalid Assembly parent"),
               "inconsistent Assembly parent/child references should fail"))
    {
        return 1;
    }

    Unit nested_assembly(assembly_member);
    nested_assembly.inj.uuid = QUuid::createUuid();
    nested_assembly.inj.injector_data.name = "nested-assembly";
    nested_assembly.type = Assebly;
    nested_assembly.assembly_parent_uuid = assembly_data.units.first().inj.uuid;
    assembly_data.units.first().assembly_child_uuids = {nested_assembly.inj.uuid};
    nested_assembly.assembly_child_uuids = {assembly_data.units.first().inj.uuid};
    assembly_data.units.append(nested_assembly);
    if (!check(!project_session::validate_references(assembly_data, {"O2", "N2"},
                                                     &reference_error) &&
                   reference_error.contains("cyclic Assembly"),
               "cyclic Assembly relationships should fail"))
    {
        return 1;
    }

    project_session::Data datum_source = source;
    datum_source.reference_geometry.kind = "datum_plane";
    datum_source.reference_geometry.file_path.clear();
    datum_source.reference_geometry.construction_direction =
        QVector3D(1.0f, 2.0f, 3.0f).normalized();
    datum_source.reference_geometry.construction_size = 24.0;
    datum_source.reference_geometry.construction_thickness = 0.2;
    const QString datum_path = temporary_directory.filePath("datum.dpmproj");
    if (!check(project_session::save(datum_path, datum_source, &error_message),
               error_message))
    {
        return 1;
    }
    project_session::Data restored_datum;
    if (!check(project_session::load(datum_path, &restored_datum, &error_message),
               error_message) ||
        !check(restored_datum.reference_geometry.kind == "datum_plane" &&
                   restored_datum.reference_geometry.file_path.isEmpty() &&
                   restored_datum.reference_geometry.construction_direction ==
                       datum_source.reference_geometry.construction_direction &&
                   restored_datum.reference_geometry.construction_size == 24.0 &&
                   restored_datum.reference_geometry.construction_thickness == 0.2,
               "Constructed reference geometry did not round-trip"))
    {
        return 1;
    }

    project_session::Data invalid_datum = datum_source;
    invalid_datum.reference_geometry.construction_direction = QVector3D();
    if (!check(!project_session::validate(invalid_datum, &validation_error) &&
                   validation_error.contains("constructed reference geometry"),
               "zero constructed reference direction should fail validation"))
    {
        return 1;
    }

    project_session::Data frame_source = datum_source;
    frame_source.reference_geometry.kind = "alignment_frame";
    frame_source.reference_geometry.construction_size = 4.5;
    const QString frame_path = temporary_directory.filePath("alignment_frame.dpmproj");
    if (!check(project_session::save(frame_path, frame_source, &error_message),
               error_message))
    {
        return 1;
    }
    project_session::Data restored_frame;
    if (!check(project_session::load(frame_path, &restored_frame, &error_message),
               error_message) ||
        !check(restored_frame.reference_geometry.kind == "alignment_frame" &&
                   restored_frame.reference_geometry.construction_size == 4.5 &&
                   restored_frame.reference_geometry.construction_direction ==
                       frame_source.reference_geometry.construction_direction,
               "Alignment frame parameters did not round-trip"))
    {
        return 1;
    }

    project_session::Data invalid_frame = frame_source;
    invalid_frame.reference_geometry.construction_radius = 0.0;
    if (!check(!project_session::validate(invalid_frame, &validation_error) &&
                   validation_error.contains("constructed reference geometry"),
               "invalid alignment frame radius should fail validation"))
    {
        return 1;
    }

    project_session::Data section_source = datum_source;
    section_source.reference_geometry.kind = "section_plane";
    section_source.reference_geometry.construction_size = 31.0;
    section_source.reference_geometry.construction_thickness = 0.35;
    const QString section_path = temporary_directory.filePath("section.dpmproj");
    if (!check(project_session::save(section_path, section_source, &error_message),
               error_message))
    {
        return 1;
    }
    project_session::Data restored_section;
    if (!check(project_session::load(section_path, &restored_section, &error_message),
               error_message) ||
        !check(restored_section.reference_geometry.kind == "section_plane" &&
                   restored_section.reference_geometry.construction_size == 31.0 &&
                   restored_section.reference_geometry.construction_thickness == 0.35,
               "Section plane parameters did not round-trip"))
    {
        return 1;
    }

    project_session::Data origin_source = datum_source;
    origin_source.reference_geometry.kind = "datum_origin";
    origin_source.reference_geometry.construction_radius = 0.42;
    const QString origin_path = temporary_directory.filePath("origin.dpmproj");
    if (!check(project_session::save(origin_path, origin_source, &error_message),
               error_message))
    {
        return 1;
    }
    project_session::Data restored_origin;
    if (!check(project_session::load(origin_path, &restored_origin, &error_message),
               error_message) ||
        !check(restored_origin.reference_geometry.kind == "datum_origin" &&
                   restored_origin.reference_geometry.construction_radius == 0.42,
               "Datum origin parameters did not round-trip"))
    {
        return 1;
    }

    QFile::remove(session_path);
    QFile::remove(assembly_path);
    QFile::remove(datum_path);
    QFile::remove(section_path);
    QFile::remove(origin_path);
    QFile::remove(frame_path);
    QFile::remove(malformed_path);
    QFile::remove(malformed_transform_path);
    QFile::remove(malformed_unit_path);
    QFile::remove(malformed_units_path);
    QFile::remove(malformed_structure_path);
    return 0;
}
