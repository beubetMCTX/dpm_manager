#include "unit_edit_dialog.h"
#include "runtime_debug.h"
#include "ui_unit_edit_dialog.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTimer>

#include <algorithm>
#include <initializer_list>
#include <limits>

namespace
{
constexpr double kPositiveMinimum = std::numeric_limits<double>::epsilon();
constexpr double kMaxConeAngleDegrees = 179.99999999999997;

void normalize_non_negative(double &value)
{
    if (!std::isfinite(value) || value < 0.0)
    {
        value = 0.0;
    }
}

void normalize_unit_interval(double &value)
{
    if (!std::isfinite(value))
    {
        value = 0.0;
        return;
    }

    value = std::max(0.0, std::min(1.0, value));
}

void normalize_non_negative_values(std::initializer_list<double *> values)
{
    for (double *value : values)
    {
        if (value != nullptr)
        {
            normalize_non_negative(*value);
        }
    }
}

void normalize_non_negative_range(double &minimum, double &maximum)
{
    normalize_non_negative(minimum);
    normalize_non_negative(maximum);
    if (minimum > maximum)
    {
        std::swap(minimum, maximum);
    }
}

void normalize_ordered_range(double &start, double &stop)
{
    if (!std::isfinite(start))
    {
        start = 0.0;
    }
    if (!std::isfinite(stop))
    {
        stop = start;
    }
    if (start > stop)
    {
        std::swap(start, stop);
    }
}

void normalize_nonzero_vector(QVector3D &vector, const QVector3D &fallback)
{
    if (!std::isfinite(vector.x()) ||
        !std::isfinite(vector.y()) ||
        !std::isfinite(vector.z()) ||
        vector.lengthSquared() <= 0.0f)
    {
        vector = fallback;
    }
}

void normalize_finite_vector(QVector3D &vector)
{
    if (!std::isfinite(vector.x()))
    {
        vector.setX(0.0f);
    }
    if (!std::isfinite(vector.y()))
    {
        vector.setY(0.0f);
    }
    if (!std::isfinite(vector.z()))
    {
        vector.setZ(0.0f);
    }
}

void refresh_field_rows(QLayout *layout)
{
    if (layout == nullptr)
    {
        return;
    }

    for (int index = 0; index < layout->count(); ++index)
    {
        QLayoutItem *item = layout->itemAt(index);
        if (item == nullptr)
        {
            continue;
        }

        if (QWidget *widget = item->widget())
        {
            if (auto *row = dynamic_cast<QUI_FieldRow *>(widget))
            {
                row->refresh_unit_display();
            }

            if (QLayout *child_layout = widget->layout())
            {
                refresh_field_rows(child_layout);
            }
        }
        else if (QLayout *child_layout = item->layout())
        {
            refresh_field_rows(child_layout);
        }
    }
}

QString unit_type_display_name(Unit_Type type)
{
    switch (type)
    {
    case injector: return "Injector";
    case line_spacer: return "Line Spacer";
    case circle_spacer: return "Circle Spacer";
    case Assebly: return "Assembly";
    }

    return "Unknown";
}

QString injection_type_display_name(Injection_Type type)
{
    switch (type)
    {
    case single: return "Single";
    case group: return "Group";
    case surface: return "Surface";
    case volume: return "Volume";
    case cone: return "Cone";
    case plain_oriface_atomizer: return "Plain Orifice Atomizer";
    case pressure_swirl_atomizer: return "Pressure Swirl Atomizer";
    case air_blast_atomizer: return "Air Blast Atomizer";
    case flat_fan_atomizer: return "Flat Fan Atomizer";
    case effervescent_atomizer: return "Effervescent Atomizer";
    case file_: return "File";
    case condensate: return "Condensate";
    }

    return "Unknown";
}

QString particle_type_display_name(DPM_Type type)
{
    switch (type)
    {
    case Massless: return "Massless";
    case Inert: return "Inert";
    case Droplet: return "Droplet";
    case Combusting: return "Combusting";
    case Multicomponent: return "Multicomponent";
    }

    return "Unknown";
}

QString cone_type_display_name(Cone_Type type)
{
    switch (type)
    {
    case point: return "point-cone";
    case hollow: return "hollow-cone";
    case ring: return "ring-cone";
    case solid: return "solid-cone";
    }

    return "point-cone";
}

QString dark_control_style_sheet()
{
    return QString(
        "QComboBox, QLineEdit, QSpinBox {"
        "  min-height: 30px;"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  padding: 4px 30px 4px 10px;"
        "}"
        "QComboBox:focus, QLineEdit:focus, QSpinBox:focus {"
        "  border: 1px solid rgb(118, 168, 255);"
        "}"
        "QComboBox:disabled, QLineEdit:disabled, QSpinBox:disabled {"
        "  border: 1px solid rgb(68, 68, 68);"
        "  background: rgb(31, 31, 31);"
        "  color: rgb(130, 130, 130);"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  margin: 1px 1px 1px 0px;"
        "  border: none;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-top-right-radius: 5px;"
        "  border-bottom-right-radius: 5px;"
        "  width: 24px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/ui/icons/chevron-down.svg);"
        "  width: 12px;"
        "  height: 12px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  padding: 4px 0px;"
        "  outline: 0;"
        "  selection-background-color: rgb(68, 103, 167);"
        "  selection-color: white;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  min-height: 24px;"
        "  padding: 4px 10px;"
        "}"
        "QSpinBox::up-button {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 24px;"
        "  height: 13px;"
        "  margin: 1px 1px 0px 0px;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-bottom: 1px solid rgb(205, 211, 220);"
        "  border-top-right-radius: 5px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QSpinBox::down-button {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: bottom right;"
        "  width: 24px;"
        "  height: 13px;"
        "  margin: 0px 1px 1px 0px;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-bottom-right-radius: 5px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "  background: rgb(58, 58, 58);"
        "}"
        "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {"
        "  background: rgb(68, 68, 68);"
        "}"
        "QSpinBox::up-arrow {"
        "  image: url(:/ui/icons/chevron-up.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QSpinBox::down-arrow {"
        "  image: url(:/ui/icons/chevron-down.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}");
}

QString dialog_label_style_sheet()
{
    return QString(
        "QLabel {"
        "  color: rgb(241, 241, 241);"
        "}"
        "QLabel:disabled {"
        "  color: rgb(130, 130, 130);"
        "}");
}

void apply_labeled_control_enabled(QWidget *editor, QWidget *label, bool enabled)
{
    if (editor != nullptr)
    {
        editor->setEnabled(enabled);
    }

    if (label != nullptr)
    {
        label->setEnabled(enabled);
    }
}

bool particle_type_supports_material(DPM_Type type)
{
    switch (type)
    {
    case Droplet:
    case Combusting:
    case Multicomponent:
        return true;
    case Massless:
    case Inert:
    default:
        return false;
    }
}

bool particle_type_supports_evaporating_species(DPM_Type type)
{
    return type == Droplet;
}

bool particle_type_supports_devolatilizing_species(DPM_Type type)
{
    return type == Combusting;
}

bool particle_type_supports_diameter_distribution(DPM_Type type)
{
    // Massless particles have no particle diameter. All physical particle
    // types can use the distribution supported by their injection type.
    return type != Massless;
}

bool particle_type_supports_inertial_models(DPM_Type type)
{
    return type != Massless;
}

bool particle_type_supports_vapor_pressure(DPM_Type type)
{
    return type == Droplet;
}

bool feature_is_disabled(Unit_Edit_Feature_State state);

bool heat_transfer_is_disabled(const Unit_Edit_Case_Context &context)
{
    return feature_is_disabled(context.energy_equation) ||
           feature_is_disabled(context.heat_transfer);
}

bool particle_type_allowed_for_injection(Injection_Type injection_type, DPM_Type particle_type)
{
    if (injection_type == condensate)
    {
        return particle_type == Droplet || particle_type == Multicomponent;
    }

    return true;
}

bool particle_type_allowed_for_case_context(const Unit_Edit_Case_Context &context,
                                            DPM_Type particle_type)
{
    // Fluent's Droplet and Combusting models require heat transfer. The
    // Multicomponent model also solves a particle energy equation.
    // Unknown context deliberately remains permissive for older callers.
    if (heat_transfer_is_disabled(context) &&
        (particle_type == Droplet ||
         particle_type == Combusting ||
         particle_type == Multicomponent))
    {
        return false;
    }

    // Fluent v242 requires at least two active gas-phase species for Droplet
    // and Combusting, unless a non-premixed/partially-premixed model is
    // active. An unknown count or model stays permissive.
    const int minimum_species =
        (particle_type == Droplet || particle_type == Combusting) ? 2 : -1;
    if (minimum_species > 0 &&
        context.active_chemistry_species_count >= 0 &&
        context.active_chemistry_species_count < minimum_species &&
        context.nonpremixed_combustion == Unit_Edit_Feature_State::Disabled)
    {
        return false;
    }

    return true;
}

DPM_Type fallback_particle_type(const Unit_Edit_Case_Context &context,
                                Injection_Type injection_type,
                                DPM_Type current_type)
{
    const DPM_Type default_candidates[] = {
        Droplet, Multicomponent, Inert, Massless, Combusting};
    const DPM_Type energy_safe_candidates[] = {
        Inert, Multicomponent, Droplet, Massless, Combusting};
    const DPM_Type *candidates = default_candidates;
    const bool heat_dependent_type =
        current_type == Droplet ||
        current_type == Combusting ||
        current_type == Multicomponent;
    const int minimum_species =
        (current_type == Droplet || current_type == Combusting) ? 2 : -1;
    const bool chemistry_is_limited =
        minimum_species > 0 &&
        context.active_chemistry_species_count >= 0 &&
        context.active_chemistry_species_count < minimum_species &&
        context.nonpremixed_combustion == Unit_Edit_Feature_State::Disabled;
    if (heat_dependent_type &&
        (heat_transfer_is_disabled(context) || chemistry_is_limited))
    {
        candidates = energy_safe_candidates;
    }

    for (int i = 0; i < 5; ++i)
    {
        const DPM_Type candidate = candidates[i];
        if (particle_type_allowed_for_injection(injection_type, candidate) &&
            particle_type_allowed_for_case_context(context, candidate))
        {
            return candidate;
        }
    }

    return Inert;
}

QWidget *create_property_header(QWidget *parent,
                                const QString &left_title,
                                const QString &middle_title,
                                const QString &right_title = QString())
{
    QWidget *header = new QWidget(parent);
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(4, 6, 4, 6);
    layout->setSpacing(6);

    auto create_header_label = [](const QString &text, int fixed_width = 0)
    {
        QLabel *label = new QLabel(text);
        label->setStyleSheet("font-weight: 600; color: rgb(85, 92, 104);");
        if (fixed_width > 0)
        {
            label->setMinimumWidth(fixed_width);
            label->setMaximumWidth(fixed_width);
        }
        return label;
    };

    layout->addWidget(create_header_label(left_title, 180), 0);
    layout->addWidget(create_header_label(middle_title), 1);

    if (!right_title.isEmpty())
    {
        layout->addWidget(create_header_label(right_title), 1);
    }

    return header;
}

QLabel *create_group_title_label(const QString &text, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    QFont label_font = label->font();
    label_font.setPointSizeF(label_font.pointSizeF() + 1.0);
    label->setFont(label_font);
    label->setStyleSheet(
        "QLabel {"
        "  color: rgb(241, 241, 241);"
        "  background: transparent;"
        "  padding: 0px;"
        "  margin: 0px;"
        "}");
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return label;
}

float vector_component_value(const QVector3D &vector, int component)
{
    switch (component)
    {
    case 0: return vector.x();
    case 1: return vector.y();
    default: return vector.z();
    }
}

void set_vector_component_value(QVector3D &vector, int component, float value)
{
    switch (component)
    {
    case 0:
        vector.setX(value);
        return;
    case 1:
        vector.setY(value);
        return;
    default:
        vector.setZ(value);
        return;
    }
}

void append_unique_option(QStringList &options, const QString &value)
{
    if (!value.isEmpty() && !options.contains(value))
    {
        options.push_back(value);
    }
}

QStringList material_options_for(const QStringList &material_names)
{
    QStringList options;
    for (const QString &material_name : material_names)
    {
        append_unique_option(options, material_name);
    }
    return options;
}

QStringList discrete_phase_domain_options_for(const QString &current_value)
{
    QStringList options;
    append_unique_option(options, current_value);
    append_unique_option(options, "none");
    append_unique_option(options, "continuous-phase");
    append_unique_option(options, "secondary-domain");
    return options;
}

bool uses_dense_discrete_phase_domain(const QString &domain)
{
    const QString normalized = domain.trimmed();
    return !normalized.isEmpty() && normalized.compare("none", Qt::CaseInsensitive) != 0;
}

bool is_gravity_drag_law(Drag_Law law)
{
    return law == grace || law == ishii_zuber;
}

bool is_dense_gas_solid_drag_law(Drag_Law law)
{
    switch (law)
    {
    case wen_yu:
    case gidaspow:
    case syamlal_obrien:
    case huilin_gidaspow:
    case gibilaro:
    case emms:
    case filtered:
        return true;
    default:
        return false;
    }
}

bool drag_law_allowed_by_context(const Unit_Edit_Case_Context &context,
                                 const Injector &injector)
{
    if (is_gravity_drag_law(injector.drag_law) &&
        feature_is_disabled(context.gravity))
    {
        return false;
    }

    if (is_dense_gas_solid_drag_law(injector.drag_law))
    {
        return injector.type == Inert &&
               uses_dense_discrete_phase_domain(injector.dpm_domain) &&
               context.dense_gas_solid != Unit_Edit_Feature_State::Disabled;
    }

    return true;
}

bool feature_is_disabled(Unit_Edit_Feature_State state)
{
    return state == Unit_Edit_Feature_State::Disabled;
}

bool feature_is_enabled(Unit_Edit_Feature_State state)
{
    return state == Unit_Edit_Feature_State::Enabled;
}

bool volume_injection_available(const Unit_Edit_Case_Context &context,
                               const QString &dpm_domain)
{
    Q_UNUSED(dpm_domain);
    return !feature_is_disabled(context.three_dimensional) &&
           !feature_is_disabled(context.unsteady_particle_tracking) &&
           context.dem != Unit_Edit_Feature_State::Enabled;
}

bool cone_injection_available(const Unit_Edit_Case_Context &context)
{
    return !feature_is_disabled(context.three_dimensional);
}

QStringList species_options_for(const QStringList &available_species, const QString &current_value)
{
    QStringList options;
    options.push_back(QString());
    append_unique_option(options, current_value);
    for (const QString &species_name : available_species)
    {
        append_unique_option(options, species_name);
    }
    return options;
}

struct Diameter_Distribution_Option
{
    int mode = 0;
    QString label;
};

int diameter_distribution_index(const Injector &injector)
{
    if (injector.tabulated_diam_dist)
    {
        return 3;
    }

    if (injector.rr_disturb && injector.rr_uniform_ln_d)
    {
        return 2;
    }

    if (injector.rr_disturb)
    {
        return 1;
    }

    return 0;
}

QString base_diameter_distribution_label(Injection_Type type)
{
    switch (type)
    {
    case group:
        return "linear";
    case surface:
    case volume:
        return "uniform";
    case cone:
        return "fixed";
    case file_:
        return "from file";
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case flat_fan_atomizer:
    case effervescent_atomizer:
        return "model-defined";
    case single:
    case condensate:
    default:
        return "fixed";
    }
}

QList<Diameter_Distribution_Option> diameter_distribution_options_for(Injection_Type type)
{
    QList<Diameter_Distribution_Option> options;
    options.push_back({0, base_diameter_distribution_label(type)});

    switch (type)
    {
    case group:
        options.push_back({1, "rosin-rammler"});
        options.push_back({2, "rosin-rammler-logarithmic"});
        break;

    case surface:
        options.push_back({1, "rosin-rammler"});
        options.push_back({2, "rosin-rammler-logarithmic"});
        options.push_back({3, "tabulated"});
        break;

    case volume:
        options.push_back({1, "rosin-rammler"});
        options.push_back({2, "rosin-rammler-logarithmic"});
        options.push_back({3, "tabulated"});
        break;

    case cone:
        options.push_back({1, "rosin-rammler"});
        options.push_back({2, "rosin-rammler-logarithmic"});
        options.push_back({3, "tabulated"});
        break;

    default:
        break;
    }

    return options;
}

bool diameter_distribution_mode_supported(Injection_Type type, int mode)
{
    const QList<Diameter_Distribution_Option> options = diameter_distribution_options_for(type);
    for (const Diameter_Distribution_Option &option : options)
    {
        if (option.mode == mode)
        {
            return true;
        }
    }

    return false;
}

void normalize_diameter_distribution_for_type(Injector &injector)
{
    // Keep legacy or externally synchronized flags mutually exclusive. The
    // logarithmic RR variant is a refinement of RR, never an independent mode.
    if (injector.tabulated_diam_dist)
    {
        injector.rr_disturb = false;
        injector.rr_uniform_ln_d = false;
    }
    else if (injector.rr_uniform_ln_d)
    {
        injector.rr_disturb = true;
    }

    const int current_mode = diameter_distribution_index(injector);
    if (diameter_distribution_mode_supported(injector.injection_type, current_mode))
    {
        return;
    }

    injector.tabulated_diam_dist = false;
    injector.rr_disturb = false;
    injector.rr_uniform_ln_d = false;
}

void apply_diameter_distribution_index(Injector &injector, int index)
{
    switch (index)
    {
    case 1:
        injector.rr_disturb = true;
        injector.rr_uniform_ln_d = false;
        injector.tabulated_diam_dist = false;
        return;
    case 2:
        injector.rr_disturb = true;
        injector.rr_uniform_ln_d = true;
        injector.tabulated_diam_dist = false;
        return;
    case 3:
        injector.tabulated_diam_dist = true;
        injector.rr_disturb = false;
        injector.rr_uniform_ln_d = false;
        return;
    case 0:
    default:
        injector.tabulated_diam_dist = false;
        injector.rr_disturb = false;
        injector.rr_uniform_ln_d = false;
        return;
    }
}

bool uses_atomizer_stagger(Injection_Type type)
{
    switch (type)
    {
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case flat_fan_atomizer:
    case effervescent_atomizer:
        return true;
    default:
        return false;
    }
}

QString property_layout_key_for(const Injector &injector)
{
    return QString("%1|%2|%3|%4|%5|%6|%7|%8|%9|%10|%11")
        .arg(static_cast<int>(injector.type))
        .arg(static_cast<int>(injector.injection_type))
        .arg(diameter_distribution_index(injector))
        .arg(static_cast<int>(injector.cone_type))
        .arg(static_cast<int>(injector.volume_specification))
        .arg(static_cast<int>(injector.volume_bgeom_shapes))
        .arg(static_cast<int>(injector.volume_streams_spec))
        .arg(injector.use_face_normal ? 1 : 0)
        .arg(injector.mass_input_on ? 1 : 0)
        .arg(injector.volfrac_input_on ? 1 : 0)
        .arg(injector.dpm_domain);
}

void normalize_seco_breakup_models(Injector &injector)
{
    bool *model_flags[] = {
        &injector.seco_breakup_tab,
        &injector.seco_breakup_wave,
        &injector.seco_break_up_khrt,
        &injector.seco_breakup_ssd,
        &injector.seco_breakup_madahushi,
        &injector.seco_breakup_schmehl};

    if (injector.type != Droplet || !injector.seco_breakup_on)
    {
        injector.seco_breakup_on = injector.type == Droplet && injector.seco_breakup_on;
        for (bool *flag : model_flags)
        {
            *flag = false;
        }
        if (injector.drag_law == dynamic_drag)
        {
            injector.drag_law = spherical;
        }
        return;
    }

    bool model_seen = false;
    for (bool *flag : model_flags)
    {
        if (*flag && model_seen)
        {
            *flag = false;
        }
        else if (*flag)
        {
            model_seen = true;
        }
    }

    if (!model_seen && injector.drag_law == dynamic_drag)
    {
        injector.drag_law = spherical;
    }

    if (injector.seco_breakup_madahushi)
    {
        injector.drag_law = dynamic_drag;
    }
}

bool uses_atomizer_stagger(const Injector &injector)
{
    return uses_atomizer_stagger(injector.injection_type) ||
           (injector.injection_type == cone && injector.cone_type == solid);
}

bool uses_standard_stagger(Injection_Type type)
{
    return type == single || type == group || type == cone;
}

bool is_atomizer_injection(Injection_Type type)
{
    return uses_atomizer_stagger(type);
}

bool requires_standard_parcel_model(Injection_Type type)
{
    return is_atomizer_injection(type) ||
           type == file_ ||
           type == surface ||
           type == volume ||
           type == condensate;
}

bool uses_generic_stream_count(Injection_Type type)
{
    switch (type)
    {
    case group:
    case surface:
    case cone:
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case flat_fan_atomizer:
    case effervescent_atomizer:
        return true;
    default:
        return false;
    }
}

bool particle_rotation_supported(Injection_Type type)
{
    switch (type)
    {
    case single:
    case group:
    case surface:
    case volume:
    case cone:
        return true;
    default:
        return false;
    }
}

void normalize_volume_bounding_geometry(Injector &injector)
{
    if (!std::isfinite(injector.volume_bgeom_radius) || injector.volume_bgeom_radius < 0.0)
    {
        injector.volume_bgeom_radius = 0.0;
    }

    constexpr double kPi = 3.14159265358979323846;
    if (!std::isfinite(injector.volume_bgeom_viconeangle))
    {
        injector.volume_bgeom_viconeangle = 0.0;
    }
    injector.volume_bgeom_viconeangle = std::max(
        0.0, std::min(injector.volume_bgeom_viconeangle, kPi));

    if (injector.volume_specification == bouning_geometry &&
        injector.volume_bgeom_shapes != hexahedron)
    {
        injector.volume_bgeom_radius = std::max(
            kPositiveMinimum, injector.volume_bgeom_radius);
    }

    if (injector.volume_specification != bouning_geometry ||
        injector.volume_bgeom_shapes == sphere)
    {
        return;
    }

    auto normalize_component = [](float &minimum, float &maximum)
    {
        if (!std::isfinite(minimum))
        {
            minimum = 0.0f;
        }
        if (!std::isfinite(maximum))
        {
            maximum = 0.0f;
        }
        if (minimum > maximum)
        {
            std::swap(minimum, maximum);
        }
    };

    normalize_component(injector.volume_bgeom_min[0], injector.volume_bgeom_max[0]);
    normalize_component(injector.volume_bgeom_min[1], injector.volume_bgeom_max[1]);
    normalize_component(injector.volume_bgeom_min[2], injector.volume_bgeom_max[2]);
}

void normalize_model_dependencies(Injector &injector)
{
    // DPM files and external synchronization can provide stale enum values.
    // Normalize them before any switch or combo-box consumes the data.
    if (injector.type < Massless || injector.type > Multicomponent)
    {
        injector.type = Droplet;
    }
    if (injector.injection_type < single || injector.injection_type > condensate)
    {
        injector.injection_type = single;
    }
    if (injector.cone_type < point || injector.cone_type > solid)
    {
        injector.cone_type = point;
    }
    if (injector.parcel_model < standard || injector.parcel_model > const_diameter)
    {
        injector.parcel_model = standard;
    }
    if (injector.drag_law < spherical || injector.drag_law > filtered)
    {
        injector.drag_law = spherical;
    }
    if (injector.volume_specification < zone || injector.volume_specification > bouning_geometry)
    {
        injector.volume_specification = zone;
    }
    if (injector.volume_streams_spec < total_parcel_count || injector.volume_streams_spec > parcel_per_cell)
    {
        injector.volume_streams_spec = total_parcel_count;
    }
    if (injector.volume_bgeom_shapes < sphere || injector.volume_bgeom_shapes > hexahedron)
    {
        injector.volume_bgeom_shapes = sphere;
    }
    if (injector.rot_drag_law < Dennis_et_al || injector.rot_drag_law > none)
    {
        injector.rot_drag_law = none;
    }
    if (injector.rot_lift_law < Oesterle_Bui_Dinh || injector.rot_lift_law > none_)
    {
        injector.rot_lift_law = none_;
    }

    if (!particle_type_allowed_for_injection(injector.injection_type, injector.type))
    {
        injector.type = Droplet;
    }

    // Keep editor-bound scalar values finite before rows or geometry consume them.
    normalize_non_negative(injector.diameter);
    normalize_non_negative(injector.diameter2);
    normalize_non_negative(injector.temperature);
    normalize_non_negative(injector.temperature2);
    normalize_non_negative(injector.flow_rate);
    normalize_non_negative(injector.flow_rate2);
    normalize_non_negative(injector.total_flow_rate);
    normalize_non_negative(injector.total_mass);
    normalize_non_negative(injector.vel_mag);
    normalize_non_negative(injector.ang_vel_mag);
    normalize_non_negative(injector.vapor_pressure);
    normalize_non_negative(injector.time_scale_constant);
    normalize_non_negative(injector.volume_packing_limit_per_cell);
    normalize_non_negative_values({
        &injector.plain_length,
        &injector.plain_corner_size,
        &injector.plain_const_a,
        &injector.pswirl_inj_press,
        &injector.airbl_rel_vel,
        &injector.effer_t_sat,
        &injector.ff_oriface_width,
        &injector.sheet_const,
        &injector.lig_const,
        &injector.effer_const,
        &injector.ff_sheet_const,
        &injector.seco_breakup_tab_y0,
        &injector.seco_breakup_wave_b1,
        &injector.seco_breakup_wave_b0,
        &injector.seco_breakup_khrt_cl,
        &injector.seco_breakup_khrt_ctau,
        &injector.seco_breakup_khrt_crt,
        &injector.seco_breakup_ssd_we_cr,
        &injector.seco_breakup_ssd_core_bu,
        &injector.seco_breakup_ssd_np_target,
        &injector.seco_breakup_ssd_x_si,
        &injector.seco_breakup_madabushi_c0,
        &injector.seco_breakup_madabushi_column_drag_cd,
        &injector.seco_breakup_madabushi_ligament_factor,
        &injector.seco_breakup_madabushi_jet_diameter,
        &injector.seco_breakup_schmehl_np});
    normalize_unit_interval(injector.volume_fraction);
    normalize_unit_interval(injector.volume_packing_limit_per_cell);
    normalize_unit_interval(injector.effer_quality);
    normalize_unit_interval(injector.liquid_fraction);
    injector.numpts = std::max(1, injector.numpts);

    normalize_non_negative_range(injector.diameter, injector.diameter2);
    normalize_non_negative_range(injector.temperature, injector.temperature2);
    normalize_non_negative_range(injector.flow_rate, injector.flow_rate2);
    normalize_ordered_range(injector.phi_start, injector.phi_stop);
    normalize_ordered_range(injector.unsteady_ca_start, injector.unsteady_ca_stop);

    normalize_nonzero_vector(injector.atomizer_axis, QVector3D(1.0f, 0.0f, 0.0f));
    normalize_nonzero_vector(injector.axis, QVector3D(1.0f, 0.0f, 0.0f));
    normalize_nonzero_vector(injector.ff_normal, QVector3D(1.0f, 0.0f, 0.0f));
    normalize_finite_vector(injector.pos);
    normalize_finite_vector(injector.pos2);
    normalize_finite_vector(injector.ff_center);
    normalize_finite_vector(injector.ff_virtual_origin);
    normalize_finite_vector(injector.vel);
    normalize_finite_vector(injector.vel2);
    normalize_finite_vector(injector.ang_vel);
    normalize_finite_vector(injector.ang_vel2);
    normalize_finite_vector(injector.volume_bgeom_min);
    normalize_finite_vector(injector.volume_bgeom_max);

    normalize_non_negative(injector.inner_diameter);
    normalize_non_negative(injector.outer_diameter);
    if (injector.outer_diameter < injector.inner_diameter)
    {
        injector.outer_diameter = injector.inner_diameter;
    }

    if (injector.stochastic && injector.cloud)
    {
        injector.cloud = false;
    }
    if (injector.type == Massless)
    {
        // Massless particles follow the continuous phase and do not expose
        // particle-cloud tracking.
        injector.cloud = false;
    }
    if (!injector.stochastic)
    {
        injector.random_eddy = false;
        injector.ntries = 1;
    }
    normalize_non_negative(injector.cloud_min_dia);
    normalize_non_negative(injector.cloud_max_dia);
    if (injector.cloud_min_dia > injector.cloud_max_dia)
    {
        injector.cloud_max_dia = injector.cloud_min_dia;
    }
    normalize_seco_breakup_models(injector);
    normalize_volume_bounding_geometry(injector);
    if (injector.injection_type == volume &&
        injector.volume_specification == bouning_geometry)
    {
        // Fluent supports parcels-per-cell only for zone-based volume
        // injections; bounding geometries use starting points instead.
        injector.volume_streams_spec = total_parcel_count;
    }
    injector.volume_streams_total = std::max(1, injector.volume_streams_total);
    injector.volume_streams_per_cell = std::max(1, injector.volume_streams_per_cell);

    if (!particle_type_supports_material(injector.type))
    {
        injector.material.clear();
    }
    if (!particle_type_supports_evaporating_species(injector.type))
    {
        injector.evaporating_species.clear();
    }
    if (injector.type != Combusting)
    {
        injector.devolatilizing_species.clear();
        injector.oxidizing_species.clear();
        injector.product_species.clear();
    }

    if (!particle_type_supports_inertial_models(injector.type))
    {
        injector.rotation_on = false;
        injector.rough_wall_on = false;
        injector.drag_law = spherical;
        injector.brownian_motion = false;
    }
    else if (injector.drag_law != Strokes_Cunningham)
    {
        injector.brownian_motion = false;
    }

    injector.parcel_number = std::max(1, injector.parcel_number);
    if (!std::isfinite(injector.parcel_mass) || injector.parcel_mass < 0.0)
    {
        injector.parcel_mass = 0.0;
    }
    if (!std::isfinite(injector.parcel_diameter) || injector.parcel_diameter < 0.0)
    {
        injector.parcel_diameter = 0.0;
    }
    injector.number_tab_diameters = std::max(1, injector.number_tab_diameters);
    normalize_unit_interval(injector.shape_factor);
    if (!std::isfinite(injector.cunningham_correction) || injector.cunningham_correction <= 0.0)
    {
        injector.cunningham_correction = 1.0;
    }

    normalize_non_negative(injector.stagger_radius);

    const bool uniform_mass_supported =
        injector.type != Massless &&
        injector.injection_type == cone &&
        (injector.cone_type == solid || injector.cone_type == ring);
    if (!uniform_mass_supported)
    {
        injector.uniform_mass_dist_on = false;
    }

    const bool swirl_fraction_supported =
        injector.type != Massless &&
        injector.injection_type == cone &&
        injector.cone_type == hollow;
    if (!swirl_fraction_supported)
    {
        injector.swirl_frac = 0.0;
    }
    else
    {
        if (!std::isfinite(injector.swirl_frac))
        {
            injector.swirl_frac = 0.0;
        }
        injector.swirl_frac = std::max(-1.0, std::min(1.0, injector.swirl_frac));
    }

    if (injector.injection_type == cone && injector.cone_type == ring)
    {
        normalize_non_negative(injector.radius);
        injector.radius = std::max(kPositiveMinimum, injector.radius);
        normalize_non_negative(injector.inner_radius);
        injector.inner_radius = std::max(0.0, std::min(injector.inner_radius, 0.95 * injector.radius));
    }
    else if (injector.injection_type == cone &&
             (injector.cone_type == hollow || injector.cone_type == solid))
    {
        normalize_non_negative(injector.radius);
        injector.radius = std::max(kPositiveMinimum, injector.radius);
    }

    if (injector.injection_type == cone)
    {
        if (!std::isfinite(injector.cone_angle))
        {
            injector.cone_angle = 0.0;
        }
        injector.cone_angle = std::max(
            0.0, std::min(injector.cone_angle, kMaxConeAngleDegrees));
    }

    injector.atomizer_disp_angle = std::isfinite(injector.atomizer_disp_angle)
                                       ? std::max(0.0, std::min(injector.atomizer_disp_angle,
                                                                kMaxConeAngleDegrees))
                                       : 0.0;

    constexpr double kHalfPi = 1.57079632679489661923;
    if (!std::isfinite(injector.half_angle))
    {
        injector.half_angle = 0.0;
    }
    injector.half_angle = std::max(0.0, std::min(injector.half_angle, kHalfPi));
    if (!std::isfinite(injector.effer_half_angle_max))
    {
        injector.effer_half_angle_max = 0.0;
    }
    injector.effer_half_angle_max = std::max(
        0.0, std::min(injector.effer_half_angle_max, kHalfPi));

    if (injector.injection_type == file_ || injector.injection_type == condensate)
    {
        normalize_non_negative(injector.unsteady_start);
        normalize_non_negative(injector.unsteady_stop);
        normalize_non_negative(injector.start_at_flow_time_in_unsteady_inj_file);
        normalize_non_negative(injector.interval_to_repeat_in_unsteady_inj_file);
        injector.unsteady_stop = std::max(injector.unsteady_start, injector.unsteady_stop);
    }

    normalize_non_negative(injector.rr_min);
    normalize_non_negative(injector.rr_max);
    normalize_non_negative(injector.rr_mean);
    normalize_non_negative(injector.rr_spread);
    injector.rr_max = std::max(injector.rr_min, injector.rr_max);
    injector.rr_mean = std::max(injector.rr_min, std::min(injector.rr_mean, injector.rr_max));
    injector.rr_spread = std::max(kPositiveMinimum, injector.rr_spread);
    injector.rr_numdia = std::max(1, injector.rr_numdia);
    injector.tabulated_diam_ref_diam_col = std::max(1, injector.tabulated_diam_ref_diam_col);
    injector.tabulated_diam_num_frac_col = std::max(1, injector.tabulated_diam_num_frac_col);
    injector.tabulated_diam_mas_frac_col = std::max(1, injector.tabulated_diam_mas_frac_col);

    if (injector.injection_type != surface)
    {
        injector.scale_by_area = false;
        injector.random_surface = false;
        injector.use_face_normal = false;
    }
    else if (injector.type == Massless)
    {
        // Massless particles have no particle velocity input to replace with
        // a face-normal magnitude.
        injector.use_face_normal = false;
    }
    else if (injector.scale_by_area && injector.random_surface)
    {
        injector.random_surface = false;
    }

    // Fluent does not expose a local reference frame for these injection
    // types. Clear a stale value loaded from an older project as well as
    // hiding the corresponding control.
    if (injector.injection_type == surface ||
        injector.injection_type == volume ||
        injector.injection_type == condensate)
    {
        injector.local_reference_frame = "global";
    }

    if (uses_atomizer_stagger(injector))
    {
        injector.spatial_staggering_std_inj_on = false;
    }
    else if (!uses_standard_stagger(injector.injection_type))
    {
        injector.spatial_staggering_atomizer_on = false;
        injector.spatial_staggering_std_inj_on = false;
    }
    else
    {
        injector.spatial_staggering_atomizer_on = false;
    }

    // Volume injections use exactly one of flow rate, total mass, or volume
    // fraction. Massless volume injections need none of these inputs.
    if (injector.injection_type != volume || injector.type == Massless)
    {
        injector.mass_input_on = false;
        injector.volfrac_input_on = false;
    }
    else if (injector.mass_input_on && injector.volfrac_input_on)
    {
        injector.volfrac_input_on = false;
    }

    // Wet combustion is a combusting-particle feature. Do not keep stale
    // liquid settings active after switching to another particle type.
    if (injector.type != Combusting)
    {
        injector.evaporating_liquid = false;
        injector.evaporating_material.clear();
        injector.liquid_fraction = -1.0;
    }
    else if (!injector.evaporating_liquid)
    {
        injector.evaporating_material.clear();
        injector.liquid_fraction = -1.0;
        injector.evaporating_species.clear();
    }

    // Fluent disables the Parcel page for DDPM injections and uses the
    // constant-diameter release method in that mode. Volume injections are
    // still valid; DDPM changes their starting-point controls instead.
    if (uses_dense_discrete_phase_domain(injector.dpm_domain))
    {
        // Unsteady stochastic tracking always uses one try. DDPM is the
        // project-level signal currently available for that tracking mode.
        injector.ntries = 1;
        injector.parcel_model = const_diameter;
    }

    if (injector.type == Massless ||
        !particle_rotation_supported(injector.injection_type))
    {
        injector.rotation_on = false;
    }

    if (requires_standard_parcel_model(injector.injection_type) &&
        !uses_dense_discrete_phase_domain(injector.dpm_domain))
    {
        // These injection types do not expose the Parcel page.
        injector.parcel_model = standard;
    }

    if (!particle_type_supports_diameter_distribution(injector.type))
    {
        apply_diameter_distribution_index(injector, 0);
    }
    else
    {
        normalize_diameter_distribution_for_type(injector);
    }
}

QString model_layout_key_for(const Injector &injector)
{
    return QString("%1|%2|%3|%4|%5|%6|%7|%8|%9|%10|%11|%12|%13|%14|%15|%16|%17|%18|%19|%20|%21")
        .arg(static_cast<int>(injector.type))
        .arg(static_cast<int>(injector.injection_type))
        .arg(injector.stochastic ? 1 : 0)
        .arg(injector.cloud ? 1 : 0)
        .arg(injector.rotation_on ? 1 : 0)
        .arg(static_cast<int>(injector.drag_law))
        .arg(injector.seco_breakup_on ? 1 : 0)
        .arg(injector.seco_breakup_tab ? 1 : 0)
        .arg(injector.seco_breakup_wave ? 1 : 0)
        .arg(injector.seco_break_up_khrt ? 1 : 0)
        .arg(injector.seco_breakup_ssd ? 1 : 0)
        .arg(injector.seco_breakup_madahushi ? 1 : 0)
        .arg(injector.seco_breakup_schmehl ? 1 : 0)
        .arg(static_cast<int>(injector.parcel_model))
        .arg(injector.evaporating_liquid ? 1 : 0)
        .arg(static_cast<int>(injector.cone_type))
        .arg(injector.uniform_mass_dist_on ? 1 : 0)
        .arg(injector.scale_by_area ? 1 : 0)
        .arg(injector.random_surface ? 1 : 0)
        .arg(injector.dpm_domain)
        .arg(injector.random_eddy ? 1 : 0);
}
}

