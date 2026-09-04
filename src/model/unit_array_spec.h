#ifndef UNIT_ARRAY_SPEC_H
#define UNIT_ARRAY_SPEC_H

#include <QVector3D>

enum class UnitArrayType
{
    Linear,
    Rotational,
    Mirror,
    Elliptical
};

enum class UnitFillPattern
{
    Square,
    Hexagonal
};

struct UnitArraySpec
{
    UnitArrayType type = UnitArrayType::Linear;
    int count = 1;
    QVector3D direction = QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D origin = QVector3D(0.0f, 0.0f, 0.0f);
    float spacing = 0.0f;
    float angle_degrees = 360.0f;
    float major_radius = 0.01f;
    float minor_radius = 0.005f;
    QVector3D plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
    bool use_reference_geometry = false;
    bool conform_to_reference_normal = false;
};

struct UnitFillSpec
{
    UnitFillPattern pattern = UnitFillPattern::Square;
    int rows = 1;
    int columns = 1;
    float spacing_x = 0.0f;
    float spacing_y = 0.0f;
    QVector3D origin = QVector3D(0.0f, 0.0f, 0.0f);
    bool circular_boundary = false;
    float boundary_radius = 0.0f;
    QVector3D direction = QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D plane_normal = QVector3D(0.0f, 0.0f, 1.0f);
    bool use_reference_geometry = false;
    bool conform_to_reference_normal = false;
    // Positive weights select multiple source injectors in a repeating ratio.
    QVector<int> source_weights = {1};
};

#endif // UNIT_ARRAY_SPEC_H
