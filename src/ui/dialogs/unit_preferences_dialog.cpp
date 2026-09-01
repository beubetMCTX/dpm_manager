#include "unit_preferences_dialog.h"

#include "qUI_components.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

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
                  QComboBox *combo)
{
    layout->addRow(new QUI_Label(label, layout->parentWidget()), combo);
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
    return result;
}
