#include "unit_array.h"

#include <QQuaternion>
#include <QtMath>

namespace
{
QVector3D normalized_or(const QVector3D &value, const QVector3D &fallback)
{
    return value.lengthSquared() > 1.0e-12f ? value.normalized() : fallback.normalized();
}

QVector3D reference_local_point(const QVector3D &value,
                                const QVector3D &origin,
                                const QVector3D &direction,
                                const QVector3D &plane_normal)
{
    const QVector3D x_axis = normalized_or(direction,
                                           QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D z_axis = normalized_or(plane_normal,
                                           QVector3D(0.0f, 0.0f, 1.0f));
    const QVector3D y_axis = QVector3D::crossProduct(z_axis, x_axis).normalized();
    return origin + x_axis * value.x() + y_axis * value.y() + z_axis * value.z();
}

QVector3D reference_local_point(const QVector3D &value,
                                const UnitArraySpec &spec)
{
    return reference_local_point(value, spec.origin, spec.direction,
                                 spec.plane_normal);
}

QVector3D rotate_vector(const QVector3D &value, const QVector3D &axis, float angle)
{
    return QQuaternion::fromAxisAndAngle(axis,
                                         static_cast<float>(qRadiansToDegrees(angle))) * value;
}

QVector3D primary_direction(const Injector &injector)
{
    switch (injector.injection_type)
    {
    case cone:
        return injector.axis;
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case flat_fan_atomizer:
    case effervescent_atomizer:
        return injector.atomizer_axis;
    default:
        return injector.vel;
    }
}

void conform_injector_to_normal(Injector &injector, const QVector3D &normal)
{
    const QVector3D source = primary_direction(injector);
    const QVector3D target = normalized_or(normal, QVector3D(0.0f, 0.0f, 1.0f));
    if (source.lengthSquared() <= 1.0e-12f || target.lengthSquared() <= 1.0e-12f)
    {
        return;
    }

    const QQuaternion rotation = QQuaternion::rotationTo(source.normalized(), target);
    const auto rotate_direction = [&](QVector3D &direction)
    {
        if (direction.lengthSquared() > 1.0e-12f)
        {
            const float magnitude = direction.length();
            direction = (rotation * direction.normalized()) * magnitude;
        }
    };
    rotate_direction(injector.vel);
    rotate_direction(injector.vel2);
    rotate_direction(injector.ang_vel);
    rotate_direction(injector.ang_vel2);
    rotate_direction(injector.atomizer_axis);
    rotate_direction(injector.axis);
    rotate_direction(injector.ff_normal);
}

void rotate_injector_data(Injector &injector, const QVector3D &origin,
                          const QVector3D &axis, float angle)
{
    const auto rotate_point = [&](QVector3D &point)
    {
        point = origin + rotate_vector(point - origin, axis, angle);
    };
    const auto rotate_direction = [&](QVector3D &direction)
    {
        direction = rotate_vector(direction, axis, angle);
    };

    rotate_point(injector.pos);
    rotate_point(injector.pos2);
    rotate_point(injector.ff_center);
    rotate_point(injector.ff_virtual_origin);
    rotate_point(injector.volume_bgeom_min);
    rotate_point(injector.volume_bgeom_max);
    if (injector.single_target_scope != Single_Target_Scope::World &&
        injector.single_target_scope != Single_Target_Scope::Reference_Local)
    {
        rotate_point(injector.single_target_hitpoint);
    }
    rotate_direction(injector.vel);
    rotate_direction(injector.vel2);
    rotate_direction(injector.ang_vel);
    rotate_direction(injector.ang_vel2);
    rotate_direction(injector.atomizer_axis);
    rotate_direction(injector.ff_normal);
    rotate_direction(injector.axis);
}

void mirror_injector_data(Injector &injector, const QVector3D &point,
                          const QVector3D &normal)
{
    const QVector3D n = normalized_or(normal, QVector3D(1.0f, 0.0f, 0.0f));
    const auto mirror_point = [&](QVector3D &value)
    {
        const float distance = QVector3D::dotProduct(value - point, n);
        value -= 2.0f * distance * n;
    };
    const auto mirror_direction = [&](QVector3D &value)
    {
        value -= 2.0f * QVector3D::dotProduct(value, n) * n;
    };

    mirror_point(injector.pos);
    mirror_point(injector.pos2);
    mirror_point(injector.ff_center);
    mirror_point(injector.ff_virtual_origin);
    mirror_point(injector.volume_bgeom_min);
    mirror_point(injector.volume_bgeom_max);
    if (injector.single_target_scope != Single_Target_Scope::World &&
        injector.single_target_scope != Single_Target_Scope::Reference_Local)
    {
        mirror_point(injector.single_target_hitpoint);
    }
    mirror_direction(injector.vel);
    mirror_direction(injector.vel2);
    mirror_direction(injector.ang_vel);
    mirror_direction(injector.ang_vel2);
    mirror_direction(injector.atomizer_axis);
    mirror_direction(injector.ff_normal);
    mirror_direction(injector.axis);
}
}

QList<Unit> expand_unit_array(const Unit &source, const UnitArraySpec &spec)
{
    QList<Unit> result;
    const int count = qBound(1, spec.count, 100000);
    const QVector3D linear_direction = normalized_or(
        spec.direction, QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D rotation_axis = linear_direction;

    for (int index = 0; index < count; ++index)
    {
        Unit child(source);
        child.inj.uuid = QUuid::createUuid();
        child.inj.injector_data.name = QString("%1[%2]")
                                           .arg(source.inj.injector_data.name)
                                           .arg(index + 1);

        if (spec.type == UnitArrayType::Linear)
        {
            const QVector3D offset = linear_direction * (spec.spacing * index);
            child.inj.injector_data.pos += offset;
            child.inj.injector_data.pos2 += offset;
            child.inj.injector_data.ff_center += offset;
            child.inj.injector_data.ff_virtual_origin += offset;
            child.inj.injector_data.volume_bgeom_min += offset;
            child.inj.injector_data.volume_bgeom_max += offset;
            if (child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::World &&
                child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::Reference_Local)
            {
                child.inj.injector_data.single_target_hitpoint += offset;
            }
        }
        else if (spec.type == UnitArrayType::Rotational)
        {
            const float step = spec.count > 0
                                   ? qDegreesToRadians(spec.angle_degrees) / spec.count
                                   : 0.0f;
            rotate_injector_data(child.inj.injector_data, spec.origin,
                                 rotation_axis, step * index);
            child.inj.injector_data.pos += rotation_axis * (spec.spacing * index);
            child.inj.injector_data.pos2 += rotation_axis * (spec.spacing * index);
        }
        else
        {
            mirror_injector_data(child.inj.injector_data, spec.origin,
                                 spec.plane_normal);
        }

        if (child.inj.injector_data.single_target_scope ==
            Single_Target_Scope::Reference_Local)
        {
            child.inj.injector_data.single_target_hitpoint =
                reference_local_point(child.inj.injector_data.single_target_hitpoint,
                spec);
        }
        if (spec.use_reference_geometry && spec.conform_to_reference_normal)
        {
            conform_injector_to_normal(child.inj.injector_data,
                                       spec.plane_normal);
        }
        child.ais_display->Set(child.inj.shape);
        result.append(std::move(child));
    }
    return result;
}

QList<Unit> expand_unit_fill(const QList<Unit> &sources, const UnitFillSpec &spec)
{
    QList<Unit> result;
    if (sources.isEmpty())
    {
        return result;
    }

    const int rows = qBound(1, spec.rows, 1000);
    const int columns = qBound(1, spec.columns, 1000);
    const QVector3D fill_x = normalized_or(spec.direction,
                                           QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D fill_z = normalized_or(spec.plane_normal,
                                           QVector3D(0.0f, 0.0f, 1.0f));
    QVector3D fill_y = QVector3D::crossProduct(fill_z, fill_x);
    if (fill_y.lengthSquared() <= 1.0e-12f)
    {
        fill_y = QVector3D(0.0f, 1.0f, 0.0f);
    }
    else
    {
        fill_y.normalize();
    }
    int child_index = 0;
    for (int row = 0; row < rows; ++row)
    {
        const bool offset_hex_row = spec.pattern == UnitFillPattern::Hexagonal &&
                                    (row % 2 != 0);
        for (int column = 0; column < columns; ++column)
        {
            const Unit &source = sources.at(child_index % sources.size());
            Unit child(source);
            child.inj.uuid = QUuid::createUuid();
            child.inj.injector_data.name = QString("%1[%2]")
                                               .arg(source.inj.injector_data.name)
                                               .arg(child_index + 1);

            const float local_x = (static_cast<float>(column) +
                                   (offset_hex_row ? 0.5f : 0.0f)) * spec.spacing_x;
            const float local_y = static_cast<float>(row) * spec.spacing_y;
            if (spec.circular_boundary)
            {
                const float dx = local_x;
                const float dy = local_y;
                if (dx * dx + dy * dy >
                    spec.boundary_radius * spec.boundary_radius)
                {
                    continue;
                }
            }
            const QVector3D target = spec.origin + fill_x * local_x +
                                     fill_y * local_y;
            const QVector3D offset = target - source.inj.injector_data.pos;
            child.inj.injector_data.pos += offset;
            child.inj.injector_data.pos2 += offset;
            child.inj.injector_data.ff_center += offset;
            child.inj.injector_data.ff_virtual_origin += offset;
            child.inj.injector_data.volume_bgeom_min += offset;
            child.inj.injector_data.volume_bgeom_max += offset;
            if (child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::World &&
                child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::Reference_Local)
            {
                child.inj.injector_data.single_target_hitpoint += offset;
            }
            if (child.inj.injector_data.single_target_scope ==
                    Single_Target_Scope::Reference_Local &&
                spec.use_reference_geometry)
            {
                child.inj.injector_data.single_target_hitpoint =
                    reference_local_point(child.inj.injector_data.single_target_hitpoint,
                                          spec.origin,
                                          spec.direction,
                                          spec.plane_normal);
            }
            if (spec.use_reference_geometry && spec.conform_to_reference_normal)
            {
                conform_injector_to_normal(child.inj.injector_data, fill_z);
            }
        child.ais_display->Set(child.inj.shape);
            result.append(std::move(child));
            ++child_index;
        }
    }
    return result;
}