unit_edit_dialog::unit_edit_dialog(Unit* control_unit,
                                   const QStringList &chemkin_species_names,
                                   const QStringList &material_names,
                                   QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::unit_edit_dialog)
    , control_unit(control_unit)
    , m_chemkin_species_names(chemkin_species_names)
    , m_material_names(material_names)
{
    ui->setupUi(this);
    // Keep lifetime under the QObject parent. This avoids deleteLater() races
    // when the main window is closing while an editor is still visible.
    setAttribute(Qt::WA_DeleteOnClose, false);
    initialize();
    setup_action_buttons();

    // for(auto i =0;i<ui->verticalLayout_number_of_stream->count();i++)
    // {
    //     QWidget *w = ui->verticalLayout_number_of_stream->itemAt(i)->widget();
    //     if(w != nullptr){
    //         w->setVisible(false);
    //     }
    // }


    //QLabel *label = new QLabel("这是一个简单的对话框", this);
    //ui->verticalLayout->addWidget(label);
    //ui->frame_layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //ui->horizontalLayout_partical_type->setParent(ui->groupBox_partical_type);



}

unit_edit_dialog::~unit_edit_dialog()
{
    runtime_debug::trace(
        QString("unit_edit_dialog destructor begin for %1")
            .arg(control_unit != nullptr ? control_unit->inj.injector_data.name : QString("<null>")));
    delete ui;
    runtime_debug::trace("unit_edit_dialog destructor end");
}

