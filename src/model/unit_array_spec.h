#ifndef UNIT_ARRAY_SPEC_H
#define UNIT_ARRAY_SPEC_H

#include <QVector3D>

enum class UnitArrayType
{
    Linear,
    Rotational,
    Mirror
};

struct UnitArraySpec
{
    UnitArrayType type = UnitArrayType::Linear;
    int count = 1;
    QVector3D direction = QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D origin = QVector3D(0.0f, 0.0f, 0.0f);
    float spacing = 0.0f;
    float angle_degrees = 360.0f;
    QVector3D plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
};

#endif // UNIT_ARRAY_SPEC_H
