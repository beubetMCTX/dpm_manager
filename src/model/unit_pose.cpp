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

void map_injector_spatial(
    Injector &injector,
    const std::function<QVector3D(const QVector3D &)> &map_point,
    const std::function<QVector3D(const QVector3D &)> &map_direction,
    bool move_target_hitpoint)
{
    injector.pos = map_point(injector.pos);
    injector.pos2 = map_point(injector.pos2);
    injector.ff_center = map_point(injector.ff_center);
    injector.ff_virtual_origin = map_point(injector.ff_virtual_origin);
    injector.volume_bgeom_min = map_point(injector.volume_bgeom_min);
    injector.volume_bgeom_max = map_point(injector.volume_bgeom_max);

    const auto turn = [&](QVector3D &direction)
    {
        direction = map_direction(direction);
    };
    turn(injector.vel);
    turn(injector.vel2);
    turn(injector.ang_vel);
    turn(injector.ang_vel2);
    turn(injector.atomizer_axis);
    turn(injector.ff_normal);
    turn(injector.axis);

    if (move_target_hitpoint)
    {
        set_injector_target_hitpoint(injector, map_point(injector.single_target_hitpoint));
    }
}

void align_injector_directions(Injector &injector, const QVector3D &direction)
{
    const auto along = [&](const QVector3D &value)
    {
        return direction * value.length();
    };
    injector.axis = along(injector.axis);
    injector.atomizer_axis = along(injector.atomizer_axis);
    injector.ff_normal = along(injector.ff_normal);
    injector.vel = along(injector.vel);
    injector.vel2 = along(injector.vel2);
}