void unit_edit_dialog::closeEvent(QCloseEvent *event)
{
    emit dialog_closed(control_unit);
    QDialog::closeEvent(event);
}

void unit_edit_dialog::reset_edit_state()
{
    m_data_modified = false;
}

void unit_edit_dialog::setup_action_buttons()
{
    if (ui == nullptr || ui->verticalLayout_frame == nullptr)
    {
        return;
    }

    auto *button_row = new QWidget(this);
    button_row->setObjectName("unitEditorActionRow");
    auto *button_layout = new QHBoxLayout(button_row);
    button_layout->setContentsMargins(20, 6, 20, 10);
    button_layout->setSpacing(8);
    button_layout->addStretch(1);

    m_apply_button = new QPushButton("Apply and Close", button_row);
    m_apply_button->setObjectName("applyChangesButton");
    m_cancel_button = new QPushButton("Cancel Changes", button_row);
    m_cancel_button->setObjectName("cancelChangesButton");
    button_layout->addWidget(m_apply_button);
    button_layout->addWidget(m_cancel_button);
    ui->verticalLayout_frame->addWidget(button_row);

    connect(m_apply_button, &QPushButton::clicked, this, [this]()
    {
        close();
    });
    connect(m_cancel_button, &QPushButton::clicked, this, [this]()
    {
        m_data_modified = false;
        emit dialog_cancelled(control_unit);
        close();
    });
}

void unit_edit_dialog::refresh_from_unit_data(Unit *unit)
{
    if (unit == nullptr || unit != control_unit || control_unit == nullptr)
    {
        return;
    }

    sync_case_context_constraints();
    normalize_model_dependencies(control_unit->inj.injector_data);
    sync_case_context_constraints();
    setWindowTitle(QString("Unit Editor - %1").arg(control_unit->inj.injector_data.name));

    if (m_injection_name_edit != nullptr)
    {
        m_injection_name_edit->sync_text(control_unit->inj.injector_data.name);
    }

    if (m_number_of_stream_spin != nullptr)
    {
        m_number_of_stream_spin->sync_from_binding();
    }

    sync_unit_type_combo();
    sync_injection_type_combo();
    sync_particle_type_group();
    sync_material_combo();
    sync_diameter_distribution_combo();
    sync_discrete_phase_domain_combo();
    sync_species_combos();
    sync_cone_type_combo();
    sync_stagger_controls();
    sync_auxiliary_panels();
    sync_particle_type_dependent_controls();

    const QString layout_key = current_property_layout_key();
    if (layout_key != m_property_layout_key)
    {
        build_point_property_rows();
    }
    else
    {
        sync_point_property_rows();
    }

    const QString model_layout_key = current_model_layout_key();
    if (model_layout_key != m_model_layout_key)
    {
        build_model_property_rows();
    }
    else
    {
        sync_model_property_rows();
    }
}

void unit_edit_dialog::refresh_geometry_from_unit_data(Unit *unit)
{
    if (unit == nullptr || unit != control_unit || control_unit == nullptr)
    {
        return;
    }

    // Dragging changes only geometry fields. Avoid refreshing selectors and
    // model pages for every mouse-move event.
    sync_point_property_rows();
}

void unit_edit_dialog::set_material_names(const QStringList &material_names)
{
    if (m_material_names == material_names)
    {
        return;
    }

    m_material_names = material_names;
    sync_material_combo();

    for (const QPointer<QUI_ComboBox> &combo : m_material_context_combos)
    {
        if (combo == nullptr)
        {
            continue;
        }

        const QStringList options = material_options_for(m_material_names);
        const QString current_value = combo->currentText();

        const QSignalBlocker blocker(combo);
        combo->set_options(options);
        const int current_index = combo->findText(current_value);
        combo->setCurrentIndex(current_index);
        if (current_index < 0)
        {
            combo->setEditText(current_value);
        }
    }
}

void unit_edit_dialog::set_chemkin_species_names(const QStringList &species_names)
{
    if (m_chemkin_species_names == species_names)
    {
        return;
    }

    m_chemkin_species_names = species_names;
    // A Chemkin import is the best available source for the active species
    // count when the caller has not supplied case-level chemistry metadata.
    if (m_chemkin_species_count_fallback)
    {
        m_case_context.active_chemistry_species_count =
            species_names.isEmpty() ? -1 : species_names.size();
    }
    sync_case_context_constraints();
    sync_particle_type_group();
    sync_particle_type_dependent_controls();
    sync_species_combos();
    sync_auxiliary_panels();
    build_point_property_rows();
    build_model_property_rows();
}

void unit_edit_dialog::set_case_context(const Unit_Edit_Case_Context &context)
{
    m_case_context = context;
    m_chemkin_species_count_fallback = context.active_chemistry_species_count < 0;
    if (control_unit == nullptr)
    {
        return;
    }

    sync_case_context_constraints();
    sync_injection_type_combo();
    sync_particle_type_group();
    sync_particle_type_dependent_controls();
    sync_species_combos();
    sync_auxiliary_panels();
    build_point_property_rows();
    build_model_property_rows();
}

void unit_edit_dialog::sync_case_context_constraints()
{
    if (control_unit == nullptr)
    {
        return;
    }

    Injector &injector = control_unit->inj.injector_data;
    if (!volume_injection_available(m_case_context, injector.dpm_domain) &&
        injector.injection_type == volume)
    {
        injector.injection_type = single;
        normalize_model_dependencies(injector);
    }
    if (!cone_injection_available(m_case_context) && injector.injection_type == cone)
    {
        injector.injection_type = single;
        normalize_model_dependencies(injector);
    }
    if (feature_is_disabled(m_case_context.three_dimensional) &&
        injector.injection_type == flat_fan_atomizer)
    {
        injector.injection_type = single;
        normalize_model_dependencies(injector);
    }
    if (heat_transfer_is_disabled(m_case_context) &&
        injector.injection_type == condensate)
    {
        injector.injection_type = single;
        normalize_model_dependencies(injector);
    }

    if (!particle_type_allowed_for_injection(injector.injection_type, injector.type) ||
        !particle_type_allowed_for_case_context(m_case_context, injector.type))
    {
        injector.type = fallback_particle_type(
            m_case_context, injector.injection_type, injector.type);
        normalize_model_dependencies(injector);
    }
    if (injector.type == Combusting &&
        feature_is_enabled(m_case_context.material_multiple_surface_reaction))
    {
        injector.oxidizing_species.clear();
        injector.product_species.clear();
    }

    if (heat_transfer_is_disabled(m_case_context))
    {
        injector.brownian_motion = false;
    }
    if (m_case_context.unsteady_particle_tracking == Unit_Edit_Feature_State::Enabled &&
        injector.stochastic)
    {
        injector.ntries = 1;
    }
    if (feature_is_disabled(m_case_context.unsteady_particle_tracking) &&
        injector.drag_law == dynamic_drag)
    {
        injector.drag_law = spherical;
    }
    if (feature_is_disabled(m_case_context.unsteady_particle_tracking))
    {
        // Fluent exposes alternate parcel release methods only for unsteady
        // particle tracking. Clear stale state when a steady case is loaded.
        injector.parcel_model = standard;
    }
    if (feature_is_disabled(m_case_context.reflect_boundary))
    {
        injector.rough_wall_on = false;
    }
    if (feature_is_disabled(m_case_context.multiple_surface_reaction))
    {
        injector.oxidizing_species.clear();
        injector.product_species.clear();
    }
    if (!drag_law_allowed_by_context(m_case_context, injector))
    {
        injector.drag_law = spherical;
    }
}

inline bool unit_edit_dialog::initialize()
{
    if (control_unit == nullptr)
    {
        return false;
    }

    setup_custom_controls();
    setWindowTitle(QString("Unit Editor - %1").arg(control_unit->inj.injector_data.name));
    initialize_unit_type_combo();
    sync_unit_type_combo();
    initialize_injection_type_combo();
    sync_injection_type_combo();
    initialize_particle_type_group();
    sync_particle_type_group();
    initialize_material_combo();
    sync_material_combo();
    initialize_diameter_distribution_combo();
    sync_diameter_distribution_combo();
    initialize_discrete_phase_domain_combo();
    sync_discrete_phase_domain_combo();
    initialize_species_combos();
    sync_species_combos();
    initialize_cone_type_combo();
    sync_cone_type_combo();
    initialize_stagger_controls();
    sync_stagger_controls();
    sync_auxiliary_panels();
    sync_particle_type_dependent_controls();
    build_point_property_rows();
    build_model_property_rows();
    return true;
}

