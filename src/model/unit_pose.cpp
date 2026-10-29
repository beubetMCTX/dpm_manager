#include "unit_pose.h"

void translate_injector_points(Injector &injector, const QVector3D &delta)
{
    injector.pos += delta;
    injector.pos2 += delta;
    injector.ff_center += delta;
    injector.ff_virtual_origin += delta;
    injector.volume_bgeom_min += delta;
    injector.volume_bgeom_max += delta;
}

void set_injector_direction(Injector &injector, const QVector3D &direction)
{
    switch (injector.injection_type)
    {
    case cone:
        injector.axis = direction;
        break;
    case flat_fan_atomizer:
        injector.ff_normal = direction;
        break;
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case effervescent_atomizer:
        injector.atomizer_axis = direction;
        break;
    default:
        injector.vel = direction;
        break;
    }
}

void set_injector_pitch_yaw(Injector &injector, double pitch_degrees,
                            double yaw_degrees)
{
    injector.single_pitch_degrees = pitch_degrees;
    injector.single_yaw_degrees = yaw_degrees;
}

void set_injector_target_hitpoint(Injector &injector, const QVector3D &target)
{
    injector.single_target_hitpoint = target;
}
