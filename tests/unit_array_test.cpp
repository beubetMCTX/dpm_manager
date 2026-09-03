#include "unit_array.h"

#include <QCoreApplication>
#include <QDebug>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        qCritical() << message;
    }
    return condition;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    Unit source;
    source.inj.injector_data.name = "master";
    source.inj.injector_data.pos = QVector3D(1.0f, 2.0f, 0.0f);
    source.inj.injector_data.pos2 = QVector3D(2.0f, 2.0f, 0.0f);
    source.inj.injector_data.vel = QVector3D(10.0f, 0.0f, 0.0f);
    source.inj.injector_data.single_target_hitpoint = QVector3D(3.0f, 2.0f, 0.0f);

    UnitArraySpec linear;
    linear.type = UnitArrayType::Linear;
    linear.count = 3;
    linear.spacing = 4.0f;
    const QList<Unit> linear_children = expand_unit_array(source, linear);
    bool ok = check(linear_children.size() == 3, "linear count mismatch") &&
              check(linear_children[2].inj.injector_data.pos == QVector3D(9.0f, 2.0f, 0.0f),
                    "linear offset mismatch") &&
              check(linear_children[0].inj.uuid != linear_children[1].inj.uuid,
                    "child UUIDs must be unique") &&
              check(source.inj.injector_data.pos == QVector3D(1.0f, 2.0f, 0.0f),
                    "source was mutated") &&
              check(linear_children[2].inj.injector_data.single_target_hitpoint ==
                        QVector3D(11.0f, 2.0f, 0.0f),
                    "linear target hitpoint mismatch");

    UnitArraySpec rotational;
    rotational.type = UnitArrayType::Rotational;
    rotational.count = 2;
    rotational.angle_degrees = 180.0f;
    rotational.direction = QVector3D(0.0f, 0.0f, 1.0f);
    const QList<Unit> rotational_children = expand_unit_array(source, rotational);
    ok = check(rotational_children.size() == 2, "rotational count mismatch") && ok;
    ok = check(qAbs(rotational_children[1].inj.injector_data.vel.y() - 10.0f) < 1.0e-4f,
               "rotational direction mismatch") && ok;
    ok = check(qAbs(rotational_children[1].inj.injector_data.single_target_hitpoint.x() + 2.0f) < 1.0e-4f &&
                   qAbs(rotational_children[1].inj.injector_data.single_target_hitpoint.y() - 3.0f) < 1.0e-4f,
               "rotational target hitpoint mismatch") && ok;

    rotational.spacing = 2.5f;
    const QList<Unit> helical_children = expand_unit_array(source, rotational);
    ok = check(qAbs(helical_children[1].inj.injector_data.pos.z() - 2.5f) < 1.0e-4f,
               "rotational axial spacing mismatch") && ok;

    UnitArraySpec elliptical;
    elliptical.type = UnitArrayType::Elliptical;
    elliptical.count = 4;
    elliptical.origin = QVector3D(0.0f, 0.0f, 0.0f);
    elliptical.direction = QVector3D(1.0f, 0.0f, 0.0f);
    elliptical.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    elliptical.major_radius = 10.0f;
    elliptical.minor_radius = 4.0f;
    const QList<Unit> elliptical_children = expand_unit_array(source, elliptical);
    ok = check(qAbs(elliptical_children[0].inj.injector_data.pos.x() - 10.0f) < 1.0e-4f &&
                   qAbs(elliptical_children[1].inj.injector_data.pos.y() - 4.0f) < 1.0e-4f,
               "elliptical array positions mismatch") && ok;

    source.inj.injector_data.single_target_scope = Single_Target_Scope::World;
    const QList<Unit> world_target_children = expand_unit_array(source, linear);
    ok = check(world_target_children[2].inj.injector_data.single_target_hitpoint ==
                   source.inj.injector_data.single_target_hitpoint,
               "world target hitpoint must not follow a linear array") && ok;

    source.inj.injector_data.single_target_scope = Single_Target_Scope::Parent_Local;
    const QList<Unit> parent_target_children = expand_unit_array(source, linear);
    ok = check(parent_target_children[2].inj.injector_data.single_target_hitpoint ==
                   QVector3D(11.0f, 2.0f, 0.0f),
               "parent-local target hitpoint must follow a linear array") && ok;

    source.inj.injector_data.single_target_scope = Single_Target_Scope::Reference_Local;
    UnitArraySpec reference_frame;
    reference_frame.type = UnitArrayType::Linear;
    reference_frame.count = 2;
    reference_frame.spacing = 4.0f;
    reference_frame.origin = QVector3D(10.0f, 20.0f, 30.0f);
    reference_frame.direction = QVector3D(0.0f, 1.0f, 0.0f);
    reference_frame.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    const QList<Unit> reference_target_children =
        expand_unit_array(source, reference_frame);
    ok = check(reference_target_children[0].inj.injector_data.single_target_hitpoint ==
                   QVector3D(8.0f, 23.0f, 30.0f),
               "reference-local target conversion mismatch") && ok;
    ok = check(reference_target_children[1].inj.injector_data.single_target_hitpoint ==
                   reference_target_children[0].inj.injector_data.single_target_hitpoint,
               "reference-local target must not follow array offset") && ok;

    Unit conform_source = source;
    conform_source.inj.injector_data.single_target_scope = Single_Target_Scope::World;
    conform_source.inj.injector_data.vel = QVector3D(1.0f, 0.0f, 0.0f);
    UnitArraySpec conform_spec;
    conform_spec.type = UnitArrayType::Linear;
    conform_spec.count = 2;
    conform_spec.use_reference_geometry = true;
    conform_spec.conform_to_reference_normal = true;
    conform_spec.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    const QList<Unit> conform_children = expand_unit_array(conform_source, conform_spec);
    ok = check(qAbs(conform_children[0].inj.injector_data.vel.z() - 1.0f) < 1.0e-4f,
               "reference normal conforming should rotate the primary direction") && ok;

    source.has_array_spec = true;
    source.array_spec = linear;
    const Unit copied_source(source);
    ok = check(copied_source.has_array_spec &&
                   copied_source.array_spec.type == UnitArrayType::Linear &&
                   copied_source.array_spec.count == linear.count,
               "Unit copy must preserve array metadata") && ok;

    UnitArraySpec mirror;
    mirror.type = UnitArrayType::Mirror;
    mirror.count = 2;
    mirror.plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
    source.inj.injector_data.single_target_scope = Single_Target_Scope::Array_Local;
    const QList<Unit> mirror_children = expand_unit_array(source, mirror);
    ok = check(mirror_children[1].inj.injector_data.pos.x() == -1.0f,
               "mirror position mismatch") && ok;
    ok = check(mirror_children[1].inj.injector_data.single_target_hitpoint.x() == -3.0f,
               "mirror target hitpoint mismatch") && ok;

    Unit second_source(source);
    second_source.inj.injector_data.name = "secondary";
    UnitFillSpec hex_fill;
    hex_fill.pattern = UnitFillPattern::Hexagonal;
    hex_fill.rows = 2;
    hex_fill.columns = 2;
    hex_fill.spacing_x = 4.0f;
    hex_fill.spacing_y = 3.0f;
    hex_fill.origin = QVector3D(10.0f, 20.0f, 0.0f);
    const QList<Unit> filled = expand_unit_fill({source, second_source}, hex_fill);
    ok = check(filled.size() == 4, "fill count mismatch") && ok;
    ok = check(filled[0].inj.injector_data.name.startsWith("master"),
               "fill seed assignment mismatch") && ok;
    ok = check(filled[1].inj.injector_data.name.startsWith("secondary"),
               "fill seed round-robin mismatch") && ok;
    ok = check(qAbs(filled[3].inj.injector_data.pos.x() - 16.0f) < 1.0e-4f &&
                   qAbs(filled[3].inj.injector_data.pos.y() - 23.0f) < 1.0e-4f,
               "hexagonal row offset mismatch") && ok;

    hex_fill.circular_boundary = true;
    hex_fill.boundary_radius = 4.1f;
    const QList<Unit> circular_filled = expand_unit_fill({source}, hex_fill);
    ok = check(circular_filled.size() == 3,
               "circular fill should reject points outside the boundary") && ok;

    UnitFillSpec reference_fill;
    reference_fill.rows = 1;
    reference_fill.columns = 2;
    reference_fill.spacing_x = 4.0f;
    reference_fill.origin = QVector3D(10.0f, 20.0f, 30.0f);
    reference_fill.direction = QVector3D(0.0f, 1.0f, 0.0f);
    reference_fill.plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    reference_fill.use_reference_geometry = true;
    reference_fill.conform_to_reference_normal = true;
    Unit target_fill_source = conform_source;
    target_fill_source.inj.injector_data.single_target_scope =
        Single_Target_Scope::Reference_Local;
    target_fill_source.inj.injector_data.single_target_hitpoint =
        QVector3D(1.0f, 2.0f, 3.0f);
    const QList<Unit> reference_filled = expand_unit_fill({target_fill_source}, reference_fill);
    ok = check(reference_filled.size() == 2 &&
                   reference_filled[1].inj.injector_data.pos == QVector3D(10.0f, 24.0f, 30.0f),
               "reference-frame fill position mismatch") && ok;
    ok = check(qAbs(reference_filled[0].inj.injector_data.vel.z() - 1.0f) < 1.0e-4f,
               "reference-frame fill should conform direction") && ok;
    ok = check(reference_filled[0].inj.injector_data.single_target_hitpoint ==
                   QVector3D(8.0f, 21.0f, 33.0f),
               "reference-frame fill should resolve local target points") && ok;
    return ok ? 0 : 1;
}