void unit_edit_dialog::setup_custom_controls()
{
    if (ui->label_material != nullptr)
    {
        ui->label_material->setText("Material");
        ui->label_material->setToolTip(
            "Selectable values come from the Materials table.");
        ui->label_material->setStyleSheet(dialog_label_style_sheet());
    }
    if (ui->label_diameter_distribution != nullptr) ui->label_diameter_distribution->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_oxidizing_species != nullptr) ui->label_oxidizing_species->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_evaporating_species != nullptr) ui->label_evaporating_species->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_devolatilizing_species != nullptr) ui->label_devolatilizing_species->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_product_species != nullptr) ui->label_product_species->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_discrete_phase_domain != nullptr) ui->label_discrete_phase_domain->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_conetype != nullptr) ui->label_conetype->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_stagger != nullptr) ui->label_stagger->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_injection_name != nullptr) ui->label_injection_name->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_injection_type != nullptr) ui->label_injection_type->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_number_of_stream != nullptr) ui->label_number_of_stream->setStyleSheet(dialog_label_style_sheet());
    if (ui->label_unit != nullptr) ui->label_unit->setStyleSheet(dialog_label_style_sheet());
    if (ui->tabWidget_injection != nullptr) ui->tabWidget_injection->setStyleSheet(qui_tab_widget_style_sheet());
    const QString tab_page_style = "background: rgb(24, 24, 24);";
    if (ui->tab_point_properties != nullptr) ui->tab_point_properties->setStyleSheet(tab_page_style);
    if (ui->tab_physical_models != nullptr) ui->tab_physical_models->setStyleSheet(tab_page_style);
    if (ui->tab_turbulent_dispersion != nullptr) ui->tab_turbulent_dispersion->setStyleSheet(tab_page_style);
    if (ui->tab_parcel != nullptr) ui->tab_parcel->setStyleSheet(tab_page_style);
    if (ui->tab_wet_combustion != nullptr) ui->tab_wet_combustion->setStyleSheet(tab_page_style);

    auto create_model_tab_layout = [](QWidget *tab, const QString &title)
    {
        auto *outer_layout = new QVBoxLayout(tab);
        outer_layout->setContentsMargins(0, 0, 0, 0);
        outer_layout->setSpacing(0);

        auto *scroll_area = new QScrollArea(tab);
        scroll_area->setWidgetResizable(true);
        scroll_area->setFrameShape(QFrame::NoFrame);
        scroll_area->setStyleSheet(qui_scroll_area_style_sheet());

        auto *content = new QWidget(scroll_area);
        content->setStyleSheet("background: rgb(24, 24, 24);");
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(4);
        layout->addWidget(create_group_title_label(title, content));
        layout->addStretch();
        scroll_area->setWidget(content);
        outer_layout->addWidget(scroll_area);
        return layout;
    };
    if (ui->tab_physical_models != nullptr)
    {
        m_physical_models_layout = create_model_tab_layout(ui->tab_physical_models, "Physical Models");
    }
    if (ui->tab_turbulent_dispersion != nullptr)
    {
        m_turbulent_dispersion_layout = create_model_tab_layout(ui->tab_turbulent_dispersion, "Turbulent Dispersion");
    }
    if (ui->tab_parcel != nullptr)
    {
        m_parcel_layout = create_model_tab_layout(ui->tab_parcel, "Parcel");
    }
    if (ui->tab_wet_combustion != nullptr)
    {
        m_wet_combustion_layout = create_model_tab_layout(ui->tab_wet_combustion, "Wet Combustion");
    }
    if (ui->scrollArea != nullptr)
    {
        ui->scrollArea->setFrameShape(QFrame::NoFrame);
        ui->scrollArea->setStyleSheet(qui_scroll_area_style_sheet());
        if (ui->scrollArea->viewport() != nullptr)
        {
            ui->scrollArea->viewport()->setStyleSheet("background: rgb(29, 29, 29); border-radius: 0px;");
        }
    }
    if (ui->scrollarea_properties != nullptr) ui->scrollarea_properties->setStyleSheet("background: rgb(29, 29, 29);");
    if (ui->cone_parameter_layout != nullptr)
    {
        ui->cone_parameter_layout->setTitle(QString());
        ui->cone_parameter_layout->setStyleSheet(qui_group_box_body_style_sheet());
    }
    if (ui->stagger_layout != nullptr)
    {
        ui->stagger_layout->setTitle(QString());
        ui->stagger_layout->setStyleSheet(qui_group_box_body_style_sheet());
    }
    if (ui->layout_cone_parameters != nullptr)
    {
        ui->layout_cone_parameters->setSpacing(1);
        ui->layout_cone_parameters->insertWidget(0, create_group_title_label("Cone Injector Parameters", this));
    }
    if (ui->layout_stagger_position != nullptr)
    {
        ui->layout_stagger_position->setSpacing(1);
        ui->layout_stagger_position->insertWidget(0, create_group_title_label("Stagger Options", this));
    }
    if (ui->cone_parameter_layout != nullptr)
    {
        ui->cone_parameter_layout->setMinimumWidth(240);
        ui->cone_parameter_layout->setMaximumWidth(260);
        ui->cone_parameter_layout->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    if (ui->stagger_layout != nullptr)
    {
        ui->stagger_layout->setMinimumWidth(180);
        ui->stagger_layout->setMaximumWidth(190);
    }

    m_unit_type_combo = new QUI_ComboBox(this);
    ui->unit_type_layout->insertWidget(1, m_unit_type_combo);
    ui->comboBox_unit->hide();

    m_injection_name_edit = new QUI_LineEdit(this);
    m_injection_name_edit->setObjectName("injectionNameEditor");
    m_injection_name_edit->set_string_mode();
    m_injection_name_edit->set_allow_empty_string(false);
    m_injection_name_edit->bind_value(&control_unit->inj.injector_data.name);
    ui->verticalLayout_injection_name->addWidget(m_injection_name_edit);
    ui->lineEdit_injection_name->hide();

    m_injection_type_combo = new QUI_ComboBox(this);
    m_injection_type_combo->setObjectName("injectionTypeEditor");
    ui->verticalLayout_injecton_type->addWidget(m_injection_type_combo);
    ui->comboBox_injection_type->hide();

    m_number_of_stream_spin = new QUI_SpinBox(this);
    m_number_of_stream_spin->setRange(1, 100000);
    m_number_of_stream_spin->bind_value(&control_unit->inj.injector_data.numpts);
    ui->verticalLayout_number_of_stream->addWidget(m_number_of_stream_spin);
    ui->spinBox_number_of_stream->hide();

    QWidget *particle_type_panel = new QWidget(this);
    auto *particle_type_panel_layout = new QVBoxLayout(particle_type_panel);
    particle_type_panel_layout->setContentsMargins(0, 0, 0, 0);
    particle_type_panel_layout->setSpacing(1);
    particle_type_panel_layout->addWidget(create_group_title_label("Particle Type", particle_type_panel));

    m_particle_type_group = new QUI_RadioGroup(QString(), particle_type_panel);
    m_particle_type_group->setObjectName("particleTypeGroup");
    particle_type_panel_layout->addWidget(m_particle_type_group);
    ui->horizontalLayout_particle_type->insertWidget(0, particle_type_panel);
    ui->groupBox_partical_type->hide();

    m_material_combo = new QUI_ComboBox(this);
    m_material_combo->setObjectName("materialEditor");
    ui->gridLayout->addWidget(m_material_combo, 2, 0);
    m_material_combo->setEditable(true);
    if (m_material_combo->lineEdit() != nullptr)
    {
        m_material_combo->lineEdit()->setReadOnly(true);
    }
    m_material_combo->setToolTip(
        "Selectable values come from the Materials table.");
    ui->comboBox_material->hide();

    m_diameter_distribution_combo = new QUI_ComboBox(this);
    m_diameter_distribution_combo->setObjectName("diameterDistributionEditor");
    ui->gridLayout->addWidget(m_diameter_distribution_combo, 2, 1);
    ui->comboBox_diameter_distribution->hide();

    m_discrete_phase_domain_combo = new QUI_ComboBox(this);
    m_discrete_phase_domain_combo->setObjectName("discretePhaseDomainEditor");
    ui->gridLayout->addWidget(m_discrete_phase_domain_combo, 2, 3);
    ui->comboBox_discrete_phase_domain->hide();

    m_evaporating_species_combo = new QUI_ComboBox(this);
    m_evaporating_species_combo->setObjectName("evaporatingSpeciesEditor");
    ui->gridLayout->addWidget(m_evaporating_species_combo, 4, 0);
    ui->comboBox_evaporating_species->hide();

    m_devolatilizing_species_combo = new QUI_ComboBox(this);
    m_devolatilizing_species_combo->setObjectName("devolatilizingSpeciesEditor");
    ui->gridLayout->addWidget(m_devolatilizing_species_combo, 4, 1);
    ui->comboBox_devolatilizing_species->hide();

    m_product_species_combo = new QUI_ComboBox(this);
    m_product_species_combo->setObjectName("productSpeciesEditor");
    ui->gridLayout->addWidget(m_product_species_combo, 4, 2);
    ui->comboBox_product_species->hide();

    m_oxidizing_species_combo = new QUI_ComboBox(this);
    m_oxidizing_species_combo->setObjectName("oxidizingSpeciesEditor");
    ui->gridLayout->addWidget(m_oxidizing_species_combo, 2, 2);
    ui->comboBox_oxidizin_species->hide();

    if (ui->cone_parameter_layout != nullptr)
    {
        ui->cone_parameter_layout->setMinimumWidth(240);
        ui->cone_parameter_layout->setMaximumWidth(260);
        ui->cone_parameter_layout->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        ui->cone_parameter_layout->hide();
    }

    m_stagger_check = new QUI_CheckBox("Stagger Options", ui->stagger_layout);
    m_stagger_check->setObjectName("staggerOptionsEditor");
    ui->verticalLayout_4->insertWidget(0, m_stagger_check);
    ui->radioButton_stagger->hide();

    m_stagger_radius_edit = new QUI_LineEdit(ui->stagger_layout);
    m_stagger_radius_edit->setObjectName("staggerRadiusEditor");
    m_stagger_radius_edit->set_double_mode();
    m_stagger_radius_edit->bind_value(&control_unit->inj.injector_data.stagger_radius);
    m_stagger_radius_edit->set_numeric_range(0.0, std::numeric_limits<double>::max());
    ui->verticalLayout_4->insertWidget(3, m_stagger_radius_edit);
    ui->lineEdit_stagger->hide();
    ui->stagger_layout->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->stagger_layout->setMinimumWidth(180);
    ui->stagger_layout->setMaximumWidth(190);
    ui->stagger_layout->hide();

    if (ui->comboBox_conetype != nullptr)
    {
        ui->comboBox_conetype->setStyleSheet(dark_control_style_sheet());
        ui->comboBox_conetype->setMinimumHeight(30);
        ui->comboBox_conetype->setMaximumHeight(30);
        ui->comboBox_conetype->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    connect(m_injection_name_edit, &QUI_LineEdit::value_committed, this, [this]()
    {
        if (control_unit != nullptr)
        {
            setWindowTitle(QString("Unit Editor - %1").arg(control_unit->inj.injector_data.name));
        }
        notify_injector_data_changed(false);
    });

    connect(m_number_of_stream_spin, &QUI_SpinBox::value_committed, this, [this](int)
    {
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_unit_type_combo()
{
    if (m_unit_type_combo == nullptr)
    {
        return;
    }

    m_unit_type_combo->clear();

    const Unit_Type ordered_types[] = {
        injector,
        line_spacer,
        circle_spacer,
        Assebly
    };

    for (Unit_Type type : ordered_types)
    {
        m_unit_type_combo->addItem(unit_type_display_name(type), static_cast<int>(type));
    }

    connect(m_unit_type_combo, &QUI_ComboBox::selection_committed, this, [this]()
    {
        if (control_unit == nullptr || m_unit_type_combo == nullptr)
        {
            return;
        }

        control_unit->type = static_cast<Unit_Type>(m_unit_type_combo->currentData().toInt());
        ui->stackedWidget_unit_type->setCurrentIndex(control_unit->type == injector ? 0 : 1);
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_injection_type_combo()
{
    if (m_injection_type_combo == nullptr)
    {
        return;
    }

    m_injection_type_combo->clear();

    const Injection_Type ordered_types[] = {
        single,
        group,
        surface,
        volume,
        cone,
        plain_oriface_atomizer,
        pressure_swirl_atomizer,
        air_blast_atomizer,
        flat_fan_atomizer,
        effervescent_atomizer,
        file_,
        condensate
    };

    for (Injection_Type type : ordered_types)
    {
        m_injection_type_combo->addItem(injection_type_display_name(type), static_cast<int>(type));
    }

    connect(m_injection_type_combo, &QUI_ComboBox::selection_committed, this, [this]()
    {
        if (control_unit == nullptr || m_injection_type_combo == nullptr)
        {
            return;
        }

        control_unit->inj.injector_data.injection_type = static_cast<Injection_Type>(
            m_injection_type_combo->currentData().toInt());
        normalize_model_dependencies(control_unit->inj.injector_data);
        sync_case_context_constraints();
        normalize_diameter_distribution_for_type(control_unit->inj.injector_data);
        sync_diameter_distribution_combo();
        sync_particle_type_group();
        sync_stagger_controls();
        sync_auxiliary_panels();
        sync_particle_type_dependent_controls();
        build_point_property_rows();
        build_model_property_rows();
        notify_injector_data_changed();
    });
}

void unit_edit_dialog::initialize_particle_type_group()
{
    if (m_particle_type_group == nullptr)
    {
        return;
    }

    m_particle_type_group->clear_options();
    const DPM_Type ordered_types[] = {Massless, Inert, Droplet, Combusting, Multicomponent};
    for (DPM_Type type : ordered_types)
    {
        m_particle_type_group->add_option(particle_type_display_name(type), static_cast<int>(type));
    }

    connect(m_particle_type_group, &QUI_RadioGroup::value_committed, this, [this](int checked_id)
    {
        if (control_unit == nullptr)
        {
            return;
        }

        control_unit->inj.injector_data.type = static_cast<DPM_Type>(checked_id);
        normalize_model_dependencies(control_unit->inj.injector_data);
        sync_case_context_constraints();
        if (!particle_type_supports_diameter_distribution(control_unit->inj.injector_data.type))
        {
            apply_diameter_distribution_index(control_unit->inj.injector_data, 0);
        }
        sync_diameter_distribution_combo();
        sync_auxiliary_panels();
        sync_particle_type_dependent_controls();
        build_point_property_rows();
        build_model_property_rows();
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_material_combo()
{
    if (m_material_combo == nullptr)
    {
        return;
    }

    m_material_combo->bind_current_text(&control_unit->inj.injector_data.material);
    connect(m_material_combo, &QUI_ComboBox::selection_committed, this, [this]()
    {
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_diameter_distribution_combo()
{
    if (m_diameter_distribution_combo == nullptr)
    {
        return;
    }

    m_diameter_distribution_combo->clear();
    const QList<Diameter_Distribution_Option> options =
        diameter_distribution_options_for(control_unit->inj.injector_data.injection_type);
    for (const Diameter_Distribution_Option &option : options)
    {
        m_diameter_distribution_combo->addItem(option.label, option.mode);
    }
    m_diameter_distribution_combo->setEnabled(options.size() > 1);

    connect(m_diameter_distribution_combo, &QUI_ComboBox::selection_committed, this, [this]()
    {
        if (control_unit == nullptr || m_diameter_distribution_combo == nullptr)
        {
            return;
        }

        apply_diameter_distribution_index(control_unit->inj.injector_data,
                                          m_diameter_distribution_combo->currentData().toInt());
        build_point_property_rows();
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_discrete_phase_domain_combo()
{
    if (m_discrete_phase_domain_combo == nullptr)
    {
        return;
    }

    m_discrete_phase_domain_combo->bind_current_text(&control_unit->inj.injector_data.dpm_domain);
    connect(m_discrete_phase_domain_combo, &QUI_ComboBox::selection_committed, this, [this]()
    {
        if (control_unit == nullptr)
        {
            return;
        }

        normalize_model_dependencies(control_unit->inj.injector_data);
        sync_case_context_constraints();
        sync_injection_type_combo();
        sync_auxiliary_panels();
        sync_particle_type_dependent_controls();
        build_point_property_rows();
        build_model_property_rows();
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::initialize_species_combos()
{
    if (control_unit == nullptr)
    {
        return;
    }

    if (m_devolatilizing_species_combo != nullptr)
    {
        m_devolatilizing_species_combo->bind_current_text(&control_unit->inj.injector_data.devolatilizing_species);
        connect(m_devolatilizing_species_combo, &QUI_ComboBox::selection_committed, this, [this]()
        {
            notify_injector_data_changed(false);
        });
    }

    if (m_evaporating_species_combo != nullptr)
    {
        m_evaporating_species_combo->bind_current_text(&control_unit->inj.injector_data.evaporating_species);
        connect(m_evaporating_species_combo, &QUI_ComboBox::selection_committed, this, [this]()
        {
            notify_injector_data_changed(false);
        });
    }

    if (m_product_species_combo != nullptr)
    {
        m_product_species_combo->bind_current_text(&control_unit->inj.injector_data.product_species);
        connect(m_product_species_combo, &QUI_ComboBox::selection_committed, this, [this]()
        {
            notify_injector_data_changed(false);
        });
    }

    if (m_oxidizing_species_combo != nullptr)
    {
        m_oxidizing_species_combo->bind_current_text(&control_unit->inj.injector_data.oxidizing_species);
        connect(m_oxidizing_species_combo, &QUI_ComboBox::selection_committed, this, [this]()
        {
            notify_injector_data_changed(false);
        });
    }
}

void unit_edit_dialog::initialize_cone_type_combo()
{
    if (ui->comboBox_conetype == nullptr)
    {
        return;
    }

    ui->comboBox_conetype->clear();

    const Cone_Type ordered_types[] = {point, hollow, ring, solid};
    for (Cone_Type type : ordered_types)
    {
        ui->comboBox_conetype->addItem(cone_type_display_name(type), static_cast<int>(type));
    }

    connect(ui->comboBox_conetype,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int)
            {
                if (control_unit == nullptr || ui->comboBox_conetype == nullptr)
                {
                    return;
                }

                control_unit->inj.injector_data.cone_type = static_cast<Cone_Type>(
                    ui->comboBox_conetype->currentData().toInt());
                normalize_model_dependencies(control_unit->inj.injector_data);
                build_point_property_rows();
                build_model_property_rows();
                notify_injector_data_changed();
            });
}

void unit_edit_dialog::initialize_stagger_controls()
{
    if (m_stagger_check == nullptr || m_stagger_radius_edit == nullptr)
    {
        return;
    }

    m_stagger_radius_edit->bind_value(&control_unit->inj.injector_data.stagger_radius);
    connect(m_stagger_check, &QUI_CheckBox::value_committed, this, [this](bool)
    {
        const bool radius_supported =
            control_unit != nullptr &&
            !uses_atomizer_stagger(control_unit->inj.injector_data);
        apply_labeled_control_enabled(
            m_stagger_radius_edit,
            ui != nullptr ? static_cast<QWidget *>(ui->label_stagger) : nullptr,
            m_stagger_check != nullptr &&
                m_stagger_check->isChecked() &&
                radius_supported);
        sync_model_property_rows();
        notify_injector_data_changed(false);
    });
    connect(m_stagger_radius_edit, &QUI_LineEdit::value_committed, this, [this]()
    {
        notify_injector_data_changed(false);
    });
}

void unit_edit_dialog::sync_unit_type_combo()
{
    if (m_unit_type_combo == nullptr || control_unit == nullptr)
    {
        return;
    }

    QSignalBlocker blocker(m_unit_type_combo);
    const int target_value = static_cast<int>(control_unit->type);
    for (int i = 0; i < m_unit_type_combo->count(); ++i)
    {
        if (m_unit_type_combo->itemData(i).toInt() == target_value)
        {
            m_unit_type_combo->setCurrentIndex(i);
            break;
        }
    }

    ui->stackedWidget_unit_type->setCurrentIndex(control_unit->type == injector ? 0 : 1);
}

void unit_edit_dialog::sync_injection_type_combo()
{
    if (m_injection_type_combo == nullptr || control_unit == nullptr)
    {
        return;
    }

    QSignalBlocker blocker(m_injection_type_combo);
    const int target_value = static_cast<int>(control_unit->inj.injector_data.injection_type);
    const bool volume_available = volume_injection_available(
        m_case_context,
        control_unit->inj.injector_data.dpm_domain);
    const bool cone_available = cone_injection_available(m_case_context);
    const bool flat_fan_available = !feature_is_disabled(m_case_context.three_dimensional);
    const bool condensate_available = !heat_transfer_is_disabled(m_case_context);
    for (int i = 0; i < m_injection_type_combo->count(); ++i)
    {
        if (auto *model = qobject_cast<QStandardItemModel *>(m_injection_type_combo->model()))
        {
            if (QStandardItem *item = model->item(i))
            {
                const Injection_Type type = static_cast<Injection_Type>(
                    m_injection_type_combo->itemData(i).toInt());
                const bool available =
                    (type != volume || volume_available) &&
                    (type != cone || cone_available) &&
                    (type != flat_fan_atomizer || flat_fan_available) &&
                    (type != condensate || condensate_available);
                item->setEnabled(available);
                item->setFlags(available
                                   ? (item->flags() | Qt::ItemIsEnabled)
                                   : (item->flags() & ~Qt::ItemIsEnabled));
            }
        }
    }

    for (int i = 0; i < m_injection_type_combo->count(); ++i)
    {
        if (m_injection_type_combo->itemData(i).toInt() == target_value)
        {
            m_injection_type_combo->setCurrentIndex(i);
            break;
        }
    }
}

void unit_edit_dialog::sync_particle_type_group()
{
    if (m_particle_type_group == nullptr || control_unit == nullptr)
    {
        return;
    }

    m_particle_type_group->set_checked_id(static_cast<int>(control_unit->inj.injector_data.type));

    const DPM_Type allowed_types[] = {Massless, Inert, Droplet, Combusting, Multicomponent};
    const QList<QRadioButton *> buttons = m_particle_type_group->findChildren<QRadioButton *>();
    for (QRadioButton *button : buttons)
    {
        if (button == nullptr)
        {
            continue;
        }

        bool allowed = true;
        for (DPM_Type type : allowed_types)
        {
            if (button->text() == particle_type_display_name(type))
            {
                allowed = particle_type_allowed_for_injection(
                    control_unit->inj.injector_data.injection_type, type) &&
                    particle_type_allowed_for_case_context(m_case_context, type);
                break;
            }
        }
        button->setEnabled(allowed);
    }
}

void unit_edit_dialog::sync_particle_type_dependent_controls()
{
    if (control_unit == nullptr)
    {
        return;
    }

    const DPM_Type particle_type = control_unit->inj.injector_data.type;
    const bool enable_material = particle_type_supports_material(particle_type);
    const bool enable_diameter_distribution = particle_type_supports_diameter_distribution(particle_type);
    const bool enable_evaporating_species =
        particle_type_supports_evaporating_species(particle_type) ||
        (particle_type == Combusting &&
         control_unit->inj.injector_data.evaporating_liquid);
    const bool enable_devolatilizing_species = particle_type_supports_devolatilizing_species(particle_type);
    const bool enable_combustion_species = particle_type == Combusting;
    const bool enable_surface_reaction_species =
        enable_combustion_species &&
        !feature_is_disabled(m_case_context.multiple_surface_reaction) &&
        !feature_is_enabled(m_case_context.material_multiple_surface_reaction);

    const bool distribution_was_reset =
        !enable_diameter_distribution &&
        diameter_distribution_index(control_unit->inj.injector_data) != 0;
    if (distribution_was_reset)
    {
        apply_diameter_distribution_index(control_unit->inj.injector_data, 0);
    }

    if (distribution_was_reset)
    {
        sync_diameter_distribution_combo();
    }

    apply_labeled_control_enabled(m_material_combo, ui->label_material, enable_material);
    apply_labeled_control_enabled(m_diameter_distribution_combo,
                                  ui->label_diameter_distribution,
                                  enable_diameter_distribution);
    apply_labeled_control_enabled(m_discrete_phase_domain_combo,
                                  ui->label_discrete_phase_domain,
                                  false);
    apply_labeled_control_enabled(m_oxidizing_species_combo,
                                  ui->label_oxidizing_species,
                                  enable_surface_reaction_species);
    apply_labeled_control_enabled(m_evaporating_species_combo,
                                  ui->label_evaporating_species,
                                  enable_evaporating_species);
    apply_labeled_control_enabled(m_devolatilizing_species_combo,
                                  ui->label_devolatilizing_species,
                                  enable_devolatilizing_species);
    apply_labeled_control_enabled(m_product_species_combo,
                                  ui->label_product_species,
                                  enable_surface_reaction_species);
}

void unit_edit_dialog::sync_material_combo()
{
    if (m_material_combo == nullptr || control_unit == nullptr)
    {
        return;
    }

    QSignalBlocker blocker(m_material_combo);
    const QStringList options = material_options_for(m_material_names);
    m_material_combo->set_options(options);

    const QString current_material = control_unit->inj.injector_data.material;
    if (m_material_names.isEmpty())
    {
        m_material_combo->setCurrentIndex(-1);
        m_material_combo->setEditText(current_material);
        return;
    }

    const int target_index = m_material_combo->findText(current_material);
    if (target_index >= 0)
    {
        m_material_combo->setCurrentIndex(target_index);
        return;
    }

    m_material_combo->setCurrentIndex(-1);
    m_material_combo->setEditText(current_material);
}

void unit_edit_dialog::sync_diameter_distribution_combo()
{
    if (m_diameter_distribution_combo == nullptr || control_unit == nullptr)
    {
        return;
    }

    const QList<Diameter_Distribution_Option> options =
        diameter_distribution_options_for(control_unit->inj.injector_data.injection_type);
    QSignalBlocker blocker(m_diameter_distribution_combo);
    m_diameter_distribution_combo->clear();
    for (const Diameter_Distribution_Option &option : options)
    {
        m_diameter_distribution_combo->addItem(option.label, option.mode);
    }
    m_diameter_distribution_combo->setEnabled(options.size() > 1);

    normalize_diameter_distribution_for_type(control_unit->inj.injector_data);
    const int target_data = diameter_distribution_index(control_unit->inj.injector_data);
    for (int i = 0; i < m_diameter_distribution_combo->count(); ++i)
    {
        if (m_diameter_distribution_combo->itemData(i).toInt() == target_data)
        {
            m_diameter_distribution_combo->setCurrentIndex(i);
            break;
        }
    }
}

void unit_edit_dialog::sync_discrete_phase_domain_combo()
{
    if (m_discrete_phase_domain_combo == nullptr || control_unit == nullptr)
    {
        return;
    }

    QSignalBlocker blocker(m_discrete_phase_domain_combo);
    const QStringList options = discrete_phase_domain_options_for(control_unit->inj.injector_data.dpm_domain);
    m_discrete_phase_domain_combo->set_options(options);
    m_discrete_phase_domain_combo->sync_from_binding();
}

void unit_edit_dialog::sync_species_combos()
{
    if (control_unit == nullptr)
    {
        return;
    }

    if (m_devolatilizing_species_combo != nullptr)
    {
        QSignalBlocker blocker(m_devolatilizing_species_combo);
        m_devolatilizing_species_combo->set_options(
            species_options_for(m_chemkin_species_names,
                                control_unit->inj.injector_data.devolatilizing_species));
        m_devolatilizing_species_combo->sync_from_binding();
    }

    if (m_evaporating_species_combo != nullptr)
    {
        QSignalBlocker blocker(m_evaporating_species_combo);
        m_evaporating_species_combo->set_options(
            species_options_for(m_chemkin_species_names,
                                control_unit->inj.injector_data.evaporating_species));
        m_evaporating_species_combo->sync_from_binding();
    }

    if (m_product_species_combo != nullptr)
    {
        QSignalBlocker blocker(m_product_species_combo);
        m_product_species_combo->set_options(
            species_options_for(m_chemkin_species_names,
                                control_unit->inj.injector_data.product_species));
        m_product_species_combo->sync_from_binding();
    }

    if (m_oxidizing_species_combo != nullptr)
    {
        QSignalBlocker blocker(m_oxidizing_species_combo);
        m_oxidizing_species_combo->set_options(
            species_options_for(m_chemkin_species_names,
                                control_unit->inj.injector_data.oxidizing_species));
        m_oxidizing_species_combo->sync_from_binding();
    }
}

void unit_edit_dialog::sync_cone_type_combo()
{
    if (ui->comboBox_conetype == nullptr || control_unit == nullptr)
    {
        return;
    }

    QSignalBlocker blocker(ui->comboBox_conetype);
    const int target_value = static_cast<int>(control_unit->inj.injector_data.cone_type);
    for (int i = 0; i < ui->comboBox_conetype->count(); ++i)
    {
        if (ui->comboBox_conetype->itemData(i).toInt() == target_value)
        {
            ui->comboBox_conetype->setCurrentIndex(i);
            break;
        }
    }
}

void unit_edit_dialog::sync_stagger_controls()
{
    if (m_stagger_check == nullptr || m_stagger_radius_edit == nullptr || control_unit == nullptr)
    {
        return;
    }

    bool *stagger_flag = uses_atomizer_stagger(control_unit->inj.injector_data)
                             ? &control_unit->inj.injector_data.spatial_staggering_atomizer_on
                             : &control_unit->inj.injector_data.spatial_staggering_std_inj_on;
    const bool stagger_supported =
        uses_atomizer_stagger(control_unit->inj.injector_data) ||
        uses_standard_stagger(control_unit->inj.injector_data.injection_type);

    QSignalBlocker check_blocker(m_stagger_check);
    m_stagger_check->setEnabled(stagger_supported);
    m_stagger_check->bind_value(stagger_flag);
    m_stagger_check->sync_from_binding();
    m_stagger_radius_edit->bind_value(&control_unit->inj.injector_data.stagger_radius);
    const bool radius_supported =
        !uses_atomizer_stagger(control_unit->inj.injector_data) &&
        uses_standard_stagger(control_unit->inj.injector_data.injection_type);
    apply_labeled_control_enabled(
        m_stagger_radius_edit,
        ui != nullptr ? static_cast<QWidget *>(ui->label_stagger) : nullptr,
        *stagger_flag && radius_supported);
}

void unit_edit_dialog::sync_auxiliary_panels()
{
    if (control_unit == nullptr)
    {
        return;
    }

    const bool show_cone_panel = (control_unit->inj.injector_data.injection_type == cone);
    const bool show_stagger_panel = true;
    // Fluent exposes generic stream count for group, surface, cone, and
    // atomizer injections. Volume injections use their own stream rules.
    const bool show_generic_stream_count = uses_generic_stream_count(
        control_unit->inj.injector_data.injection_type);

    if (ui->label_number_of_stream != nullptr)
    {
        ui->label_number_of_stream->setVisible(show_generic_stream_count);
    }
    if (m_number_of_stream_spin != nullptr)
    {
        m_number_of_stream_spin->setVisible(show_generic_stream_count);
    }

    if (ui->cone_parameter_layout != nullptr)
    {
        ui->cone_parameter_layout->setVisible(show_cone_panel);
    }
    if (ui->label_conetype != nullptr)
    {
        ui->label_conetype->setVisible(show_cone_panel);
    }
    if (ui->comboBox_conetype != nullptr)
    {
        ui->comboBox_conetype->setVisible(show_cone_panel);
    }

    if (ui->tabWidget_injection != nullptr && ui->tab_parcel != nullptr)
    {
        const int parcel_index = ui->tabWidget_injection->indexOf(ui->tab_parcel);
        if (parcel_index >= 0)
        {
            const bool parcel_available = !uses_dense_discrete_phase_domain(
                control_unit->inj.injector_data.dpm_domain) &&
                control_unit->inj.injector_data.injection_type != surface &&
                control_unit->inj.injector_data.injection_type != volume &&
                control_unit->inj.injector_data.injection_type != condensate &&
                !feature_is_disabled(m_case_context.unsteady_particle_tracking);
            ui->tabWidget_injection->setTabEnabled(parcel_index, parcel_available);
            if (!parcel_available && ui->tabWidget_injection->currentIndex() == parcel_index)
            {
                ui->tabWidget_injection->setCurrentIndex(0);
            }
        }
    }

    if (ui->stagger_layout == nullptr)
    {
        if (ui->tabWidget_injection != nullptr && ui->tab_wet_combustion != nullptr)
        {
            const int wet_index = ui->tabWidget_injection->indexOf(ui->tab_wet_combustion);
            if (wet_index >= 0)
            {
                ui->tabWidget_injection->setTabVisible(
                    wet_index,
                    control_unit->inj.injector_data.type == Droplet);
            }
        }
        return;
    }

    ui->stagger_layout->setVisible(show_stagger_panel);
    if (m_stagger_check != nullptr)
    {
        m_stagger_check->setVisible(show_stagger_panel);
    }
    if (ui->label_stagger != nullptr)
    {
        ui->label_stagger->setVisible(show_stagger_panel);
    }
    if (m_stagger_radius_edit != nullptr)
    {
        m_stagger_radius_edit->setVisible(show_stagger_panel);
    }

    if (ui->tabWidget_injection != nullptr && ui->tab_wet_combustion != nullptr)
    {
        const int wet_index = ui->tabWidget_injection->indexOf(ui->tab_wet_combustion);
        if (wet_index >= 0)
        {
            const bool show_wet_combustion =
                control_unit->inj.injector_data.type == Combusting;
            ui->tabWidget_injection->setTabVisible(wet_index, show_wet_combustion);
            if (!show_wet_combustion && ui->tabWidget_injection->currentIndex() == wet_index)
            {
                ui->tabWidget_injection->setCurrentIndex(0);
            }
        }
    }
}

QString unit_edit_dialog::current_property_layout_key() const
{
    if (control_unit == nullptr)
    {
        return QString();
    }

    return property_layout_key_for(control_unit->inj.injector_data);
}

QString unit_edit_dialog::current_model_layout_key() const
{
    if (control_unit == nullptr)
    {
        return QString();
    }

    return model_layout_key_for(control_unit->inj.injector_data);
}

void unit_edit_dialog::build_point_property_rows()
{
    if (ui->verticalLayout_3 == nullptr || control_unit == nullptr)
    {
        return;
    }

    clear_layout(ui->verticalLayout_3);
    m_property_row_syncers.clear();
    m_property_layout_key = current_property_layout_key();

    Injector &injector = control_unit->inj.injector_data;
    Injector *injector_ptr = &injector;

    auto add_info_label = [this](const QString &text)
    {
        QLabel *label = new QLabel(text, ui->scrollarea_properties);
        label->setWordWrap(true);
        label->setStyleSheet("color: rgb(110, 116, 126); padding: 4px 2px;");
        ui->verticalLayout_3->addWidget(label);
    };

    auto add_single_row = [this](const QString &label,
                                 const QString &unit,
                                 double *value_ptr,
                                 bool geometry_changed = true,
                                 double minimum = -std::numeric_limits<double>::max(),
                                 double maximum = std::numeric_limits<double>::max())
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_double_mode();
        row->primary_editor()->set_numeric_range(minimum, maximum);
        row->primary_editor()->bind_value(value_ptr);
        m_property_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_bound_value();
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this, [this, geometry_changed]()
        {
            notify_injector_data_changed(geometry_changed);
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_single_int_row = [this](const QString &label,
                                     const QString &unit,
                                     int *value_ptr,
                                     int minimum = std::numeric_limits<int>::min(),
                                     int maximum = std::numeric_limits<int>::max())
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_integer_mode();
        row->primary_editor()->set_numeric_range(minimum, maximum);
        row->primary_editor()->bind_value(value_ptr);
        m_property_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_text(QString::number(*value_ptr));
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this, [this]()
        {
            notify_injector_data_changed();
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_single_string_row = [this](const QString &label, const QString &unit, QString *value_ptr)
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_string_mode();
        row->primary_editor()->set_allow_empty_string(true);
        row->primary_editor()->bind_value(value_ptr);
        m_property_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_text(*value_ptr);
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                [this]() { notify_injector_data_changed(false); });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_single_bool_row = [this](const QString &label, bool *value_ptr) -> QWidget *
    {
        QWidget *row = new QWidget(ui->scrollarea_properties);
        row->setObjectName(QString("propertyRow_%1").arg(label.simplified().replace(' ', '_')));
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 1, 0, 1);
        layout->setSpacing(6);
        auto *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(180);
        label_widget->setMaximumWidth(180);
        auto *check_box = new QUI_CheckBox(QString(), row);
        check_box->bind_value(value_ptr);
        layout->addWidget(label_widget);
        layout->addWidget(check_box, 1);
        m_property_row_syncers.push_back([check_box]()
        {
            if (check_box != nullptr)
            {
                check_box->sync_from_binding();
            }
        });
        connect(check_box, &QUI_CheckBox::value_committed, this,
                [this](bool) { notify_injector_data_changed(); });
        ui->verticalLayout_3->addWidget(row);
        return row;
    };

    auto add_int_list_row = [this](const QString &label, QVector<int> *value_ptr)
    {
        if (value_ptr == nullptr)
        {
            return;
        }

        QUI_FieldRow *row = new QUI_FieldRow(label, "ids", ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_string_mode();
        row->primary_editor()->sync_text([value_ptr]()
        {
            QStringList values;
            for (int value : *value_ptr)
            {
                values.push_back(QString::number(value));
            }
            return values.join(", ");
        }());
        m_property_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                QStringList values;
                for (int value : *value_ptr)
                {
                    values.push_back(QString::number(value));
                }
                editor->sync_text(values.join(", "));
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                [this, editor = row->primary_editor(), value_ptr]()
        {
            const QString source = editor->text().trimmed();
            QVector<int> parsed_values;
            bool valid = true;
            if (!source.isEmpty())
            {
                const QStringList tokens = source.split(
                    QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
                for (const QString &token : tokens)
                {
                    bool ok = false;
                    const int value = token.toInt(&ok);
                    if (!ok)
                    {
                        valid = false;
                        break;
                    }
                    parsed_values.push_back(value);
                }
            }

            if (!valid)
            {
                QStringList values;
                for (int value : *value_ptr)
                {
                    values.push_back(QString::number(value));
                }
                editor->sync_text(values.join(", "));
                return;
            }

            *value_ptr = parsed_values;
            notify_injector_data_changed();
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_combo_row = [this](const QString &label,
                                const QStringList &options,
                                const std::function<int()> &value_getter,
                                const std::function<void(int)> &value_setter)
    {
        QWidget *row = new QWidget(ui->scrollarea_properties);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 1, 0, 1);
        layout->setSpacing(6);

        QUI_Label *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(180);
        label_widget->setMaximumWidth(180);
        QUI_ComboBox *combo = new QUI_ComboBox(row);
        combo->setObjectName(QString("propertyEditor_%1").arg(label.simplified().replace(' ', '_')));
        combo->set_options(options);
        combo->setCurrentIndex(qBound(0, value_getter(), options.size() - 1));
        layout->addWidget(label_widget, 0);
        layout->addWidget(combo, 1);

        m_property_row_syncers.push_back([combo, value_getter]()
        {
            if (combo != nullptr)
            {
                const QSignalBlocker blocker(combo);
                combo->setCurrentIndex(value_getter());
            }
        });
        connect(combo, &QUI_ComboBox::selection_committed, this,
                [this, combo, value_setter]()
        {
            value_setter(combo->currentIndex());
            notify_injector_data_changed();
            QTimer::singleShot(0, this, [this]()
            {
                if (control_unit != nullptr)
                {
                    build_point_property_rows();
                }
            });
        });

        ui->verticalLayout_3->addWidget(row);
    };

    auto add_bool_row = [this](const QString &label, bool *value_ptr)
    {
        QWidget *row = new QWidget(ui->scrollarea_properties);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 1, 0, 1);
        layout->setSpacing(6);

        QUI_Label *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(180);
        label_widget->setMaximumWidth(180);
        QUI_CheckBox *check_box = new QUI_CheckBox(QString(), row);
        check_box->bind_value(value_ptr);
        layout->addWidget(label_widget, 0);
        layout->addWidget(check_box, 1);

        m_property_row_syncers.push_back([check_box]()
        {
            if (check_box != nullptr)
            {
                check_box->sync_from_binding();
            }
        });
        connect(check_box, &QUI_CheckBox::value_committed, this,
                [this](bool)
        {
            notify_injector_data_changed();
        });

        ui->verticalLayout_3->addWidget(row);
    };

    auto add_range_row = [this](const QString &label,
                                const QString &unit,
                                double *first_ptr,
                                double *second_ptr,
                                bool geometry_changed = true)
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->set_layout_mode(QUI_FieldRow::Layout_Mode::RangeValue);
        row->primary_editor()->set_double_mode();
        row->secondary_editor()->set_double_mode();
        row->primary_editor()->bind_value(first_ptr);
        row->secondary_editor()->bind_value(second_ptr);
        m_property_row_syncers.push_back([primary = row->primary_editor(), secondary = row->secondary_editor(), first_ptr, second_ptr]()
        {
            if (primary != nullptr && first_ptr != nullptr)
            {
                primary->sync_bound_value();
            }
            if (secondary != nullptr && second_ptr != nullptr)
            {
                secondary->sync_bound_value();
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this, [this, geometry_changed]()
        {
            notify_injector_data_changed(geometry_changed);
        });
        connect(row->secondary_editor(), &QUI_LineEdit::value_committed, this, [this, geometry_changed]()
        {
            notify_injector_data_changed(geometry_changed);
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_single_vector_row = [this](const QString &label, const QString &unit, QVector3D *vector, int component)
    {
        if (component == 2 && feature_is_disabled(m_case_context.three_dimensional))
        {
            return;
        }

        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_double_mode();
        row->primary_editor()->sync_numeric_value(vector_component_value(*vector, component));
        m_property_row_syncers.push_back([editor = row->primary_editor(), vector, component]()
        {
            if (editor != nullptr && vector != nullptr)
            {
                editor->sync_numeric_value(vector_component_value(*vector, component));
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this, [this, editor = row->primary_editor(), vector, component]()
        {
            double value = 0.0;
            if (editor->numeric_value_in_storage(value))
            {
                set_vector_component_value(*vector, component, static_cast<float>(value));
                notify_injector_data_changed();
            }
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_range_vector_row = [this](const QString &label,
                                       const QString &unit,
                                       QVector3D *first_vector,
                                       QVector3D *second_vector,
                                       int component)
    {
        if (component == 2 && feature_is_disabled(m_case_context.three_dimensional))
        {
            return;
        }

        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->set_layout_mode(QUI_FieldRow::Layout_Mode::RangeValue);
        row->primary_editor()->set_double_mode();
        row->secondary_editor()->set_double_mode();
        row->primary_editor()->sync_numeric_value(vector_component_value(*first_vector, component));
        row->secondary_editor()->sync_numeric_value(vector_component_value(*second_vector, component));
        m_property_row_syncers.push_back([primary = row->primary_editor(),
                                          secondary = row->secondary_editor(),
                                          first_vector,
                                          second_vector,
                                          component]()
        {
            if (primary != nullptr && first_vector != nullptr)
            {
                primary->sync_numeric_value(vector_component_value(*first_vector, component));
            }
            if (secondary != nullptr && second_vector != nullptr)
            {
                secondary->sync_numeric_value(vector_component_value(*second_vector, component));
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this, [this, editor = row->primary_editor(), first_vector, component]()
        {
            double value = 0.0;
            if (editor->numeric_value_in_storage(value))
            {
                set_vector_component_value(*first_vector, component, static_cast<float>(value));
                notify_injector_data_changed();
            }
        });
        connect(row->secondary_editor(), &QUI_LineEdit::value_committed, this, [this, editor = row->secondary_editor(), second_vector, component]()
        {
            double value = 0.0;
            if (editor->numeric_value_in_storage(value))
            {
                set_vector_component_value(*second_vector, component, static_cast<float>(value));
                notify_injector_data_changed();
            }
        });
        ui->verticalLayout_3->addWidget(row);
    };

    auto add_single_header = [this]()
    {
        ui->verticalLayout_3->addWidget(create_property_header(ui->scrollarea_properties, "Variable", "Value"));
    };

    auto add_range_header = [this]()
    {
        ui->verticalLayout_3->addWidget(create_property_header(ui->scrollarea_properties, "Variable", "First Point", "Last Point"));
    };

    const bool massless_particle = injector.type == Massless;

    switch (injector.injection_type)
    {
    case group:
        add_range_header();
        add_range_vector_row("X-Position", "mm", &injector.pos, &injector.pos2, 0);
        add_range_vector_row("Y-Position", "mm", &injector.pos, &injector.pos2, 1);
        add_range_vector_row("Z-Position", "mm", &injector.pos, &injector.pos2, 2);
        if (!massless_particle)
        {
            add_range_vector_row("X-Velocity", "m/s", &injector.vel, &injector.vel2, 0);
            add_range_vector_row("Y-Velocity", "m/s", &injector.vel, &injector.vel2, 1);
            add_range_vector_row("Z-Velocity", "m/s", &injector.vel, &injector.vel2, 2);
            add_range_row("Diameter", "m", &injector.diameter, &injector.diameter2);
            add_range_row("Temperature", "K", &injector.temperature, &injector.temperature2);
            add_range_row("Flow Rate", "kg/s", &injector.flow_rate, &injector.flow_rate2);
        }
        break;

    case cone:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Axis", "-", &injector.axis, 0);
        add_single_vector_row("Y-Axis", "-", &injector.axis, 1);
        add_single_vector_row("Z-Axis", "-", &injector.axis, 2);
        add_single_row("Cone Angle", "deg", &injector.cone_angle, true, 0.0, kMaxConeAngleDegrees);
        if (injector.cone_type != point)
        {
            add_single_row("Outer Radius", "mm", &injector.radius, true, kPositiveMinimum);
        }
        if (injector.cone_type == ring)
        {
            add_single_row("Inner Radius", "mm", &injector.inner_radius, true, 0.0);
        }
        if (!massless_particle)
        {
            add_single_row("Velocity Magnitude", "m/s", &injector.vel_mag, false, 0.0);
            add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate, false, 0.0);
        }
        break;

    case volume:
    {
        add_single_header();
        add_combo_row(
            "Volume Specification",
            {"Zone", "Bounding Geometry"},
            [injector_ptr]() { return static_cast<int>(injector_ptr->volume_specification); },
            [injector_ptr](int value) { injector_ptr->volume_specification = static_cast<Volume_Specification>(value); });
        if (injector.volume_specification == zone)
        {
            add_int_list_row("Volume Zones", &injector.volume_zones);
        }
        else
        {
            add_combo_row(
                "Bounding Shape",
                {"Sphere", "Cylinder", "Cone", "Hexahedron"},
                 [injector_ptr]() { return static_cast<int>(injector_ptr->volume_bgeom_shapes); },
                 [injector_ptr](int value) { injector_ptr->volume_bgeom_shapes = static_cast<Volume_Bgeom_Shapes>(value); });

            // Bounding geometry fields are meaningful only for the selected
            // shape. Zone-based volume injections do not expose any of them.
            switch (injector.volume_bgeom_shapes)
            {
            case sphere:
                add_single_vector_row("X-Center", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Center", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Center", "mm", &injector.volume_bgeom_min, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius, true, kPositiveMinimum);
                break;
            case cylinder:
                add_single_vector_row("X-Min", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Min", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Min", "mm", &injector.volume_bgeom_min, 2);
                add_single_vector_row("X-Max", "mm", &injector.volume_bgeom_max, 0);
                add_single_vector_row("Y-Max", "mm", &injector.volume_bgeom_max, 1);
                add_single_vector_row("Z-Max", "mm", &injector.volume_bgeom_max, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius, true, kPositiveMinimum);
                break;
            case cone_:
                add_single_vector_row("X-Min", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Min", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Min", "mm", &injector.volume_bgeom_min, 2);
                add_single_vector_row("X-Max", "mm", &injector.volume_bgeom_max, 0);
                add_single_vector_row("Y-Max", "mm", &injector.volume_bgeom_max, 1);
                add_single_vector_row("Z-Max", "mm", &injector.volume_bgeom_max, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius, true, kPositiveMinimum);
                add_single_row("Cone Angle", "rad", &injector.volume_bgeom_viconeangle, true, 0.0, 3.14159265358979323846);
                break;
            case hexahedron:
            default:
                add_single_vector_row("X-Min", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Min", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Min", "mm", &injector.volume_bgeom_min, 2);
                add_single_vector_row("X-Max", "mm", &injector.volume_bgeom_max, 0);
                add_single_vector_row("Y-Max", "mm", &injector.volume_bgeom_max, 1);
                add_single_vector_row("Z-Max", "mm", &injector.volume_bgeom_max, 2);
                break;
            }
        }
        const bool ddpm_volume = uses_dense_discrete_phase_domain(injector.dpm_domain);
        if (ddpm_volume)
        {
            // DDPM calculates starting points automatically from the mesh and
            // packing limit, so it has no Parcel Specification selector.
            injector.volume_streams_spec = total_parcel_count;
        }
        else
        {
            const QStringList stream_specification_options =
                injector.volume_specification == bouning_geometry
                    ? QStringList{"Total Parcel Count"}
                    : QStringList{"Total Parcel Count", "Parcel Per Cell"};
            add_combo_row(
                "Stream Specification",
                stream_specification_options,
                [injector_ptr]() { return static_cast<int>(injector_ptr->volume_streams_spec); },
                [injector_ptr](int value) { injector_ptr->volume_streams_spec = static_cast<Volume_Streams_Spec>(value); });
            if (injector.volume_streams_spec == total_parcel_count)
            {
                add_single_int_row("Total Streams", "-", &injector.volume_streams_total, 1);
            }
            else
            {
                add_single_int_row("Streams Per Cell", "-", &injector.volume_streams_per_cell, 1);
            }
        }
        if (injector.type != Massless)
        {
            if (injector.mass_input_on)
            {
                add_single_row("Total Mass", "kg", &injector.total_mass, false, 0.0);
            }
            else if (injector.volfrac_input_on)
            {
                add_single_row("Volume Fraction", "-", &injector.volume_fraction, false, 0.0, 1.0);
            }
            else
            {
                add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate, false, 0.0);
            }
            if (ddpm_volume)
            {
                add_single_row("Packing Limit", "-", &injector.volume_packing_limit_per_cell, false, 0.0, 1.0);
            }
            auto *mass_input_row = add_single_bool_row("Mass Input", &injector.mass_input_on);
            auto *volume_fraction_input_row = add_single_bool_row(
                "Volume Fraction Input", &injector.volfrac_input_on);
            if (mass_input_row != nullptr)
            {
                mass_input_row->setEnabled(
                    !injector.volfrac_input_on || injector.mass_input_on);
                auto *mass_check = mass_input_row->findChild<QUI_CheckBox*>();
                if (mass_check != nullptr)
                {
                    mass_check->setObjectName("volumeMassInputEditor");
                    connect(mass_check, &QUI_CheckBox::value_committed, this,
                            [this, injector_ptr](bool checked)
                    {
                        if (checked)
                        {
                            injector_ptr->volfrac_input_on = false;
                        }
                        notify_injector_data_changed(false);
                        QMetaObject::invokeMethod(this, [this]()
                        {
                            build_point_property_rows();
                        }, Qt::QueuedConnection);
                    });
                }
            }
            if (volume_fraction_input_row != nullptr)
            {
                volume_fraction_input_row->setEnabled(
                    !injector.mass_input_on || injector.volfrac_input_on);
                auto *volume_fraction_check =
                    volume_fraction_input_row->findChild<QUI_CheckBox*>();
                if (volume_fraction_check != nullptr)
                {
                    volume_fraction_check->setObjectName("volumeFractionInputEditor");
                    connect(volume_fraction_check, &QUI_CheckBox::value_committed, this,
                            [this, injector_ptr](bool checked)
                    {
                        if (checked)
                        {
                            injector_ptr->mass_input_on = false;
                        }
                        notify_injector_data_changed(false);
                        QMetaObject::invokeMethod(this, [this]()
                        {
                            build_point_property_rows();
                        }, Qt::QueuedConnection);
                    });
                }
            }
        }
        break;
    }

    case plain_oriface_atomizer:
        add_single_header();
        if (massless_particle)
        {
            add_single_vector_row("X-Position", "mm", &injector.pos, 0);
            add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
            add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        }
        if (!massless_particle)
        {
            if (!feature_is_disabled(m_case_context.three_dimensional))
            {
                add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
                add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
                add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
            }
            add_single_row("Temperature", "K", &injector.temperature, true, 0.0);
            add_single_row("Flow Rate", "kg/s", &injector.total_flow_rate, true, 0.0);
            add_single_row("Vapor Pressure", "Pa", &injector.vapor_pressure, true, 0.0);
            add_single_row("Injector Inner Diameter", "m", &injector.inner_diameter, true, 0.0);
            add_single_row("Orifice Length", "m", &injector.plain_length, true, 0.0);
            add_single_row("Corner Radius of Curvature", "m", &injector.plain_corner_size, true, 0.0);
            add_single_row("Constant A", "-", &injector.plain_const_a, true, 0.0);
        }
        break;

    case pressure_swirl_atomizer:
        add_single_header();
        if (massless_particle)
        {
            add_single_vector_row("X-Position", "mm", &injector.pos, 0);
            add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
            add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        }
        if (!massless_particle)
        {
            if (!feature_is_disabled(m_case_context.three_dimensional))
            {
                add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
                add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
                add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
            }
            add_single_row("Temperature", "K", &injector.temperature, true, 0.0);
            add_single_row("Flow Rate", "kg/s", &injector.total_flow_rate, true, 0.0);
            add_single_row("Vapor Pressure", "Pa", &injector.vapor_pressure, true, 0.0);
            add_single_row("Injector Inner Diameter", "m", &injector.inner_diameter, true, 0.0);
            add_single_row("Spray Half Angle", "rad", &injector.half_angle, true, 0.0, 1.5707963267948966);
            add_single_row("Upstream Pressure", "Pa", &injector.pswirl_inj_press, true, 0.0);
            add_single_row("Sheet Constant", "-", &injector.sheet_const, true, 0.0);
            add_single_row("Ligament Constant", "-", &injector.lig_const, true, 0.0);
            add_single_row("Atomizer Dispersion Angle", "deg", &injector.atomizer_disp_angle, true, 0.0, kMaxConeAngleDegrees);
        }
        break;

    case air_blast_atomizer:
        add_single_header();
        if (massless_particle)
        {
            add_single_vector_row("X-Position", "mm", &injector.pos, 0);
            add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
            add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        }
        if (!massless_particle)
        {
            if (!feature_is_disabled(m_case_context.three_dimensional))
            {
                add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
                add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
                add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
            }
            add_single_row("Temperature", "K", &injector.temperature, true, 0.0);
            add_single_row("Flow Rate", "kg/s", &injector.total_flow_rate, true, 0.0);
            add_single_row("Vapor Pressure", "Pa", &injector.vapor_pressure, true, 0.0);
            add_single_row("Injector Inner Diameter", "m", &injector.inner_diameter, true, 0.0);
            add_single_row("Injector Outer Diameter", "m", &injector.outer_diameter, true, 0.0);
            add_single_row("Spray Half Angle", "rad", &injector.half_angle, true, 0.0, 1.5707963267948966);
            add_single_row("Relative Velocity", "m/s", &injector.airbl_rel_vel, true, 0.0);
            add_single_row("Sheet Constant", "-", &injector.sheet_const, true, 0.0);
            add_single_row("Ligament Constant", "-", &injector.lig_const, true, 0.0);
            add_single_row("Atomizer Dispersion Angle", "deg", &injector.atomizer_disp_angle, true, 0.0, kMaxConeAngleDegrees);
        }
        break;

    case flat_fan_atomizer:
        add_single_header();
        add_single_vector_row("X-Center", "mm", &injector.ff_center, 0);
        add_single_vector_row("Y-Center", "mm", &injector.ff_center, 1);
        add_single_vector_row("Z-Center", "mm", &injector.ff_center, 2);
        add_single_vector_row("X-Virtual Origin", "mm", &injector.ff_virtual_origin, 0);
        add_single_vector_row("Y-Virtual Origin", "mm", &injector.ff_virtual_origin, 1);
        add_single_vector_row("Z-Virtual Origin", "mm", &injector.ff_virtual_origin, 2);
        add_single_vector_row("X-Fan Normal", "-", &injector.ff_normal, 0);
        add_single_vector_row("Y-Fan Normal", "-", &injector.ff_normal, 1);
        add_single_vector_row("Z-Fan Normal", "-", &injector.ff_normal, 2);
        if (!massless_particle)
        {
            add_single_row("Temperature", "K", &injector.temperature, true, 0.0);
            add_single_row("Flow Rate", "kg/s", &injector.total_flow_rate, true, 0.0);
            add_single_row("Spray Half Angle", "rad", &injector.half_angle, true, 0.0, 1.5707963267948966);
            add_single_row("Orifice Width", "m", &injector.ff_oriface_width, true, 0.0);
            add_single_row("Phi Start", "rad", &injector.phi_start);
            add_single_row("Phi Stop", "rad", &injector.phi_stop);
            add_single_row("Flat Fan Sheet Constant", "-", &injector.ff_sheet_const, true, 0.0);
            add_single_row("Atomizer Dispersion Angle", "deg", &injector.atomizer_disp_angle, true, 0.0, kMaxConeAngleDegrees);
        }
        break;

    case effervescent_atomizer:
        add_single_header();
        if (massless_particle)
        {
            add_single_vector_row("X-Position", "mm", &injector.pos, 0);
            add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
            add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        }
        if (!massless_particle)
        {
            if (!feature_is_disabled(m_case_context.three_dimensional))
            {
                add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
                add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
                add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
            }
            add_single_row("Temperature", "K", &injector.temperature, true, 0.0);
            add_single_row("Flow Rate", "kg/s", &injector.total_flow_rate, true, 0.0);
            add_single_row("Vapor Pressure", "Pa", &injector.vapor_pressure, true, 0.0);
            add_single_row("Injector Inner Diameter", "m", &injector.inner_diameter, true, 0.0);
            add_single_row("Mixture Quality", "-", &injector.effer_quality, true, 0.0, 1.0);
            add_single_row("Saturation Temperature", "K", &injector.effer_t_sat, true, 0.0);
            add_single_row("Dispersion Constant", "-", &injector.effer_const, true, 0.0);
            add_single_row("Maximum Half Angle", "rad", &injector.effer_half_angle_max, true, 0.0, 1.5707963267948966);
        }
        break;

    case condensate:
        add_single_header();
        add_single_row("Start Time", "s", &injector.unsteady_start, false, 0.0);
        add_single_row("Stop Time", "s", &injector.unsteady_stop, false, 0.0);
        break;

    case surface:
        add_range_header();
        add_int_list_row("Surface Zone IDs", &injector.surfaces);
        add_int_list_row("Boundary IDs", &injector.boundary);
        if (!massless_particle)
        {
            add_range_vector_row("X-Position", "mm", &injector.pos, &injector.pos2, 0);
            add_range_vector_row("Y-Position", "mm", &injector.pos, &injector.pos2, 1);
            add_range_vector_row("Z-Position", "mm", &injector.pos, &injector.pos2, 2);
        }
        if (!massless_particle)
        {
            if (injector.use_face_normal)
            {
                add_single_row("Velocity Magnitude", "m/s", &injector.vel_mag, false, 0.0);
            }
            else
            {
                add_range_vector_row("X-Velocity", "m/s", &injector.vel, &injector.vel2, 0);
                add_range_vector_row("Y-Velocity", "m/s", &injector.vel, &injector.vel2, 1);
                add_range_vector_row("Z-Velocity", "m/s", &injector.vel, &injector.vel2, 2);
            }
            add_range_row("Diameter", "m", &injector.diameter, &injector.diameter2);
            add_range_row("Temperature", "K", &injector.temperature, &injector.temperature2);
            add_range_row("Flow Rate", "kg/s", &injector.flow_rate, &injector.flow_rate2);
        }
        break;

    case file_:
        add_single_header();
        add_single_string_row("DPM File", "path", &injector.dpm_fname);
        add_single_row("Start Time", "s", &injector.unsteady_start, false, 0.0);
        add_single_row("Stop Time", "s", &injector.unsteady_stop, false, 0.0);
        add_single_row("Start Flow-Time in File", "s",
                       &injector.start_at_flow_time_in_unsteady_inj_file,
                       false,
                       0.0);
        add_single_row("Repeat Interval in File", "s",
                       &injector.interval_to_repeat_in_unsteady_inj_file,
                       false,
                       0.0);
        break;

    case single:
    default:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        if (!massless_particle)
        {
            add_combo_row(
                "Direction Mode",
                {"Vector", "Pitch/Yaw", "Target Hitpoint"},
                [&injector]()
                {
                    return static_cast<int>(injector.single_direction_mode);
                },
                [&injector](int value)
                {
                    injector.single_direction_mode =
                        static_cast<Single_Direction_Mode>(value);
                });

            switch (injector.single_direction_mode)
            {
            case Single_Direction_Mode::Pitch_Yaw:
                add_single_row("Pitch", "deg", &injector.single_pitch_degrees,
                               true, -90.0, 90.0);
                add_single_row("Yaw", "deg", &injector.single_yaw_degrees,
                               true, -360.0, 360.0);
                break;
            case Single_Direction_Mode::Target_Hitpoint:
                add_combo_row(
                    "Target Scope",
                    {"World", "Array Local", "Parent Local", "Reference Local"},
                    [&injector]()
                    {
                        return static_cast<int>(injector.single_target_scope);
                    },
                    [&injector](int value)
                    {
                        injector.single_target_scope =
                            static_cast<Single_Target_Scope>(value);
                    });
                add_single_vector_row("X-Target Hitpoint", "mm",
                                      &injector.single_target_hitpoint, 0);
                add_single_vector_row("Y-Target Hitpoint", "mm",
                                      &injector.single_target_hitpoint, 1);
                add_single_vector_row("Z-Target Hitpoint", "mm",
                                      &injector.single_target_hitpoint, 2);
                break;
            case Single_Direction_Mode::Vector:
            default:
                add_single_vector_row("X-Velocity", "m/s", &injector.vel, 0);
                add_single_vector_row("Y-Velocity", "m/s", &injector.vel, 1);
                add_single_vector_row("Z-Velocity", "m/s", &injector.vel, 2);
                break;
            }
            add_single_row("Diameter", "m", &injector.diameter);
            add_single_row("Temperature", "K", &injector.temperature);
            add_single_row("Flow Rate", "kg/s", &injector.flow_rate);
        }
        if (!massless_particle &&
            (injector.injection_type == plain_oriface_atomizer ||
            injector.injection_type == pressure_swirl_atomizer ||
            injector.injection_type == air_blast_atomizer ||
            injector.injection_type == effervescent_atomizer))
        {
            add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
            add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
            add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        }
        break;

    }

    const int diameter_mode = diameter_distribution_index(injector);
    if (particle_type_supports_diameter_distribution(injector.type) &&
        (diameter_mode == 1 || diameter_mode == 2) &&
        diameter_distribution_mode_supported(injector.injection_type, diameter_mode))
    {
        add_info_label(injector.rr_uniform_ln_d
                           ? "Rosin-Rammler logarithmic parameters"
                           : "Rosin-Rammler parameters");
        add_single_row("RR Min Diameter", "m", &injector.rr_min, false, 0.0);
        add_single_row("RR Max Diameter", "m", &injector.rr_max, false, 0.0);
        add_single_row("RR Mean Diameter", "m", &injector.rr_mean, false, 0.0);
        add_single_row("RR Spread", "-", &injector.rr_spread, false, std::numeric_limits<double>::epsilon());
        add_single_int_row("RR Diameter Count", "-", &injector.rr_numdia, 1);
    }
    else if (particle_type_supports_diameter_distribution(injector.type) &&
             diameter_mode == 3 &&
             diameter_distribution_mode_supported(injector.injection_type, diameter_mode))
    {
        add_info_label("Tabulated diameter distribution parameters");
        add_single_string_row("Table Name", "-", &injector.tabulated_diam_table_name);
        add_single_int_row("Diameter Column", "-", &injector.tabulated_diam_ref_diam_col, 1);
        add_single_int_row("Number Fraction Column", "-", &injector.tabulated_diam_num_frac_col, 1);
        add_single_int_row("Mass Fraction Column", "-", &injector.tabulated_diam_mas_frac_col, 1);
        add_single_bool_row("Accumulated Number Fraction", &injector.tabulated_diam_num_frac_accum);
        add_single_bool_row("Accumulated Mass Fraction", &injector.tabulated_diam_mas_frac_accum);
    }

    ui->verticalLayout_3->addStretch();
}

void unit_edit_dialog::sync_point_property_rows()
{
    refresh_field_rows(ui->verticalLayout_3);

    for (const std::function<void()> &syncer : m_property_row_syncers)
    {
        if (syncer)
        {
            syncer();
        }
    }
}

void unit_edit_dialog::build_model_property_rows()
{
    if (control_unit == nullptr)
    {
        return;
    }

    auto reset_layout = [this](QVBoxLayout *layout, const QString &title)
    {
        if (layout == nullptr)
        {
            return;
        }

        clear_layout(layout);
        layout->addWidget(create_group_title_label(title, layout->parentWidget()));
    };
    reset_layout(m_physical_models_layout, "Physical Models");
    reset_layout(m_turbulent_dispersion_layout, "Turbulent Dispersion");
    reset_layout(m_parcel_layout, "Parcel");
    reset_layout(m_wet_combustion_layout, "Wet Combustion");
    m_model_row_syncers.clear();
    m_material_context_combos.clear();

    Injector &injector = control_unit->inj.injector_data;
    Injector *injector_ptr = &injector;
    // Normalize legacy or externally edited combinations before building rows.
    normalize_model_dependencies(injector);
    sync_case_context_constraints();
    m_model_layout_key = current_model_layout_key();

    auto rebuild_model_rows = [this]()
    {
        QMetaObject::invokeMethod(this, [this]()
        {
            build_model_property_rows();
        }, Qt::QueuedConnection);
    };

    auto add_double_row = [this](QVBoxLayout *layout,
                                 const QString &label,
                                 const QString &unit,
                                 double *value_ptr,
                                 double minimum = -std::numeric_limits<double>::max(),
                                 double maximum = std::numeric_limits<double>::max())
    {
        if (layout == nullptr)
        {
            return;
        }

        auto *row = new QUI_FieldRow(label, unit, layout->parentWidget());
        row->setObjectName(QString("modelRow_%1").arg(label.simplified().replace(' ', '_')));
        row->set_label_width(220);
        row->primary_editor()->set_double_mode();
        row->primary_editor()->set_numeric_range(minimum, maximum);
        row->primary_editor()->bind_value(value_ptr);
        m_model_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_bound_value();
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                [this]() { notify_injector_data_changed(false); });
        layout->addWidget(row);
    };

    auto add_int_row = [this](QVBoxLayout *layout,
                              const QString &label,
                              int *value_ptr,
                              int minimum = std::numeric_limits<int>::min(),
                              int maximum = std::numeric_limits<int>::max())
    {
        if (layout == nullptr)
        {
            return;
        }

        auto *row = new QUI_FieldRow(label, QString(), layout->parentWidget());
        row->setObjectName(QString("modelRow_%1").arg(label.simplified().replace(' ', '_')));
        row->set_label_width(220);
        row->primary_editor()->set_integer_mode();
        row->primary_editor()->set_numeric_range(minimum, maximum);
        row->primary_editor()->bind_value(value_ptr);
        m_model_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_text(QString::number(*value_ptr));
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                [this]() { notify_injector_data_changed(false); });
        layout->addWidget(row);
    };

    auto add_string_row = [this](QVBoxLayout *layout,
                                 const QString &label,
                                 QString *value_ptr) -> QWidget *
    {
        if (layout == nullptr || value_ptr == nullptr)
        {
            return nullptr;
        }

        auto *row = new QUI_FieldRow(label, QString(), layout->parentWidget());
        row->setObjectName(QString("modelRow_%1").arg(label.simplified().replace(' ', '_')));
        row->set_label_width(220);
        row->primary_editor()->set_string_mode();
        row->primary_editor()->bind_value(value_ptr);
        m_model_row_syncers.push_back([editor = row->primary_editor(), value_ptr]()
        {
            if (editor != nullptr && value_ptr != nullptr)
            {
                editor->sync_text(*value_ptr);
            }
        });
        connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                [this]() { notify_injector_data_changed(false); });
        layout->addWidget(row);
        return row;
    };

    auto add_vector_row = [this](QVBoxLayout *layout,
                                 const QString &prefix,
                                 const QString &unit,
                                 QVector3D *vector,
                                 bool angular_velocity = false)
    {
        if (layout == nullptr || vector == nullptr)
        {
            return;
        }

        const QString labels[] = {"X-", "Y-", "Z-"};
        for (int component = 0; component < 3; ++component)
        {
            if (feature_is_disabled(m_case_context.three_dimensional) &&
                ((angular_velocity && component != 2) ||
                 (!angular_velocity && component == 2)))
            {
                continue;
            }

            auto *row = new QUI_FieldRow(labels[component] + prefix, unit, layout->parentWidget());
            row->set_label_width(220);
            row->primary_editor()->set_double_mode();
            row->primary_editor()->sync_numeric_value(vector_component_value(*vector, component));
            m_model_row_syncers.push_back([editor = row->primary_editor(), vector, component]()
            {
                if (editor != nullptr && vector != nullptr)
                {
                    editor->sync_numeric_value(vector_component_value(*vector, component));
                }
            });
            connect(row->primary_editor(), &QUI_LineEdit::value_committed, this,
                    [this, editor = row->primary_editor(), vector, component]()
            {
                double value = 0.0;
                if (editor->numeric_value_in_storage(value))
                {
                    set_vector_component_value(*vector, component, static_cast<float>(value));
                    notify_injector_data_changed(false);
                }
            });
            layout->addWidget(row);
        }
    };

    auto add_bool_row = [this, rebuild_model_rows](QVBoxLayout *layout,
                               const QString &label,
                               bool *value_ptr) -> QWidget *
    {
        if (layout == nullptr)
        {
            return nullptr;
        }

        auto *row = new QWidget(layout->parentWidget());
        row->setObjectName(QString("modelRow_%1").arg(label.simplified().replace(' ', '_')));
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 1, 0, 1);
        row_layout->setSpacing(6);
        auto *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(220);
        label_widget->setMaximumWidth(220);
        auto *check_box = new QUI_CheckBox(QString(), row);
        check_box->bind_value(value_ptr);
        row_layout->addWidget(label_widget);
        row_layout->addWidget(check_box, 1);
        m_model_row_syncers.push_back([check_box]()
        {
            if (check_box != nullptr)
            {
                check_box->sync_from_binding();
            }
        });
        connect(check_box, &QUI_CheckBox::value_committed, this,
                [this, label, rebuild_model_rows](bool)
        {
            notify_injector_data_changed(false);
            if (label == "Rotation")
            {
                rebuild_model_rows();
            }
            else if (label == "Use Face Normal")
            {
                QMetaObject::invokeMethod(this, [this]()
                {
                    build_point_property_rows();
                }, Qt::QueuedConnection);
            }
            else if (label == "Random Eddy")
            {
                rebuild_model_rows();
            }
            else if (label == "Atomizer Staggering" ||
                     label == "Standard Injection Staggering")
            {
                sync_stagger_controls();
            }
        });
        layout->addWidget(row);
        return row;
    };

    auto add_combo_row = [this, rebuild_model_rows](QVBoxLayout *layout,
                                const QString &label,
                                const QStringList &options,
                                const std::function<int()> &getter,
                                const std::function<void(int)> &setter,
                                QList<int> option_values = {},
                                bool enabled = true) -> QWidget *
    {
        if (layout == nullptr || options.isEmpty())
        {
            return nullptr;
        }

        auto *row = new QWidget(layout->parentWidget());
        row->setObjectName(QString("modelRow_%1").arg(label.simplified().replace(' ', '_')));
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 1, 0, 1);
        row_layout->setSpacing(6);
        auto *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(220);
        label_widget->setMaximumWidth(220);
        auto *combo = new QUI_ComboBox(row);
        combo->setObjectName(QString("modelEditor_%1").arg(label.simplified().replace(' ', '_')));
        combo->set_options(options);
        if (option_values.size() == options.size())
        {
            for (int index = 0; index < options.size(); ++index)
            {
                combo->setItemData(index, option_values.at(index));
            }
            combo->setCurrentIndex(qMax(0, combo->findData(getter())));
        }
        else
        {
            combo->setCurrentIndex(qBound(0, getter(), options.size() - 1));
        }
        row_layout->addWidget(label_widget);
        row_layout->addWidget(combo, 1);
        row->setEnabled(enabled);
        m_model_row_syncers.push_back([combo, getter, options, option_values]()
        {
            if (combo != nullptr)
            {
                const QSignalBlocker blocker(combo);
                if (option_values.size() == options.size())
                {
                    combo->setCurrentIndex(qMax(0, combo->findData(getter())));
                }
                else
                {
                    combo->setCurrentIndex(getter());
                }
            }
        });
        connect(combo, &QUI_ComboBox::selection_committed, this,
                [this, combo, setter, option_values, options, rebuild_model_rows]()
        {
            setter(option_values.size() == options.size()
                       ? combo->currentData().toInt()
                       : combo->currentIndex());
            notify_injector_data_changed(false);
            rebuild_model_rows();
        });
        layout->addWidget(row);
        return row;
    };

    auto add_string_combo_row = [this](QVBoxLayout *layout,
                                       const QString &label,
                                       QStringList options,
                                       QString *value_ptr)
    {
        if (layout == nullptr || value_ptr == nullptr)
        {
            return;
        }

        if (options.isEmpty())
        {
            options.push_back(QString());
        }

        auto *row = new QWidget(layout->parentWidget());
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 1, 0, 1);
        row_layout->setSpacing(6);
        auto *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(220);
        label_widget->setMaximumWidth(220);
        auto *combo = new QUI_ComboBox(row);
        if (label == "Evaporating Material")
        {
            combo->setObjectName("evaporatingMaterialModelEditor");
            m_material_context_combos.append(combo);
        }
        combo->set_options(options);
        combo->setEditable(true);
        if (combo->lineEdit() != nullptr)
        {
            combo->lineEdit()->setReadOnly(true);
        }
        const int current_index = combo->findText(*value_ptr);
        combo->setCurrentIndex(current_index);
        if (current_index < 0)
        {
            combo->setEditText(*value_ptr);
        }
        row_layout->addWidget(label_widget);
        row_layout->addWidget(combo, 1);
        m_model_row_syncers.push_back([combo, value_ptr]()
        {
            if (combo != nullptr && value_ptr != nullptr)
            {
                const QSignalBlocker blocker(combo);
                const int index = combo->findText(*value_ptr);
                combo->setCurrentIndex(index);
                if (index < 0)
                {
                    combo->setEditText(*value_ptr);
                }
            }
        });
        connect(combo, &QUI_ComboBox::selection_committed, this,
                [this, combo, value_ptr]()
        {
            *value_ptr = combo->currentText();
            notify_injector_data_changed(false);
        });
        layout->addWidget(row);
    };

    if (m_physical_models_layout != nullptr)
    {
        const bool inertial_models_supported = particle_type_supports_inertial_models(injector.type);
        const bool rotation_supported = inertial_models_supported &&
            particle_rotation_supported(injector.injection_type);
        const bool vapor_pressure_supported = particle_type_supports_vapor_pressure(injector.type);
        const bool atomizer_staggering = uses_atomizer_stagger(injector);

        const bool reference_frame_supported =
            injector.injection_type != surface &&
            injector.injection_type != volume &&
            injector.injection_type != condensate;
        if (reference_frame_supported)
        {
            add_string_row(m_physical_models_layout,
                           "Local Reference Frame",
                           &injector.local_reference_frame);
        }
        auto *collision_partner_row = add_string_row(
            m_physical_models_layout, "Collision Partner", &injector.collision_partner);
        if (collision_partner_row != nullptr)
        {
            // Collision partner is a DDPM setting. Keep legacy text visible,
            // but prevent editing while no discrete-phase domain is active.
            collision_partner_row->setEnabled(
                uses_dense_discrete_phase_domain(injector.dpm_domain));
        }
        if (inertial_models_supported)
        {
            add_string_row(m_physical_models_layout, "Drag Function", &injector.drag_fcn);
            if (rotation_supported)
            {
                add_bool_row(m_physical_models_layout, "Rotation", &injector.rotation_on);
                if (injector.rotation_on)
                {
                    if (injector.injection_type == cone)
                    {
                        add_double_row(m_physical_models_layout,
                                       "Angular Velocity Magnitude",
                                       "rad/s",
                                       &injector.ang_vel_mag,
                                       0.0);
                    }
                    else
                    {
                        add_vector_row(m_physical_models_layout,
                                       "Angular Velocity",
                                       "rad/s",
                                       &injector.ang_vel,
                                       true);
                        add_vector_row(m_physical_models_layout,
                                       "Angular Velocity (Last)",
                                       "rad/s",
                                       &injector.ang_vel2,
                                       true);
                    }
                    add_combo_row(m_physical_models_layout, "Rotational Drag Law",
                                  {"Dennis et al.", "None"},
                                  [injector_ptr]() { return static_cast<int>(injector_ptr->rot_drag_law); },
                                  [injector_ptr](int value) { injector_ptr->rot_drag_law = static_cast<Rot_Drag_Law>(value); });
                    add_combo_row(m_physical_models_layout, "Rotational Lift Law",
                                  {"Oesterle-Bui Dinh", "Tsuji et al.", "Rubinow-Keller", "None"},
                                  [injector_ptr]() { return static_cast<int>(injector_ptr->rot_lift_law); },
                                  [injector_ptr](int value) { injector_ptr->rot_lift_law = static_cast<Rot_Lift_Law>(value); });
                }
            }
        }
        if (vapor_pressure_supported)
        {
            add_double_row(m_physical_models_layout, "Vapor Pressure", "Pa", &injector.vapor_pressure, 0.0);
        }
        const bool swirl_fraction_supported =
            injector.type != Massless &&
            injector.injection_type == cone &&
            injector.cone_type == hollow;
        if (swirl_fraction_supported)
        {
            add_double_row(m_physical_models_layout, "Swirl Fraction", "-", &injector.swirl_frac, -1.0, 1.0);
        }
        const bool uniform_mass_supported =
            injector.type != Massless &&
            injector.injection_type == cone &&
            (injector.cone_type == solid || injector.cone_type == ring);
        if (uniform_mass_supported)
        {
            add_bool_row(m_physical_models_layout,
                         "Uniform Mass Distribution",
                         &injector.uniform_mass_dist_on);
        }
        if (atomizer_staggering)
        {
            add_bool_row(m_physical_models_layout, "Atomizer Staggering", &injector.spatial_staggering_atomizer_on);
        }
        else if (uses_standard_stagger(injector.injection_type))
        {
            add_bool_row(m_physical_models_layout, "Standard Injection Staggering", &injector.spatial_staggering_std_inj_on);
        }
        auto *continuous_phase_domain_row = add_string_row(
            m_physical_models_layout, "Continuous Phase Domain", &injector.cphace_domain);
        if (continuous_phase_domain_row != nullptr)
        {
            continuous_phase_domain_row->setEnabled(
                uses_dense_discrete_phase_domain(injector.dpm_domain));
        }
        if (inertial_models_supported)
        {
            auto *rough_wall_row = add_bool_row(
                m_physical_models_layout, "Rough Wall", &injector.rough_wall_on);
            if (rough_wall_row != nullptr)
            {
                rough_wall_row->setEnabled(
                    !feature_is_disabled(m_case_context.reflect_boundary));
            }
            auto *brownian_row = add_bool_row(
                m_physical_models_layout, "Brownian Motion", &injector.brownian_motion);
            if (brownian_row != nullptr)
            {
                brownian_row->setEnabled(
                    injector.drag_law == Strokes_Cunningham &&
                    !heat_transfer_is_disabled(m_case_context));
            }
            const bool dynamic_drag_supported =
                injector.type == Droplet && injector.seco_breakup_on &&
                !feature_is_disabled(m_case_context.unsteady_particle_tracking) &&
                (injector.seco_breakup_tab || injector.seco_breakup_wave ||
                 injector.seco_break_up_khrt || injector.seco_breakup_ssd ||
                 injector.seco_breakup_madahushi || injector.seco_breakup_schmehl);
            QStringList drag_options = {
                "Spherical", "Nonspherical", "Stokes-Cunningham", "High Mach Number"};
            QList<int> drag_values = {
                spherical, nonspherical, Strokes_Cunningham, high_Mach_number};
            if (dynamic_drag_supported)
            {
                drag_options.push_back("Dynamic Drag");
                drag_values.push_back(dynamic_drag);
            }
            const bool gravity_drag_available =
                !feature_is_disabled(m_case_context.gravity);
            if (gravity_drag_available || is_gravity_drag_law(injector.drag_law))
            {
                drag_options.push_back("Grace");
                drag_values.push_back(grace);
                drag_options.push_back("Ishii-Zuber");
                drag_values.push_back(ishii_zuber);
            }

            const bool dense_drag_available =
                injector.type == Inert &&
                uses_dense_discrete_phase_domain(injector.dpm_domain) &&
                m_case_context.dense_gas_solid != Unit_Edit_Feature_State::Disabled;
            const Drag_Law dense_drag_laws[] = {
                wen_yu, gidaspow, syamlal_obrien, huilin_gidaspow,
                gibilaro, emms, filtered};
            const QString dense_drag_labels[] = {
                "Wen-Yu", "Gidaspow", "Syamlal-O'Brien", "Huilin-Gidaspow",
                "Gibilaro", "EMMS", "Filtered"};
            for (int index = 0; index < 7; ++index)
            {
                if (dense_drag_available || injector.drag_law == dense_drag_laws[index])
                {
                    drag_options.push_back(dense_drag_labels[index]);
                    drag_values.push_back(dense_drag_laws[index]);
                }
            }

            auto *drag_row = add_combo_row(
                m_physical_models_layout,
                "Drag Law",
                drag_options,
                [injector_ptr]() { return static_cast<int>(injector_ptr->drag_law); },
                [injector_ptr](int value) { injector_ptr->drag_law = static_cast<Drag_Law>(value); },
                drag_values);
            if (drag_row != nullptr)
            {
                drag_row->setEnabled(
                    drag_law_allowed_by_context(m_case_context, injector));
            }
            if (injector.drag_law == nonspherical)
            {
                add_double_row(m_physical_models_layout, "Shape Factor", "-", &injector.shape_factor, 0.0, 1.0);
            }
            if (injector.drag_law == Strokes_Cunningham)
            {
                add_double_row(m_physical_models_layout,
                               "Cunningham Correction",
                               "-",
                               &injector.cunningham_correction,
                               std::numeric_limits<double>::epsilon());
            }
        }
        const bool breakup_supported = injector.type == Droplet;
        if (breakup_supported)
        {
            add_bool_row(m_physical_models_layout, "SECO Breakup", &injector.seco_breakup_on);
            const QList<bool *> breakup_model_flags = {
                &injector.seco_breakup_tab,
                &injector.seco_breakup_wave,
                &injector.seco_break_up_khrt,
                &injector.seco_breakup_ssd,
                &injector.seco_breakup_madahushi,
                &injector.seco_breakup_schmehl};
            const QString breakup_model_labels[] = {
                "SECO Tabulated",
                "SECO Wave",
                "SECO KHRT",
                "SECO SSD",
                "SECO Madabhushi",
                "SECO Schmehl"};
            const char *breakup_model_names[] = {
                "secoTabulatedModelEditor",
                "secoWaveModelEditor",
                "secoKHRTModelEditor",
                "secoSSDModelEditor",
                "secoMadabhushiModelEditor",
                "secoSchmehlModelEditor"};

            auto *breakup_row = m_physical_models_layout->itemAt(
                m_physical_models_layout->count() - 1)->widget();
            auto *breakup_check = breakup_row != nullptr
                ? breakup_row->findChild<QUI_CheckBox*>()
                : nullptr;
            if (breakup_check != nullptr)
            {
                breakup_check->setObjectName("secoBreakupEditor");
            }
            if (breakup_check != nullptr)
            {
                connect(breakup_check, &QUI_CheckBox::value_committed, this,
                        [this](bool checked)
                {
                    if (control_unit == nullptr)
                    {
                        return;
                    }
                    Injector &current_injector = control_unit->inj.injector_data;
                    if (!checked)
                    {
                        current_injector.seco_breakup_tab = false;
                        current_injector.seco_breakup_wave = false;
                        current_injector.seco_break_up_khrt = false;
                        current_injector.seco_breakup_ssd = false;
                        current_injector.seco_breakup_madahushi = false;
                        current_injector.seco_breakup_schmehl = false;
                    }
                    notify_injector_data_changed(false);
                    QMetaObject::invokeMethod(this, [this]()
                    {
                        build_model_property_rows();
                    }, Qt::QueuedConnection);
                });
            }

            if (injector.seco_breakup_on)
            {
                const bool breakup_model_selected = std::any_of(
                    breakup_model_flags.cbegin(), breakup_model_flags.cend(),
                    [](const bool *flag) { return *flag; });
                QWidget *breakup_model_rows[6] = {};
                for (int model_index = 0; model_index < breakup_model_flags.size(); ++model_index)
                {
                    breakup_model_rows[model_index] = add_bool_row(
                        m_physical_models_layout,
                        breakup_model_labels[model_index],
                        breakup_model_flags[model_index]);
                    if (breakup_model_rows[model_index] != nullptr)
                    {
                        breakup_model_rows[model_index]->setEnabled(
                            !breakup_model_selected || *breakup_model_flags[model_index]);
                        auto *model_check = breakup_model_rows[model_index]->findChild<QUI_CheckBox*>();
                        if (model_check != nullptr)
                        {
                            model_check->setObjectName(breakup_model_names[model_index]);
                            connect(model_check, &QUI_CheckBox::value_committed, this,
                                    [this, breakup_model_flags, model_index](bool checked)
                            {
                                if (control_unit == nullptr)
                                {
                                    return;
                                }
                                if (checked)
                                {
                                    for (int other = 0; other < breakup_model_flags.size(); ++other)
                                    {
                                        if (other != model_index)
                                        {
                                            *breakup_model_flags[other] = false;
                                        }
                                    }
                                }
                                notify_injector_data_changed(false);
                                QMetaObject::invokeMethod(this, [this]()
                                {
                                    build_model_property_rows();
                                }, Qt::QueuedConnection);
                            });
                        }
                    }
                }
                if (injector.seco_breakup_tab)
                {
                    add_double_row(m_physical_models_layout, "SECO Tabulated Y0", "-", &injector.seco_breakup_tab_y0);
                    add_int_row(m_physical_models_layout,
                                "Tabulated Diameter Count",
                                &injector.number_tab_diameters,
                                1);
                }
                if (injector.seco_breakup_wave)
                {
                    add_double_row(m_physical_models_layout, "SECO Wave B1", "-", &injector.seco_breakup_wave_b1);
                    add_double_row(m_physical_models_layout, "SECO Wave B0", "-", &injector.seco_breakup_wave_b0);
                }
                if (injector.seco_break_up_khrt)
                {
                    add_double_row(m_physical_models_layout, "SECO KHRT CL", "-", &injector.seco_breakup_khrt_cl);
                    add_double_row(m_physical_models_layout, "SECO KHRT Ctau", "-", &injector.seco_breakup_khrt_ctau);
                    add_double_row(m_physical_models_layout, "SECO KHRT CRT", "-", &injector.seco_breakup_khrt_crt);
                }
                if (injector.seco_breakup_ssd)
                {
                    add_double_row(m_physical_models_layout, "SECO SSD We-Cr", "-", &injector.seco_breakup_ssd_we_cr);
                    add_double_row(m_physical_models_layout, "SECO SSD Core BU", "-", &injector.seco_breakup_ssd_core_bu);
                    add_double_row(m_physical_models_layout, "SECO SSD NP Target", "-", &injector.seco_breakup_ssd_np_target);
                    add_double_row(m_physical_models_layout, "SECO SSD X-SI", "-", &injector.seco_breakup_ssd_x_si);
                }
                if (injector.seco_breakup_madahushi)
                {
                    add_double_row(m_physical_models_layout, "SECO Madabhushi C0", "-", &injector.seco_breakup_madabushi_c0);
                    add_double_row(m_physical_models_layout, "SECO Madabhushi Column CD", "-", &injector.seco_breakup_madabushi_column_drag_cd);
                    add_double_row(m_physical_models_layout, "SECO Madabhushi Ligament", "-", &injector.seco_breakup_madabushi_ligament_factor);
                    add_double_row(m_physical_models_layout, "SECO Madabhushi Jet Diameter", "m", &injector.seco_breakup_madabushi_jet_diameter);
                }
                if (injector.seco_breakup_schmehl)
                {
                    add_double_row(m_physical_models_layout, "SECO Schmehl NP", "-", &injector.seco_breakup_schmehl_np);
                }
            }
        }
        m_physical_models_layout->addStretch();
    }

    if (m_turbulent_dispersion_layout != nullptr)
    {
        auto *stochastic_row = add_bool_row(
            m_turbulent_dispersion_layout, "Stochastic Tracking", &injector.stochastic);
        auto *stochastic_check = stochastic_row != nullptr
            ? stochastic_row->findChild<QUI_CheckBox*>()
            : nullptr;
        if (stochastic_check != nullptr)
        {
            stochastic_check->setObjectName("stochasticTrackingEditor");
        }
        if (injector.stochastic)
        {
            add_bool_row(m_turbulent_dispersion_layout, "Random Eddy", &injector.random_eddy);
            add_int_row(m_turbulent_dispersion_layout, "Eddy Attempts", &injector.ntries, 1);
            if (uses_dense_discrete_phase_domain(injector.dpm_domain) ||
                m_case_context.unsteady_particle_tracking == Unit_Edit_Feature_State::Enabled)
            {
                if (QWidget *eddy_attempts_row = m_turbulent_dispersion_layout->parentWidget()->findChild<QWidget *>(
                        "modelRow_Eddy_Attempts"))
                {
                    eddy_attempts_row->setEnabled(false);
                }
            }
            add_double_row(m_turbulent_dispersion_layout, "Time Scale Constant", "s", &injector.time_scale_constant, 0.0);
            if (QWidget *time_scale_row = m_turbulent_dispersion_layout->parentWidget()->findChild<QWidget *>(
                    "modelRow_Time_Scale_Constant"))
            {
                time_scale_row->setEnabled(injector.random_eddy);
            }
        }
        auto *cloud_row = add_bool_row(
            m_turbulent_dispersion_layout, "Cloud Tracking", &injector.cloud);
        auto *cloud_check = cloud_row != nullptr
            ? cloud_row->findChild<QUI_CheckBox*>()
            : nullptr;
        if (cloud_check != nullptr)
        {
            cloud_check->setObjectName("cloudTrackingEditor");
        }
        if (stochastic_row != nullptr)
        {
            stochastic_row->setEnabled(!injector.cloud);
        }
        if (cloud_row != nullptr)
        {
            cloud_row->setEnabled(!injector.stochastic && injector.type != Massless);
        }
        if (stochastic_check != nullptr)
        {
            connect(stochastic_check, &QUI_CheckBox::value_committed, this,
                    [this, injector_ptr](bool checked)
            {
                if (checked)
                {
                    injector_ptr->cloud = false;
                }
                notify_injector_data_changed(false);
                QMetaObject::invokeMethod(this, [this]()
                {
                    build_model_property_rows();
                }, Qt::QueuedConnection);
            });
        }
        if (cloud_check != nullptr)
        {
            connect(cloud_check, &QUI_CheckBox::value_committed, this,
                    [this, injector_ptr](bool checked)
            {
                if (checked)
                {
                    injector_ptr->stochastic = false;
                }
                notify_injector_data_changed(false);
                QMetaObject::invokeMethod(this, [this]()
                {
                    build_model_property_rows();
                }, Qt::QueuedConnection);
            });
        }
        if (injector.cloud)
        {
            add_double_row(m_turbulent_dispersion_layout, "Cloud Minimum Diameter", "m", &injector.cloud_min_dia, 0.0);
            add_double_row(m_turbulent_dispersion_layout, "Cloud Maximum Diameter", "m", &injector.cloud_max_dia, 0.0);
        }
        if (injector.injection_type == surface)
        {
            auto *scale_area_row = add_bool_row(
                m_turbulent_dispersion_layout, "Scale By Area", &injector.scale_by_area);
            auto *random_surface_row = add_bool_row(
                m_turbulent_dispersion_layout, "Random Surface", &injector.random_surface);
            if (injector.type != Massless)
            {
                add_bool_row(m_turbulent_dispersion_layout, "Use Face Normal", &injector.use_face_normal);
            }

            const bool surface_distribution_selected =
                injector.scale_by_area || injector.random_surface;
            if (scale_area_row != nullptr)
            {
                scale_area_row->setEnabled(
                    !surface_distribution_selected || injector.scale_by_area);
                auto *scale_check = scale_area_row->findChild<QUI_CheckBox*>();
                if (scale_check != nullptr)
                {
                    scale_check->setObjectName("surfaceScaleByAreaEditor");
                    connect(scale_check, &QUI_CheckBox::value_committed, this,
                            [this, injector_ptr](bool checked)
                    {
                        if (checked)
                        {
                            injector_ptr->random_surface = false;
                        }
                        notify_injector_data_changed(false);
                        QMetaObject::invokeMethod(this, [this]()
                        {
                            build_model_property_rows();
                        }, Qt::QueuedConnection);
                    });
                }
            }
            if (random_surface_row != nullptr)
            {
                random_surface_row->setEnabled(
                    !surface_distribution_selected || injector.random_surface);
                auto *random_check = random_surface_row->findChild<QUI_CheckBox*>();
                if (random_check != nullptr)
                {
                    random_check->setObjectName("randomSurfaceEditor");
                    connect(random_check, &QUI_CheckBox::value_committed, this,
                            [this, injector_ptr](bool checked)
                    {
                        if (checked)
                        {
                            injector_ptr->scale_by_area = false;
                        }
                        notify_injector_data_changed(false);
                        QMetaObject::invokeMethod(this, [this]()
                        {
                            build_model_property_rows();
                        }, Qt::QueuedConnection);
                    });
                }
            }
        }
        m_turbulent_dispersion_layout->addStretch();
    }

    if (m_parcel_layout != nullptr)
    {
        const bool standard_parcel_model = requires_standard_parcel_model(
            injector.injection_type);
        add_combo_row(m_parcel_layout, "Parcel Model",
                      standard_parcel_model
                          ? QStringList{"Standard"}
                          : QStringList{"Standard", "Constant Number", "Constant Mass", "Constant Diameter"},
                      [injector_ptr]() { return static_cast<int>(injector_ptr->parcel_model); },
                      [injector_ptr](int value) { injector_ptr->parcel_model = static_cast<Parcel_Model>(value); },
                      standard_parcel_model ? QList<int>{standard} : QList<int>{},
                      !standard_parcel_model);
        switch (injector.parcel_model)
        {
        case const_number:
            add_int_row(m_parcel_layout, "Parcel Number", &injector.parcel_number, 1);
            break;
        case const_mass:
            add_double_row(m_parcel_layout, "Parcel Mass", "kg", &injector.parcel_mass, 0.0);
            break;
        case const_diameter:
            add_double_row(m_parcel_layout, "Parcel Diameter", "m", &injector.parcel_diameter, 0.0);
            break;
        case standard:
        default:
            add_int_row(m_parcel_layout, "Parcel Number", &injector.parcel_number, 1);
            add_double_row(m_parcel_layout, "Total Mass", "kg", &injector.total_mass, 0.0);
            break;
        }
        m_parcel_layout->addStretch();
    }

    if (m_wet_combustion_layout != nullptr)
    {
        auto *evaporating_liquid_row = add_bool_row(
            m_wet_combustion_layout, "Evaporating Liquid", &injector.evaporating_liquid);
        if (evaporating_liquid_row != nullptr)
        {
            auto *evaporating_liquid_check =
                evaporating_liquid_row->findChild<QUI_CheckBox*>();
            if (evaporating_liquid_check != nullptr)
            {
                evaporating_liquid_check->setObjectName("evaporatingLiquidModelEditor");
                connect(evaporating_liquid_check,
                        &QUI_CheckBox::value_committed,
                        this,
                        [this, injector_ptr](bool checked)
                {
                    if (!checked)
                    {
                        injector_ptr->evaporating_material.clear();
                        injector_ptr->liquid_fraction = -1.0;
                        injector_ptr->evaporating_species.clear();
                    }
                    sync_particle_type_dependent_controls();
                    sync_species_combos();
                    notify_injector_data_changed(false);
                    QMetaObject::invokeMethod(this, [this]()
                    {
                        build_model_property_rows();
                    }, Qt::QueuedConnection);
                });
            }
        }
        if (injector.evaporating_liquid)
        {
            add_string_combo_row(m_wet_combustion_layout,
                                 "Evaporating Material",
                                 m_material_names,
                                 &injector.evaporating_material);
            add_double_row(m_wet_combustion_layout, "Liquid Fraction", "-", &injector.liquid_fraction, 0.0, 1.0);
        }
        auto *info = new QLabel(
            "Evaporating material and species are selected in the injector properties panel.",
            m_wet_combustion_layout->parentWidget());
        info->setWordWrap(true);
        info->setStyleSheet("color: rgb(110, 116, 126); padding: 4px 2px;");
        m_wet_combustion_layout->addWidget(info);
        m_wet_combustion_layout->addStretch();
    }
}

void unit_edit_dialog::sync_model_property_rows()
{
    QVBoxLayout *model_layouts[] = {
        m_physical_models_layout,
        m_turbulent_dispersion_layout,
        m_parcel_layout,
        m_wet_combustion_layout};
    for (QVBoxLayout *layout : model_layouts)
    {
        refresh_field_rows(layout);
    }

    for (const std::function<void()> &syncer : m_model_row_syncers)
    {
        if (syncer)
        {
            syncer();
        }
    }

}

void unit_edit_dialog::clear_layout(QLayout *layout)
{
    if (layout == nullptr)
    {
        return;
    }

    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
        {
            // Property rows are rebuilt synchronously. Keeping old rows queued
            // for deletion makes stale controls accumulate during type changes.
            delete widget;
        }

        if (QLayout *child_layout = item->layout())
        {
            clear_layout(child_layout);
            delete child_layout;
        }

        delete item;
    }
}

void unit_edit_dialog::notify_injector_data_changed(bool geometry_changed)
{
    if (control_unit == nullptr)
    {
        return;
    }

    // Every editor path converges here. Normalize immediately so a direct
    // field edit cannot leave an invalid combination in the live injector
    // while the view and geometry refresh are still pending.
    normalize_model_dependencies(control_unit->inj.injector_data);
    sync_case_context_constraints();

    m_data_modified = true;
    emit injector_data_changed(control_unit);
    if (geometry_changed)
    {
        emit injector_geometry_changed(control_unit);
    }
}
