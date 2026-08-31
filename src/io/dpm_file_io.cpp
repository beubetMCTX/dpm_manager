#include "dpm_file_io.h"

#include <QFileInfo>
#include <QRegularExpression>
#define  Kill_Read  *ok=false;delete(in);delete(file);return unit;

dpm_file_io::dpm_file_io() {}

namespace
{
QString make_parse_error(const QString& file_name, const QString& title, const QString& detail)
{
    return QString("Failed to parse '%1' in %2: %3").arg(title, file_name, detail);
}

bool show_parse_error(const QString& file_name, const QString& title, const QString& detail)
{
    QMessageBox::critical(nullptr, "DPM Parse Error", make_parse_error(file_name, title, detail));
    return false;
}

bool validate_dpm_file_path(const QString& file_path, QString* error_message)
{
    if (error_message != nullptr)
    {
        error_message->clear();
    }

    if (file_path.trimmed().isEmpty())
    {
        return false;
    }

    const QFileInfo file_info(file_path);
    if (!file_info.exists() || !file_info.isFile())
    {
        if (error_message != nullptr)
        {
            *error_message = QString("DPM file does not exist or is not a regular file: %1")
                                 .arg(file_path);
        }
        return false;
    }

    if (file_info.size() <= 0)
    {
        if (error_message != nullptr)
        {
            *error_message = QString("DPM file is empty: %1").arg(file_path);
        }
        return false;
    }

    return true;
}

QString normalize_dpm_content(const QString& content)
{
    QString normalized = content;
    if (!normalized.isEmpty() && normalized.front() == QChar::ByteOrderMark)
    {
        normalized.remove(0, 1);
    }
    return normalized.trimmed();
}

bool is_wrapped_unit_list(const QString& content)
{
    if (content.isEmpty() || content.front() != '(')
    {
        return false;
    }

    int index = 1;
    while (index < content.size() && content[index].isSpace())
    {
        ++index;
    }

    return index < content.size() && content[index] == '(';
}

bool looks_like_unit_block(const QString& block)
{
    static const QRegularExpression unit_regex("^\\s*\\(\\s*[^\\s()]+\\s+\\(");
    return unit_regex.match(block).hasMatch();
}

QStringList split_dpm_blocks(const QString& content)
{
    QStringList blocks;
    const QString normalized = normalize_dpm_content(content);
    if (normalized.isEmpty())
    {
        return blocks;
    }

    const bool wrapped_units = is_wrapped_unit_list(normalized);
    const int capture_depth = wrapped_units ? 2 : 1;

    int depth = 0;
    int start = -1;

    for (int i = 0; i < normalized.size(); ++i)
    {
        if (normalized[i] == '(')
        {
            ++depth;
            if (depth == capture_depth)
            {
                start = i;
            }
        }
        else if (normalized[i] == ')')
        {
            if (depth == capture_depth && start >= 0)
            {
                blocks.push_back(normalized.mid(start, i - start + 1));
                start = -1;
            }

            if (depth > 0)
            {
                --depth;
            }
        }
    }

    if (blocks.size() == 1 && !looks_like_unit_block(blocks.front()) &&
        normalized.size() > 2 && normalized.front() == '(' && normalized.back() == ')')
    {
        return split_dpm_blocks(normalized.mid(1, normalized.size() - 2));
    }

    return blocks;
}

bool parse_unit_name(const QString& block, QString& name, const QString& file_name)
{
    static const QRegularExpression head_regex("^\\s*\\(\\s*([^\\s()]+)");
    QRegularExpressionMatch match = head_regex.match(block);
    if (!match.hasMatch())
    {
        return show_parse_error(file_name, "unit-head", "missing unit name");
    }

    name = match.captured(1).trimmed();
    return !name.isEmpty() || show_parse_error(file_name, "unit-head", "empty unit name");
}

bool extract_dot_payload(const QString& block, const QString& title, QString& payload, int occurrence = 0)
{
    const QRegularExpression regex(
        QStringLiteral("\\(\\s*%1\\s*\\.\\s*([^()]*)\\)")
            .arg(QRegularExpression::escape(title)));

    QRegularExpressionMatchIterator iterator = regex.globalMatch(block);
    int current_index = 0;
    while (iterator.hasNext())
    {
        QRegularExpressionMatch match = iterator.next();
        if (current_index == occurrence)
        {
            payload = match.captured(1).trimmed();
            return true;
        }
        ++current_index;
    }

    return false;
}

bool extract_list_payload(const QString& block, const QString& title, QString& payload)
{
    const QRegularExpression regex(
        QStringLiteral("\\(\\s*%1\\b([^()]*)\\)")
            .arg(QRegularExpression::escape(title)));

    QRegularExpressionMatch match = regex.match(block);
    if (!match.hasMatch())
    {
        return false;
    }

    payload = match.captured(1).trimmed();
    return true;
}

template<typename EnumType>
bool parse_enum_contains(const QString& raw_value,
                         EnumType& value,
                         const QList<QPair<QString, EnumType>>& mappings)
{
    const QString normalized = raw_value.trimmed().toLower();
    for (const auto& mapping : mappings)
    {
        if (normalized.contains(mapping.first))
        {
            value = mapping.second;
            return true;
        }
    }

    return false;
}

template<typename T>
struct dpm_scalar_converter;

template<>
struct dpm_scalar_converter<int>
{
    static bool convert(const QString&, const QString& raw_value, int& value)
    {
        bool ok = false;
        value = raw_value.trimmed().toInt(&ok);
        return ok;
    }
};

template<>
struct dpm_scalar_converter<double>
{
    static bool convert(const QString&, const QString& raw_value, double& value)
    {
        bool ok = false;
        value = raw_value.trimmed().toDouble(&ok);
        return ok;
    }
};

template<>
struct dpm_scalar_converter<QString>
{
    static bool convert(const QString& title, const QString& raw_value, QString& value)
    {
        if (title == "dpm-fname")
        {
            value = "\" \"";
            return true;
        }

        value = raw_value.trimmed();
        if (value == "#f")
        {
            value.clear();
        }
        return true;
    }
};

template<>
struct dpm_scalar_converter<bool>
{
    static bool convert(const QString&, const QString& raw_value, bool& value)
    {
        const QString normalized = raw_value.trimmed().toLower();
        if (normalized == "#t")
        {
            value = true;
            return true;
        }
        if (normalized == "#f")
        {
            value = false;
            return true;
        }
        return false;
    }
};

template<>
struct dpm_scalar_converter<DPM_Type>
{
    static bool convert(const QString&, const QString& raw_value, DPM_Type& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"massless", Massless},
            {"inert", Inert},
            {"droplet", Droplet},
            {"combusting", Combusting},
            {"multicomponent", Multicomponent}
        });
    }
};

template<>
struct dpm_scalar_converter<Injection_Type>
{
    static bool convert(const QString&, const QString& raw_value, Injection_Type& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"single", single},
            {"group", group},
            {"surface", surface},
            {"volume", volume},
            {"cone", cone},
            {"plain-orifice-atomizer", plain_oriface_atomizer},
            {"pressure-swirl-atomizer", pressure_swirl_atomizer},
            {"air-blast_atomizer", air_blast_atomizer},
            {"flat-fan-atomizer", flat_fan_atomizer},
            {"effervescent-atomizer", effervescent_atomizer},
            {"file", file_},
            {"condensate", condensate}
        });
    }
};

template<>
struct dpm_scalar_converter<Cone_Type>
{
    static bool convert(const QString&, const QString& raw_value, Cone_Type& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"point", point},
            {"hollow", hollow},
            {"ring", ring},
            {"solid", solid}
        });
    }
};

