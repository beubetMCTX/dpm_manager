#ifndef UNIT_PREFERENCES_DIALOG_H
#define UNIT_PREFERENCES_DIALOG_H

#include <QDialog>

#include "unit_system.h"

class QComboBox;

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
};

#endif // UNIT_PREFERENCES_DIALOG_H
