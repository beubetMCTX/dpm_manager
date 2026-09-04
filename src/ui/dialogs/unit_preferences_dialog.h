#ifndef UNIT_PREFERENCES_DIALOG_H
#define UNIT_PREFERENCES_DIALOG_H

#include <QDialog>

#include "unit_system.h"

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSlider;

class UnitPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UnitPreferencesDialog(const Unit_Preferences &preferences,
                                   QWidget *parent = nullptr);

    Unit_Preferences preferences() const;

private:
    QComboBox *m_length_combo = nullptr;
    QComboBox *m_angle_combo = nullptr;
    QComboBox *m_velocity_combo = nullptr;
    QComboBox *m_mass_combo = nullptr;
    QComboBox *m_mass_flow_combo = nullptr;
    QComboBox *m_time_combo = nullptr;
    QComboBox *m_pressure_combo = nullptr;
    QComboBox *m_temperature_combo = nullptr;
    QSlider *m_transparency_slider = nullptr;
    QLabel *m_transparency_value = nullptr;
    QDoubleSpinBox *m_translation_snap = nullptr;
    QDoubleSpinBox *m_rotation_snap = nullptr;
    QCheckBox *m_show_injector_axes = nullptr;
    QCheckBox *m_show_reference_axes = nullptr;
};

#endif // UNIT_PREFERENCES_DIALOG_H