template<>
struct dpm_scalar_converter<Parcel_Model>
{
    static bool convert(const QString&, const QString& raw_value, Parcel_Model& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"0", standard},
            {"1", const_number},
            {"2", const_mass},
            {"3", const_diameter}
        });
    }
};

template<>
struct dpm_scalar_converter<Drag_Law>
{
    static bool convert(const QString&, const QString& raw_value, Drag_Law& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"spherical", spherical},
            {"nonspherical", nonspherical},
            {"strokes", Strokes_Cunningham},
            {"mach", high_Mach_number},
            {"dynamic", dynamic_drag}
        });
    }
};

template<>
struct dpm_scalar_converter<Volume_Streams_Spec>
{
    static bool convert(const QString&, const QString& raw_value, Volume_Streams_Spec& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"total", total_parcel_count},
            {"cell", parcel_per_cell}
        });
    }
};

template<>
struct dpm_scalar_converter<Volume_Specification>
{
    static bool convert(const QString&, const QString& raw_value, Volume_Specification& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"zone", zone},
            {"bouning", bouning_geometry}
        });
    }
};

template<>
struct dpm_scalar_converter<Volume_Bgeom_Shapes>
{
    static bool convert(const QString&, const QString& raw_value, Volume_Bgeom_Shapes& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"sphere", sphere},
            {"cylinder", cylinder},
            {"cone", cone_},
            {"hex", hexahedron}
        });
    }
};

template<>
struct dpm_scalar_converter<Rot_Drag_Law>
{
    static bool convert(const QString&, const QString& raw_value, Rot_Drag_Law& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"dennis", Dennis_et_al},
            {"none", none}
        });
    }
};

template<>
struct dpm_scalar_converter<Rot_Lift_Law>
{
    static bool convert(const QString&, const QString& raw_value, Rot_Lift_Law& value)
    {
        return parse_enum_contains(raw_value, value, {
            {"oest", Oesterle_Bui_Dinh},
            {"tsuji", Tsuji_et_al},
            {"rubinow", Rubinow_Keller},
            {"none", none_}
        });
    }
};

template<typename T>
bool parse_dpm_dot_field(const QString& block,
                         const QString& title,
                         T& value,
                         const QString& file_name,
                         int occurrence = 0)
{
    QString payload;
    if (!extract_dot_payload(block, title, payload, occurrence))
    {
        return show_parse_error(file_name, title, "field not found");
    }

    if (!dpm_scalar_converter<T>::convert(title, payload, value))
    {
        return show_parse_error(file_name, title, QString("invalid value '%1'").arg(payload));
    }

    return true;
}

bool parse_dpm_list_field(const QString& block,
                          const QString& title,
                          QVector<int>& values,
                          const QString& file_name)
{
    QString payload;
    if (!extract_list_payload(block, title, payload))
    {
        return show_parse_error(file_name, title, "field not found");
    }

    QString normalized = payload;
    normalized.remove('(');
    normalized.remove(')');
    normalized = normalized.trimmed();

    values.clear();
    if (normalized.isEmpty())
    {
        return true;
    }

    if (normalized.startsWith('.'))
    {
        values.push_back(-1);
        return true;
    }

    const QStringList tokens = normalized.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString& token : tokens)
    {
        bool ok = false;
        const int parsed = token.toInt(&ok);
        if (!ok)
        {
            return show_parse_error(file_name, title, QString("invalid list token '%1'").arg(token));
        }
        values.push_back(parsed);
    }

    return true;
}

bool parse_dpm_repeated_vector3_field(const QString& block,
                                      const QString& title,
                                      QVector3D& value,
                                      const QString& file_name)
{
    double x_value = 0.0;
    double y_value = 0.0;
    double z_value = 0.0;

    if (!parse_dpm_dot_field(block, title, x_value, file_name, 0) ||
        !parse_dpm_dot_field(block, title, y_value, file_name, 1) ||
        !parse_dpm_dot_field(block, title, z_value, file_name, 2))
    {
        return false;
    }

    value.setX(static_cast<float>(x_value));
    value.setY(static_cast<float>(y_value));
    value.setZ(static_cast<float>(z_value));
    return true;
}

bool parse_dpm_named_vector3_field(const QString& block,
                                   const QString& x_title,
                                   const QString& y_title,
                                   const QString& z_title,
                                   QVector3D& value,
                                   const QString& file_name)
{
    double x_value = 0.0;
    double y_value = 0.0;
    double z_value = 0.0;

    if (!parse_dpm_dot_field(block, x_title, x_value, file_name) ||
        !parse_dpm_dot_field(block, y_title, y_value, file_name) ||
        !parse_dpm_dot_field(block, z_title, z_value, file_name))
    {
        return false;
    }

    value.setX(static_cast<float>(x_value));
    value.setY(static_cast<float>(y_value));
    value.setZ(static_cast<float>(z_value));
    return true;
}

bool parse_dpm_prefixed_vector3_field(const QString& block,
                                      const QString& title,
                                      QVector3D& value,
                                      const QString& file_name)
{
    return parse_dpm_named_vector3_field(block,
                                         "x-" + title,
                                         "y-" + title,
                                         "z-" + title,
                                         value,
                                         file_name);
}

bool parse_dpm_suffixed_vector3_field(const QString& block,
                                      const QString& title,
                                      QVector3D& value,
                                      const QString& file_name)
{
    return parse_dpm_named_vector3_field(block,
                                         title + "-x",
                                         title + "-y",
                                         title + "-z",
                                         value,
                                         file_name);
}

struct ScalarFieldRule
{
    const char* title;
    bool (*apply)(const QString& block, Injector& injector, const QString& file_name, const char* title);
};

template<typename T, T Injector::*Member>
bool apply_scalar_rule(const QString& block, Injector& injector, const QString& file_name, const char* title)
{
    return parse_dpm_dot_field(block, QString::fromLatin1(title), injector.*Member, file_name);
}

struct ListFieldRule
{
    const char* title;
    QVector<int> Injector::*member;
};

struct VectorFieldRule
{
    const char* x_title;
    const char* y_title;
    const char* z_title;
    QVector3D Injector::*member;
};

