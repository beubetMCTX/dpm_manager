#ifndef TRANSFORM_SNAP_H
#define TRANSFORM_SNAP_H

#include <cmath>

#include <gp_Pnt.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

namespace transform_snap
{
inline double snap_scalar(double value, double increment)
{
    if (!std::isfinite(value) || !std::isfinite(increment) || increment <= 0.0)
    {
        return value;
    }
    return std::round(value / increment) * increment;
}

inline gp_Trsf snap_rotation_about_pivot(gp_Trsf transformation,
                                         const gp_Pnt &pivot,
                                         double increment)
{
    if (!std::isfinite(increment) || increment <= 0.0)
    {
        return transformation;
    }

    gp_XYZ rotation_axis;
    Standard_Real rotation_angle = 0.0;
    if (!transformation.GetRotation(rotation_axis, rotation_angle) ||
        rotation_axis.Modulus() <= Precision::Confusion())
    {
        return transformation;
    }

    const Standard_Real snapped_angle = snap_scalar(rotation_angle, increment);
    transformation.SetRotationPart(gp_Quaternion(
        gp_Vec(rotation_axis), snapped_angle));

    // Recompute the translation required to keep the requested pivot fixed
    // after replacing the unsnapped rotation with its snapped equivalent.
    gp_Pnt transformed_pivot = pivot;
    transformed_pivot.Transform(transformation);
    const gp_Vec correction(transformed_pivot, pivot);
    transformation.SetTranslationPart(
        transformation.TranslationPart() + correction.XYZ());
    return transformation;
}
}

#endif // TRANSFORM_SNAP_H
