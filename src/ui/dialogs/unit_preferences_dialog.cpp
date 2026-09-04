#include "unit_preferences_dialog.h"

#include "qUI_components.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
QComboBox *make_unit_combo(Unit_Dimension dimension,
                           const QString &selected,
                           QWidget *parent)
{
    auto *combo = new QUI_ComboBox(parent);
    combo->set_options(UnitSystem::symbols(dimension));
    const int index = combo->findText(selected);
    if (index >= 0)
    {
        combo->setCurrentIndex(index);
    }
    return combo;
}

void add_unit_row(QFormLayout *layout,
                  const QString &label,
                  QWidget *widget)
{
    layout->addRow(new QUI_Label(label, layout->parentWidget()), widget);
}
}

UnitPreferencesDialog::UnitPreferencesDialog(const Unit_Preferences &preferences,
                                             QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Display Units");
    setModal(true);
    setMinimumWidth(360);

    auto *root_layout = new QVBoxLayout(this);
    auto *description = new QLabel(
        "Choose units shown in injector property editors. Model values remain stored in their internal units.",
        this);
    description->setWordWrap(true);
    description->setStyleSheet("color: rgb(160, 166, 176); padding: 2px 0px 8px 0px;");
    root_layout->addWidget(description);

    auto *form_layout = new QFormLayout();
    form_layout->setHorizontalSpacing(18);
    form_layout->setVerticalSpacing(8);

    m_length_combo = make_unit_combo(Unit_Dimension::Length, preferences.length, this);
    m_angle_combo = make_unit_combo(Unit_Dimension::Angle, preferences.angle, this);
    m_velocity_combo = make_unit_combo(Unit_Dimension::Velocity, preferences.velocity, this);
    m_mass_combo = make_unit_combo(Unit_Dimension::Mass, preferences.mass, this);
    m_mass_flow_combo = make_unit_combo(Unit_Dimension::MassFlow, preferences.mass_flow, this);
    m_time_combo = make_unit_combo(Unit_Dimension::Time, preferences.time, this);
    m_pressure_combo = make_unit_combo(Unit_Dimension::Pressure, preferences.pressure, this);
    m_temperature_combo = make_unit_combo(Unit_Dimension::Temperature, preferences.temperature, this);

    add_unit_row(form_layout, "Length", m_length_combo);
    add_unit_row(form_layout, "Angle", m_angle_combo);
    add_unit_row(form_layout, "Velocity", m_velocity_combo);
    add_unit_row(form_layout, "Mass", m_mass_combo);
    add_unit_row(form_layout, "Mass Flow", m_mass_flow_combo);
    add_unit_row(form_layout, "Time", m_time_combo);
    add_unit_row(form_layout, "Pressure", m_pressure_combo);
    add_unit_row(form_layout, "Temperature", m_temperature_combo);
    m_translation_snap = new QDoubleSpinBox(this);
    m_translation_snap->setRange(0.0, 1.0e6);
    m_translation_snap->setDecimals(6);
    m_translation_snap->setSingleStep(0.1);
    m_translation_snap->setSuffix(" " + preferences.length);
    m_translation_snap->setValue(UnitSystem::from_base(
        preferences.translation_snap, preferences.length));
    add_unit_row(form_layout, "Translation Snap", m_translation_snap);

    m_rotation_snap = new QDoubleSpinBox(this);
    m_rotation_snap->setRange(0.0, 360.0);
    m_rotation_snap->setDecimals(3);
    m_rotation_snap->setSingleStep(1.0);
    m_rotation_snap->setSuffix(" " + preferences.angle);
    m_rotation_snap->setValue(UnitSystem::from_base(
        preferences.rotation_snap, preferences.angle));
    add_unit_row(form_layout, "Rotation Snap", m_rotation_snap);

    auto *transparency_widget = new QWidget(this);
    auto *transparency_layout = new QHBoxLayout(transparency_widget);
    transparency_layout->setContentsMargins(0, 0, 0, 0);
    m_transparency_slider = new QSlider(Qt::Horizontal, transparency_widget);
    m_transparency_slider->setRange(0, 100);
    m_transparency_slider->setValue(qRound(
        preferences.reference_geometry_transparency * 100.0));
    m_transparency_value = new QLabel(transparency_widget);
    m_transparency_value->setMinimumWidth(48);
    transparency_layout->addWidget(m_transparency_slider, 1);
    transparency_layout->addWidget(m_transparency_value);
    const auto update_transparency_label = [this](int value)
    {
        if (m_transparency_value != nullptr)
        {
            m_transparency_value->setText(QString::number(value) + "%");
        }
    };
    connect(m_transparency_slider, &QSlider::valueChanged,
            this, update_transparency_label);
    update_transparency_label(m_transparency_slider->value());
    add_unit_row(form_layout, "Model Transparency", transparency_widget);
    m_show_injector_axes = new QCheckBox("Show injector local axes", this);
    m_show_reference_axes = new QCheckBox("Show reference local axes", this);
    m_show_injector_axes->setChecked(preferences.show_injector_local_axes);
    m_show_reference_axes->setChecked(preferences.show_reference_local_axes);
    form_layout->addRow(m_show_injector_axes);
    form_layout->addRow(m_show_reference_axes);
    root_layout->addLayout(form_layout);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root_layout->addWidget(buttons);
}

Unit_Preferences UnitPreferencesDialog::preferences() const
{
    Unit_Preferences result;
    result.length = m_length_combo->currentText();
    result.angle = m_angle_combo->currentText();
    result.velocity = m_velocity_combo->currentText();
    result.mass = m_mass_combo->currentText();
    result.mass_flow = m_mass_flow_combo->currentText();
    result.time = m_time_combo->currentText();
    result.pressure = m_pressure_combo->currentText();
    result.temperature = m_temperature_combo->currentText();
    const double transparency = m_transparency_slider->value() / 100.0;
    result.injector_transparency = transparency;
    result.reference_geometry_transparency = transparency;
    result.translation_snap = UnitSystem::to_base(
        m_translation_snap->value(), result.length);
    result.rotation_snap = UnitSystem::to_base(
        m_rotation_snap->value(), result.angle);
    result.show_injector_local_axes = m_show_injector_axes->isChecked();
    result.show_reference_local_axes = m_show_reference_axes->isChecked();
    return result;
}