bool parse_dpm_unit_block(const QString& block, Unit& unit, const QString& file_name)
{
    auto& injector = unit.inj.injector_data;

    if (!parse_unit_name(block, injector.name, file_name)) return false;

    static const ScalarFieldRule scalar_rules[] = {
        {"type", &apply_scalar_rule<DPM_Type, &Injector::type>},
        {"injection-type", &apply_scalar_rule<Injection_Type, &Injector::injection_type>},
        {"local-reference-frame", &apply_scalar_rule<QString, &Injector::local_reference_frame>},
        {"numpts", &apply_scalar_rule<int, &Injector::numpts>},
        {"dpm-fname", &apply_scalar_rule<QString, &Injector::dpm_fname>},
        {"stochastic-on", &apply_scalar_rule<bool, &Injector::stochastic>},
        {"random-eddy-on", &apply_scalar_rule<bool, &Injector::random_eddy>},
        {"ntries", &apply_scalar_rule<int, &Injector::ntries>},
        {"time-scale-constant", &apply_scalar_rule<double, &Injector::time_scale_constant>},
        {"cloud-on", &apply_scalar_rule<bool, &Injector::cloud>},
        {"cloud-min-dia", &apply_scalar_rule<double, &Injector::cloud_min_dia>},
        {"cloud-max-dia", &apply_scalar_rule<double, &Injector::cloud_max_dia>},
        {"material", &apply_scalar_rule<QString, &Injector::material>},
        {"scale-by-area", &apply_scalar_rule<bool, &Injector::scale_by_area>},
        {"use-face-normal", &apply_scalar_rule<bool, &Injector::use_face_normal>},
        {"random-surface?", &apply_scalar_rule<bool, &Injector::random_surface>},
        {"tabulated-diam-dist?", &apply_scalar_rule<bool, &Injector::tabulated_diam_dist>},
        {"tabulated-diam-table-name", &apply_scalar_rule<QString, &Injector::tabulated_diam_table_name>},
        {"tabulated-diam-ref-diam-col", &apply_scalar_rule<int, &Injector::tabulated_diam_ref_diam_col>},
        {"tabulated-diam-num-frac-col", &apply_scalar_rule<int, &Injector::tabulated_diam_num_frac_col>},
        {"tabulated-diam-mas-frac-col", &apply_scalar_rule<int, &Injector::tabulated_diam_mas_frac_col>},
        {"tabulated-diam-num-frac-accum?", &apply_scalar_rule<bool, &Injector::tabulated_diam_num_frac_accum>},
        {"tabulated-diam-mas-frac-accum?", &apply_scalar_rule<bool, &Injector::tabulated_diam_mas_frac_accum>},
        {"devolatilizing-species", &apply_scalar_rule<QString, &Injector::devolatilizing_species>},
        {"evaporating-species", &apply_scalar_rule<QString, &Injector::evaporating_species>},
        {"oxidizing-species", &apply_scalar_rule<QString, &Injector::oxidizing_species>},
        {"product-species", &apply_scalar_rule<QString, &Injector::product_species>},
        {"rr-distrib", &apply_scalar_rule<bool, &Injector::rr_disturb>},
        {"rr-uniform-ln-d", &apply_scalar_rule<bool, &Injector::rr_uniform_ln_d>},
        {"evaporating-liquid-on", &apply_scalar_rule<bool, &Injector::evaporating_liquid>},
        {"evaporating-material", &apply_scalar_rule<QString, &Injector::evaporating_material>},
        {"liquid-fraction", &apply_scalar_rule<double, &Injector::liquid_fraction>},
        {"dpm-domain", &apply_scalar_rule<QString, &Injector::dpm_domain>},
        {"collision-partner", &apply_scalar_rule<QString, &Injector::collision_partner>},
        {"parcel-number", &apply_scalar_rule<int, &Injector::parcel_number>},
        {"parcel-mass", &apply_scalar_rule<double, &Injector::parcel_mass>},
        {"parcel-diameter", &apply_scalar_rule<double, &Injector::parcel_diameter>},
        {"parcel-model", &apply_scalar_rule<Parcel_Model, &Injector::parcel_model>},
        {"drag-law", &apply_scalar_rule<Drag_Law, &Injector::drag_law>},
        {"shape-factor", &apply_scalar_rule<double, &Injector::shape_factor>},
        {"cunningham-correction", &apply_scalar_rule<double, &Injector::cunningham_correction>},
        {"drag-fcn", &apply_scalar_rule<QString, &Injector::drag_fcn>},
        {"brownian-motion", &apply_scalar_rule<bool, &Injector::brownian_motion>},
        {"seco-breakup-on?", &apply_scalar_rule<bool, &Injector::seco_breakup_on>},
        {"seco-breakup-tab?", &apply_scalar_rule<bool, &Injector::seco_breakup_tab>},
        {"seco-breakup-wave?", &apply_scalar_rule<bool, &Injector::seco_breakup_wave>},
        {"seco-breakup-khrt?", &apply_scalar_rule<bool, &Injector::seco_break_up_khrt>},
        {"seco-breakup-ssd?", &apply_scalar_rule<bool, &Injector::seco_breakup_ssd>},
        {"seco-breakup-madabhushi?", &apply_scalar_rule<bool, &Injector::seco_breakup_madahushi>},
        {"seco-breakup-schmehl?", &apply_scalar_rule<bool, &Injector::seco_breakup_schmehl>},
        {"seco-breakup-tab-y0", &apply_scalar_rule<double, &Injector::seco_breakup_tab_y0>},
        {"number-tab-diameters", &apply_scalar_rule<int, &Injector::number_tab_diameters>},
        {"seco-breakup-wave-b1", &apply_scalar_rule<double, &Injector::seco_breakup_wave_b1>},
        {"seco-breakup-wave-b0", &apply_scalar_rule<double, &Injector::seco_breakup_wave_b0>},
        {"seco-breakup-khrt-cl", &apply_scalar_rule<double, &Injector::seco_breakup_khrt_cl>},
        {"seco-breakup-khrt-ctau", &apply_scalar_rule<double, &Injector::seco_breakup_khrt_ctau>},
        {"seco-breakup-khrt-crt", &apply_scalar_rule<double, &Injector::seco_breakup_khrt_crt>},
        {"seco-breakup-ssd-we-cr", &apply_scalar_rule<double, &Injector::seco_breakup_ssd_we_cr>},
        {"seco-breakup-ssd-core-bu", &apply_scalar_rule<double, &Injector::seco_breakup_ssd_core_bu>},
        {"seco-breakup-ssd-np-target", &apply_scalar_rule<double, &Injector::seco_breakup_ssd_np_target>},
        {"seco-breakup-ssd-x-si", &apply_scalar_rule<double, &Injector::seco_breakup_ssd_x_si>},
        {"seco-breakup-madabhushi-c0", &apply_scalar_rule<double, &Injector::seco_breakup_madabushi_c0>},
        {"seco-breakup-madabhushi-column-drag-cd", &apply_scalar_rule<double, &Injector::seco_breakup_madabushi_column_drag_cd>},
        {"seco-breakup-madabhushi-ligament-factor", &apply_scalar_rule<double, &Injector::seco_breakup_madabushi_ligament_factor>},
        {"seco-breakup-madabhushi-jet-diameter", &apply_scalar_rule<double, &Injector::seco_breakup_madabushi_jet_diameter>},
        {"seco-breakup-schmehl-np", &apply_scalar_rule<double, &Injector::seco_breakup_schmehl_np>},
        {"volume-specification", &apply_scalar_rule<Volume_Specification, &Injector::volume_specification>},
        {"volume-streams-spec", &apply_scalar_rule<Volume_Streams_Spec, &Injector::volume_streams_spec>},
        {"volume-streams-total", &apply_scalar_rule<int, &Injector::volume_streams_total>},
        {"volume-streams-per-cell", &apply_scalar_rule<int, &Injector::volume_streams_per_cell>},
        {"volume-packing-limit-per-cell", &apply_scalar_rule<double, &Injector::volume_packing_limit_per_cell>},
        {"volume-bgeom-shapes", &apply_scalar_rule<Volume_Bgeom_Shapes, &Injector::volume_bgeom_shapes>},
        {"volume-bgeom-radius", &apply_scalar_rule<double, &Injector::volume_bgeom_radius>},
        {"volume-bgeom-viconeangle", &apply_scalar_rule<double, &Injector::volume_bgeom_viconeangle>},
        {"mass-input-on", &apply_scalar_rule<bool, &Injector::mass_input_on>},
        {"volfrac-input-on", &apply_scalar_rule<bool, &Injector::volfrac_input_on>},
        {"rotation-on?", &apply_scalar_rule<bool, &Injector::rotation_on>},
        {"rot-drag-law", &apply_scalar_rule<Rot_Drag_Law, &Injector::rot_drag_law>},
        {"rot-lift-law", &apply_scalar_rule<Rot_Lift_Law, &Injector::rot_lift_law>},
        {"cone-type", &apply_scalar_rule<Cone_Type, &Injector::cone_type>},
        {"uniform-mass-dist-on?", &apply_scalar_rule<bool, &Injector::uniform_mass_dist_on>},
        {"spatial-staggering/std-inj/on?", &apply_scalar_rule<bool, &Injector::spatial_staggering_std_inj_on>},
        {"spatial-staggering/atomizer/on?", &apply_scalar_rule<bool, &Injector::spatial_staggering_atomizer_on>},
        {"stagger-radius", &apply_scalar_rule<double, &Injector::stagger_radius>},
        {"rough-wall-on?", &apply_scalar_rule<bool, &Injector::rough_wall_on>},
        {"cphase-domain", &apply_scalar_rule<QString, &Injector::cphace_domain>},
        {"diameter", &apply_scalar_rule<double, &Injector::diameter>},
        {"diameter2", &apply_scalar_rule<double, &Injector::diameter2>},
        {"temperature", &apply_scalar_rule<double, &Injector::temperature>},
        {"temperature2", &apply_scalar_rule<double, &Injector::temperature2>},
        {"flow-rate", &apply_scalar_rule<double, &Injector::flow_rate>},
        {"flow-rate2", &apply_scalar_rule<double, &Injector::flow_rate2>},
        {"unsteady-start", &apply_scalar_rule<double, &Injector::unsteady_start>},
        {"unsteady-stop", &apply_scalar_rule<double, &Injector::unsteady_stop>},
        {"start-at-flow-time-in-unsteady-inj-file", &apply_scalar_rule<double, &Injector::start_at_flow_time_in_unsteady_inj_file>},
        {"interval-to-repeat-in-unsteady-inj-file", &apply_scalar_rule<double, &Injector::interval_to_repeat_in_unsteady_inj_file>},
        {"unsteady-ca-start", &apply_scalar_rule<double, &Injector::unsteady_ca_start>},
        {"unsteady-ca-stop", &apply_scalar_rule<double, &Injector::unsteady_ca_stop>},
        {"vapor-pressure", &apply_scalar_rule<double, &Injector::vapor_pressure>},
        {"inner-diameter", &apply_scalar_rule<double, &Injector::inner_diameter>},
        {"outer-diameter", &apply_scalar_rule<double, &Injector::outer_diameter>},
        {"half-angle", &apply_scalar_rule<double, &Injector::half_angle>},
        {"plain-length", &apply_scalar_rule<double, &Injector::plain_length>},
        {"plain-corner-size", &apply_scalar_rule<double, &Injector::plain_corner_size>},
        {"plain-const-a", &apply_scalar_rule<double, &Injector::plain_const_a>},
        {"pswirl-inj-press", &apply_scalar_rule<double, &Injector::pswirl_inj_press>},
        {"airbl-rel-vel", &apply_scalar_rule<double, &Injector::airbl_rel_vel>},
        {"effer-quality", &apply_scalar_rule<double, &Injector::effer_quality>},
        {"effer-t-sat", &apply_scalar_rule<double, &Injector::effer_t_sat>},
        {"ff-orifice-width", &apply_scalar_rule<double, &Injector::ff_oriface_width>},
        {"phi-start", &apply_scalar_rule<double, &Injector::phi_start>},
        {"phi-stop", &apply_scalar_rule<double, &Injector::phi_stop>},
        {"sheet-const", &apply_scalar_rule<double, &Injector::sheet_const>},
        {"lig-const", &apply_scalar_rule<double, &Injector::lig_const>},
        {"effer-const", &apply_scalar_rule<double, &Injector::effer_const>},
        {"effer-half-angle-max", &apply_scalar_rule<double, &Injector::effer_half_angle_max>},
        {"ff-sheet-const", &apply_scalar_rule<double, &Injector::ff_sheet_const>},
        {"atomizer-disp-angle", &apply_scalar_rule<double, &Injector::atomizer_disp_angle>},
        {"vel-mag", &apply_scalar_rule<double, &Injector::vel_mag>},
        {"ang-vel-mag", &apply_scalar_rule<double, &Injector::ang_vel_mag>},
        {"cone-angle", &apply_scalar_rule<double, &Injector::cone_angle>},
        {"inner-radius", &apply_scalar_rule<double, &Injector::inner_radius>},
        {"radius", &apply_scalar_rule<double, &Injector::radius>},
        {"swirl-frac", &apply_scalar_rule<double, &Injector::swirl_frac>},
        {"total-flow-rate", &apply_scalar_rule<double, &Injector::total_flow_rate>},
        {"total-mass", &apply_scalar_rule<double, &Injector::total_mass>},
        {"volume-fraction", &apply_scalar_rule<double, &Injector::volume_fraction>},
        {"rr-min", &apply_scalar_rule<double, &Injector::rr_min>},
        {"rr-max", &apply_scalar_rule<double, &Injector::rr_max>},
        {"rr-mean", &apply_scalar_rule<double, &Injector::rr_mean>},
        {"rr-spread", &apply_scalar_rule<double, &Injector::rr_spread>},
        {"rr-numdia", &apply_scalar_rule<int, &Injector::rr_numdia>}
    };

    static const ListFieldRule list_rules[] = {
        {"surfaces", &Injector::surfaces},
        {"boundary", &Injector::boundary},
        {"volume-zones", &Injector::volume_zones}
    };

    static const VectorFieldRule vector_rules[] = {
        {"volume-bgeom-xmin", "volume-bgeom-ymin", "volume-bgeom-zmin", &Injector::volume_bgeom_min},
        {"volume-bgeom-xmax", "volume-bgeom-ymax", "volume-bgeom-zmax", &Injector::volume_bgeom_max},
        {"x-pos", "y-pos", "z-pos", &Injector::pos},
        {"x-pos2", "y-pos2", "z-pos2", &Injector::pos2},
        {"ff-center-x", "ff-center-y", "ff-center-z", &Injector::ff_center},
        {"ff-virtual-origin-x", "ff-virtual-origin-y", "ff-virtual-origin-z", &Injector::ff_virtual_origin},
        {"ff-normal-x", "ff-normal-y", "ff-normal-z", &Injector::ff_normal},
        {"x-vel", "y-vel", "z-vel", &Injector::vel},
        {"x-vel2", "y-vel2", "z-vel2", &Injector::vel2},
        {"x-ang-vel", "y-ang-vel", "z-ang-vel", &Injector::ang_vel},
        {"x-ang-vel2", "y-ang-vel2", "z-ang-vel2", &Injector::ang_vel2},
        {"atomizer-x-axis", "atomizer-y-axis", "atomizer-z-axis", &Injector::atomizer_axis},
        {"x-axis", "y-axis", "z-axis", &Injector::axis},
        {"x-posr", "y-posr", "z-posr", &Injector::posr},
        {"x-posu", "y-posu", "z-posu", &Injector::posu}
    };

    for (const ScalarFieldRule& rule : scalar_rules)
    {
        if (!rule.apply(block, injector, file_name, rule.title))
        {
            return false;
        }
    }

    for (const ListFieldRule& rule : list_rules)
    {
        if (!parse_dpm_list_field(block, QString::fromLatin1(rule.title), injector.*(rule.member), file_name))
        {
            return false;
        }
    }

    for (const VectorFieldRule& rule : vector_rules)
    {
        if (!parse_dpm_named_vector3_field(block,
                                           QString::fromLatin1(rule.x_title),
                                           QString::fromLatin1(rule.y_title),
                                           QString::fromLatin1(rule.z_title),
                                           injector.*(rule.member),
                                           file_name))
        {
            return false;
        }
    }

    return true;
}
} // namespace


