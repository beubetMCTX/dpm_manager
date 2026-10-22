#include "unit_array.h"

#include <QQuaternion>
#include <QSet>
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
    QVector3D y_axis = QVector3D::crossProduct(z_axis, x_axis);
    if (y_axis.lengthSquared() <= 1.0e-12f)
    {
        y_axis = QVector3D(0.0f, 1.0f, 0.0f);
    }
    else
    {
        y_axis.normalize();
    }
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

struct EllipticalPlacement
{
    QVector3D center;
    QVector3D axis;
    float angle = 0.0f;
    QVector3D target;
};

// Place copies on the nominal ellipse. The injector position is not an extra
// constraint, so angle 0 is origin + direction * major radius.
EllipticalPlacement elliptical_placement(const QVector3D &source_position,
                                         const UnitArraySpec &spec,
                                         int index,
                                         int count)
{
    Q_UNUSED(source_position);
    const QVector3D axis = normalized_or(spec.plane_normal,
                                         QVector3D(0.0f, 0.0f, 1.0f));
    const QVector3D x_axis = normalized_or(spec.direction,
                                           QVector3D(1.0f, 0.0f, 0.0f));
    QVector3D y_axis = QVector3D::crossProduct(axis, x_axis);
    if (y_axis.lengthSquared() <= 1.0e-12f)
    {
        y_axis = QVector3D(0.0f, 1.0f, 0.0f);
    }
    else
    {
        y_axis.normalize();
    }
    const float angle = count > 0
                            ? qDegreesToRadians(spec.angle_degrees) /
                                  static_cast<float>(count) * static_cast<float>(index)
                            : 0.0f;
    EllipticalPlacement placement;
    placement.center = spec.origin;
    placement.axis = axis;
    placement.angle = angle;
    placement.target = spec.origin +
                       x_axis * (spec.major_radius * std::cos(angle)) +
                       y_axis * (spec.minor_radius * std::sin(angle));
    return placement;
}

struct RotationalPlacement
{
    QVector3D pivot;
    QVector3D axis;
    float angle = 0.0f;
    QVector3D anchor;
};

QVector3D axis_perpendicular(const QVector3D &axis, const QVector3D &hint)
{
    QVector3D radial = QVector3D::crossProduct(axis, hint);
    if (radial.lengthSquared() <= 1.0e-12f)
    {
        radial = QVector3D::crossProduct(axis, QVector3D(0.0f, 0.0f, 1.0f));
    }
    if (radial.lengthSquared() <= 1.0e-12f)
    {
        radial = QVector3D::crossProduct(axis, QVector3D(0.0f, 1.0f, 0.0f));
    }
    return radial.normalized();
}

