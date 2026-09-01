#include "unit_edit_dialog.h"
#include "runtime_debug.h"
#include "ui_unit_edit_dialog.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QTimer>

namespace
{
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
    return type == Droplet;
}

bool particle_type_supports_inertial_models(DPM_Type type)
{
    return type != Massless;
}

bool particle_type_supports_vapor_pressure(DPM_Type type)
{
    return type == Droplet;
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
    case volume:
        options.push_back({1, "rosin-rammler"});
        options.push_back({2, "rosin-rammler-logarithmic"});
        if (type == surface)
        {
            options.push_back({3, "tabulated"});
        }
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
    return QString("%1|%2|%3|%4|%5|%6|%7")
        .arg(static_cast<int>(injector.type))
        .arg(static_cast<int>(injector.injection_type))
        .arg(diameter_distribution_index(injector))
        .arg(static_cast<int>(injector.cone_type))
        .arg(static_cast<int>(injector.volume_specification))
        .arg(static_cast<int>(injector.volume_bgeom_shapes))
        .arg(static_cast<int>(injector.volume_streams_spec));
}

QString model_layout_key_for(const Injector &injector)
{
    return QString("%1|%2|%3|%4|%5|%6|%7|%8|%9|%10|%11|%12|%13|%14|%15")
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
        .arg(injector.evaporating_liquid ? 1 : 0);
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

void unit_edit_dialog::set_material_names(const QStringList &material_names)
{
    if (m_material_names == material_names)
    {
        return;
    }

    m_material_names = material_names;
    sync_material_combo();
    build_model_property_rows();
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
    particle_type_panel_layout->addWidget(m_particle_type_group);
    ui->horizontalLayout_particle_type->insertWidget(0, particle_type_panel);
    ui->groupBox_partical_type->hide();

    m_material_combo = new QUI_ComboBox(this);
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
    ui->gridLayout->addWidget(m_diameter_distribution_combo, 2, 1);
    ui->comboBox_diameter_distribution->hide();

    m_discrete_phase_domain_combo = new QUI_ComboBox(this);
    ui->gridLayout->addWidget(m_discrete_phase_domain_combo, 2, 3);
    ui->comboBox_discrete_phase_domain->hide();

    m_evaporating_species_combo = new QUI_ComboBox(this);
    ui->gridLayout->addWidget(m_evaporating_species_combo, 4, 0);
    ui->comboBox_evaporating_species->hide();

    m_devolatilizing_species_combo = new QUI_ComboBox(this);
    ui->gridLayout->addWidget(m_devolatilizing_species_combo, 4, 1);
    ui->comboBox_devolatilizing_species->hide();

    m_product_species_combo = new QUI_ComboBox(this);
    ui->gridLayout->addWidget(m_product_species_combo, 4, 2);
    ui->comboBox_product_species->hide();

    m_oxidizing_species_combo = new QUI_ComboBox(this);
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
    ui->verticalLayout_4->insertWidget(0, m_stagger_check);
    ui->radioButton_stagger->hide();

    m_stagger_radius_edit = new QUI_LineEdit(ui->stagger_layout);
    m_stagger_radius_edit->set_double_mode();
    m_stagger_radius_edit->bind_value(&control_unit->inj.injector_data.stagger_radius);
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
        normalize_diameter_distribution_for_type(control_unit->inj.injector_data);
        sync_diameter_distribution_combo();
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
                build_point_property_rows();
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
        apply_labeled_control_enabled(
            m_stagger_radius_edit,
            ui != nullptr ? static_cast<QWidget *>(ui->label_stagger) : nullptr,
            m_stagger_check != nullptr && m_stagger_check->isChecked());
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
    for (int i = 0; i < m_injection_type_combo->count(); ++i)
    {
        if (m_injection_type_combo->itemData(i).toInt() == target_value)
        {
            m_injection_type_combo->setCurrentIndex(i);
            return;
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
    const bool enable_evaporating_species = particle_type_supports_evaporating_species(particle_type);
    const bool enable_devolatilizing_species = particle_type_supports_devolatilizing_species(particle_type);

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
                                  false);
    apply_labeled_control_enabled(m_evaporating_species_combo,
                                  ui->label_evaporating_species,
                                  enable_evaporating_species);
    apply_labeled_control_enabled(m_devolatilizing_species_combo,
                                  ui->label_devolatilizing_species,
                                  enable_devolatilizing_species);
    apply_labeled_control_enabled(m_product_species_combo,
                                  ui->label_product_species,
                                  false);
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

    bool *stagger_flag = uses_atomizer_stagger(control_unit->inj.injector_data.injection_type)
                             ? &control_unit->inj.injector_data.spatial_staggering_atomizer_on
                             : &control_unit->inj.injector_data.spatial_staggering_std_inj_on;

    QSignalBlocker check_blocker(m_stagger_check);
    m_stagger_check->bind_value(stagger_flag);
    m_stagger_check->sync_from_binding();
    m_stagger_radius_edit->bind_value(&control_unit->inj.injector_data.stagger_radius);
    apply_labeled_control_enabled(
        m_stagger_radius_edit,
        ui != nullptr ? static_cast<QWidget *>(ui->label_stagger) : nullptr,
        *stagger_flag);
}

void unit_edit_dialog::sync_auxiliary_panels()
{
    if (control_unit == nullptr)
    {
        return;
    }

    const bool show_cone_panel = (control_unit->inj.injector_data.injection_type == cone);
    const bool show_stagger_panel = true;

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
                control_unit->inj.injector_data.type == Droplet;
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
                                 bool geometry_changed = true)
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_double_mode();
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

    auto add_single_int_row = [this](const QString &label, const QString &unit, int *value_ptr)
    {
        QUI_FieldRow *row = new QUI_FieldRow(label, unit, ui->scrollarea_properties);
        row->set_label_width(180);
        row->primary_editor()->set_integer_mode();
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

    auto add_single_bool_row = [this](const QString &label, bool *value_ptr)
    {
        QWidget *row = new QWidget(ui->scrollarea_properties);
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

    switch (injector.injection_type)
    {
    case group:
        add_range_header();
        add_range_vector_row("X-Position", "mm", &injector.pos, &injector.pos2, 0);
        add_range_vector_row("Y-Position", "mm", &injector.pos, &injector.pos2, 1);
        add_range_vector_row("Z-Position", "mm", &injector.pos, &injector.pos2, 2);
        add_range_vector_row("X-Velocity", "m/s", &injector.vel, &injector.vel2, 0);
        add_range_vector_row("Y-Velocity", "m/s", &injector.vel, &injector.vel2, 1);
        add_range_vector_row("Z-Velocity", "m/s", &injector.vel, &injector.vel2, 2);
        add_range_row("Diameter", "m", &injector.diameter, &injector.diameter2);
        add_range_row("Temperature", "K", &injector.temperature, &injector.temperature2);
        add_range_row("Flow Rate", "kg/s", &injector.flow_rate, &injector.flow_rate2);
        break;

    case cone:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Axis", "-", &injector.axis, 0);
        add_single_vector_row("Y-Axis", "-", &injector.axis, 1);
        add_single_vector_row("Z-Axis", "-", &injector.axis, 2);
        add_single_row("Cone Angle", "deg", &injector.cone_angle);
        add_single_row("Outer Radius", "mm", &injector.radius);
        if (injector.cone_type == hollow || injector.cone_type == ring)
        {
            add_single_row("Inner Radius", "mm", &injector.inner_radius);
        }
        add_single_row("Velocity Magnitude", "m/s", &injector.vel_mag);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
        break;

    case volume:
        add_single_header();
        add_combo_row(
            "Volume Specification",
            {"Zone", "Bounding Geometry"},
            [&injector]() { return static_cast<int>(injector.volume_specification); },
            [&injector](int value) { injector.volume_specification = static_cast<Volume_Specification>(value); });
        if (injector.volume_specification == zone)
        {
            add_int_list_row("Volume Zones", &injector.volume_zones);
        }
        else
        {
            add_combo_row(
                "Bounding Shape",
                {"Sphere", "Cylinder", "Cone", "Hexahedron"},
                 [&injector]() { return static_cast<int>(injector.volume_bgeom_shapes); },
                 [&injector](int value) { injector.volume_bgeom_shapes = static_cast<Volume_Bgeom_Shapes>(value); });

            // Bounding geometry fields are meaningful only for the selected
            // shape. Zone-based volume injections do not expose any of them.
            switch (injector.volume_bgeom_shapes)
            {
            case sphere:
                add_single_vector_row("X-Center", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Center", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Center", "mm", &injector.volume_bgeom_min, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius);
                break;
            case cylinder:
                add_single_vector_row("X-Min", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Min", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Min", "mm", &injector.volume_bgeom_min, 2);
                add_single_vector_row("X-Max", "mm", &injector.volume_bgeom_max, 0);
                add_single_vector_row("Y-Max", "mm", &injector.volume_bgeom_max, 1);
                add_single_vector_row("Z-Max", "mm", &injector.volume_bgeom_max, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius);
                break;
            case cone_:
                add_single_vector_row("X-Min", "mm", &injector.volume_bgeom_min, 0);
                add_single_vector_row("Y-Min", "mm", &injector.volume_bgeom_min, 1);
                add_single_vector_row("Z-Min", "mm", &injector.volume_bgeom_min, 2);
                add_single_vector_row("X-Max", "mm", &injector.volume_bgeom_max, 0);
                add_single_vector_row("Y-Max", "mm", &injector.volume_bgeom_max, 1);
                add_single_vector_row("Z-Max", "mm", &injector.volume_bgeom_max, 2);
                add_single_row("Radius", "mm", &injector.volume_bgeom_radius);
                add_single_row("Cone Angle", "rad", &injector.volume_bgeom_viconeangle);
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
        add_combo_row(
            "Stream Specification",
            {"Total Parcel Count", "Parcel Per Cell"},
            [&injector]() { return static_cast<int>(injector.volume_streams_spec); },
            [&injector](int value) { injector.volume_streams_spec = static_cast<Volume_Streams_Spec>(value); });
        if (injector.volume_streams_spec == total_parcel_count)
        {
            add_single_int_row("Total Streams", "-", &injector.volume_streams_total);
        }
        else
        {
            add_single_int_row("Streams Per Cell", "-", &injector.volume_streams_per_cell);
        }
        add_single_row("Volume Fraction", "-", &injector.volume_fraction);
        add_single_row("Packing Limit", "-", &injector.volume_packing_limit_per_cell);
        add_bool_row("Mass Input", &injector.mass_input_on);
        add_bool_row("Volume Fraction Input", &injector.volfrac_input_on);
        break;

    case plain_oriface_atomizer:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
        add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
        add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        add_single_row("Diameter", "m", &injector.diameter);
        add_single_row("Outer Diameter", "m", &injector.outer_diameter);
        add_single_row("Plain Length", "m", &injector.plain_length);
        add_single_row("Plain Corner Size", "m", &injector.plain_corner_size);
        add_single_row("Plain Constant A", "-", &injector.plain_const_a);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
        break;

    case pressure_swirl_atomizer:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
        add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
        add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        add_single_row("Inner Diameter", "m", &injector.inner_diameter);
        add_single_row("Outer Diameter", "m", &injector.outer_diameter);
        add_single_row("Half Angle", "rad", &injector.half_angle);
        add_single_row("Injection Pressure", "Pa", &injector.pswirl_inj_press);
        add_single_row("Sheet Constant", "-", &injector.sheet_const);
        add_single_row("Ligament Constant", "-", &injector.lig_const);
        add_single_row("Atomizer Dispersion Angle", "deg", &injector.atomizer_disp_angle);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
        break;

    case air_blast_atomizer:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
        add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
        add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        add_single_row("Inner Diameter", "m", &injector.inner_diameter);
        add_single_row("Outer Diameter", "m", &injector.outer_diameter);
        add_single_row("Air Relative Velocity", "m/s", &injector.airbl_rel_vel);
        add_single_row("Atomizer Dispersion Angle", "deg", &injector.atomizer_disp_angle);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
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
        add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
        add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
        add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        add_single_row("Orifice Width", "m", &injector.ff_oriface_width);
        add_single_row("Phi Start", "rad", &injector.phi_start);
        add_single_row("Phi Stop", "rad", &injector.phi_stop);
        add_single_row("Sheet Constant", "-", &injector.ff_sheet_const);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
        break;

    case effervescent_atomizer:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
        add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
        add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        add_single_row("Diameter", "m", &injector.diameter);
        add_single_row("Outer Diameter", "m", &injector.outer_diameter);
        add_single_row("Effervescent Quality", "-", &injector.effer_quality);
        add_single_row("Saturation Temperature", "K", &injector.effer_t_sat);
        add_single_row("Effervescent Constant", "-", &injector.effer_const);
        add_single_row("Maximum Half Angle", "rad", &injector.effer_half_angle_max);
        add_single_row("Total Flow Rate", "kg/s", &injector.total_flow_rate);
        break;

    case condensate:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Axis", "-", &injector.axis, 0);
        add_single_vector_row("Y-Axis", "-", &injector.axis, 1);
        add_single_vector_row("Z-Axis", "-", &injector.axis, 2);
        add_single_vector_row("X-Velocity", "m/s", &injector.vel, 0);
        add_single_vector_row("Y-Velocity", "m/s", &injector.vel, 1);
        add_single_vector_row("Z-Velocity", "m/s", &injector.vel, 2);
        add_single_row("Velocity Magnitude", "m/s", &injector.vel_mag);
        add_single_row("Radius", "m", &injector.radius);
        add_single_row("Flow Rate", "kg/s", &injector.flow_rate);
        break;

    case surface:
        add_range_header();
        add_int_list_row("Surface Zone IDs", &injector.surfaces);
        add_int_list_row("Boundary IDs", &injector.boundary);
        add_range_vector_row("X-Position", "mm", &injector.pos, &injector.pos2, 0);
        add_range_vector_row("Y-Position", "mm", &injector.pos, &injector.pos2, 1);
        add_range_vector_row("Z-Position", "mm", &injector.pos, &injector.pos2, 2);
        add_range_vector_row("X-Velocity", "m/s", &injector.vel, &injector.vel2, 0);
        add_range_vector_row("Y-Velocity", "m/s", &injector.vel, &injector.vel2, 1);
        add_range_vector_row("Z-Velocity", "m/s", &injector.vel, &injector.vel2, 2);
        add_range_row("Diameter", "m", &injector.diameter, &injector.diameter2);
        add_range_row("Temperature", "K", &injector.temperature, &injector.temperature2);
        add_range_row("Flow Rate", "kg/s", &injector.flow_rate, &injector.flow_rate2);
        break;

    case single:
    case file_:
    default:
        add_single_header();
        add_single_vector_row("X-Position", "mm", &injector.pos, 0);
        add_single_vector_row("Y-Position", "mm", &injector.pos, 1);
        add_single_vector_row("Z-Position", "mm", &injector.pos, 2);
        add_single_vector_row("X-Velocity", "m/s", &injector.vel, 0);
        add_single_vector_row("Y-Velocity", "m/s", &injector.vel, 1);
        add_single_vector_row("Z-Velocity", "m/s", &injector.vel, 2);
        add_single_row("Diameter", "m", &injector.diameter);
        add_single_row("Temperature", "K", &injector.temperature);
        add_single_row("Flow Rate", "kg/s", &injector.flow_rate);
        if (injector.injection_type == plain_oriface_atomizer ||
            injector.injection_type == pressure_swirl_atomizer ||
            injector.injection_type == air_blast_atomizer ||
            injector.injection_type == effervescent_atomizer)
        {
            add_single_vector_row("X-Atomizer Axis", "-", &injector.atomizer_axis, 0);
            add_single_vector_row("Y-Atomizer Axis", "-", &injector.atomizer_axis, 1);
            add_single_vector_row("Z-Atomizer Axis", "-", &injector.atomizer_axis, 2);
        }
        if (injector.injection_type == file_)
        {
            add_single_string_row("DPM File", "path", &injector.dpm_fname);
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
        add_single_row("RR Min Diameter", "m", &injector.rr_min);
        add_single_row("RR Max Diameter", "m", &injector.rr_max);
        add_single_row("RR Mean Diameter", "m", &injector.rr_mean);
        add_single_row("RR Spread", "-", &injector.rr_spread);
        add_single_int_row("RR Diameter Count", "-", &injector.rr_numdia);
    }
    else if (particle_type_supports_diameter_distribution(injector.type) &&
             diameter_mode == 3 &&
             diameter_distribution_mode_supported(injector.injection_type, diameter_mode))
    {
        add_info_label("Tabulated diameter distribution parameters");
        add_single_string_row("Table Name", "-", &injector.tabulated_diam_table_name);
        add_single_int_row("Diameter Column", "-", &injector.tabulated_diam_ref_diam_col);
        add_single_int_row("Number Fraction Column", "-", &injector.tabulated_diam_num_frac_col);
        add_single_int_row("Mass Fraction Column", "-", &injector.tabulated_diam_mas_frac_col);
        add_single_bool_row("Accumulated Number Fraction", &injector.tabulated_diam_num_frac_accum);
        add_single_bool_row("Accumulated Mass Fraction", &injector.tabulated_diam_mas_frac_accum);
    }

    ui->verticalLayout_3->addStretch();
}

void unit_edit_dialog::sync_point_property_rows()
{
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
    m_model_layout_key = current_model_layout_key();

    Injector &injector = control_unit->inj.injector_data;

    auto add_double_row = [this](QVBoxLayout *layout,
                                 const QString &label,
                                 const QString &unit,
                                 double *value_ptr)
    {
        if (layout == nullptr)
        {
            return;
        }

        auto *row = new QUI_FieldRow(label, unit, layout->parentWidget());
        row->set_label_width(220);
        row->primary_editor()->set_double_mode();
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
                              int *value_ptr)
    {
        if (layout == nullptr)
        {
            return;
        }

        auto *row = new QUI_FieldRow(label, QString(), layout->parentWidget());
        row->set_label_width(220);
        row->primary_editor()->set_integer_mode();
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
                                 QString *value_ptr)
    {
        if (layout == nullptr || value_ptr == nullptr)
        {
            return;
        }

        auto *row = new QUI_FieldRow(label, QString(), layout->parentWidget());
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
    };

    auto add_vector_row = [this](QVBoxLayout *layout,
                                 const QString &prefix,
                                 const QString &unit,
                                 QVector3D *vector)
    {
        if (layout == nullptr || vector == nullptr)
        {
            return;
        }

        const QString labels[] = {"X-", "Y-", "Z-"};
        for (int component = 0; component < 3; ++component)
        {
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

    auto add_bool_row = [this](QVBoxLayout *layout,
                               const QString &label,
                               bool *value_ptr)
    {
        if (layout == nullptr)
        {
            return;
        }

        auto *row = new QWidget(layout->parentWidget());
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
                [this](bool)
        {
            notify_injector_data_changed(false);
        });
        layout->addWidget(row);
    };

    auto add_combo_row = [this](QVBoxLayout *layout,
                                const QString &label,
                                const QStringList &options,
                                const std::function<int()> &getter,
                                const std::function<void(int)> &setter)
    {
        if (layout == nullptr || options.isEmpty())
        {
            return;
        }

        auto *row = new QWidget(layout->parentWidget());
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 1, 0, 1);
        row_layout->setSpacing(6);
        auto *label_widget = new QUI_Label(label, row);
        label_widget->setMinimumWidth(220);
        label_widget->setMaximumWidth(220);
        auto *combo = new QUI_ComboBox(row);
        combo->set_options(options);
        combo->setCurrentIndex(qBound(0, getter(), options.size() - 1));
        row_layout->addWidget(label_widget);
        row_layout->addWidget(combo, 1);
        m_model_row_syncers.push_back([combo, getter]()
        {
            if (combo != nullptr)
            {
                const QSignalBlocker blocker(combo);
                combo->setCurrentIndex(getter());
            }
        });
        connect(combo, &QUI_ComboBox::selection_committed, this,
                [this, combo, setter]()
        {
            setter(combo->currentIndex());
            notify_injector_data_changed(false);
        });
        layout->addWidget(row);
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

        if (!value_ptr->isEmpty() && !options.contains(*value_ptr))
        {
            options.prepend(*value_ptr);
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
        combo->set_options(options);
        combo->setCurrentIndex(qMax(0, options.indexOf(*value_ptr)));
        row_layout->addWidget(label_widget);
        row_layout->addWidget(combo, 1);
        m_model_row_syncers.push_back([combo, value_ptr]()
        {
            if (combo != nullptr && value_ptr != nullptr)
            {
                const QSignalBlocker blocker(combo);
                const int index = combo->findText(*value_ptr);
                combo->setCurrentIndex(qMax(0, index));
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
        const bool vapor_pressure_supported = particle_type_supports_vapor_pressure(injector.type);
        const bool atomizer_staggering = uses_atomizer_stagger(injector.injection_type);

        add_string_row(m_physical_models_layout, "Local Reference Frame", &injector.local_reference_frame);
        add_string_row(m_physical_models_layout, "Collision Partner", &injector.collision_partner);
        if (inertial_models_supported)
        {
            add_string_row(m_physical_models_layout, "Drag Function", &injector.drag_fcn);
            add_bool_row(m_physical_models_layout, "Rotation", &injector.rotation_on);
            if (injector.rotation_on)
            {
                add_vector_row(m_physical_models_layout, "Angular Velocity", "rad/s", &injector.ang_vel);
                add_vector_row(m_physical_models_layout, "Angular Velocity (Last)", "rad/s", &injector.ang_vel2);
                add_double_row(m_physical_models_layout, "Angular Velocity Magnitude", "rad/s", &injector.ang_vel_mag);
                add_combo_row(m_physical_models_layout, "Rotational Drag Law",
                              {"Dennis et al.", "None"},
                              [&injector]() { return static_cast<int>(injector.rot_drag_law); },
                              [&injector](int value) { injector.rot_drag_law = static_cast<Rot_Drag_Law>(value); });
                add_combo_row(m_physical_models_layout, "Rotational Lift Law",
                              {"Oesterle-Bui Dinh", "Tsuji et al.", "Rubinow-Keller", "None"},
                              [&injector]() { return static_cast<int>(injector.rot_lift_law); },
                              [&injector](int value) { injector.rot_lift_law = static_cast<Rot_Lift_Law>(value); });
            }
        }
        if (vapor_pressure_supported)
        {
            add_double_row(m_physical_models_layout, "Vapor Pressure", "Pa", &injector.vapor_pressure);
        }
        add_double_row(m_physical_models_layout, "Swirl Fraction", "-", &injector.swirl_frac);
        add_bool_row(m_physical_models_layout, "Uniform Mass Distribution", &injector.uniform_mass_dist_on);
        if (atomizer_staggering)
        {
            add_bool_row(m_physical_models_layout, "Atomizer Staggering", &injector.spatial_staggering_atomizer_on);
        }
        else
        {
            add_bool_row(m_physical_models_layout, "Standard Injection Staggering", &injector.spatial_staggering_std_inj_on);
        }
        add_string_row(m_physical_models_layout, "Continuous Phase Domain", &injector.cphace_domain);
        if (inertial_models_supported)
        {
            add_bool_row(m_physical_models_layout, "Rough Wall", &injector.rough_wall_on);
            add_bool_row(m_physical_models_layout, "Brownian Motion", &injector.brownian_motion);
            add_combo_row(m_physical_models_layout, "Drag Law",
                          {"Spherical", "Nonspherical", "Stokes-Cunningham", "High Mach Number", "Dynamic Drag"},
                          [&injector]() { return static_cast<int>(injector.drag_law); },
                          [&injector](int value) { injector.drag_law = static_cast<Drag_Law>(value); });
            if (injector.drag_law == nonspherical)
            {
                add_double_row(m_physical_models_layout, "Shape Factor", "-", &injector.shape_factor);
            }
            if (injector.drag_law == Strokes_Cunningham)
            {
                add_double_row(m_physical_models_layout, "Cunningham Correction", "-", &injector.cunningham_correction);
            }
        }
        const bool breakup_supported = injector.type == Droplet;
        if (breakup_supported)
        {
            add_bool_row(m_physical_models_layout, "SECO Breakup", &injector.seco_breakup_on);
            if (injector.seco_breakup_on)
            {
                add_bool_row(m_physical_models_layout, "SECO Tabulated", &injector.seco_breakup_tab);
                add_bool_row(m_physical_models_layout, "SECO Wave", &injector.seco_breakup_wave);
                add_bool_row(m_physical_models_layout, "SECO KHRT", &injector.seco_break_up_khrt);
                add_bool_row(m_physical_models_layout, "SECO SSD", &injector.seco_breakup_ssd);
                add_bool_row(m_physical_models_layout, "SECO Madabhushi", &injector.seco_breakup_madahushi);
                add_bool_row(m_physical_models_layout, "SECO Schmehl", &injector.seco_breakup_schmehl);
                if (injector.seco_breakup_tab)
                {
                    add_double_row(m_physical_models_layout, "SECO Tabulated Y0", "-", &injector.seco_breakup_tab_y0);
                    add_int_row(m_physical_models_layout, "Tabulated Diameter Count", &injector.number_tab_diameters);
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
        add_bool_row(m_turbulent_dispersion_layout, "Stochastic Tracking", &injector.stochastic);
        if (injector.stochastic)
        {
            add_bool_row(m_turbulent_dispersion_layout, "Random Eddy", &injector.random_eddy);
            add_int_row(m_turbulent_dispersion_layout, "Eddy Attempts", &injector.ntries);
            add_double_row(m_turbulent_dispersion_layout, "Time Scale Constant", "s", &injector.time_scale_constant);
        }
        add_bool_row(m_turbulent_dispersion_layout, "Cloud Tracking", &injector.cloud);
        if (injector.cloud)
        {
            add_double_row(m_turbulent_dispersion_layout, "Cloud Minimum Diameter", "m", &injector.cloud_min_dia);
            add_double_row(m_turbulent_dispersion_layout, "Cloud Maximum Diameter", "m", &injector.cloud_max_dia);
        }
        if (injector.injection_type == surface)
        {
            add_bool_row(m_turbulent_dispersion_layout, "Scale By Area", &injector.scale_by_area);
            add_bool_row(m_turbulent_dispersion_layout, "Use Face Normal", &injector.use_face_normal);
            add_bool_row(m_turbulent_dispersion_layout, "Random Surface", &injector.random_surface);
        }
        if (injector.injection_type == file_)
        {
            add_double_row(m_turbulent_dispersion_layout, "Unsteady Start", "s", &injector.unsteady_start);
            add_double_row(m_turbulent_dispersion_layout, "Unsteady Stop", "s", &injector.unsteady_stop);
            add_double_row(m_turbulent_dispersion_layout, "Flow-Time Start", "s", &injector.start_at_flow_time_in_unsteady_inj_file);
            add_double_row(m_turbulent_dispersion_layout, "Repeat Interval", "s", &injector.interval_to_repeat_in_unsteady_inj_file);
            add_double_row(m_turbulent_dispersion_layout, "Cone-Angle Start", "rad", &injector.unsteady_ca_start);
            add_double_row(m_turbulent_dispersion_layout, "Cone-Angle Stop", "rad", &injector.unsteady_ca_stop);
        }
        m_turbulent_dispersion_layout->addStretch();
    }

    if (m_parcel_layout != nullptr)
    {
        add_combo_row(m_parcel_layout, "Parcel Model",
                      {"Standard", "Constant Number", "Constant Mass", "Constant Diameter"},
                      [&injector]() { return static_cast<int>(injector.parcel_model); },
                      [&injector](int value) { injector.parcel_model = static_cast<Parcel_Model>(value); });
        switch (injector.parcel_model)
        {
        case const_number:
            add_int_row(m_parcel_layout, "Parcel Number", &injector.parcel_number);
            break;
        case const_mass:
            add_double_row(m_parcel_layout, "Parcel Mass", "kg", &injector.parcel_mass);
            break;
        case const_diameter:
            add_double_row(m_parcel_layout, "Parcel Diameter", "m", &injector.parcel_diameter);
            break;
        case standard:
        default:
            add_int_row(m_parcel_layout, "Parcel Number", &injector.parcel_number);
            add_double_row(m_parcel_layout, "Total Mass", "kg", &injector.total_mass);
            break;
        }
        m_parcel_layout->addStretch();
    }

    if (m_wet_combustion_layout != nullptr)
    {
        add_bool_row(m_wet_combustion_layout, "Evaporating Liquid", &injector.evaporating_liquid);
        if (injector.evaporating_liquid)
        {
            add_string_combo_row(m_wet_combustion_layout,
                                 "Evaporating Material",
                                 m_material_names,
                                 &injector.evaporating_material);
            add_double_row(m_wet_combustion_layout, "Liquid Fraction", "-", &injector.liquid_fraction);
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

    m_data_modified = true;
    emit injector_data_changed(control_unit);
    if (geometry_changed)
    {
        emit injector_geometry_changed(control_unit);
    }
}
