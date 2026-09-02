#ifndef UNIT_EDIT_DIALOG_H
#define UNIT_EDIT_DIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>

#include <functional>
#include <vector>

#include <AIS_Shape.hxx>
#include <QEvent>
#include "unit.h"
#include "qUI_components.h"

namespace Ui {
class unit_edit_dialog;
}

class unit_edit_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit unit_edit_dialog(Unit* control_unit,
                              const QStringList &chemkin_species_names = {},
                              const QStringList &material_names = {},
                              QWidget *parent = nullptr);

    ~unit_edit_dialog();

    void refresh_from_unit_data(Unit *unit);
    void refresh_geometry_from_unit_data(Unit *unit);
    void set_chemkin_species_names(const QStringList &species_names);
    void set_material_names(const QStringList &material_names);
    void reset_edit_state();
    bool has_unsaved_changes() const { return m_data_modified; }

signals:
    void injector_data_changed(Unit *unit);
    void injector_geometry_changed(Unit *unit);
    void dialog_cancelled(Unit *unit);
    void dialog_closed(Unit *unit);

protected:
    // 可以重写 resizeEvent！
    void resizeEvent(QResizeEvent *event) override
    {
        // 先调用基类实现
        QDialog::resizeEvent(event);
    }
    void closeEvent(QCloseEvent *event) override;

private slots:


private:
    Ui::unit_edit_dialog *ui;
    Unit* control_unit;
    QUI_ComboBox *m_unit_type_combo = nullptr;
    QUI_LineEdit *m_injection_name_edit = nullptr;
    QUI_ComboBox *m_injection_type_combo = nullptr;
    QUI_SpinBox *m_number_of_stream_spin = nullptr;
    QUI_RadioGroup *m_particle_type_group = nullptr;
    QUI_ComboBox *m_material_combo = nullptr;
    QUI_ComboBox *m_diameter_distribution_combo = nullptr;
    QUI_ComboBox *m_discrete_phase_domain_combo = nullptr;
    QUI_ComboBox *m_devolatilizing_species_combo = nullptr;
    QUI_ComboBox *m_evaporating_species_combo = nullptr;
    QUI_ComboBox *m_product_species_combo = nullptr;
    QUI_ComboBox *m_oxidizing_species_combo = nullptr;
    QUI_CheckBox *m_stagger_check = nullptr;
    QUI_LineEdit *m_stagger_radius_edit = nullptr;
    QString m_property_layout_key;
    QString m_model_layout_key;
    QStringList m_chemkin_species_names;
    QStringList m_material_names;
    std::vector<std::function<void()>> m_property_row_syncers;
    std::vector<std::function<void()>> m_model_row_syncers;
    QVBoxLayout *m_physical_models_layout = nullptr;
    QVBoxLayout *m_turbulent_dispersion_layout = nullptr;
    QVBoxLayout *m_parcel_layout = nullptr;
    QVBoxLayout *m_wet_combustion_layout = nullptr;
    QPushButton *m_apply_button = nullptr;
    QPushButton *m_cancel_button = nullptr;
    bool m_data_modified = false;

    inline bool initialize();
    void setup_custom_controls();
    void initialize_unit_type_combo();
    void initialize_injection_type_combo();
    void initialize_particle_type_group();
    void initialize_material_combo();
    void initialize_diameter_distribution_combo();
    void initialize_discrete_phase_domain_combo();
    void initialize_species_combos();
    void initialize_cone_type_combo();
    void initialize_stagger_controls();
    void sync_unit_type_combo();
    void sync_injection_type_combo();
    void sync_particle_type_group();
    void sync_particle_type_dependent_controls();
    void sync_material_combo();
    void sync_diameter_distribution_combo();
    void sync_discrete_phase_domain_combo();
    void sync_species_combos();
    void sync_cone_type_combo();
    void sync_stagger_controls();
    void sync_auxiliary_panels();
    QString current_property_layout_key() const;
    QString current_model_layout_key() const;
    void build_point_property_rows();
    void sync_point_property_rows();
    void build_model_property_rows();
    void sync_model_property_rows();
    void clear_layout(QLayout *layout);
    void setup_action_buttons();
    void notify_injector_data_changed(bool geometry_changed = true);
};

#endif // UNIT_EDIT_DIALOG_H