// The editor origin, axis, radius, and plane normal define the circle. Copies
// are rotated about the source and then translated onto that circle.
RotationalPlacement rotational_placement(const QVector3D &source_position,
                                         const UnitArraySpec &spec,
                                         int index,
                                         int count)
{
    Q_UNUSED(source_position);
    const QVector3D axis = normalized_or(spec.direction,
                                         QVector3D(0.0f, 0.0f, 1.0f));
    float radius = spec.orbit_radius;
    if (qAbs(radius) <= 1.0e-8f)
    {
        radius = 0.01f;
    }
    const float angle = count > 0
                            ? qDegreesToRadians(spec.angle_degrees) /
                                  static_cast<float>(count) * static_cast<float>(index)
                            : 0.0f;
    const QVector3D radial = rotate_vector(
        axis_perpendicular(axis, spec.plane_normal) * radius, axis, angle);
    RotationalPlacement placement;
    placement.pivot = spec.origin;
    placement.axis = axis;
    placement.angle = angle;
    placement.anchor = spec.origin + radial +
                       axis * (spec.spacing * static_cast<float>(index));
    return placement;
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

void translate_injector_spatial(Injector &injector, const QVector3D &offset)
{
    injector.pos += offset;
    injector.pos2 += offset;
    injector.ff_center += offset;
    injector.ff_virtual_origin += offset;
    injector.volume_bgeom_min += offset;
    injector.volume_bgeom_max += offset;
    if (injector.single_target_scope != Single_Target_Scope::World &&
        injector.single_target_scope != Single_Target_Scope::Reference_Local)
    {
        injector.single_target_hitpoint += offset;
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

void transform_unit_pattern_frames(Unit &unit, const QVector3D &pivot,
                                   const QVector3D &axis, float angle,
                                   const QVector3D &translation)
{
    const auto rotate_point = [&](const QVector3D &point)
    {
        return pivot + rotate_vector(point - pivot, axis, angle) + translation;
    };
    const auto rotate_direction = [&](const QVector3D &direction)
    {
        return rotate_vector(direction, axis, angle);
    };
    const QVector3D frame_axis = normalized_or(axis, QVector3D(0.0f, 0.0f, 1.0f));
    const QQuaternion frame_rotation = QQuaternion::fromAxisAndAngle(
        frame_axis, static_cast<float>(qRadiansToDegrees(angle)));
    const auto transform_array = [&](UnitArraySpec &spec)
    {
        spec.origin = rotate_point(spec.origin);
        spec.direction = rotate_direction(spec.direction);
        spec.plane_normal = rotate_direction(spec.plane_normal);
        if (!array_heading_is_identity(frame_rotation) &&
            !array_heading_is_identity(spec.heading))
        {
            spec.heading = (frame_rotation * spec.heading *
                            frame_rotation.conjugated()).normalized();
        }
    };
    const auto transform_fill = [&](UnitFillSpec &spec)
    {
        spec.origin = rotate_point(spec.origin);
        spec.direction = rotate_direction(spec.direction);
        spec.plane_normal = rotate_direction(spec.plane_normal);
    };

    if (!unit.array_specs.isEmpty())
    {
        for (UnitArraySpec &spec : unit.array_specs)
        {
            transform_array(spec);
        }
        unit.array_spec = unit.array_specs.last();
    }
    else if (unit.has_array_spec)
    {
        transform_array(unit.array_spec);
    }
    if (unit.has_fill_spec)
    {
        transform_fill(unit.fill_spec);
    }
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

void mirror_unit_pattern_frames(Unit &unit, const QVector3D &point,
                                const QVector3D &normal)
{
    const QVector3D n = normalized_or(normal, QVector3D(1.0f, 0.0f, 0.0f));
    const auto mirror_point = [&](const QVector3D &value)
    {
        const float distance = QVector3D::dotProduct(value - point, n);
        return value - 2.0f * distance * n;
    };
    const auto mirror_direction = [&](const QVector3D &value)
    {
        return value - 2.0f * QVector3D::dotProduct(value, n) * n;
    };
    const auto mirror_array = [&](UnitArraySpec &spec)
    {
        spec.origin = mirror_point(spec.origin);
        spec.direction = mirror_direction(spec.direction);
        spec.plane_normal = mirror_direction(spec.plane_normal);
    };
    const auto mirror_fill = [&](UnitFillSpec &spec)
    {
        spec.origin = mirror_point(spec.origin);
        spec.direction = mirror_direction(spec.direction);
        spec.plane_normal = mirror_direction(spec.plane_normal);
    };

    if (!unit.array_specs.isEmpty())
    {
        for (UnitArraySpec &spec : unit.array_specs)
        {
            mirror_array(spec);
        }
        unit.array_spec = unit.array_specs.last();
    }
    else if (unit.has_array_spec)
    {
        mirror_array(unit.array_spec);
    }
    if (unit.has_fill_spec)
    {
        mirror_fill(unit.fill_spec);
    }
}

QList<QUuid> stable_ids_for_count(int count,
                                   const QList<QUuid> &preferred,
                                   const QList<QUuid> &fallback)
{
    QList<QUuid> result;
    QSet<QUuid> used;
    result.reserve(count);
    for (int index = 0; index < count; ++index)
    {
        QUuid candidate;
        if (index < fallback.size())
        {
            candidate = fallback.at(index);
        }
        if (candidate.isNull() && index < preferred.size())
        {
            candidate = preferred.at(index);
        }
        if (candidate.isNull() || used.contains(candidate))
        {
            do
            {
                candidate = QUuid::createUuid();
            }
            while (candidate.isNull() || used.contains(candidate));
        }
        used.insert(candidate);
        result.append(candidate);
    }
    return result;
}

void append_array_instance_path(Unit &root, int placement_index,
                                const QUuid &placement_uuid,
                                const QUuid &layer_uuid)
{
    root.array_instance_path.append(placement_index);
    root.array_instance_key.append(placement_uuid);
    root.array_layer_uuid = layer_uuid;
    root.array_overrides.clear();
    for (const std::shared_ptr<Unit> &child : root.child_units)
    {
        if (child != nullptr)
        {
            append_array_instance_path(*child, placement_index,
                                       placement_uuid, layer_uuid);
        }
    }
}
}

void initialize_new_array_spec_identity(UnitArraySpec &spec)
{
    spec.layer_uuid = QUuid::createUuid();
    spec.placement_uuids.clear();
    ensure_array_spec_identity(spec);
}

void reconcile_array_spec_identity(UnitArraySpec &spec,
                                   const UnitArraySpec &previous)
{
    spec.layer_uuid = previous.layer_uuid.isNull()
                          ? QUuid::createUuid()
                          : previous.layer_uuid;
    const int count = qBound(1, spec.count, 100000);
    spec.placement_uuids = stable_ids_for_count(
        count, spec.placement_uuids, previous.placement_uuids);
}

void ensure_array_spec_identity(UnitArraySpec &spec)
{
    if (spec.layer_uuid.isNull())
    {
        spec.layer_uuid = QUuid::createUuid();
    }
    const int count = qBound(1, spec.count, 100000);
    spec.placement_uuids = stable_ids_for_count(
        count, spec.placement_uuids, {});
}

void initialize_new_fill_spec_identity(UnitFillSpec &spec)
{
    spec.fill_uuid = QUuid::createUuid();
    spec.placement_uuids.clear();
    ensure_fill_spec_identity(spec);
}

void reconcile_fill_spec_identity(UnitFillSpec &spec,
                                  const UnitFillSpec &previous)
{
    spec.fill_uuid = previous.fill_uuid.isNull()
                         ? QUuid::createUuid()
                         : previous.fill_uuid;
    const int slot_count = qBound(1, spec.rows, 1000) *
                           qBound(1, spec.columns, 1000);
    spec.placement_uuids = stable_ids_for_count(
        slot_count, spec.placement_uuids, previous.placement_uuids);
}

void ensure_fill_spec_identity(UnitFillSpec &spec)
{
    if (spec.fill_uuid.isNull())
    {
        spec.fill_uuid = QUuid::createUuid();
    }
    const int slot_count = qBound(1, spec.rows, 1000) *
                           qBound(1, spec.columns, 1000);
    spec.placement_uuids = stable_ids_for_count(
        slot_count, spec.placement_uuids, {});
}

QList<Unit> expand_unit_array(const Unit &source, const UnitArraySpec &spec)
{
    QList<Unit> result;
    const int count = qBound(1, spec.count, 100000);
    const QVector3D linear_direction = normalized_or(
        spec.direction, QVector3D(1.0f, 0.0f, 0.0f));

    for (int index = 0; index < count; ++index)
    {
        Unit child(source);
        child.inj.uuid = QUuid::createUuid();
        child.prototype_uuid = source.inj.uuid;
        child.prototype_chain = source.prototype_chain;
        child.prototype_chain.append(source.inj.uuid);
        child.array_instance_path = {index};
        child.array_instance_key = {
            index < spec.placement_uuids.size()
                ? spec.placement_uuids.at(index)
                : QUuid::createUuid()};
        child.array_layer_uuid = spec.layer_uuid;
        child.array_overrides.clear();
        child.inj.injector_data.name = QString("%1[%2]")
                                           .arg(source.inj.injector_data.name)
                                           .arg(index + 1);

        const QVector3D source_position = source.inj.injector_data.pos;
        if (spec.type == UnitArrayType::Linear)
        {
            const QVector3D anchor = spec.origin +
                                     linear_direction * (spec.spacing * index);
            translate_injector_spatial(child.inj.injector_data,
                                       anchor - source_position);
        }
        else if (spec.type == UnitArrayType::Elliptical)
        {
            const EllipticalPlacement placement = elliptical_placement(
                source_position, spec, index, count);
            rotate_injector_data(child.inj.injector_data, source_position,
                                 placement.axis, placement.angle);
            translate_injector_spatial(
                child.inj.injector_data,
                placement.target - child.inj.injector_data.pos);
        }
        else if (spec.type == UnitArrayType::Rotational)
        {
            const RotationalPlacement placement = rotational_placement(
                source_position, spec, index, count);
            rotate_injector_data(child.inj.injector_data, source_position,
                                 placement.axis, placement.angle);
            translate_injector_spatial(
                child.inj.injector_data,
                placement.anchor - child.inj.injector_data.pos);
        }
        else if (index % 2 != 0)
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
        apply_array_heading(child, spec.heading);
        if (!child.inj.create_injector())
        {
            continue;
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
    QVector<int> weights;
    for (int i = 0; i < sources.size(); ++i)
    {
        const int weight = i < spec.source_weights.size()
                               ? spec.source_weights.at(i)
                               : 1;
        weights.append(qMax(1, weight));
    }
    int total_weight = 0;
    for (const int weight : weights)
    {
        total_weight += weight;
    }
    int placement_index = 0;
    for (int row = 0; row < rows; ++row)
    {
        const bool offset_hex_row = spec.pattern == UnitFillPattern::Hexagonal &&
                                    (row % 2 != 0);
        for (int column = 0; column < columns; ++column)
        {
            int weighted_slot = placement_index % total_weight;
            int source_index = 0;
            while (source_index + 1 < weights.size() &&
                   weighted_slot >= weights.at(source_index))
            {
                weighted_slot -= weights.at(source_index);
                ++source_index;
            }
            const Unit &source = sources.at(source_index);
            Unit child(source);
            child.inj.uuid = QUuid::createUuid();
            child.prototype_uuid = source.inj.uuid;
            child.prototype_chain = source.prototype_chain;
            child.prototype_chain.append(source.inj.uuid);
            child.array_instance_path = {placement_index};
            const int slot_index = row * columns + column;
            child.array_instance_key = {
                slot_index < spec.placement_uuids.size()
                    ? spec.placement_uuids.at(slot_index)
                    : QUuid::createUuid()};
            child.array_layer_uuid = spec.fill_uuid;
            child.array_overrides.clear();
            child.inj.injector_data.name = QString("%1[%2]")
                                               .arg(source.inj.injector_data.name)
                                               .arg(placement_index + 1);

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
            if (!child.inj.create_injector())
            {
                continue;
            }
            child.ais_display->Set(child.inj.shape);
            result.append(std::move(child));
            ++placement_index;
        }
    }
    return result;
}

bool has_persistent_children(const Unit &unit)
{
    for (const std::shared_ptr<Unit> &child : unit.child_units)
    {
        if (child != nullptr &&
            !(child->is_array_child && child->follows_array))
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Unit> clone_unit_tree_impl(const Unit &source,
                                           QHash<QUuid, QUuid> &uuid_map,
                                           bool mark_as_derived,
                                           bool include_generated_children)
{
    const std::shared_ptr<Unit> clone = std::make_shared<Unit>(source);
    const QUuid source_uuid = source.inj.uuid;
    const QUuid clone_uuid = QUuid::createUuid();
    uuid_map.insert(source_uuid, clone_uuid);

    clone->inj.uuid = clone_uuid;
    clone->inj.injector_data.name += QStringLiteral(" [Clone]");
    clone->assembly_parent_uuid = QUuid();
    clone->assembly_child_uuids.clear();
    clone->child_units.clear();
    clone->is_array_child = false;
    clone->follows_array = true;
    clone->array_parent_uuid = QUuid();
    clone->prototype_uuid = QUuid();
    clone->prototype_chain.clear();
    if (mark_as_derived)
    {
        clone->is_array_child = true;
        clone->follows_array = true;
        clone->prototype_uuid = source_uuid;
        clone->prototype_chain = source.prototype_chain;
        clone->prototype_chain.append(source_uuid);
    }

    for (const std::shared_ptr<Unit> &source_child : source.child_units)
    {
        const bool detached_array_instance =
            source_child != nullptr && source_child->is_array_child &&
            !source_child->follows_array && !source_child->has_array_spec &&
            !source_child->has_fill_spec;
        if (source_child == nullptr ||
            (detached_array_instance && !include_generated_children) ||
            (source_child->is_array_child && source_child->follows_array &&
             !include_generated_children))
        {
            continue;
        }
        const bool child_is_persistent_array_source =
            source_child->is_array_child && !source_child->follows_array &&
            (source_child->has_array_spec || source_child->has_fill_spec);
        const std::shared_ptr<Unit> child = clone_unit_tree_impl(
            *source_child, uuid_map, mark_as_derived,
            include_generated_children || child_is_persistent_array_source);
        child->assembly_parent_uuid = clone_uuid;
        clone->assembly_child_uuids.append(child->inj.uuid);
        clone->child_units.append(child);
    }
    return clone;
}

std::shared_ptr<Unit> clone_unit_tree(const Unit &source,
                                      QHash<QUuid, QUuid> &uuid_map,
                                      bool mark_as_derived)
{
    return clone_unit_tree_impl(source, uuid_map, mark_as_derived, false);
}

void transform_unit_tree(Unit &root, const QVector3D &pivot,
                         const QVector3D &axis, float angle_radians,
                         const QVector3D &translation)
{
    const QVector3D usable_axis = normalized_or(
        axis, QVector3D(0.0f, 0.0f, 1.0f));
    rotate_injector_data(root.inj.injector_data, pivot, usable_axis,
                         angle_radians);
    transform_unit_pattern_frames(root, pivot, usable_axis, angle_radians,
                                  translation);
    root.inj.injector_data.pos += translation;
    root.inj.injector_data.pos2 += translation;
    root.inj.injector_data.ff_center += translation;
    root.inj.injector_data.ff_virtual_origin += translation;
    root.inj.injector_data.volume_bgeom_min += translation;
    root.inj.injector_data.volume_bgeom_max += translation;
    if (root.inj.injector_data.single_target_scope != Single_Target_Scope::World &&
        root.inj.injector_data.single_target_scope != Single_Target_Scope::Reference_Local)
    {
        root.inj.injector_data.single_target_hitpoint += translation;
    }

    for (const std::shared_ptr<Unit> &child : root.child_units)
    {
        if (child != nullptr)
        {
            transform_unit_tree(*child, pivot, usable_axis, angle_radians,
                                translation);
        }
    }
}

void mirror_unit_tree(Unit &root, const QVector3D &point,
                      const QVector3D &normal)
{
    mirror_injector_data(root.inj.injector_data, point, normal);
    mirror_unit_pattern_frames(root, point, normal);
    for (const std::shared_ptr<Unit> &child : root.child_units)
    {
        if (child != nullptr)
        {
            mirror_unit_tree(*child, point, normal);
        }
    }
}

void sync_single_pitch_yaw_heading(Unit &node, const QQuaternion &heading)
{
    Injector &injector = node.inj.injector_data;
    if (injector.injection_type == single &&
        injector.single_direction_mode == Single_Direction_Mode::Pitch_Yaw)
    {
        const double pitch = qDegreesToRadians(injector.single_pitch_degrees);
        const double yaw = qDegreesToRadians(injector.single_yaw_degrees);
        QVector3D direction(
            static_cast<float>(std::cos(pitch) * std::cos(yaw)),
            static_cast<float>(std::cos(pitch) * std::sin(yaw)),
            static_cast<float>(std::sin(pitch)));
        direction = heading.rotatedVector(direction);
        if (direction.lengthSquared() > 1.0e-12f)
        {
            direction.normalize();
            injector.single_pitch_degrees = static_cast<float>(qRadiansToDegrees(
                std::asin(qBound(-1.0f, direction.z(), 1.0f))));
            injector.single_yaw_degrees = static_cast<float>(qRadiansToDegrees(
                std::atan2(direction.y(), direction.x())));
        }
    }
    for (const std::shared_ptr<Unit> &child : node.child_units)
    {
        if (child != nullptr)
        {
            sync_single_pitch_yaw_heading(*child, heading);
        }
    }
}

void rotate_heading_target_hitpoint(Unit &node, const QVector3D &pivot,
                                    const QVector3D &axis, float angle_radians)
{
    Injector &injector = node.inj.injector_data;
    if (injector.injection_type == single &&
        injector.single_direction_mode == Single_Direction_Mode::Target_Hitpoint &&
        (injector.single_target_scope == Single_Target_Scope::World ||
         injector.single_target_scope == Single_Target_Scope::Reference_Local))
    {
        // Placement keeps these hitpoints fixed. Heading is an aim change,
        // so the point has to orbit the injector or the spray never yaws.
        injector.single_target_hitpoint =
            pivot + rotate_vector(injector.single_target_hitpoint - pivot,
                                  axis, angle_radians);
    }
    for (const std::shared_ptr<Unit> &child : node.child_units)
    {
        if (child != nullptr)
        {
            rotate_heading_target_hitpoint(*child, pivot, axis, angle_radians);
        }
    }
}

void apply_array_heading_about_pivot(Unit &root, const QVector3D &pivot,
                                     const QQuaternion &heading)
{
    if (array_heading_is_identity(heading))
    {
        return;
    }
    const QQuaternion rotation = heading.normalized();
    sync_single_pitch_yaw_heading(root, rotation);
    QVector3D axis;
    float angle_degrees = 0.0f;
    rotation.getAxisAndAngle(&axis, &angle_degrees);
    if (axis.lengthSquared() <= 1.0e-12f || std::abs(angle_degrees) <= 1.0e-4f)
    {
        return;
    }
    const float angle_radians = qDegreesToRadians(angle_degrees);
    transform_unit_tree(root, pivot, axis, angle_radians);
    rotate_heading_target_hitpoint(root, pivot, axis, angle_radians);
}

void apply_array_heading(Unit &root, const QQuaternion &heading)
{
    apply_array_heading_about_pivot(root, root.inj.injector_data.pos, heading);
}

void conform_unit_tree_to_normal(Unit &root, const QVector3D &normal)
{
    const QVector3D source = primary_direction(root.inj.injector_data);
    const QVector3D target = normalized_or(
        normal, QVector3D(0.0f, 0.0f, 1.0f));
    if (source.lengthSquared() > 1.0e-12f &&
        target.lengthSquared() > 1.0e-12f)
    {
        const QQuaternion rotation =
            QQuaternion::rotationTo(source.normalized(), target);
        const auto rotate_direction = [&](QVector3D &direction)
        {
            if (direction.lengthSquared() > 1.0e-12f)
            {
                const float magnitude = direction.length();
                direction = (rotation * direction.normalized()) * magnitude;
            }
        };
        rotate_direction(root.inj.injector_data.vel);
        rotate_direction(root.inj.injector_data.vel2);
        rotate_direction(root.inj.injector_data.ang_vel);
        rotate_direction(root.inj.injector_data.ang_vel2);
        rotate_direction(root.inj.injector_data.atomizer_axis);
        rotate_direction(root.inj.injector_data.axis);
        rotate_direction(root.inj.injector_data.ff_normal);
    }
    for (const std::shared_ptr<Unit> &child : root.child_units)
    {
        if (child != nullptr)
        {
            conform_unit_tree_to_normal(*child, normal);
        }
    }
}

bool rebuild_unit_tree_geometry(Unit &root)
{
    if (!root.inj.create_injector())
    {
        return false;
    }

    for (const std::shared_ptr<Unit> &child : root.child_units)
    {
        if (child != nullptr && !rebuild_unit_tree_geometry(*child))
        {
            return false;
        }
    }
    return true;
}

QList<std::shared_ptr<Unit>> expand_unit_tree_array(const Unit &source,
                                                    const UnitArraySpec &spec)
{
    QList<std::shared_ptr<Unit>> result;
    if (spec.type != UnitArrayType::Linear &&
        spec.type != UnitArrayType::Rotational &&
        spec.type != UnitArrayType::Mirror &&
        spec.type != UnitArrayType::Elliptical)
    {
        return result;
    }

    const int count = qBound(1, spec.count, 100000);
    const QVector3D direction = normalized_or(
        spec.direction, QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D plane_normal = normalized_or(
        spec.plane_normal, QVector3D(0.0f, 0.0f, 1.0f));
    for (int index = 0; index < count; ++index)
    {
        QHash<QUuid, QUuid> uuid_map;
        const std::shared_ptr<Unit> instance =
            clone_unit_tree(source, uuid_map, true);
        if (instance == nullptr)
        {
            continue;
        }
        const QUuid placement_uuid = index < spec.placement_uuids.size()
                                         ? spec.placement_uuids.at(index)
                                         : QUuid::createUuid();
        append_array_instance_path(*instance, index, placement_uuid,
                                    spec.layer_uuid);
        instance->inj.injector_data.name =
            QStringLiteral("%1[%2]").arg(source.inj.injector_data.name)
                                     .arg(index + 1);
        const QVector3D source_position = source.inj.injector_data.pos;
        if (spec.type == UnitArrayType::Linear)
        {
            const QVector3D anchor = spec.origin +
                                     direction * (spec.spacing * index);
            transform_unit_tree(*instance, QVector3D(), direction, 0.0f,
                                anchor - source_position);
        }
        else if (spec.type == UnitArrayType::Elliptical)
        {
            const EllipticalPlacement placement = elliptical_placement(
                source_position, spec, index, count);
            transform_unit_tree(*instance, source_position, placement.axis,
                                placement.angle,
                                placement.target - source_position);
        }
        else
        {
            if (spec.type == UnitArrayType::Rotational)
            {
                const RotationalPlacement placement = rotational_placement(
                    source_position, spec, index, count);
                transform_unit_tree(*instance, source_position, placement.axis,
                                    placement.angle,
                                    placement.anchor - source_position);
            }
            else if (index % 2 != 0)
            {
                mirror_unit_tree(*instance, spec.origin, spec.plane_normal);
            }
        }
        apply_array_heading(*instance, spec.heading);
        if (!rebuild_unit_tree_geometry(*instance))
        {
            continue;
        }
        result.append(instance);
    }
    return result;
}

QList<std::shared_ptr<Unit>> expand_unit_tree_fill(
    const QList<std::shared_ptr<Unit>> &sources, const UnitFillSpec &spec)
{
    QList<std::shared_ptr<Unit>> result;
    if (sources.isEmpty())
    {
        return result;
    }

    QList<Unit> source_values;
    for (const std::shared_ptr<Unit> &source : sources)
    {
        if (source != nullptr)
        {
            source_values.append(*source);
        }
    }
    const QList<Unit> placements = expand_unit_fill(source_values, spec);
    for (const Unit &placement : placements)
    {
        int source_index = -1;
        int matched_name_length = -1;
        for (int index = 0; index < source_values.size(); ++index)
        {
            const QString source_name =
                source_values.at(index).inj.injector_data.name;
            if (placement.inj.injector_data.name.startsWith(source_name) &&
                source_name.size() > matched_name_length)
            {
                source_index = index;
                matched_name_length = source_name.size();
            }
        }
        if (source_index < 0 && sources.size() == 1)
        {
            source_index = 0;
        }
        if (source_index < 0 || source_index >= sources.size())
        {
            continue;
        }

        QHash<QUuid, QUuid> uuid_map;
        const std::shared_ptr<Unit> instance =
            clone_unit_tree(*sources.at(source_index), uuid_map, true);
        if (instance == nullptr)
        {
            continue;
        }
        const int placement_index = placement.array_instance_path.isEmpty()
            ? result.size()
            : placement.array_instance_path.last();
        const QUuid placement_uuid = placement.array_instance_key.isEmpty()
                                         ? QUuid::createUuid()
                                         : placement.array_instance_key.last();
        append_array_instance_path(*instance, placement_index, placement_uuid,
                                    spec.fill_uuid);
        instance->inj.injector_data.name = placement.inj.injector_data.name;
        const QVector3D offset = placement.inj.injector_data.pos -
                                  sources.at(source_index)->inj.injector_data.pos;
        transform_unit_tree(*instance, QVector3D(),
                            QVector3D(0.0f, 0.0f, 1.0f), 0.0f, offset);
        if (spec.use_reference_geometry && spec.conform_to_reference_normal)
        {
            conform_unit_tree_to_normal(*instance, spec.plane_normal);
        }
        if (!rebuild_unit_tree_geometry(*instance))
        {
            continue;
        }
        result.append(instance);
    }
    return result;
}

static QVector3D injector_spray_direction(const Injector &injector)
{
    QVector3D direction;
    switch (injector.injection_type)
    {
    case single:
        if (injector.single_direction_mode == Single_Direction_Mode::Pitch_Yaw)
        {
            const double pitch = qDegreesToRadians(injector.single_pitch_degrees);
            const double yaw = qDegreesToRadians(injector.single_yaw_degrees);
            direction = QVector3D(
                static_cast<float>(std::cos(pitch) * std::cos(yaw)),
                static_cast<float>(std::cos(pitch) * std::sin(yaw)),
                static_cast<float>(std::sin(pitch)));
        }
        else if (injector.single_direction_mode == Single_Direction_Mode::Target_Hitpoint)
        {
            direction = injector.single_target_hitpoint - injector.pos;
        }
        else
        {
            direction = injector.vel;
        }
        break;
    case cone:
        direction = injector.axis;
        break;
    case flat_fan_atomizer:
        direction = injector.ff_normal;
        break;
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case effervescent_atomizer:
        direction = injector.atomizer_axis;
        break;
    default:
        direction = injector.vel;
        break;
    }
    return direction.lengthSquared() > 1.0e-12f
               ? direction.normalized()
               : QVector3D(0.0f, 0.0f, 1.0f);
}

void array_frame_from_injector(const Injector &injector,
                               QVector3D *origin,
                               QVector3D *direction,
                               QVector3D *plane_normal)
{
    const QVector3D spray = injector_spray_direction(injector);
    QVector3D offset = QVector3D::crossProduct(QVector3D(0.0f, 0.0f, 1.0f), spray);
    if (offset.lengthSquared() <= 1.0e-12f)
    {
        offset = QVector3D::crossProduct(QVector3D(0.0f, 1.0f, 0.0f), spray);
    }
    offset.normalize();

    if (origin != nullptr)
    {
        *origin = injector.injection_type == flat_fan_atomizer
                      ? injector.ff_center
                      : injector.pos;
    }
    if (direction != nullptr)
    {
        *direction = offset;
    }
    if (plane_normal != nullptr)
    {
        *plane_normal = spray;
    }
}

ArrayPatternGuide array_pattern_guide(const QVector3D &source_position,
                                      const UnitArraySpec &spec)
{
    ArrayPatternGuide guide;
    const int count = qBound(1, spec.count, 100000);
    if (spec.type == UnitArrayType::Linear)
    {
        const QVector3D direction = normalized_or(spec.direction,
                                                  QVector3D(1.0f, 0.0f, 0.0f));
        const QVector3D delta = direction * (spec.spacing * static_cast<float>(
                                                  qMax(0, count - 1)));
        if (delta.lengthSquared() <= 1.0e-12f)
        {
            return guide;
        }
        guide.kind = ArrayGuideKind::Segment;
        guide.origin = spec.origin;
        guide.normal = delta.normalized();
        guide.length = delta.length();
        return guide;
    }
    if (spec.type == UnitArrayType::Rotational)
    {
        const RotationalPlacement placement = rotational_placement(
            source_position, spec, 0, count);
        const QVector3D radial = placement.anchor - spec.origin;
        if (radial.lengthSquared() <= 1.0e-12f)
        {
            return guide;
        }
        const float radius = radial.length();
        guide.kind = ArrayGuideKind::Loop;
        guide.origin = spec.origin;
        guide.normal = placement.axis;
        guide.major = radial;
        guide.minor = QVector3D::crossProduct(placement.axis, radial.normalized())
                          .normalized() * radius;
        guide.length = radius;
        return guide;
    }
    if (spec.type == UnitArrayType::Elliptical)
    {
        const EllipticalPlacement placement = elliptical_placement(
            source_position, spec, 0, count);
        const QVector3D x_axis = normalized_or(spec.direction,
                                               QVector3D(1.0f, 0.0f, 0.0f));
        QVector3D y_axis = QVector3D::crossProduct(placement.axis, x_axis);
        if (y_axis.lengthSquared() <= 1.0e-12f)
        {
            y_axis = QVector3D(0.0f, 1.0f, 0.0f);
        }
        else
        {
            y_axis.normalize();
        }
        if (qAbs(spec.major_radius) <= 1.0e-8f)
        {
            return guide;
        }
        guide.kind = ArrayGuideKind::Loop;
        guide.origin = placement.center;
        guide.normal = placement.axis;
        guide.major = x_axis * spec.major_radius;
        guide.minor = y_axis * spec.minor_radius;
        guide.length = qAbs(spec.major_radius);
        return guide;
    }
    if (spec.type == UnitArrayType::Mirror)
    {
        const float reach = qMax(0.02f, (source_position - spec.origin).length());
        guide.kind = ArrayGuideKind::Plane;
        guide.origin = spec.origin;
        guide.normal = normalized_or(spec.plane_normal, QVector3D(0.0f, 0.0f, 1.0f));
        guide.half_extent = reach;
        guide.length = reach;
    }
    return guide;
}
