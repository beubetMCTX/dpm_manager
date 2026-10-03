#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QLineEdit>
#include <qvector.h>
#include <QList>
#include <QLabel>
#include <QPointer>
#include <QStringList>
#include <QToolBar>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>
#include <QByteArray>
#include <QHash>

#include <QFileDialog>
#include <QMessageBox>

#include <TopTools_HSequenceOfShape.hxx>

#include "occtwidget.h"
#include "unit.h"
#include "app_config.h"
#include "project_session.h"

class QGroupBox;
class QComboBox;
class QStackedWidget;
class QSpinBox;
class QTimer;


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SpeciesColorDialog;
class SpeciesMaterialDialog;
class UnitPreferencesDialog;
class QMenu;

class ObjectTreeWidget : public QTreeWidget
{
public:
    explicit ObjectTreeWidget(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {
    }

    QList<QTreeWidgetItem *> all_items() const;
    int count() const { return all_items().size(); }
    QTreeWidgetItem *item(int index) const;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    OCCTWidget* m_3d_widget;
    QList<Unit> units;
    QTabWidget* tab_widget;

    // Supply solver/case capabilities to current and future unit editors.
    void set_unit_editor_case_context(const Unit_Edit_Case_Context &context);

    // Headless regression hook used by the shipped application target. It
    // exercises the same runtime Unit tree used by the project UI.
    bool run_project_session_self_test(QString *error_message = nullptr);

public:


private slots:
    void on_actionRead_triggered();

    void on_actionSave_DPM_triggered();

    void on_actionOpen_Project_triggered();

    void on_actionSave_Project_triggered();

    void on_actionSave_Project_As_triggered();

    void on_actionValidate_Project_triggered();

    void on_actionExport_Diagnostics_triggered();

    void on_actionOpen_Config_Folder_triggered();
    void on_actionOpen_Logs_Folder_triggered();

    void on_actionRead_Base_Geometry_triggered();

    void on_actionRead_Chemkin_Files_triggered();

    void on_actionSpecies_Colors_triggered();

    void on_actionSpecies_Materials_triggered();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void closeEvent(QCloseEvent *event) override;
    bool confirm_project_change(const QString &action_description,
                                bool restore_saved_project_on_discard = false);
    QList<Unit> build_test_injector_units() const;
    bool load_chemkin_file(const QString &file_path,
                           bool show_error_message_box,
                           bool show_success_feedback);
    void restore_last_chemkin_file();
    void restore_recent_projects();
    void remember_project_path(const QString &file_path);
    void update_recent_projects_menu();
    void restore_material_table();
    void restore_reference_geometry();
    void save_reference_geometry_state();
    bool save_project_session(const QString &file_path);
    bool save_current_project_session();
    bool save_project_session_as();
    bool load_project_session(const QString &file_path);
    void mark_project_dirty();
    void update_project_session_title();
    void apply_material_entries(const QList<MaterialConfigEntry> &entries,
                                bool save_to_config,
                                bool show_status_feedback);
    void update_chemkin_status();
    void create_reference_geometry_panel();
    void apply_reference_geometry_display_units();
    void update_reference_geometry_panel();
    void update_reference_geometry_controls();
    void create_array_editor_panel();
    void apply_array_editor_display_units();
    void refresh_array_editor_panel();
    void load_array_editor_layer(int layer_index);
    bool build_array_editor_spec(UnitArraySpec *output,
                                 bool show_warning = false);
    bool build_fill_editor_spec(UnitFillSpec *output,
                                bool show_warning = false);
    void update_array_editor_preview();
    void position_viewport_interaction_toolbar();
    void apply_reference_geometry_transform();
    void create_object_list_panel();
    void update_object_list_panel();
    void update_object_list_item(const QUuid &uuid, const QString &name);
    QList<QUuid> selected_object_unit_uuids() const;
    QString object_list_unit_display_name(
        const std::shared_ptr<Unit> &unit) const;
    void update_object_list_selection(const QUuid &uuid, bool reference_geometry);
    void refresh_object_list_selection_colors();
    void update_unit_position_controls();
    void restore_window_layout();
    void save_window_layout();
    void reset_window_layout();
    void open_unit_preferences_dialog();
    void close_auxiliary_windows_for_shutdown();
    void sync_unit_from_occt(Unit *changed_unit);
    void sync_unit_position_from_occt(Unit *changed_unit);
    void sync_unit_from_occt_impl(Unit *changed_unit, bool recompute_dirty);
    void sync_persistent_units_from_occt();
    int assign_species_to_unassigned_units();
    project_session::Data collect_project_data() const;
    void refresh_project_dirty_state();

    Ui::MainWindow *ui;
    QStringList m_chemkin_species_names;
    QString m_chemkin_file_path;
    QString m_project_session_file_path;
    bool m_project_dirty = false;
    bool m_project_baseline_initialized = false;
    QByteArray m_saved_project_fingerprint;
    QTimer *m_dirty_refresh_timer = nullptr;
    bool m_loading_project_session = false;
    QList<MaterialConfigEntry> m_material_entries;
    QToolBar *m_chemkin_toolbar = nullptr;
    QToolBar *m_viewport_interaction_toolbar = nullptr;
    QLabel *m_chemkin_status_label = nullptr;
    QLineEdit *m_chemkin_path_edit = nullptr;
    QMenu *m_recent_projects_menu = nullptr;
    QStringList m_recent_project_paths;
    QPointer<SpeciesColorDialog> m_species_color_dialog;
    QPointer<SpeciesMaterialDialog> m_species_material_dialog;
    bool m_auxiliary_shutdown_started = false;
    QDockWidget *m_reference_geometry_dock = nullptr;
    QDoubleSpinBox *m_reference_position_x = nullptr;
    QDoubleSpinBox *m_reference_position_y = nullptr;
    QDoubleSpinBox *m_reference_position_z = nullptr;
    QDoubleSpinBox *m_reference_rotation_x = nullptr;
    QDoubleSpinBox *m_reference_rotation_y = nullptr;
    QDoubleSpinBox *m_reference_rotation_z = nullptr;
    QCheckBox *m_reference_geometry_lock = nullptr;
    QPushButton *m_apply_reference_transform = nullptr;
    QPushButton *m_reset_reference_transform = nullptr;
    QPushButton *m_align_reference_face = nullptr;
    QPushButton *m_clear_reference_geometry = nullptr;
    QPushButton *m_create_datum_plane = nullptr;
    QPushButton *m_create_datum_axis = nullptr;
    QPushButton *m_create_datum_origin = nullptr;
    QPushButton *m_create_section_plane = nullptr;
    QPushButton *m_toggle_section_clipping = nullptr;
    QPushButton *m_create_alignment_frame = nullptr;
    QLabel *m_reference_geometry_path = nullptr;
    QLabel *m_reference_face_origin = nullptr;
    QLabel *m_reference_face_normal = nullptr;
    QDockWidget *m_object_list_dock = nullptr;
    ObjectTreeWidget *m_object_list = nullptr;
    QDockWidget *m_array_editor_dock = nullptr;
    QLabel *m_array_editor_source_label = nullptr;
    QUuid m_array_editor_source_uuid;
    QListWidget *m_array_editor_layers = nullptr;
    QGroupBox *m_array_editor_layers_group = nullptr;
    QGroupBox *m_array_editor_pattern_group = nullptr;
    QGroupBox *m_array_editor_parameters_group = nullptr;
    QGroupBox *m_array_editor_frame_group = nullptr;
    QGroupBox *m_array_editor_fill_group = nullptr;
    QComboBox *m_array_editor_type = nullptr;
    QSpinBox *m_array_editor_count = nullptr;
    QStackedWidget *m_array_editor_parameter_stack = nullptr;
    QDoubleSpinBox *m_array_editor_linear_spacing = nullptr;
    QDoubleSpinBox *m_array_editor_rotational_angle = nullptr;
    QDoubleSpinBox *m_array_editor_rotational_spacing = nullptr;
    QDoubleSpinBox *m_array_editor_major_radius = nullptr;
    QDoubleSpinBox *m_array_editor_minor_radius = nullptr;
    QDoubleSpinBox *m_array_editor_elliptical_angle = nullptr;
    QComboBox *m_array_editor_frame_mode = nullptr;
    QDoubleSpinBox *m_array_editor_origin_x = nullptr;
    QDoubleSpinBox *m_array_editor_origin_y = nullptr;
    QDoubleSpinBox *m_array_editor_origin_z = nullptr;
    QDoubleSpinBox *m_array_editor_direction_x = nullptr;
    QDoubleSpinBox *m_array_editor_direction_y = nullptr;
    QDoubleSpinBox *m_array_editor_direction_z = nullptr;
    QDoubleSpinBox *m_array_editor_normal_x = nullptr;
    QDoubleSpinBox *m_array_editor_normal_y = nullptr;
    QDoubleSpinBox *m_array_editor_normal_z = nullptr;
    QCheckBox *m_array_editor_conform_normal = nullptr;
    QPushButton *m_array_editor_add_layer = nullptr;
    QPushButton *m_array_editor_update_layer = nullptr;
    QPushButton *m_array_editor_remove_layer = nullptr;
    QPushButton *m_array_editor_move_layer_up = nullptr;
    QPushButton *m_array_editor_move_layer_down = nullptr;
    QComboBox *m_array_editor_fill_pattern = nullptr;
    QSpinBox *m_array_editor_fill_rows = nullptr;
    QSpinBox *m_array_editor_fill_columns = nullptr;
    QDoubleSpinBox *m_array_editor_fill_spacing_x = nullptr;
    QDoubleSpinBox *m_array_editor_fill_spacing_y = nullptr;
    QCheckBox *m_array_editor_fill_circular = nullptr;
    QDoubleSpinBox *m_array_editor_fill_boundary_radius = nullptr;
    QLineEdit *m_array_editor_fill_weights = nullptr;
    QPushButton *m_array_editor_update_fill = nullptr;
    bool m_array_editor_updating = false;
    QLineEdit *m_object_filter = nullptr;
    QDoubleSpinBox *m_unit_position_x = nullptr;
    QDoubleSpinBox *m_unit_position_y = nullptr;
    QDoubleSpinBox *m_unit_position_z = nullptr;
    QDoubleSpinBox *m_unit_direction_x = nullptr;
    QDoubleSpinBox *m_unit_direction_y = nullptr;
    QDoubleSpinBox *m_unit_direction_z = nullptr;
    QDoubleSpinBox *m_unit_pitch = nullptr;
    QDoubleSpinBox *m_unit_yaw = nullptr;
    QDoubleSpinBox *m_unit_target_x = nullptr;
    QDoubleSpinBox *m_unit_target_y = nullptr;
    QDoubleSpinBox *m_unit_target_z = nullptr;
    QComboBox *m_unit_target_scope = nullptr;
    QGroupBox *m_unit_position_group = nullptr;
    QGroupBox *m_unit_direction_group = nullptr;
    QComboBox *m_unit_direction_mode = nullptr;
    QStackedWidget *m_unit_direction_stack = nullptr;
    QHash<QString, bool> m_tree_expansion_state;
};
#endif // MAINWINDOW_H