[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm_head(QFile *file,QTextStream *in,QString &name)
{
    QChar space,space2;
    QChar leftblanket;
    *in>>space>>leftblanket>>name>>space2;
    qDebug()<<name;
    if(leftblanket=='(')
    {
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\n开头:时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,int &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')')  return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nint变量：\n"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,double &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')') return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\ndouble变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,QString &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<"!";
    if(title=="dpm-fname")
    {
        *in>>data;
        data="\" \"";
        qDebug()<<data;
        return true;
    }
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data=="#f") data.clear();
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nstring变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,QVector3D &vect,Coord coord)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    float data;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp.contains(title,Qt::CaseSensitivity::CaseInsensitive)&&dot=='.'&&rightblanket==')')
    {
        switch(coord)
        {

        case x:vect.setX(data);break;
        case y:vect.setY(data);break;
        case z:vect.setZ(data);break;

        }

        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nvector变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,DPM_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("massless",Qt::CaseInsensitive)) enum_data=Massless;
        else if(data.contains("inert",Qt::CaseInsensitive)) enum_data=Inert;
        else if(data.contains("droplet",Qt::CaseInsensitive)) enum_data=Droplet;
        else if(data.contains("combusting",Qt::CaseInsensitive)) enum_data=Combusting;
        else if(data.contains("multicomponent",Qt::CaseInsensitive)) enum_data=Multicomponent;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量DPM_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Injection_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
             if(data.contains("single",Qt::CaseInsensitive)) enum_data=single;
        else if(data.contains("group",Qt::CaseInsensitive)) enum_data=group;
        else if(data.contains("surface",Qt::CaseInsensitive)) enum_data=surface;
        else if(data.contains("volume",Qt::CaseInsensitive)) enum_data=volume;
        else if(data.contains("cone",Qt::CaseInsensitive)) enum_data=cone;
        else if(data.contains("plain-orifice-atomizer",Qt::CaseInsensitive)) enum_data=plain_oriface_atomizer;
        else if(data.contains("pressure-swirl-atomizer",Qt::CaseInsensitive)) enum_data=pressure_swirl_atomizer;
        else if(data.contains("air-blast_atomizer",Qt::CaseInsensitive)) enum_data=air_blast_atomizer;
        else if(data.contains("flat-fan-atomizer",Qt::CaseInsensitive)) enum_data=flat_fan_atomizer;
        else if(data.contains("effervescent-atomizer",Qt::CaseInsensitive)) enum_data=effervescent_atomizer;
        else if(data.contains("file",Qt::CaseInsensitive)) enum_data=file_;
        else if(data.contains("condensate",Qt::CaseInsensitive)) enum_data=condensate;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Injection_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Cone_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("point",Qt::CaseInsensitive)) enum_data=point;
        else if(data.contains("hollow",Qt::CaseInsensitive)) enum_data=hollow;
        else if(data.contains("ring",Qt::CaseInsensitive)) enum_data=ring;
        else if(data.contains("solid",Qt::CaseInsensitive)) enum_data=solid;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Cone_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Parcel_Model &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("0")) enum_data=standard;
        else if(data.contains("1")) enum_data=const_number;
        else if(data.contains("2")) enum_data=const_mass;
        else if(data.contains("3")) enum_data=const_diameter;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Parcel_Model时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Drag_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("spherical",Qt::CaseInsensitive)) enum_data=spherical;
        else if(data.contains("nonspherical",Qt::CaseInsensitive)) enum_data=nonspherical;
        else if(data.contains("Strokes",Qt::CaseInsensitive)) enum_data=Strokes_Cunningham;
        else if(data.contains("mach",Qt::CaseInsensitive)) enum_data=high_Mach_number;
        else if(data.contains("dynamic",Qt::CaseInsensitive)) enum_data=dynamic_drag;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量DPM_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nstring变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Streams_Spec &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("total",Qt::CaseInsensitive)) enum_data=total_parcel_count;
        else if(data.contains("cell",Qt::CaseInsensitive)) enum_data=parcel_per_cell;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Stream_Spec时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Specification &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("zone",Qt::CaseInsensitive)) enum_data=zone;
        else if(data.contains("bouning",Qt::CaseInsensitive)) enum_data=bouning_geometry;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Specification时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Bgeom_Shapes &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("sphere",Qt::CaseInsensitive)) enum_data=sphere;
        else if(data.contains("cylinder",Qt::CaseInsensitive)) enum_data=cylinder;
        else if(data.contains("cone",Qt::CaseInsensitive)) enum_data=cone_;
        else if(data.contains("hex",Qt::CaseInsensitive)) enum_data=hexahedron;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Bgeom_Shapes时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Rot_Drag_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("dennis",Qt::CaseInsensitive)) enum_data=Dennis_et_al;
        else if(data.contains("none",Qt::CaseInsensitive)) enum_data=none;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Rot_Drag_Law时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,Rot_Lift_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("oest",Qt::CaseInsensitive)) enum_data=Oesterle_Bui_Dinh;
        else if(data.contains("tsuji",Qt::CaseInsensitive)) enum_data=Tsuji_et_al;
        else if(data.contains("rubinow",Qt::CaseInsensitive)) enum_data=Rubinow_Keller;
        else if(data.contains("none",Qt::CaseInsensitive)) enum_data=none_;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Rot_Lift_Law时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,bool &bool_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("#f",Qt::CaseInsensitive)) bool_data=false;
        else if(data.contains("#t",Qt::CaseInsensitive)) bool_data=true;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nbool变量"+title+"时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nbool变量:"+title+"时出现错误");
        return false;
    }
}

