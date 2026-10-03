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
    const auto transform_array = [&](UnitArraySpec &spec)
    {
        spec.origin = rotate_point(spec.origin);
        spec.direction = rotate_direction(spec.direction);
        spec.plane_normal = rotate_direction(spec.plane_normal);
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
    const QVector3D rotation_axis = linear_direction;

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
        else if (spec.type == UnitArrayType::Elliptical)
        {
            const QVector3D ellipse_axis = normalized_or(
                spec.plane_normal, QVector3D(0.0f, 0.0f, 1.0f));
            const QVector3D ellipse_x = normalized_or(
                spec.direction, QVector3D(1.0f, 0.0f, 0.0f));
            QVector3D ellipse_y = QVector3D::crossProduct(ellipse_axis, ellipse_x);
            if (ellipse_y.lengthSquared() <= 1.0e-12f)
            {
                ellipse_y = QVector3D(0.0f, 1.0f, 0.0f);
            }
            else
            {
                ellipse_y.normalize();
            }
            const float angle = spec.count > 0
                                    ? qDegreesToRadians(spec.angle_degrees) /
                                          static_cast<float>(spec.count) * index
                                    : 0.0f;
            rotate_injector_data(child.inj.injector_data, spec.origin,
                                 ellipse_axis, angle);
            const QVector3D target = spec.origin +
                                     ellipse_x * (spec.major_radius * std::cos(angle)) +
                                     ellipse_y * (spec.minor_radius * std::sin(angle));
            const QVector3D offset = target - child.inj.injector_data.pos;
            child.inj.injector_data.pos += offset;
            child.inj.injector_data.pos2 += offset;
            child.inj.injector_data.ff_center += offset;
            child.inj.injector_data.ff_virtual_origin += offset;
            child.inj.injector_data.volume_bgeom_min += offset;
            child.inj.injector_data.volume_bgeom_max += offset;
        }
        else if (spec.type == UnitArrayType::Rotational)
        {
            const float step = spec.count > 0
                                   ? qDegreesToRadians(spec.angle_degrees) / spec.count
                                   : 0.0f;
            rotate_injector_data(child.inj.injector_data, spec.origin,
                                 rotation_axis, step * index);
            const QVector3D axial_offset = rotation_axis * (spec.spacing * index);
            child.inj.injector_data.pos += axial_offset;
            child.inj.injector_data.pos2 += axial_offset;
            child.inj.injector_data.ff_center += axial_offset;
            child.inj.injector_data.ff_virtual_origin += axial_offset;
            child.inj.injector_data.volume_bgeom_min += axial_offset;
            child.inj.injector_data.volume_bgeom_max += axial_offset;
            if (child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::World &&
                child.inj.injector_data.single_target_scope !=
                    Single_Target_Scope::Reference_Local)
            {
                child.inj.injector_data.single_target_hitpoint += axial_offset;
            }
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
        if (source_child == nullptr ||
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
    const float angle_step = spec.type == UnitArrayType::Rotational && count > 0
                                 ? qDegreesToRadians(spec.angle_degrees) /
                                       static_cast<float>(count)
                                 : 0.0f;
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
        if (spec.type == UnitArrayType::Linear)
        {
            transform_unit_tree(*instance, QVector3D(), direction, 0.0f,
                                direction * (spec.spacing * index));
        }
        else if (spec.type == UnitArrayType::Elliptical)
        {
            const float angle = count > 0
                                    ? qDegreesToRadians(spec.angle_degrees) /
                                          static_cast<float>(count) * index
                                    : 0.0f;
            const QVector3D ellipse_y = normalized_or(
                QVector3D::crossProduct(plane_normal, direction),
                QVector3D(0.0f, 1.0f, 0.0f));
            const QVector3D target = spec.origin +
                                     direction * (spec.major_radius * std::cos(angle)) +
                                     ellipse_y * (spec.minor_radius * std::sin(angle));
            const QVector3D rotated_source = spec.origin +
                rotate_vector(source.inj.injector_data.pos - spec.origin,
                              plane_normal, angle);
            transform_unit_tree(*instance, spec.origin, plane_normal, angle,
                                target - rotated_source);
        }
        else
        {
            if (spec.type == UnitArrayType::Rotational)
            {
                transform_unit_tree(*instance, spec.origin, direction,
                                    angle_step * index,
                                    direction * (spec.spacing * index));
            }
            else if (index % 2 != 0)
            {
                mirror_unit_tree(*instance, spec.origin, spec.plane_normal);
            }
        }
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
