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
                    "source was mutated");

    UnitArraySpec rotational;
    rotational.type = UnitArrayType::Rotational;
    rotational.count = 2;
    rotational.angle_degrees = 180.0f;
    rotational.direction = QVector3D(0.0f, 0.0f, 1.0f);
    const QList<Unit> rotational_children = expand_unit_array(source, rotational);
    ok = check(rotational_children.size() == 2, "rotational count mismatch") && ok;
    ok = check(qAbs(rotational_children[1].inj.injector_data.vel.y() - 10.0f) < 1.0e-4f,
               "rotational direction mismatch") && ok;

    UnitArraySpec mirror;
    mirror.type = UnitArrayType::Mirror;
    mirror.count = 2;
    mirror.plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
    const QList<Unit> mirror_children = expand_unit_array(source, mirror);
    ok = check(mirror_children[1].inj.injector_data.pos.x() == -1.0f,
               "mirror position mismatch") && ok;
    return ok ? 0 : 1;
}