[[deprecated("Use regex parser helpers instead.")]]
bool read_dpm(QFile *file,QTextStream *in,const QString title,QVector<int> vect)
{
    QChar space;
    QChar leftblanket;
    QString temp,data;
    vect.clear();
    if(title=="surfaces")
    {
        *in>>space>>leftblanket>>temp>>data;
        if(data.contains("."))
        {
            vect.push_back(-1);
            *in>>data;
            qDebug()<<data;
        }
        else
        {
            while(1)
            {

                if(data.contains(")"))
                {
                    data.chop(1);
                    vect.push_back(data.toInt());
                    //qDebug()<<data.toInt();
                    break;
                }
                else
                {
                    vect.push_back(data.toInt());
                    *in>>data;
                }
                //qDebug()<<data.toInt();
            }
        }
        return true;
    }
    else if(title=="boundary")
    {
        *in>>space>>leftblanket>>temp>>data;
        while(1)
        {
            if(data.contains(")"))
            {
                data.chop(1);
                vect.push_back(data.toInt());
                //qDebug()<<data;
                break;
            }
            else
            {
                vect.push_back(data.toInt());
                //qDebug()<<data;
                *in>>data;
            }
        }
        return true;
    }
    else if(title=="volume-zones")
    {
        *in>>temp;
        if(temp.contains(")")) return true;
        else
        {
            while(1)
            {
                *in>>data;
                if(data.contains(")"))
                {
                    data.chop(1);
                    vect.push_back(data.toInt());
                    //qDebug()<<data;
                    break;
                }
                else
                {
                    vect.push_back(data.toInt());
                    //qDebug()<<data;
                    *in>>data;
                }
            }
            return true;
        }
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nvector<int>变量:"+title+"时出现错误");
        return false;
    }
}

QString Read_File_Dialog()
{
    QString file_path = QFileDialog::getOpenFileName(
        nullptr,
        "选择文件",
        ".",
        "DPM Files (*.dpm);;Text Files (*.txt);;All Files (*.*)");
    qDebug()<<file_path;
    return file_path;
}


