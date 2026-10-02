#include "project_session.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <QCryptographicHash>

#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>


namespace
{
constexpr int kSessionSchemaVersion = 3;
constexpr int kFirstSupportedSchemaVersion = 1;

QJsonArray vector_to_json(const QVector3D &value)
{
    QJsonArray result;
    result.append(value.x());
    result.append(value.y());
    result.append(value.z());
    return result;
}

bool vector_from_json(const QJsonValue &json_value, QVector3D *value)
{
    const QJsonArray array = json_value.toArray();
    if (array.size() != 3 || value == nullptr)
    {
        return false;
    }

    for (const QJsonValue &component : array)
    {
        if (!component.isDouble() || !std::isfinite(component.toDouble()))
        {
            return false;
        }
    }

    *value = QVector3D(static_cast<float>(array.at(0).toDouble()),
                       static_cast<float>(array.at(1).toDouble()),
                       static_cast<float>(array.at(2).toDouble()));
    return true;
}

QJsonArray int_list_to_json(const QVector<int> &values)
{
    QJsonArray result;
    for (const int value : values)
    {
        result.append(value);
    }
    return result;
}

QVector<int> int_list_from_json(const QJsonValue &json_value,
                                const QVector<int> &fallback)
{
    const QJsonArray array = json_value.toArray();
    if (array.isEmpty() && !json_value.isArray())
    {
        return fallback;
    }

    QVector<int> result;
    for (const QJsonValue &value : array)
    {
        if (value.isDouble())
        {
            result.append(value.toInt());
        }
    }
    return result;
}

QJsonArray uuid_list_to_json(const QVector<QUuid> &values)
{
    QJsonArray result;
    for (const QUuid &value : values)
    {
        if (!value.isNull())
        {
            result.append(value.toString(QUuid::WithoutBraces));
        }
    }
    return result;
}

bool uuid_list_from_json(const QJsonValue &json_value,
                         QVector<QUuid> *values)
{
    if (values == nullptr)
    {
        return false;
    }
    if (json_value.isUndefined())
    {
        return true;
    }
    if (!json_value.isArray())
    {
        return false;
    }

    values->clear();
    for (const QJsonValue &value : json_value.toArray())
    {
        if (!value.isString())
        {
            return false;
        }
        const QUuid uuid(value.toString());
        if (uuid.isNull())
        {
            return false;
        }
        values->append(uuid);
    }
    return true;
}

QJsonObject array_spec_to_json(const UnitArraySpec &spec);
bool array_spec_from_json(const QJsonValue &json_value, UnitArraySpec *spec);
bool unit_from_json(const QJsonValue &json_value, Unit *unit);
void set_error(QString *error_message, const QString &message);

void injector_to_json(const Injector &value, QJsonObject *object)
{
    if (object == nullptr)
    {
        return;
    }

#define SAVE_STRING(field) object->insert(#field, value.field)
#define SAVE_INT(field) object->insert(#field, value.field)
#define SAVE_DOUBLE(field) object->insert(#field, value.field)
#define SAVE_BOOL(field) object->insert(#field, value.field)
#define SAVE_ENUM(field) object->insert(#field, static_cast<int>(value.field))
#define SAVE_VECTOR(field) object->insert(#field, vector_to_json(value.field))
#define SAVE_INT_LIST(field) object->insert(#field, int_list_to_json(value.field))

    SAVE_STRING(name);
    SAVE_ENUM(type);
    SAVE_ENUM(injection_type);
    SAVE_STRING(local_reference_frame);
    SAVE_ENUM(single_direction_mode);
    SAVE_DOUBLE(single_pitch_degrees);
    SAVE_DOUBLE(single_yaw_degrees);
    SAVE_VECTOR(single_target_hitpoint);
    SAVE_ENUM(single_target_scope);
    SAVE_INT(numpts);
    SAVE_STRING(dpm_fname);
    SAVE_INT_LIST(surfaces);
    SAVE_INT_LIST(boundary);
    SAVE_BOOL(stochastic);
    SAVE_BOOL(random_eddy);
    SAVE_INT(ntries);
    SAVE_DOUBLE(time_scale_constant);
    SAVE_BOOL(cloud);
    SAVE_DOUBLE(cloud_min_dia);
    SAVE_DOUBLE(cloud_max_dia);
    SAVE_STRING(material);
    SAVE_BOOL(scale_by_area);
    SAVE_BOOL(use_face_normal);
    SAVE_BOOL(random_surface);
    SAVE_BOOL(tabulated_diam_dist);
    SAVE_STRING(tabulated_diam_table_name);
    SAVE_INT(tabulated_diam_ref_diam_col);
    SAVE_INT(tabulated_diam_num_frac_col);
    SAVE_INT(tabulated_diam_mas_frac_col);
    SAVE_BOOL(tabulated_diam_num_frac_accum);
    SAVE_BOOL(tabulated_diam_mas_frac_accum);
    SAVE_STRING(devolatilizing_species);
    SAVE_STRING(evaporating_species);
    SAVE_STRING(oxidizing_species);
    SAVE_STRING(product_species);
    SAVE_BOOL(rr_disturb);
    SAVE_BOOL(rr_uniform_ln_d);
    SAVE_BOOL(evaporating_liquid);
    SAVE_STRING(evaporating_material);
    SAVE_DOUBLE(liquid_fraction);
    SAVE_STRING(dpm_domain);
    SAVE_STRING(collision_partner);
    SAVE_INT(parcel_number);
    SAVE_DOUBLE(parcel_mass);
    SAVE_DOUBLE(parcel_diameter);
    SAVE_ENUM(parcel_model);
    SAVE_ENUM(drag_law);
    SAVE_DOUBLE(shape_factor);
    SAVE_DOUBLE(cunningham_correction);
    SAVE_STRING(drag_fcn);
    SAVE_BOOL(brownian_motion);
    SAVE_BOOL(seco_breakup_on);
    SAVE_BOOL(seco_breakup_tab);
    SAVE_BOOL(seco_breakup_wave);
    SAVE_BOOL(seco_break_up_khrt);
    SAVE_BOOL(seco_breakup_ssd);
    SAVE_BOOL(seco_breakup_madahushi);
    SAVE_BOOL(seco_breakup_schmehl);
    SAVE_DOUBLE(seco_breakup_tab_y0);
    SAVE_INT(number_tab_diameters);
    SAVE_DOUBLE(seco_breakup_wave_b1);
    SAVE_DOUBLE(seco_breakup_wave_b0);
    SAVE_DOUBLE(seco_breakup_khrt_cl);
    SAVE_DOUBLE(seco_breakup_khrt_ctau);
    SAVE_DOUBLE(seco_breakup_khrt_crt);
    SAVE_DOUBLE(seco_breakup_ssd_we_cr);
    SAVE_DOUBLE(seco_breakup_ssd_core_bu);
    SAVE_DOUBLE(seco_breakup_ssd_np_target);
    SAVE_DOUBLE(seco_breakup_ssd_x_si);
    SAVE_DOUBLE(seco_breakup_madabushi_c0);
    SAVE_DOUBLE(seco_breakup_madabushi_column_drag_cd);
    SAVE_DOUBLE(seco_breakup_madabushi_ligament_factor);
    SAVE_DOUBLE(seco_breakup_madabushi_jet_diameter);
    SAVE_DOUBLE(seco_breakup_schmehl_np);
    SAVE_ENUM(volume_specification);
    SAVE_INT_LIST(volume_zones);
    SAVE_ENUM(volume_streams_spec);
    SAVE_INT(volume_streams_total);
    SAVE_INT(volume_streams_per_cell);
    SAVE_DOUBLE(volume_packing_limit_per_cell);
    SAVE_ENUM(volume_bgeom_shapes);
    SAVE_VECTOR(volume_bgeom_min);
    SAVE_VECTOR(volume_bgeom_max);
    SAVE_DOUBLE(volume_bgeom_radius);
    SAVE_DOUBLE(volume_bgeom_viconeangle);
    SAVE_BOOL(mass_input_on);
    SAVE_BOOL(volfrac_input_on);
    SAVE_BOOL(rotation_on);
    SAVE_ENUM(rot_drag_law);
    SAVE_ENUM(rot_lift_law);
    SAVE_ENUM(cone_type);
    SAVE_BOOL(uniform_mass_dist_on);
    SAVE_BOOL(spatial_staggering_std_inj_on);
    SAVE_BOOL(spatial_staggering_atomizer_on);
    SAVE_DOUBLE(stagger_radius);
    SAVE_BOOL(rough_wall_on);
    SAVE_STRING(cphace_domain);
    SAVE_VECTOR(pos);
    SAVE_VECTOR(pos2);
    SAVE_VECTOR(ff_center);
    SAVE_VECTOR(ff_virtual_origin);
    SAVE_VECTOR(ff_normal);
    SAVE_VECTOR(vel);
    SAVE_VECTOR(vel2);
    SAVE_VECTOR(ang_vel);
    SAVE_VECTOR(ang_vel2);
    SAVE_VECTOR(atomizer_axis);
    SAVE_DOUBLE(diameter);
    SAVE_DOUBLE(diameter2);
    SAVE_DOUBLE(temperature);
    SAVE_DOUBLE(temperature2);
    SAVE_DOUBLE(flow_rate);
    SAVE_DOUBLE(flow_rate2);
    SAVE_DOUBLE(unsteady_start);
    SAVE_DOUBLE(unsteady_stop);
    SAVE_DOUBLE(start_at_flow_time_in_unsteady_inj_file);
    SAVE_DOUBLE(interval_to_repeat_in_unsteady_inj_file);
    SAVE_DOUBLE(unsteady_ca_start);
    SAVE_DOUBLE(unsteady_ca_stop);
    SAVE_DOUBLE(vapor_pressure);
    SAVE_DOUBLE(inner_diameter);
    SAVE_DOUBLE(outer_diameter);
    SAVE_DOUBLE(half_angle);
    SAVE_DOUBLE(plain_length);
    SAVE_DOUBLE(plain_corner_size);
    SAVE_DOUBLE(plain_const_a);
    SAVE_DOUBLE(pswirl_inj_press);
    SAVE_DOUBLE(airbl_rel_vel);
    SAVE_DOUBLE(effer_quality);
    SAVE_DOUBLE(effer_t_sat);
    SAVE_DOUBLE(ff_oriface_width);
    SAVE_DOUBLE(phi_start);
    SAVE_DOUBLE(phi_stop);
    SAVE_DOUBLE(sheet_const);
    SAVE_DOUBLE(lig_const);
    SAVE_DOUBLE(effer_const);
    SAVE_DOUBLE(effer_half_angle_max);
    SAVE_DOUBLE(ff_sheet_const);
    SAVE_DOUBLE(atomizer_disp_angle);
    SAVE_VECTOR(axis);
    SAVE_DOUBLE(vel_mag);
    SAVE_DOUBLE(ang_vel_mag);
    SAVE_DOUBLE(cone_angle);
    SAVE_DOUBLE(inner_radius);
    SAVE_DOUBLE(radius);
    SAVE_DOUBLE(swirl_frac);
    SAVE_DOUBLE(total_flow_rate);
    SAVE_DOUBLE(total_mass);
    SAVE_DOUBLE(volume_fraction);
    SAVE_DOUBLE(rr_min);
    SAVE_DOUBLE(rr_max);
    SAVE_DOUBLE(rr_mean);
    SAVE_DOUBLE(rr_spread);
    SAVE_INT(rr_numdia);
    SAVE_VECTOR(posr);
    SAVE_VECTOR(posu);

#undef SAVE_STRING
#undef SAVE_INT
#undef SAVE_DOUBLE
#undef SAVE_BOOL
#undef SAVE_ENUM
#undef SAVE_VECTOR
#undef SAVE_INT_LIST
}

bool injector_from_json(const QJsonObject &object, Injector *value)
{
    if (value == nullptr)
    {
        return false;
    }

    bool vectors_valid = true;

#define READ_STRING(field) if (object.contains(#field)) value->field = object.value(#field).toString(value->field)
#define READ_INT(field) if (object.contains(#field)) value->field = object.value(#field).toInt(value->field)
#define READ_DOUBLE(field) if (object.contains(#field)) value->field = object.value(#field).toDouble(value->field)
#define READ_BOOL(field) if (object.contains(#field)) value->field = object.value(#field).toBool(value->field)
#define READ_ENUM(field, type_name) if (object.contains(#field)) value->field = static_cast<type_name>(object.value(#field).toInt(static_cast<int>(value->field)))
#define READ_VECTOR(field) \
    if (object.contains(#field) && \
        !vector_from_json(object.value(#field), &value->field)) \
    { \
        vectors_valid = false; \
    }
#define READ_INT_LIST(field) if (object.contains(#field)) value->field = int_list_from_json(object.value(#field), value->field)

    READ_STRING(name);
    READ_ENUM(type, DPM_Type);
    READ_ENUM(injection_type, Injection_Type);
    READ_STRING(local_reference_frame);
    READ_ENUM(single_direction_mode, Single_Direction_Mode);
    READ_DOUBLE(single_pitch_degrees);
    READ_DOUBLE(single_yaw_degrees);
    READ_VECTOR(single_target_hitpoint);
    READ_ENUM(single_target_scope, Single_Target_Scope);
    READ_INT(numpts);
    READ_STRING(dpm_fname);
    READ_INT_LIST(surfaces);
    READ_INT_LIST(boundary);
    READ_BOOL(stochastic);
    READ_BOOL(random_eddy);
    READ_INT(ntries);
    READ_DOUBLE(time_scale_constant);
    READ_BOOL(cloud);
    READ_DOUBLE(cloud_min_dia);
    READ_DOUBLE(cloud_max_dia);
    READ_STRING(material);
    READ_BOOL(scale_by_area);
    READ_BOOL(use_face_normal);
    READ_BOOL(random_surface);
    READ_BOOL(tabulated_diam_dist);
    READ_STRING(tabulated_diam_table_name);
    READ_INT(tabulated_diam_ref_diam_col);
    READ_INT(tabulated_diam_num_frac_col);
    READ_INT(tabulated_diam_mas_frac_col);
    READ_BOOL(tabulated_diam_num_frac_accum);
    READ_BOOL(tabulated_diam_mas_frac_accum);
    READ_STRING(devolatilizing_species);
    READ_STRING(evaporating_species);
    READ_STRING(oxidizing_species);
    READ_STRING(product_species);
    READ_BOOL(rr_disturb);
    READ_BOOL(rr_uniform_ln_d);
    READ_BOOL(evaporating_liquid);
    READ_STRING(evaporating_material);
    READ_DOUBLE(liquid_fraction);
    READ_STRING(dpm_domain);
    READ_STRING(collision_partner);
    READ_INT(parcel_number);
    READ_DOUBLE(parcel_mass);
    READ_DOUBLE(parcel_diameter);
    READ_ENUM(parcel_model, Parcel_Model);
    READ_ENUM(drag_law, Drag_Law);
    READ_DOUBLE(shape_factor);
    READ_DOUBLE(cunningham_correction);
    READ_STRING(drag_fcn);
    READ_BOOL(brownian_motion);
    READ_BOOL(seco_breakup_on);
    READ_BOOL(seco_breakup_tab);
    READ_BOOL(seco_breakup_wave);
    READ_BOOL(seco_break_up_khrt);
    READ_BOOL(seco_breakup_ssd);
    READ_BOOL(seco_breakup_madahushi);
    READ_BOOL(seco_breakup_schmehl);
    READ_DOUBLE(seco_breakup_tab_y0);
    READ_INT(number_tab_diameters);
    READ_DOUBLE(seco_breakup_wave_b1);
    READ_DOUBLE(seco_breakup_wave_b0);
    READ_DOUBLE(seco_breakup_khrt_cl);
    READ_DOUBLE(seco_breakup_khrt_ctau);
    READ_DOUBLE(seco_breakup_khrt_crt);
    READ_DOUBLE(seco_breakup_ssd_we_cr);
    READ_DOUBLE(seco_breakup_ssd_core_bu);
    READ_DOUBLE(seco_breakup_ssd_np_target);
    READ_DOUBLE(seco_breakup_ssd_x_si);
    READ_DOUBLE(seco_breakup_madabushi_c0);
    READ_DOUBLE(seco_breakup_madabushi_column_drag_cd);
    READ_DOUBLE(seco_breakup_madabushi_ligament_factor);
    READ_DOUBLE(seco_breakup_madabushi_jet_diameter);
    READ_DOUBLE(seco_breakup_schmehl_np);
    READ_ENUM(volume_specification, Volume_Specification);
    READ_INT_LIST(volume_zones);
    READ_ENUM(volume_streams_spec, Volume_Streams_Spec);
    READ_INT(volume_streams_total);
    READ_INT(volume_streams_per_cell);
    READ_DOUBLE(volume_packing_limit_per_cell);
    READ_ENUM(volume_bgeom_shapes, Volume_Bgeom_Shapes);
    READ_VECTOR(volume_bgeom_min);
    READ_VECTOR(volume_bgeom_max);
    READ_DOUBLE(volume_bgeom_radius);
    READ_DOUBLE(volume_bgeom_viconeangle);
    READ_BOOL(mass_input_on);
    READ_BOOL(volfrac_input_on);
    READ_BOOL(rotation_on);
    READ_ENUM(rot_drag_law, Rot_Drag_Law);
    READ_ENUM(rot_lift_law, Rot_Lift_Law);
    READ_ENUM(cone_type, Cone_Type);
    READ_BOOL(uniform_mass_dist_on);
    READ_BOOL(spatial_staggering_std_inj_on);
    READ_BOOL(spatial_staggering_atomizer_on);
    READ_DOUBLE(stagger_radius);
    READ_BOOL(rough_wall_on);
    READ_STRING(cphace_domain);
    READ_VECTOR(pos);
    READ_VECTOR(pos2);
    READ_VECTOR(ff_center);
    READ_VECTOR(ff_virtual_origin);
    READ_VECTOR(ff_normal);
    READ_VECTOR(vel);
    READ_VECTOR(vel2);
    READ_VECTOR(ang_vel);
    READ_VECTOR(ang_vel2);
    READ_VECTOR(atomizer_axis);
    READ_DOUBLE(diameter);
    READ_DOUBLE(diameter2);
    READ_DOUBLE(temperature);
    READ_DOUBLE(temperature2);
    READ_DOUBLE(flow_rate);
    READ_DOUBLE(flow_rate2);
    READ_DOUBLE(unsteady_start);
    READ_DOUBLE(unsteady_stop);
    READ_DOUBLE(start_at_flow_time_in_unsteady_inj_file);
    READ_DOUBLE(interval_to_repeat_in_unsteady_inj_file);
    READ_DOUBLE(unsteady_ca_start);
    READ_DOUBLE(unsteady_ca_stop);
    READ_DOUBLE(vapor_pressure);
    READ_DOUBLE(inner_diameter);
    READ_DOUBLE(outer_diameter);
    READ_DOUBLE(half_angle);
    READ_DOUBLE(plain_length);
    READ_DOUBLE(plain_corner_size);
    READ_DOUBLE(plain_const_a);
    READ_DOUBLE(pswirl_inj_press);
    READ_DOUBLE(airbl_rel_vel);
    READ_DOUBLE(effer_quality);
    READ_DOUBLE(effer_t_sat);
    READ_DOUBLE(ff_oriface_width);
    READ_DOUBLE(phi_start);
    READ_DOUBLE(phi_stop);
    READ_DOUBLE(sheet_const);
    READ_DOUBLE(lig_const);
    READ_DOUBLE(effer_const);
    READ_DOUBLE(effer_half_angle_max);
    READ_DOUBLE(ff_sheet_const);
    READ_DOUBLE(atomizer_disp_angle);
    READ_VECTOR(axis);
    READ_DOUBLE(vel_mag);
    READ_DOUBLE(ang_vel_mag);
    READ_DOUBLE(cone_angle);
    READ_DOUBLE(inner_radius);
    READ_DOUBLE(radius);
    READ_DOUBLE(swirl_frac);
    READ_DOUBLE(total_flow_rate);
    READ_DOUBLE(total_mass);
    READ_DOUBLE(volume_fraction);
    READ_DOUBLE(rr_min);
    READ_DOUBLE(rr_max);
    READ_DOUBLE(rr_mean);
    READ_DOUBLE(rr_spread);
    READ_INT(rr_numdia);
    READ_VECTOR(posr);
    READ_VECTOR(posu);

#undef READ_STRING
#undef READ_INT
#undef READ_DOUBLE
#undef READ_BOOL
#undef READ_ENUM
#undef READ_VECTOR
#undef READ_INT_LIST

    return vectors_valid;
}

QJsonObject unit_to_json(const Unit &unit)
{
    QJsonObject result;
    result.insert("uuid", unit.inj.uuid.toString(QUuid::WithoutBraces));
    result.insert("unit_type", static_cast<int>(unit.type));
    result.insert("has_array_spec", unit.has_array_spec);
    result.insert("array_parent_uuid",
                  unit.array_parent_uuid.toString(QUuid::WithoutBraces));
    result.insert("is_array_child", unit.is_array_child);
    result.insert("follows_array", unit.follows_array);
    if (!unit.array_instance_path.isEmpty())
    {
        result.insert("array_instance_path",
                      int_list_to_json(unit.array_instance_path));
    }
    if (!unit.array_instance_key.isEmpty())
    {
        result.insert("array_instance_key",
                      uuid_list_to_json(unit.array_instance_key));
    }
    if (!unit.array_layer_uuid.isNull())
    {
        result.insert("array_layer_uuid",
                      unit.array_layer_uuid.toString(QUuid::WithoutBraces));
    }
    if (!unit.array_overrides.isEmpty())
    {
        QJsonArray overrides;
        for (const UnitArrayOverride &array_override : unit.array_overrides)
        {
            QJsonObject item;
            item.insert("instance_path",
                        int_list_to_json(array_override.instance_path));
            if (!array_override.instance_key.isEmpty())
            {
                item.insert("instance_key",
                            uuid_list_to_json(array_override.instance_key));
            }
            item.insert("override_physical", array_override.override_physical);
            item.insert("override_geometry", array_override.override_geometry);
            QJsonObject snapshot;
            injector_to_json(array_override.snapshot, &snapshot);
            item.insert("snapshot", snapshot);
            overrides.append(item);
        }
        result.insert("array_overrides", overrides);
    }
    if (unit.is_array_child)
    {
        // Following children are rebuilt from the parent specification. An
        // independent child keeps its layer marker so the object tree can
        // restore its position after a project reload.
        result.insert("array_layer", unit.array_layer);
    }
    result.insert("assembly_parent_uuid",
                  unit.assembly_parent_uuid.toString(QUuid::WithoutBraces));
    QJsonArray assembly_children;
    for (const QUuid &uuid : unit.assembly_child_uuids)
    {
        assembly_children.append(uuid.toString(QUuid::WithoutBraces));
    }
    result.insert("assembly_child_uuids", assembly_children);
    result.insert("assembly_local_position",
                  vector_to_json(unit.assembly_local_position));
    result.insert("assembly_local_rotation",
                  vector_to_json(unit.assembly_local_rotation));
    result.insert("has_fill_spec", unit.has_fill_spec);
    if (unit.has_fill_spec)
    {
        QJsonObject fill_spec;
        if (!unit.fill_spec.fill_uuid.isNull())
        {
            fill_spec.insert("fill_uuid",
                             unit.fill_spec.fill_uuid.toString(QUuid::WithoutBraces));
        }
        fill_spec.insert("placement_uuids",
                         uuid_list_to_json(unit.fill_spec.placement_uuids));
        fill_spec.insert("pattern", static_cast<int>(unit.fill_spec.pattern));
        fill_spec.insert("rows", unit.fill_spec.rows);
        fill_spec.insert("columns", unit.fill_spec.columns);
        fill_spec.insert("spacing_x", unit.fill_spec.spacing_x);
        fill_spec.insert("spacing_y", unit.fill_spec.spacing_y);
        fill_spec.insert("origin", vector_to_json(unit.fill_spec.origin));
        fill_spec.insert("circular_boundary", unit.fill_spec.circular_boundary);
        fill_spec.insert("boundary_radius", unit.fill_spec.boundary_radius);
        fill_spec.insert("direction", vector_to_json(unit.fill_spec.direction));
        fill_spec.insert("plane_normal", vector_to_json(unit.fill_spec.plane_normal));
        fill_spec.insert("use_reference_geometry", unit.fill_spec.use_reference_geometry);
        fill_spec.insert("conform_to_reference_normal",
                         unit.fill_spec.conform_to_reference_normal);
        QJsonArray source_weights;
        for (const int weight : unit.fill_spec.source_weights)
        {
            source_weights.append(weight);
        }
        fill_spec.insert("source_weights", source_weights);
        result.insert("fill_spec", fill_spec);
        QJsonArray sources;
        for (const QUuid &uuid : unit.fill_source_uuids)
        {
            sources.append(uuid.toString(QUuid::WithoutBraces));
        }
        result.insert("fill_source_uuids", sources);
    }
    if (unit.has_array_spec)
    {
        result.insert("array_spec", array_spec_to_json(unit.array_spec));
        QJsonArray array_specs;
        const QList<UnitArraySpec> specs = unit.array_specs.isEmpty()
            ? QList<UnitArraySpec>{unit.array_spec}
            : unit.array_specs;
        for (const UnitArraySpec &spec : specs)
        {
            array_specs.append(array_spec_to_json(spec));
        }
        result.insert("array_specs", array_specs);
    }
    QJsonObject injector;
    injector_to_json(unit.inj.injector_data, &injector);
    result.insert("injector", injector);
    return result;
}

QJsonObject unit_tree_to_json(const Unit &unit,
                              const QHash<QUuid, const Unit *> &unit_index,
                              QSet<QUuid> *visited)
{
    QJsonObject result = unit_to_json(unit);
    QList<QUuid> child_ids;

    for (auto it = unit_index.constBegin(); it != unit_index.constEnd(); ++it)
    {
        const Unit *candidate = it.value();
        if (candidate == nullptr || candidate->inj.uuid.isNull() ||
            (candidate->is_array_child && candidate->follows_array))
        {
            continue;
        }

        const bool assembly_child =
            candidate->assembly_parent_uuid == unit.inj.uuid;
        const bool array_child =
            candidate->array_parent_uuid == unit.inj.uuid;
        if (assembly_child || array_child)
        {
            child_ids.append(candidate->inj.uuid);
        }
    }
    std::sort(child_ids.begin(), child_ids.end(),
              [](const QUuid &left, const QUuid &right)
              {
                  return left.toString(QUuid::WithoutBraces) <
                         right.toString(QUuid::WithoutBraces);
              });

    QJsonArray children;

    if (visited != nullptr)
    {
        visited->insert(unit.inj.uuid);
    }

    for (const QUuid &child_uuid : child_ids)
    {
        const Unit *child = unit_index.value(child_uuid, nullptr);
        if (child == nullptr || child->inj.uuid.isNull() ||
            (child->is_array_child && child->follows_array) ||
            (visited != nullptr && visited->contains(child->inj.uuid)))
        {
            continue;
        }

        QJsonObject child_json = unit_tree_to_json(*child, unit_index, visited);
        child_json.insert("tree_parent_uuid",
                          unit.inj.uuid.toString(QUuid::WithoutBraces));
        child_json.insert(
            "tree_relation",
            child->assembly_parent_uuid == unit.inj.uuid ? "assembly" : "array");
        children.append(child_json);
    }

    if (!children.isEmpty())
    {
        result.insert("children", children);
    }
    return result;
}

bool parse_unit_tree(const QJsonValue &json_value,
                     QList<Unit> *units,
                     QSet<QUuid> *visited,
                     QString *error_message)
{
    if (units == nullptr || visited == nullptr || !json_value.isObject())
    {
        set_error(error_message, "Project session contains an invalid unit tree node.");
        return false;
    }

    Unit unit;
    if (!unit_from_json(json_value, &unit))
    {
        set_error(error_message, "Project session contains an invalid unit tree node.");
        return false;
    }

    if (unit.inj.uuid.isNull() || visited->contains(unit.inj.uuid))
    {
        set_error(error_message,
                  "Project session unit tree contains a duplicate UUID.");
        return false;
    }
    visited->insert(unit.inj.uuid);
    units->append(unit);

    const QJsonValue children_value = json_value.toObject().value("children");
    if (!children_value.isUndefined() && !children_value.isArray())
    {
        set_error(error_message,
                  "Project session contains an invalid unit tree children array.");
        return false;
    }

    for (const QJsonValue &child_value : children_value.toArray())
    {
        if (!child_value.isObject())
        {
            set_error(error_message,
                      "Project session contains an invalid unit tree child.");
            return false;
        }

        const QJsonObject child_object = child_value.toObject();
        const QUuid child_uuid(child_object.value("uuid").toString());
        const QUuid tree_parent_uuid(
            child_object.value("tree_parent_uuid").toString());
        const QString tree_relation =
            child_object.value("tree_relation").toString();
        const bool valid_legacy_parent =
            child_object.value("tree_parent_uuid").isUndefined() &&
            QUuid(child_object.value("assembly_parent_uuid").toString()) ==
                unit.inj.uuid;
        if (child_uuid.isNull() ||
            (!valid_legacy_parent && tree_parent_uuid != unit.inj.uuid) ||
            (!valid_legacy_parent && tree_relation != "assembly" &&
             tree_relation != "array"))
        {
            set_error(error_message,
                      "Project session contains an inconsistent unit tree parent reference.");
            return false;
        }

        if (!parse_unit_tree(child_value, units, visited, error_message))
        {
            return false;
        }
    }
    return true;
}

bool unit_from_json(const QJsonValue &json_value, Unit *unit)
{
    if (unit == nullptr || !json_value.isObject())
    {
        return false;
    }

    const QJsonObject object = json_value.toObject();
    const QUuid uuid(object.value("uuid").toString());
    const QJsonObject injector_object = object.value("injector").toObject();
    if (uuid.isNull() || injector_object.isEmpty())
    {
        return false;
    }

    unit->type = static_cast<Unit_Type>(object.value("unit_type").toInt(static_cast<int>(injector)));
    unit->inj.uuid = uuid;
    if (!injector_from_json(injector_object, &unit->inj.injector_data))
    {
        return false;
    }
    unit->has_array_spec = object.value("has_array_spec").toBool(false);
    unit->array_parent_uuid = QUuid(object.value("array_parent_uuid").toString());
    unit->is_array_child = object.value("is_array_child").toBool(false);
    unit->follows_array = object.value("follows_array").toBool(true);
    unit->array_instance_path.clear();
    const QJsonValue instance_path_value = object.value("array_instance_path");
    if (instance_path_value.isArray())
    {
        for (const QJsonValue &value : instance_path_value.toArray())
        {
            if (!value.isDouble() || value.toInt() < 0)
            {
                return false;
            }
            unit->array_instance_path.append(value.toInt());
        }
    }
    unit->array_instance_key.clear();
    if (!uuid_list_from_json(object.value("array_instance_key"),
                             &unit->array_instance_key))
    {
        return false;
    }
    unit->array_layer_uuid = QUuid(
        object.value("array_layer_uuid").toString());
    unit->array_overrides.clear();
    const QJsonValue overrides_value = object.value("array_overrides");
    if (!overrides_value.isUndefined() && !overrides_value.isArray())
    {
        return false;
    }
    for (const QJsonValue &override_value : overrides_value.toArray())
    {
        if (!override_value.isObject())
        {
            return false;
        }
        const QJsonObject override_object = override_value.toObject();
        UnitArrayOverride array_override;
        const QJsonValue path_value = override_object.value("instance_path");
        if (!path_value.isArray())
        {
            return false;
        }
        for (const QJsonValue &value : path_value.toArray())
        {
            if (!value.isDouble() || value.toInt() < 0)
            {
                return false;
            }
            array_override.instance_path.append(value.toInt());
        }
        if (!uuid_list_from_json(override_object.value("instance_key"),
                                 &array_override.instance_key))
        {
            return false;
        }
        array_override.override_physical =
            override_object.value("override_physical").toBool(false);
        array_override.override_geometry =
            override_object.value("override_geometry").toBool(false);
        if (!override_object.value("snapshot").isObject() ||
            !injector_from_json(override_object.value("snapshot").toObject(),
                                &array_override.snapshot))
        {
            return false;
        }
        unit->array_overrides.append(std::move(array_override));
    }
    unit->array_layer = qMax(0, object.value("array_layer").toInt(0));
    unit->assembly_parent_uuid = QUuid(
        object.value("assembly_parent_uuid").toString());
    for (const QJsonValue &child : object.value("assembly_child_uuids").toArray())
    {
        const QUuid child_uuid(child.toString());
        if (!child_uuid.isNull())
        {
            unit->assembly_child_uuids.append(child_uuid);
        }
    }
    if (!vector_from_json(object.value("assembly_local_position"),
                          &unit->assembly_local_position))
    {
        unit->assembly_local_position = unit->inj.injector_data.pos;
    }
    if (!vector_from_json(object.value("assembly_local_rotation"),
                          &unit->assembly_local_rotation))
    {
        unit->assembly_local_rotation = QVector3D();
    }
    unit->has_fill_spec = object.value("has_fill_spec").toBool(false);
    if (unit->has_fill_spec && object.value("fill_spec").isObject())
    {
        const QJsonObject fill_spec = object.value("fill_spec").toObject();
        unit->fill_spec.fill_uuid = QUuid(
            fill_spec.value("fill_uuid").toString());
        if (!uuid_list_from_json(fill_spec.value("placement_uuids"),
                                 &unit->fill_spec.placement_uuids))
        {
            return false;
        }
        unit->fill_spec.pattern = static_cast<UnitFillPattern>(
            fill_spec.value("pattern").toInt(static_cast<int>(UnitFillPattern::Square)));
        unit->fill_spec.rows = fill_spec.value("rows").toInt(1);
        unit->fill_spec.columns = fill_spec.value("columns").toInt(1);
        unit->fill_spec.spacing_x = static_cast<float>(
            fill_spec.value("spacing_x").toDouble(0.0));
        unit->fill_spec.spacing_y = static_cast<float>(
            fill_spec.value("spacing_y").toDouble(0.0));
        vector_from_json(fill_spec.value("origin"), &unit->fill_spec.origin);
        unit->fill_spec.circular_boundary =
            fill_spec.value("circular_boundary").toBool(false);
        unit->fill_spec.boundary_radius = static_cast<float>(
            fill_spec.value("boundary_radius").toDouble(0.0));
        vector_from_json(fill_spec.value("direction"), &unit->fill_spec.direction);
        vector_from_json(fill_spec.value("plane_normal"), &unit->fill_spec.plane_normal);
        unit->fill_spec.use_reference_geometry =
            fill_spec.value("use_reference_geometry").toBool(false);
        unit->fill_spec.conform_to_reference_normal =
            fill_spec.value("conform_to_reference_normal").toBool(false);
        unit->fill_spec.source_weights.clear();
        const QJsonValue weights_value = fill_spec.value("source_weights");
        if (weights_value.isArray())
        {
            for (const QJsonValue &weight_value : weights_value.toArray())
            {
                unit->fill_spec.source_weights.append(qMax(1, weight_value.toInt(1)));
            }
        }
        const QJsonArray sources = object.value("fill_source_uuids").toArray();
        for (const QJsonValue &source : sources)
        {
            const QUuid uuid(source.toString());
            if (!uuid.isNull())
            {
                unit->fill_source_uuids.append(uuid);
            }
        }
        if (unit->fill_spec.source_weights.isEmpty())
        {
            unit->fill_spec.source_weights = QVector<int>(
                qMax(1, unit->fill_source_uuids.size()), 1);
        }
    }
    unit->array_specs.clear();
    if (object.value("array_specs").isArray())
    {
        for (const QJsonValue &value : object.value("array_specs").toArray())
        {
            UnitArraySpec spec;
            if (!array_spec_from_json(value, &spec))
            {
                return false;
            }
            unit->array_specs.append(spec);
        }
    }
    if (unit->has_array_spec && object.value("array_spec").isObject())
    {
        if (!array_spec_from_json(object.value("array_spec"),
                                  &unit->array_spec))
        {
            return false;
        }
    }
    if (unit->array_specs.isEmpty() && unit->has_array_spec)
    {
        unit->array_specs.append(unit->array_spec);
    }
    if (!unit->array_specs.isEmpty())
    {
        unit->has_array_spec = true;
        unit->array_spec = unit->array_specs.last();
    }
    return unit->inj.create_injector();
}

void set_error(QString *error_message, const QString &message)
{
    if (error_message != nullptr)
    {
        *error_message = message;
    }
}

bool is_finite_vector(const QVector3D &value)
{
    return std::isfinite(static_cast<double>(value.x())) &&
           std::isfinite(static_cast<double>(value.y())) &&
           std::isfinite(static_cast<double>(value.z()));
}

bool is_usable_reference_frame(const QVector3D &x_axis,
                               const QVector3D &z_axis)
{
    return x_axis.lengthSquared() > 1.0e-12f &&
           z_axis.lengthSquared() > 1.0e-12f &&
           QVector3D::crossProduct(x_axis, z_axis).lengthSquared() > 1.0e-12f;
}

QJsonObject array_spec_to_json(const UnitArraySpec &spec)
{
    QJsonObject result;
    if (!spec.layer_uuid.isNull())
    {
        result.insert("layer_uuid",
                      spec.layer_uuid.toString(QUuid::WithoutBraces));
    }
    result.insert("placement_uuids", uuid_list_to_json(spec.placement_uuids));
    result.insert("type", static_cast<int>(spec.type));
    result.insert("count", spec.count);
    result.insert("direction", vector_to_json(spec.direction));
    result.insert("origin", vector_to_json(spec.origin));
    result.insert("spacing", spec.spacing);
    result.insert("angle_degrees", spec.angle_degrees);
    result.insert("major_radius", spec.major_radius);
    result.insert("minor_radius", spec.minor_radius);
    result.insert("plane_normal", vector_to_json(spec.plane_normal));
    result.insert("use_reference_geometry", spec.use_reference_geometry);
    result.insert("conform_to_reference_normal", spec.conform_to_reference_normal);
    return result;
}

bool array_spec_from_json(const QJsonValue &json_value, UnitArraySpec *spec)
{
    if (spec == nullptr || !json_value.isObject())
    {
        return false;
    }

    const QJsonObject object = json_value.toObject();
    const QUuid layer_uuid(object.value("layer_uuid").toString());
    if (!layer_uuid.isNull())
    {
        spec->layer_uuid = layer_uuid;
    }
    if (!uuid_list_from_json(object.value("placement_uuids"),
                             &spec->placement_uuids))
    {
        return false;
    }
    spec->type = static_cast<UnitArrayType>(
        object.value("type").toInt(static_cast<int>(UnitArrayType::Linear)));
    spec->count = object.value("count").toInt(1);
    vector_from_json(object.value("direction"), &spec->direction);
    vector_from_json(object.value("origin"), &spec->origin);
    spec->spacing = static_cast<float>(object.value("spacing").toDouble(0.0));
    spec->angle_degrees = static_cast<float>(
        object.value("angle_degrees").toDouble(360.0));
    spec->major_radius = static_cast<float>(
        object.value("major_radius").toDouble(10.0));
    spec->minor_radius = static_cast<float>(
        object.value("minor_radius").toDouble(5.0));
    vector_from_json(object.value("plane_normal"), &spec->plane_normal);
    spec->use_reference_geometry =
        object.value("use_reference_geometry").toBool(false);
    spec->conform_to_reference_normal =
        object.value("conform_to_reference_normal").toBool(false);
    return true;
}

bool is_valid_array_spec(const UnitArraySpec &spec)
{
    const int array_type = static_cast<int>(spec.type);
    return array_type >= static_cast<int>(UnitArrayType::Linear) &&
           array_type <= static_cast<int>(UnitArrayType::Elliptical) &&
           spec.count >= 1 && spec.count <= 100000 &&
           is_finite_vector(spec.direction) &&
           is_finite_vector(spec.origin) &&
           is_finite_vector(spec.plane_normal) &&
           std::isfinite(spec.spacing) &&
           std::isfinite(spec.angle_degrees) &&
           std::isfinite(spec.major_radius) &&
           std::isfinite(spec.minor_radius) &&
           (!spec.use_reference_geometry ||
            is_usable_reference_frame(spec.direction, spec.plane_normal)) &&
           (spec.type != UnitArrayType::Elliptical ||
            (spec.major_radius > 0.0f && spec.minor_radius >= 0.0f));
}

QString session_path_for_storage(const QString &path, const QString &session_file_path)
{
    if (path.trimmed().isEmpty() || !QFileInfo(path).isAbsolute())
    {
        return path;
    }

    if (session_file_path.trimmed().isEmpty())
    {
        return QFileInfo(path).absoluteFilePath();
    }

    const QDir session_directory(QFileInfo(session_file_path).absolutePath());
    const QString relative_path = session_directory.relativeFilePath(
        QFileInfo(path).absoluteFilePath());
    return QFileInfo(relative_path).isAbsolute() ? path : relative_path;
}

QString session_path_for_runtime(const QString &path, const QString &session_file_path)
{
    if (path.trimmed().isEmpty() || QFileInfo(path).isAbsolute())
    {
        return path;
    }

    const QDir session_directory(QFileInfo(session_file_path).absolutePath());
    return QFileInfo(session_directory.absoluteFilePath(path)).absoluteFilePath();
}

QJsonObject data_to_json(const project_session::Data &data,
                         const QString &file_path,
                         bool include_timestamp)
{
    QJsonObject root;
    root.insert("schema_version", kSessionSchemaVersion);
    if (include_timestamp)
    {
        root.insert("created_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    root.insert("chemkin_file_path",
                session_path_for_storage(data.chemkin_file_path, file_path));

    if (data.has_unit_preferences)
    {
        QJsonObject unit_preferences;
        unit_preferences.insert("length", data.unit_preferences.length);
        unit_preferences.insert("angle", data.unit_preferences.angle);
        unit_preferences.insert("velocity", data.unit_preferences.velocity);
        unit_preferences.insert("mass", data.unit_preferences.mass);
        unit_preferences.insert("mass_flow", data.unit_preferences.mass_flow);
        unit_preferences.insert("time", data.unit_preferences.time);
        unit_preferences.insert("pressure", data.unit_preferences.pressure);
        unit_preferences.insert("temperature", data.unit_preferences.temperature);
        root.insert("unit_preferences", unit_preferences);
    }

    QJsonObject species_colors;
    for (auto it = data.species_colors.constBegin();
         it != data.species_colors.constEnd();
         ++it)
    {
        if (it.value().isValid())
        {
            species_colors.insert(it.key(), it.value().name(QColor::HexRgb).toUpper());
        }
    }
    root.insert("species_colors", species_colors);

    QJsonArray units;
    QHash<QUuid, const Unit *> unit_index;
    for (const Unit &unit : data.units)
    {
        units.append(unit_to_json(unit));
        unit_index.insert(unit.inj.uuid, &unit);
    }
    root.insert("units", units);

    // Keep a recursive representation alongside the legacy flat list. The
    // flat list remains useful for old readers and structural validation,
    // while the tree preserves the intended Assembly ownership explicitly.
    QJsonArray unit_tree;
    QSet<QUuid> visited_tree_units;
    for (const Unit &unit : data.units)
    {
        if (!unit.assembly_parent_uuid.isNull() ||
            !unit.array_parent_uuid.isNull() ||
            (unit.is_array_child && unit.follows_array) ||
            visited_tree_units.contains(unit.inj.uuid))
        {
            continue;
        }
        unit_tree.append(unit_tree_to_json(unit, unit_index,
                                           &visited_tree_units));
    }
    root.insert("unit_tree", unit_tree);

    QJsonArray materials;
    for (const MaterialConfigEntry &entry : data.materials)
    {
        QJsonObject material;
        material.insert("name", entry.name);
        material.insert("density", entry.density);
        materials.append(material);
    }
    root.insert("materials", materials);

    QJsonObject reference_geometry;
    reference_geometry.insert("kind", data.reference_geometry.kind);
    reference_geometry.insert(
        "file_path",
        session_path_for_storage(data.reference_geometry.file_path, file_path));
    reference_geometry.insert("position", vector_to_json(data.reference_geometry.position));
    reference_geometry.insert("rotation", vector_to_json(data.reference_geometry.rotation));
    reference_geometry.insert("locked", data.reference_geometry.locked);
    reference_geometry.insert("visible", data.reference_geometry.visible);
    reference_geometry.insert("section_clipping",
                             data.reference_geometry.section_clipping);
    reference_geometry.insert("construction_direction",
                              vector_to_json(data.reference_geometry.construction_direction));
    reference_geometry.insert("construction_size", data.reference_geometry.construction_size);
    reference_geometry.insert("construction_thickness",
                             data.reference_geometry.construction_thickness);
    reference_geometry.insert("construction_radius", data.reference_geometry.construction_radius);
    root.insert("reference_geometry", reference_geometry);
    return root;
}

bool migrate_schema_v1_to_v2(QJsonObject *root, QString *error_message)
{
    if (root == nullptr)
    {
        set_error(error_message, "Unable to migrate an empty project session.");
        return false;
    }

    const QJsonValue units_value = root->value("units");
    if (!units_value.isArray())
    {
        set_error(error_message,
                  "Project session migration requires a valid units array.");
        return false;
    }

    QJsonArray units = units_value.toArray();
    for (int index = 0; index < units.size(); ++index)
    {
        if (!units.at(index).isObject())
        {
            set_error(error_message,
                      "Project session migration found an invalid unit entry.");
            return false;
        }

        QJsonObject unit = units.at(index).toObject();
        const QJsonValue legacy_array_specs = unit.value("array_specs");
        if (legacy_array_specs.isObject())
        {
            QJsonArray normalized_specs;
            normalized_specs.append(legacy_array_specs);
            unit.insert("array_specs", normalized_specs);
        }

        if (unit.value("has_fill_spec").toBool(false) &&
            unit.value("fill_spec").isObject())
        {
            QJsonObject fill_spec = unit.value("fill_spec").toObject();
            if (!fill_spec.contains("source_weights"))
            {
                QJsonArray weights;
                const QJsonArray source_uuids =
                    unit.value("fill_source_uuids").toArray();
                for (int source_index = 0;
                     source_index < qMax(1, source_uuids.size());
                     ++source_index)
                {
                    weights.append(1);
                }
                fill_spec.insert("source_weights", weights);
                unit.insert("fill_spec", fill_spec);
            }
        }

        units[index] = unit;
    }

    root->insert("units", units);
    // Schema v1 had no authoritative recursive tree. If a file was edited or
    // produced by a transitional writer that left a stale unit_tree behind,
    // rebuild from migrated flat units instead of accepting contradictory data.
    root->remove("unit_tree");
    root->insert("schema_version", kSessionSchemaVersion);
    return true;
}
}

namespace project_session
{
bool validate(const Data &data, QString *error_message)
{
    QSet<QUuid> unit_ids;
    QStringList injector_names;
    for (const Unit &unit : data.units)
    {
        if (unit.inj.uuid.isNull())
        {
            set_error(error_message, "Project contains a unit with an empty UUID.");
            return false;
        }

        if (unit_ids.contains(unit.inj.uuid))
        {
            set_error(error_message, "Project contains duplicate unit UUIDs.");
            return false;
        }
        unit_ids.insert(unit.inj.uuid);

        if (unit.type == injector || unit.type == array)
        {
            const QString name = unit.inj.injector_data.name.trimmed();
            if (!name.isEmpty() && injector_names.contains(name, Qt::CaseInsensitive))
            {
                set_error(error_message,
                          QString("Project contains duplicate injector name: %1").arg(name));
                return false;
            }
            if (!name.isEmpty())
            {
                injector_names.append(name);
            }
        }

        if (unit.type < injector || unit.type > array)
        {
            set_error(error_message, "Project contains an invalid unit type.");
            return false;
        }

        if (unit.array_layer < 0)
        {
            set_error(error_message,
                      "Project contains a negative array layer marker.");
            return false;
        }
        if (unit.is_array_child)
        {
            if (unit.array_parent_uuid.isNull() ||
                !unit_ids.contains(unit.array_parent_uuid))
            {
                set_error(error_message,
                          "Project contains an array child with an invalid parent reference.");
                return false;
            }
        }
        else if (!unit.array_parent_uuid.isNull())
        {
            set_error(error_message,
                      "Project contains a non-array unit with an array parent reference.");
            return false;
        }
        QSet<QString> override_paths;
        for (const UnitArrayOverride &array_override : unit.array_overrides)
        {
            if (array_override.instance_path.isEmpty() ||
                (!array_override.override_physical &&
                 !array_override.override_geometry))
            {
                set_error(error_message,
                          "Project contains an invalid array override scope.");
                return false;
            }
            QStringList path_parts;
            for (const int path_index : array_override.instance_path)
            {
                if (path_index < 0)
                {
                    set_error(error_message,
                              "Project contains a negative array override path index.");
                    return false;
                }
                path_parts.append(QString::number(path_index));
            }
            const QString path_key = path_parts.join(',');
            if (override_paths.contains(path_key))
            {
                set_error(error_message,
                          "Project contains duplicate array override paths.");
                return false;
            }
            override_paths.insert(path_key);
        }
        if (unit.has_array_spec)
        {
            QList<UnitArraySpec> specs = unit.array_specs;
            if (specs.isEmpty())
            {
                specs.append(unit.array_spec);
            }
            if (!is_valid_array_spec(unit.array_spec) ||
                std::any_of(specs.cbegin(), specs.cend(),
                            [](const UnitArraySpec &spec)
                            {
                                return !is_valid_array_spec(spec);
                            }))
            {
                set_error(error_message,
                          "Project contains invalid array specification values.");
                return false;
            }
        }

        if (unit.has_array_spec && unit.has_fill_spec)
        {
            set_error(error_message,
                      "Project unit cannot contain both Array and Fill specifications.");
            return false;
        }

        if (unit.has_fill_spec)
        {
            const UnitFillSpec &spec = unit.fill_spec;
            const int fill_pattern = static_cast<int>(spec.pattern);
            if (fill_pattern < static_cast<int>(UnitFillPattern::Square) ||
                fill_pattern > static_cast<int>(UnitFillPattern::Hexagonal) ||
                spec.rows < 1 || spec.rows > 1000 ||
                spec.columns < 1 || spec.columns > 1000 ||
                !is_finite_vector(spec.origin) ||
                !is_finite_vector(spec.direction) ||
                !is_finite_vector(spec.plane_normal) ||
                !std::isfinite(spec.spacing_x) ||
                !std::isfinite(spec.spacing_y) ||
                !std::isfinite(spec.boundary_radius) ||
                spec.source_weights.isEmpty() ||
                (!unit.fill_source_uuids.isEmpty() &&
                 spec.source_weights.size() != unit.fill_source_uuids.size()) ||
                std::any_of(spec.source_weights.cbegin(),
                            spec.source_weights.cend(),
                            [](int weight) { return weight <= 0; }) ||
                std::accumulate(spec.source_weights.cbegin(),
                                spec.source_weights.cend(), 0LL) > 1000000LL ||
                (spec.use_reference_geometry &&
                 !is_usable_reference_frame(spec.direction, spec.plane_normal)) ||
                (spec.circular_boundary && spec.boundary_radius < 0.0f))
            {
                set_error(error_message,
                          "Project contains invalid fill specification values.");
                return false;
            }
            for (const QUuid &source_uuid : unit.fill_source_uuids)
            {
                if (!unit_ids.contains(source_uuid))
                {
                    set_error(error_message,
                              "Project contains a Fill source with an invalid UUID.");
                    return false;
                }
            }
        }
    }

    if (!validate_material_entries(data.materials, error_message))
    {
        return false;
    }

    if (data.has_unit_preferences &&
        !UnitSystem::validate_preferences(data.unit_preferences, error_message))
    {
        return false;
    }

    for (auto it = data.species_colors.constBegin();
         it != data.species_colors.constEnd();
         ++it)
    {
        if (it.key().trimmed().isEmpty() || !it.value().isValid())
        {
            set_error(error_message, "Project contains an invalid species color entry.");
            return false;
        }
    }

    if (!is_finite_vector(data.reference_geometry.position) ||
        !is_finite_vector(data.reference_geometry.rotation))
    {
        set_error(error_message,
                  "Project contains a reference geometry transform with non-finite values.");
        return false;
    }

    const QString reference_kind = data.reference_geometry.kind.trimmed().toLower();
    if (reference_kind != "file" && reference_kind != "datum_plane" &&
        reference_kind != "datum_axis" && reference_kind != "datum_origin" &&
        reference_kind != "section_plane" && reference_kind != "alignment_frame")
    {
        set_error(error_message, "Project contains an invalid reference geometry kind.");
        return false;
    }
    if (reference_kind != "file" &&
        (!is_finite_vector(data.reference_geometry.construction_direction) ||
         data.reference_geometry.construction_direction.lengthSquared() <= 1.0e-12f ||
         !std::isfinite(data.reference_geometry.construction_size) ||
         data.reference_geometry.construction_size <= 0.0 ||
         ((reference_kind == "datum_plane" || reference_kind == "section_plane") &&
          (!std::isfinite(data.reference_geometry.construction_thickness) ||
           data.reference_geometry.construction_thickness <= 0.0)) ||
         (reference_kind == "datum_axis" &&
          (!std::isfinite(data.reference_geometry.construction_radius) ||
           data.reference_geometry.construction_radius <= 0.0))) ||
         ((reference_kind == "datum_origin" || reference_kind == "alignment_frame") &&
          (!std::isfinite(data.reference_geometry.construction_radius) ||
           data.reference_geometry.construction_radius <= 0.0)))
    {
        set_error(error_message,
                  "Project contains invalid constructed reference geometry parameters.");
        return false;
    }

    if (error_message != nullptr)
    {
        error_message->clear();
    }
    return true;
}

bool validate_references(const Data &data,
                         const QStringList &chemkin_species_names,
                         QString *error_message)
{
    QStringList errors;
    QStringList material_names;
    for (const MaterialConfigEntry &entry : data.materials)
    {
        const QString name = entry.name.trimmed();
        if (!name.isEmpty() && !material_names.contains(name, Qt::CaseInsensitive))
        {
            material_names.append(name);
        }
    }

    QStringList species_names;
    for (const QString &species : chemkin_species_names)
    {
        const QString name = species.trimmed();
        if (!name.isEmpty() && !species_names.contains(name, Qt::CaseInsensitive))
        {
            species_names.append(name);
        }
    }

    QHash<QUuid, int> unit_indices;
    for (int index = 0; index < data.units.size(); ++index)
    {
        const QUuid uuid = data.units.at(index).inj.uuid;
        if (uuid.isNull() || unit_indices.contains(uuid))
        {
            continue;
        }
        unit_indices.insert(uuid, index);
    }
    for (int index = 0; index < data.units.size(); ++index)
    {
        const Unit &unit = data.units.at(index);
        const QUuid uuid = unit.inj.uuid;
        if (uuid.isNull())
        {
            continue;
        }
        if (!unit.assembly_parent_uuid.isNull())
        {
            const int parent_index = unit_indices.value(unit.assembly_parent_uuid, -1);
            if (parent_index < 0 ||
                !data.units.at(parent_index).assembly_child_uuids.contains(uuid))
            {
                errors.append(QString("Unit '%1' has an invalid Assembly parent reference.")
                                  .arg(unit.inj.injector_data.name));
            }
        }
        for (const QUuid &child_uuid : unit.assembly_child_uuids)
        {
            const int child_index = unit_indices.value(child_uuid, -1);
            if (child_index < 0 ||
                data.units.at(child_index).assembly_parent_uuid != uuid)
            {
                errors.append(QString("Unit '%1' has an invalid Assembly child reference.")
                                  .arg(unit.inj.injector_data.name));
            }
        }
    }

    QSet<QUuid> visiting;
    QSet<QUuid> visited;
    const auto has_assembly_cycle = [&](const QUuid &uuid,
                                        const auto &self) -> bool
    {
        if (visiting.contains(uuid))
        {
            return true;
        }
        if (visited.contains(uuid))
        {
            return false;
        }
        visiting.insert(uuid);
        const int index = unit_indices.value(uuid, -1);
        if (index >= 0)
        {
            for (const QUuid &child_uuid : data.units.at(index).assembly_child_uuids)
            {
                if (unit_indices.contains(child_uuid) && self(child_uuid, self))
                {
                    return true;
                }
            }
        }
        visiting.remove(uuid);
        visited.insert(uuid);
        return false;
    };
    for (auto it = unit_indices.constBegin(); it != unit_indices.constEnd(); ++it)
    {
        if (has_assembly_cycle(it.key(), has_assembly_cycle))
        {
            errors.append("Project contains a cyclic Assembly relationship.");
            break;
        }
    }

    for (int index = 0; index < data.units.size(); ++index)
    {
        const Injector &injector = data.units.at(index).inj.injector_data;
        const QString label = injector.name.trimmed().isEmpty()
            ? QString("unit %1").arg(index + 1)
            : injector.name.trimmed();

        const QList<QPair<const char *, const QString *>> material_fields = {
            {"material", &injector.material},
            {"evaporating-material", &injector.evaporating_material}
        };
        for (const auto &field : material_fields)
        {
            const QString value = field.second->trimmed();
            if (!value.isEmpty() && !material_names.contains(value, Qt::CaseInsensitive))
            {
                errors.append(QString("%1 references missing material '%2' in %3.")
                                  .arg(label, value, QString::fromLatin1(field.first)));
            }
        }

        const QList<QPair<const char *, const QString *>> species_fields = {
            {"devolatilizing-species", &injector.devolatilizing_species},
            {"evaporating-species", &injector.evaporating_species},
            {"oxidizing-species", &injector.oxidizing_species},
            {"product-species", &injector.product_species}
        };
        if (data.chemkin_file_path.trimmed().isEmpty())
        {
            for (const auto &field : species_fields)
            {
                const QString value = field.second->trimmed();
                if (!value.isEmpty())
                {
                    errors.append(QString("%1 references Chemkin species '%2' in %3, but no Chemkin file is loaded.")
                                      .arg(label, value, QString::fromLatin1(field.first)));
                }
            }
        }
        else
        {
            for (const auto &field : species_fields)
            {
                const QString value = field.second->trimmed();
                if (!value.isEmpty() && !species_names.contains(value, Qt::CaseInsensitive))
                {
                    errors.append(QString("%1 references missing Chemkin species '%2' in %3.")
                                      .arg(label, value, QString::fromLatin1(field.first)));
                }
            }
        }
    }

    errors.removeDuplicates();
    if (errors.isEmpty())
    {
        if (error_message != nullptr)
        {
            error_message->clear();
        }
        return true;
    }

    if (error_message != nullptr)
    {
        *error_message = QString("Project reference preflight found %1 problem(s):\n- %2")
                             .arg(errors.size())
                             .arg(errors.join("\n- "));
    }
    return false;
}

bool save(const QString &file_path, const Data &data, QString *error_message)
{
    if (file_path.trimmed().isEmpty())
    {
        set_error(error_message, "Project session path is empty.");
        return false;
    }

    if (!validate(data, error_message))
    {
        return false;
    }

    const QJsonObject root = data_to_json(data, file_path, true);

    QSaveFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        set_error(error_message, QString("Unable to write project session: %1").arg(file_path));
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        set_error(error_message, QString("Unable to finalize project session: %1").arg(file_path));
        return false;
    }
    return true;
}

QByteArray fingerprint(const Data &data)
{
    // The empty session path makes absolute paths stay absolute; the
    // timestamp is omitted by design.
    const QJsonObject root = data_to_json(data, QString(), false);
    return QCryptographicHash::hash(
               QJsonDocument(root).toJson(QJsonDocument::Compact),
               QCryptographicHash::Sha256)
        .toHex();
}

bool load(const QString &file_path, Data *data, QString *error_message)
{
    if (file_path.trimmed().isEmpty())
    {
        set_error(error_message, "Project session path is empty.");
        return false;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        set_error(error_message, QString("Unable to open project session: %1").arg(file_path));
        return false;
    }

    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject())
    {
        set_error(error_message,
                  QString("Invalid project session: %1 (%2)")
                      .arg(file_path, parse_error.errorString()));
        return false;
    }

    QJsonObject root = document.object();
    const QJsonValue version_value = root.value("schema_version");
    if (!version_value.isDouble() ||
        !std::isfinite(version_value.toDouble()) ||
        version_value.toDouble() < std::numeric_limits<int>::min() ||
        version_value.toDouble() > std::numeric_limits<int>::max() ||
        version_value.toDouble() != std::floor(version_value.toDouble()))
    {
        set_error(error_message, "Project session contains an invalid schema version.");
        return false;
    }

    const int version = version_value.toInt(-1);
    if (version < kFirstSupportedSchemaVersion ||
        version > kSessionSchemaVersion)
    {
        set_error(error_message, QString("Unsupported project session schema version: %1").arg(version));
        return false;
    }

    if (version == 1 && !migrate_schema_v1_to_v2(&root, error_message))
    {
        return false;
    }

    const QJsonValue units_value = root.value("units");
    const QJsonValue unit_tree_value = root.value("unit_tree");
    if (!units_value.isArray() && !unit_tree_value.isArray())
    {
        set_error(error_message,
                  "Project session is missing a valid units or unit_tree array.");
        return false;
    }
    if (!unit_tree_value.isUndefined() && !unit_tree_value.isArray())
    {
        set_error(error_message,
                  "Project session contains an invalid unit_tree array.");
        return false;
    }

    const QJsonValue materials_value = root.value("materials");
    if (!materials_value.isUndefined() && !materials_value.isArray())
    {
        set_error(error_message,
                  "Project session contains an invalid materials array.");
        return false;
    }

    const QJsonValue reference_geometry_value = root.value("reference_geometry");
    if (!reference_geometry_value.isUndefined() &&
        !reference_geometry_value.isObject())
    {
        set_error(error_message,
                  "Project session contains an invalid reference_geometry object.");
        return false;
    }

    const QJsonValue chemkin_path_value = root.value("chemkin_file_path");
    if (!chemkin_path_value.isUndefined() && !chemkin_path_value.isString())
    {
        set_error(error_message,
                  "Project session contains an invalid Chemkin file path.");
        return false;
    }

    Data parsed;
    parsed.chemkin_file_path = session_path_for_runtime(
        root.value("chemkin_file_path").toString(), file_path);

    const QJsonValue unit_preferences_value = root.value("unit_preferences");
    if (!unit_preferences_value.isUndefined())
    {
        if (!unit_preferences_value.isObject())
        {
            set_error(error_message,
                      "Project session contains invalid unit preferences.");
            return false;
        }

        const QJsonObject unit_preferences = unit_preferences_value.toObject();
        const auto read_preference = [&unit_preferences](const char *key,
                                                          QString *target)
        {
            const QJsonValue value = unit_preferences.value(QString::fromLatin1(key));
            if (value.isUndefined())
            {
                return true;
            }
            if (!value.isString())
            {
                return false;
            }
            *target = value.toString();
            return true;
        };
        if (!read_preference("length", &parsed.unit_preferences.length) ||
            !read_preference("angle", &parsed.unit_preferences.angle) ||
            !read_preference("velocity", &parsed.unit_preferences.velocity) ||
            !read_preference("mass", &parsed.unit_preferences.mass) ||
            !read_preference("mass_flow", &parsed.unit_preferences.mass_flow) ||
            !read_preference("time", &parsed.unit_preferences.time) ||
            !read_preference("pressure", &parsed.unit_preferences.pressure) ||
            !read_preference("temperature", &parsed.unit_preferences.temperature))
        {
            set_error(error_message,
                      "Project session contains a unit preference with an invalid type.");
            return false;
        }
        parsed.has_unit_preferences = true;
    }
    const QJsonValue species_colors_value = root.value("species_colors");
    if (!species_colors_value.isUndefined() && !species_colors_value.isObject())
    {
        set_error(error_message,
                  "Project session contains an invalid species_colors object.");
        return false;
    }
    const QJsonObject species_colors = species_colors_value.toObject();
    for (auto it = species_colors.constBegin(); it != species_colors.constEnd(); ++it)
    {
        if (it.key().trimmed().isEmpty())
        {
            set_error(error_message, "Project session contains an empty species color key.");
            return false;
        }

        if (!it.value().isString())
        {
            set_error(error_message,
                      QString("Project session contains an invalid color for species: %1")
                          .arg(it.key()));
            return false;
        }

        const QColor color(it.value().toString());
        if (!color.isValid())
        {
            set_error(error_message,
                      QString("Project session contains an invalid color for species: %1")
                          .arg(it.key()));
            return false;
        }
        parsed.species_colors.insert(it.key(), color);
    }

    QList<Unit> flat_units;
    for (const QJsonValue &unit_value : units_value.toArray())
    {
        Unit unit;
        if (!unit_from_json(unit_value, &unit))
        {
            set_error(error_message, "Project session contains an invalid injector entry.");
            return false;
        }
        flat_units.append(std::move(unit));
    }

    if (unit_tree_value.isArray())
    {
        QSet<QUuid> tree_ids;
        for (const QJsonValue &root_value : unit_tree_value.toArray())
        {
            if (!parse_unit_tree(root_value, &parsed.units, &tree_ids,
                                 error_message))
            {
                return false;
            }
        }

        QSet<QUuid> flat_ids;
        for (const Unit &unit : flat_units)
        {
            flat_ids.insert(unit.inj.uuid);
        }
        if (flat_ids != tree_ids)
        {
            set_error(error_message,
                      "Project session flat units and unit tree do not contain the same UUIDs.");
            return false;
        }
    }
    else
    {
        parsed.units = std::move(flat_units);
    }

    for (const QJsonValue &material_value : root.value("materials").toArray())
    {
        if (!material_value.isObject())
        {
            set_error(error_message, "Project session contains an invalid material entry.");
            return false;
        }

        const QJsonObject material = material_value.toObject();
        if (!material.value("name").isString() ||
            !material.value("density").isDouble() ||
            !std::isfinite(material.value("density").toDouble()))
        {
            set_error(error_message,
                      "Project session contains a material with invalid field types.");
            return false;
        }
        const QString name = material.value("name").toString().trimmed();
        if (name.isEmpty())
        {
            set_error(error_message, "Project session contains a material with an empty name.");
            return false;
        }
        MaterialConfigEntry entry;
        entry.name = name;
        entry.density = material.value("density").toDouble(0.0);
        parsed.materials.append(entry);
    }

    const QJsonObject reference_geometry = reference_geometry_value.toObject();
    parsed.reference_geometry.kind = reference_geometry.value("kind").toString("file").trimmed().toLower();
    if (parsed.reference_geometry.kind != "file" &&
        parsed.reference_geometry.kind != "datum_plane" &&
        parsed.reference_geometry.kind != "datum_axis" &&
        parsed.reference_geometry.kind != "datum_origin" &&
        parsed.reference_geometry.kind != "section_plane" &&
        parsed.reference_geometry.kind != "alignment_frame")
    {
        set_error(error_message,
                  "Project session contains an invalid reference geometry kind.");
        return false;
    }
    if (reference_geometry.contains("file_path") &&
        !reference_geometry.value("file_path").isString())
    {
        set_error(error_message,
                  "Project session contains an invalid reference geometry file path.");
        return false;
    }
    if ((reference_geometry.contains("locked") &&
         !reference_geometry.value("locked").isBool()) ||
        (reference_geometry.contains("visible") &&
         !reference_geometry.value("visible").isBool()) ||
        (reference_geometry.contains("section_clipping") &&
         !reference_geometry.value("section_clipping").isBool()))
    {
        set_error(error_message,
                  "Project session contains invalid reference geometry visibility flags.");
        return false;
    }
    parsed.reference_geometry.file_path = session_path_for_runtime(
        reference_geometry.value("file_path").toString(), file_path);
    if (parsed.reference_geometry.kind != "file")
    {
        parsed.reference_geometry.file_path.clear();
    }
    if ((reference_geometry.contains("position") &&
         !vector_from_json(reference_geometry.value("position"),
                           &parsed.reference_geometry.position)) ||
        (reference_geometry.contains("rotation") &&
         !vector_from_json(reference_geometry.value("rotation"),
                           &parsed.reference_geometry.rotation)) ||
        (reference_geometry.contains("construction_direction") &&
         !vector_from_json(reference_geometry.value("construction_direction"),
                           &parsed.reference_geometry.construction_direction)))
    {
        set_error(error_message,
                  "Project session contains an invalid reference geometry transform.");
        return false;
    }
    parsed.reference_geometry.locked = reference_geometry.value("locked").toBool(false);
    parsed.reference_geometry.visible = reference_geometry.value("visible").toBool(true);
    parsed.reference_geometry.section_clipping =
        reference_geometry.value("section_clipping").toBool(false);
    if (reference_geometry.contains("construction_size"))
    {
        parsed.reference_geometry.construction_size =
            reference_geometry.value("construction_size").toDouble(10.0);
    }
    if (reference_geometry.contains("construction_thickness"))
    {
        parsed.reference_geometry.construction_thickness =
            reference_geometry.value("construction_thickness").toDouble(0.01);
    }
    if (reference_geometry.contains("construction_radius"))
    {
        parsed.reference_geometry.construction_radius =
            reference_geometry.value("construction_radius").toDouble(0.05);
    }
    if (parsed.reference_geometry.kind != "file")
    {
        const QVector3D direction = parsed.reference_geometry.construction_direction;
        if (direction.lengthSquared() <= 1.0e-12f ||
            !std::isfinite(parsed.reference_geometry.construction_size) ||
            parsed.reference_geometry.construction_size <= 0.0 ||
            ((parsed.reference_geometry.kind == "datum_plane" ||
              parsed.reference_geometry.kind == "section_plane") &&
             (!std::isfinite(parsed.reference_geometry.construction_thickness) ||
              parsed.reference_geometry.construction_thickness <= 0.0)) ||
            ((parsed.reference_geometry.kind == "datum_axis" ||
              parsed.reference_geometry.kind == "datum_origin" ||
              parsed.reference_geometry.kind == "alignment_frame") &&
             (!std::isfinite(parsed.reference_geometry.construction_radius) ||
              parsed.reference_geometry.construction_radius <= 0.0)))
        {
            set_error(error_message,
                      "Project session contains invalid constructed reference geometry parameters.");
            return false;
        }
    }

    if (!validate(parsed, error_message))
    {
        return false;
    }

    if (data != nullptr)
    {
        *data = std::move(parsed);
    }
    return true;
}
}