[[deprecated("Use regex parser helpers instead.")]]
void Ignore_input(QTextStream *in,int ignore_number)
{
    QString T;
    int i=0;
    while(1)
    {
        *in>>T;
        if(T.contains(")"))
        {
            i+=T.count(')');
            if(i>=ignore_number) break;
        }
    }

}

[[deprecated("Use regex parser helpers instead.")]]
bool read_end(QTextStream *in,QString name)
{
    QString temp;
    *in>>temp;
    if(!in->atEnd())
    {
        qDebug()<<temp<<name<<"not end";
        return false;
    }
    else
    {
        qDebug()<<temp<<name<<"end";
        return true;
    }
    // else
    // {
    //     QMessageBox::critical(nullptr, "错误", "在读取"+unit.inj->name+"文件尾时出现错误");
    //     return false;
    // }
}

[[deprecated("Use read_dpm_file(file_path, ...) instead.")]]
QList<Unit> read_single_dpm_file(bool *ok)
{
    return read_single_dpm_file_regex(ok);

    QFile *file=new QFile(Read_File_Dialog());
    QList<Unit> unit;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        //QMessageBox::critical(nullptr, "错误", "无法打开文件: " + file->errorString());
        *ok=false;
        delete(file);
        return unit;
    }
    else
    {
        QTextStream *in=new QTextStream(file);
        Unit iterator;
        while(1)
        {
            if(!read_dpm_head(file,in,iterator.inj.injector_data.name)){Kill_Read};
            if(!read_dpm(file,in,"type",iterator.inj.injector_data.type)){Kill_Read};
            if(!read_dpm(file,in,"injection-type",iterator.inj.injector_data.injection_type)){Kill_Read};
            if(!read_dpm(file,in,"local-reference-frame",iterator.inj.injector_data.local_reference_frame)){Kill_Read};
            if(!read_dpm(file,in,"numpts",iterator.inj.injector_data.numpts)){Kill_Read};
            if(!read_dpm(file,in,"dpm-fname",iterator.inj.injector_data.dpm_fname)){Kill_Read};
            if(!read_dpm(file,in,"surfaces",iterator.inj.injector_data.surfaces)){Kill_Read};
            if(!read_dpm(file,in,"boundary",iterator.inj.injector_data.boundary)){Kill_Read};
            //基础配置
            if (!read_dpm(file, in, "stochastic-on", iterator.inj.injector_data.stochastic)) { Kill_Read };
            if (!read_dpm(file, in, "random-eddy-on", iterator.inj.injector_data.random_eddy)) { Kill_Read };
            if (!read_dpm(file, in, "ntries", iterator.inj.injector_data.ntries)) { Kill_Read };
            if (!read_dpm(file, in, "time-scale-constant", iterator.inj.injector_data.time_scale_constant)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-on", iterator.inj.injector_data.cloud)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-min-dia", iterator.inj.injector_data.cloud_min_dia)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-max-dia", iterator.inj.injector_data.cloud_max_dia)) { Kill_Read };
            if (!read_dpm(file, in, "material", iterator.inj.injector_data.material)) { Kill_Read };
            if (!read_dpm(file, in, "scale-by-area", iterator.inj.injector_data.scale_by_area)) { Kill_Read };
            if (!read_dpm(file, in, "use-face-normal", iterator.inj.injector_data.use_face_normal)) { Kill_Read };
            if (!read_dpm(file, in, "random-surface?", iterator.inj.injector_data.random_surface)) { Kill_Read };
            //表格雾化
            if (!read_dpm(file, in, "tabulated-diam-dist?", iterator.inj.injector_data.tabulated_diam_dist)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-table-name", iterator.inj.injector_data.tabulated_diam_table_name)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-ref-diam-col", iterator.inj.injector_data.tabulated_diam_ref_diam_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-num-frac-col", iterator.inj.injector_data.tabulated_diam_num_frac_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-mas-frac-col", iterator.inj.injector_data.tabulated_diam_mas_frac_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-num-frac-accum?", iterator.inj.injector_data.tabulated_diam_num_frac_accum)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-mas-frac-accum?", iterator.inj.injector_data.tabulated_diam_mas_frac_accum)) { Kill_Read };
            // 组分与分布
            if (!read_dpm(file, in, "devolatilizing-species", iterator.inj.injector_data.devolatilizing_species)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-species", iterator.inj.injector_data.evaporating_species)) { Kill_Read };
            if (!read_dpm(file, in, "oxidizing-species", iterator.inj.injector_data.oxidizing_species)) { Kill_Read };
            if (!read_dpm(file, in, "product-species", iterator.inj.injector_data.product_species)) { Kill_Read };
            if (!read_dpm(file, in, "rr-distrib", iterator.inj.injector_data.rr_disturb)) { Kill_Read };
            if (!read_dpm(file, in, "rr-uniform-ln-d", iterator.inj.injector_data.rr_uniform_ln_d)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-liquid-on", iterator.inj.injector_data.evaporating_liquid)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-material", iterator.inj.injector_data.evaporating_material)) { Kill_Read };
            if (!read_dpm(file, in, "liquid-fraction", iterator.inj.injector_data.liquid_fraction)) { Kill_Read };
            // DPM域与碰撞
            if (!read_dpm(file, in, "dpm-domain", iterator.inj.injector_data.dpm_domain)) { Kill_Read };
            if (!read_dpm(file, in, "collision-partner", iterator.inj.injector_data.collision_partner)) { Kill_Read };
            //multiple-surface
            Ignore_input(in,1);
            // 颗粒聚团模型
            if (!read_dpm(file, in, "parcel-number", iterator.inj.injector_data.parcel_number)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-mass", iterator.inj.injector_data.parcel_mass)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-diameter", iterator.inj.injector_data.parcel_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-model", iterator.inj.injector_data.parcel_model)) { Kill_Read };

            // 曳力与运动
            if (!read_dpm(file, in, "drag-law", iterator.inj.injector_data.drag_law)) { Kill_Read };
            if (!read_dpm(file, in, "shape-factor", iterator.inj.injector_data.shape_factor)) { Kill_Read };
            if (!read_dpm(file, in, "cunningham-correction", iterator.inj.injector_data.cunningham_correction)) { Kill_Read };
            if (!read_dpm(file, in, "drag-fcn", iterator.inj.injector_data.drag_fcn)) { Kill_Read };

            //htc
            Ignore_input(in,3);

            if (!read_dpm(file, in, "brownian-motion", iterator.inj.injector_data.brownian_motion)) { Kill_Read };

            // 颗粒破碎模型
            if (!read_dpm(file, in, "seco-breakup-on?", iterator.inj.injector_data.seco_breakup_on)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-tab?", iterator.inj.injector_data.seco_breakup_tab)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave?", iterator.inj.injector_data.seco_breakup_wave)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt?", iterator.inj.injector_data.seco_break_up_khrt)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd?", iterator.inj.injector_data.seco_breakup_ssd)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi?", iterator.inj.injector_data.seco_breakup_madahushi)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-schmehl?", iterator.inj.injector_data.seco_breakup_schmehl)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-tab-y0", iterator.inj.injector_data.seco_breakup_tab_y0)) { Kill_Read };
            if (!read_dpm(file, in, "number-tab-diameters", iterator.inj.injector_data.number_tab_diameters)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave-b1", iterator.inj.injector_data.seco_breakup_wave_b1)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave-b0", iterator.inj.injector_data.seco_breakup_wave_b0)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-cl", iterator.inj.injector_data.seco_breakup_khrt_cl)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-ctau", iterator.inj.injector_data.seco_breakup_khrt_ctau)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-crt", iterator.inj.injector_data.seco_breakup_khrt_crt)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-we-cr", iterator.inj.injector_data.seco_breakup_ssd_we_cr)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-core-bu", iterator.inj.injector_data.seco_breakup_ssd_core_bu)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-np-target", iterator.inj.injector_data.seco_breakup_ssd_np_target)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-x-si", iterator.inj.injector_data.seco_breakup_ssd_x_si)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-c0", iterator.inj.injector_data.seco_breakup_madabushi_c0)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-column-drag-cd", iterator.inj.injector_data.seco_breakup_madabushi_column_drag_cd)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-ligament-factor", iterator.inj.injector_data.seco_breakup_madabushi_ligament_factor)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-jet-diameter", iterator.inj.injector_data.seco_breakup_madabushi_jet_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-schmehl-np", iterator.inj.injector_data.seco_breakup_schmehl_np)) { Kill_Read };


            // 物理定律与UDF
            Ignore_input(in,12);//laws
            Ignore_input(in,2);//udf
            // if (!read_dpm(file, in, "laws", iterator.inj->laws)) { Kill_Read };
            // if (!read_dpm(file, in, "switch", iterator.inj->swit)) { Kill_Read };
            // if (!read_dpm(file, in, "udf-inject-init", iterator.inj->udf_inject_init)) { Kill_Read };
            // if (!read_dpm(file, in, "udf-heat-mass", iterator.inj->udf_heat_mass)) { Kill_Read };
            //component
            Ignore_input(in,1);
            //体积喷注设置
            if (!read_dpm(file, in, "volume-specification", iterator.inj.injector_data.volume_specification)) { Kill_Read };
            if (!read_dpm(file, in, "volume-zones", iterator.inj.injector_data.volume_zones)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-spec", iterator.inj.injector_data.volume_streams_spec)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-total", iterator.inj.injector_data.volume_streams_total)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-per-cell", iterator.inj.injector_data.volume_streams_per_cell)) { Kill_Read };
            if (!read_dpm(file, in, "volume-packing-limit-per-cell", iterator.inj.injector_data.volume_packing_limit_per_cell)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-shapes", iterator.inj.injector_data.volume_bgeom_shapes)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-xmin", iterator.inj.injector_data.volume_bgeom_min,x)) { Kill_Read }; // 假设QVector3D分量单独读取
            if (!read_dpm(file, in, "volume-bgeom-ymin", iterator.inj.injector_data.volume_bgeom_min,y)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-zmin", iterator.inj.injector_data.volume_bgeom_min,z)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-xmax", iterator.inj.injector_data.volume_bgeom_max,x)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-ymax", iterator.inj.injector_data.volume_bgeom_max,y)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-zmax", iterator.inj.injector_data.volume_bgeom_max,z)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-radius", iterator.inj.injector_data.volume_bgeom_radius)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-viconeangle", iterator.inj.injector_data.volume_bgeom_viconeangle)) { Kill_Read };
            if (!read_dpm(file, in, "mass-input-on", iterator.inj.injector_data.mass_input_on)) { Kill_Read };
            if (!read_dpm(file, in, "volfrac-input-on", iterator.inj.injector_data.volfrac_input_on)) { Kill_Read };

            // 旋转与圆锥配置
            if (!read_dpm(file, in, "rotation-on?", iterator.inj.injector_data.rotation_on)) { Kill_Read };
            if (!read_dpm(file, in, "rot-drag-law", iterator.inj.injector_data.rot_drag_law)) { Kill_Read };
            if (!read_dpm(file, in, "rot-lift-law", iterator.inj.injector_data.rot_lift_law)) { Kill_Read };
            if (!read_dpm(file, in, "cone-type", iterator.inj.injector_data.cone_type)) { Kill_Read };
            if (!read_dpm(file, in, "uniform-mass-dist-on?", iterator.inj.injector_data.uniform_mass_dist_on)) { Kill_Read };
            if (!read_dpm(file, in, "spatial-staggering/std-inj/on?", iterator.inj.injector_data.spatial_staggering_std_inj_on)) { Kill_Read };
            if (!read_dpm(file, in, "spatial-staggering/atomizer/on?", iterator.inj.injector_data.spatial_staggering_atomizer_on)) { Kill_Read };
            if (!read_dpm(file, in, "stagger-radius", iterator.inj.injector_data.stagger_radius)) { Kill_Read };
            if (!read_dpm(file, in, "rough-wall-on?", iterator.inj.injector_data.rough_wall_on)) { Kill_Read };
            if (!read_dpm(file, in, "cphase-domain", iterator.inj.injector_data.cphace_domain)) { Kill_Read };

            // 位置与速度（QVector3D分量）
            if (!read_dpm(file, in, "pos", iterator.inj.injector_data.pos,x)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.injector_data.pos2,x)) { Kill_Read };
            if (!read_dpm(file, in, "pos", iterator.inj.injector_data.pos,y)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.injector_data.pos2,y)) { Kill_Read };
            if (!read_dpm(file, in, "pos", iterator.inj.injector_data.pos,z)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.injector_data.pos2,z)) { Kill_Read };

            // 扁平风扇坐标
            if (!read_dpm(file, in, "ff-center", iterator.inj.injector_data.ff_center,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-center", iterator.inj.injector_data.ff_center,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-center", iterator.inj.injector_data.ff_center,z)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.injector_data.ff_virtual_origin,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.injector_data.ff_virtual_origin,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.injector_data.ff_virtual_origin,z)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.injector_data.ff_normal,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.injector_data.ff_normal,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.injector_data.ff_normal,z)) { Kill_Read };

            // 速度与角速度
            if (!read_dpm(file, in, "vel", iterator.inj.injector_data.vel,x)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.injector_data.vel2,x)) { Kill_Read };
            if (!read_dpm(file, in, "vel", iterator.inj.injector_data.vel,y)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.injector_data.vel2,y)) { Kill_Read };
            if (!read_dpm(file, in, "vel", iterator.inj.injector_data.vel,z)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.injector_data.vel2,z)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.injector_data.ang_vel,x)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.injector_data.ang_vel2,x)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.injector_data.ang_vel,y)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.injector_data.ang_vel2,y)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.injector_data.ang_vel,z)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.injector_data.ang_vel2,z)) { Kill_Read };

            // 雾化器与几何参数
            if (!read_dpm(file, in, "atomizer-x-axis", iterator.inj.injector_data.atomizer_axis,x)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-y-axis", iterator.inj.injector_data.atomizer_axis,y)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-z-axis", iterator.inj.injector_data.atomizer_axis,z)) { Kill_Read };
            if (!read_dpm(file, in, "diameter", iterator.inj.injector_data.diameter)) { Kill_Read };
            if (!read_dpm(file, in, "diameter2", iterator.inj.injector_data.diameter2)) { Kill_Read };
            if (!read_dpm(file, in, "temperature", iterator.inj.injector_data.temperature)) { Kill_Read };
            if (!read_dpm(file, in, "temperature2", iterator.inj.injector_data.temperature2)) { Kill_Read };
            if (!read_dpm(file, in, "flow-rate", iterator.inj.injector_data.flow_rate)) { Kill_Read };
            if (!read_dpm(file, in, "flow-rate2", iterator.inj.injector_data.flow_rate2)) { Kill_Read };

            // 非稳态参数
            if (!read_dpm(file, in, "unsteady-start", iterator.inj.injector_data.unsteady_start)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-stop", iterator.inj.injector_data.unsteady_stop)) { Kill_Read };
            if (!read_dpm(file, in, "start-at-flow-time-in-unsteady-inj-file", iterator.inj.injector_data.start_at_flow_time_in_unsteady_inj_file)) { Kill_Read };
            if (!read_dpm(file, in, "interval-to-repeat-in-unsteady-inj-file", iterator.inj.injector_data.interval_to_repeat_in_unsteady_inj_file)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-ca-start", iterator.inj.injector_data.unsteady_ca_start)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-ca-stop", iterator.inj.injector_data.unsteady_ca_stop)) { Kill_Read };

            // 物性参数
            if (!read_dpm(file, in, "vapor-pressure", iterator.inj.injector_data.vapor_pressure)) { Kill_Read };
            if (!read_dpm(file, in, "inner-diameter", iterator.inj.injector_data.inner_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "outer-diameter", iterator.inj.injector_data.outer_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "half-angle", iterator.inj.injector_data.half_angle)) { Kill_Read };
            if (!read_dpm(file, in, "plain-length", iterator.inj.injector_data.plain_length)) { Kill_Read };
            if (!read_dpm(file, in, "plain-corner-size", iterator.inj.injector_data.plain_corner_size)) { Kill_Read };
            if (!read_dpm(file, in, "plain-const-a", iterator.inj.injector_data.plain_const_a)) { Kill_Read };
            if (!read_dpm(file, in, "pswirl-inj-press", iterator.inj.injector_data.pswirl_inj_press)) { Kill_Read };
            if (!read_dpm(file, in, "airbl-rel-vel", iterator.inj.injector_data.airbl_rel_vel)) { Kill_Read };
            if (!read_dpm(file, in, "effer-quality", iterator.inj.injector_data.effer_quality)) { Kill_Read };
            if (!read_dpm(file, in, "effer-t-sat", iterator.inj.injector_data.effer_t_sat)) { Kill_Read };
            if (!read_dpm(file, in, "ff-orifice-width", iterator.inj.injector_data.ff_oriface_width)) { Kill_Read };
            if (!read_dpm(file, in, "phi-start", iterator.inj.injector_data.phi_start)) { Kill_Read };
            if (!read_dpm(file, in, "phi-stop", iterator.inj.injector_data.phi_stop)) { Kill_Read };
            if (!read_dpm(file, in, "sheet-const", iterator.inj.injector_data.sheet_const)) { Kill_Read };
            if (!read_dpm(file, in, "lig-const", iterator.inj.injector_data.lig_const)) { Kill_Read };
            if (!read_dpm(file, in, "effer-const", iterator.inj.injector_data.effer_const)) { Kill_Read };
            if (!read_dpm(file, in, "effer-half-angle-max", iterator.inj.injector_data.effer_half_angle_max)) { Kill_Read };
            if (!read_dpm(file, in, "ff-sheet-const", iterator.inj.injector_data.ff_sheet_const)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-disp-angle", iterator.inj.injector_data.atomizer_disp_angle)) { Kill_Read };

            // 轴与速度参数
            if (!read_dpm(file, in, "axis", iterator.inj.injector_data.axis,x)) { Kill_Read };
            if (!read_dpm(file, in, "axis", iterator.inj.injector_data.axis,y)) { Kill_Read };
            if (!read_dpm(file, in, "axis", iterator.inj.injector_data.axis,z)) { Kill_Read };
            if (!read_dpm(file, in, "vel-mag", iterator.inj.injector_data.vel_mag)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel-mag", iterator.inj.injector_data.ang_vel_mag)) { Kill_Read };
            if (!read_dpm(file, in, "cone-angle", iterator.inj.injector_data.cone_angle)) { Kill_Read };
            if (!read_dpm(file, in, "inner-radius", iterator.inj.injector_data.inner_radius)) { Kill_Read };
            if (!read_dpm(file, in, "radius", iterator.inj.injector_data.radius)) { Kill_Read };
            if (!read_dpm(file, in, "swirl-frac", iterator.inj.injector_data.swirl_frac)) { Kill_Read };

            // 流量与质量
            if (!read_dpm(file, in, "total-flow-rate", iterator.inj.injector_data.total_flow_rate)) { Kill_Read };
            if (!read_dpm(file, in, "total-mass", iterator.inj.injector_data.total_mass)) { Kill_Read };
            if (!read_dpm(file, in, "volume-fraction", iterator.inj.injector_data.volume_fraction)) { Kill_Read };

            // RR分布参数
            if (!read_dpm(file, in, "rr-min", iterator.inj.injector_data.rr_min)) { Kill_Read };
            if (!read_dpm(file, in, "rr-max", iterator.inj.injector_data.rr_max)) { Kill_Read };
            if (!read_dpm(file, in, "rr-mean", iterator.inj.injector_data.rr_mean)) { Kill_Read };
            if (!read_dpm(file, in, "rr-spread", iterator.inj.injector_data.rr_spread)) { Kill_Read };
            if (!read_dpm(file, in, "rr-numdia", iterator.inj.injector_data.rr_numdia)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.injector_data.posr,x)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.injector_data.posr,y)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.injector_data.posr,z)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.injector_data.posu,x)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.injector_data.posu,y)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.injector_data.posu,z)) { Kill_Read };

            qDebug()<<"-2";

            unit.push_back(iterator);

            qDebug()<<"-1";

            if(read_end(in,iterator.inj.injector_data.name)) break;

        }
        qDebug()<<"0";
        //if(!read_dpm(file,in,"",)) {*ok=false;return unit;};
        *ok=true;
        qDebug()<<"1";
        delete(in);
        qDebug()<<"2";
        delete(file);
        qDebug()<<"3";
        return unit;
    }
}

QList<Unit> read_dpm_file(const QString &file_path,
                          bool *ok,
                          QString *error_message,
                          bool show_error_message_box)
{
    if (ok != nullptr)
    {
        *ok = false;
    }

    if (error_message != nullptr)
    {
        error_message->clear();
    }

    QList<Unit> units;
    QString validation_error;
    if (!validate_dpm_file_path(file_path, &validation_error))
    {
        if (error_message != nullptr)
        {
            *error_message = validation_error;
        }
        if (show_error_message_box &&
            !file_path.trimmed().isEmpty() && !validation_error.trimmed().isEmpty())
        {
            QMessageBox::critical(nullptr, "DPM Parse Error", validation_error);
        }
        return units;
    }

    QFile file(file_path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString message = QString("Unable to open DPM file: %1").arg(file_path);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_error_message_box)
        {
            QMessageBox::critical(nullptr, "DPM Parse Error", message);
        }
        return units;
    }

    const QString file_name = file.fileName();
    const QString content = QString::fromUtf8(file.readAll());
    if (content.trimmed().isEmpty())
    {
        const QString message = QString("DPM file is empty: %1").arg(file_name);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_error_message_box)
        {
            QMessageBox::critical(nullptr, "DPM Parse Error", message);
        }
        return units;
    }

    const QStringList blocks = split_dpm_blocks(content);

    if (blocks.isEmpty())
    {
        const QString message = QString("No top-level DPM block found in %1").arg(file_name);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_error_message_box)
        {
            QMessageBox::critical(nullptr, "DPM Parse Error", message);
        }
        return units;
    }

    for (const QString& block : blocks)
    {
        Unit unit;
        if (!parse_dpm_unit_block(block, unit, file_name))
        {
            if (error_message != nullptr)
            {
                *error_message = QString("Unable to parse a DPM injector block in %1").arg(file_name);
            }
            return QList<Unit>();
        }
        units.push_back(unit);
    }

    if (ok != nullptr)
    {
        *ok = true;
    }
    return units;
}

[[deprecated("Use read_dpm_file(file_path, ...) instead.")]]
QList<Unit> read_single_dpm_file_regex(bool *ok)
{
    return read_dpm_file(Read_File_Dialog(), ok, nullptr);
}
