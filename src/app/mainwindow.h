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
#include <QListWidget>
#include <QByteArray>

#include <QFileDialog>
#include <QMessageBox>

#include <TopTools_HSequenceOfShape.hxx>

#include "occtwidget.h"
#include "unit.h"
#include "app_config.h"
#include "project_session.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SpeciesColorDialog;
class SpeciesMaterialDialog;
class UnitPreferencesDialog;
class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    OCCTWidget* m_3d_widget;
    QList<Unit> units;
    QTabWidget* tab_widget;

public:


private slots:
    void on_actionRead_triggered();

    void on_actionSave_DPM_triggered();

    void on_actionOpen_Project_triggered();

    void on_actionSave_Project_triggered();

    void on_actionSave_Project_As_triggered();

    void on_actionValidate_Project_triggered();

    void on_actionRead_Base_Geometry_triggered();

    void on_actionRead_Chemkin_Files_triggered();

    void on_actionSpecies_Colors_triggered();

    void on_actionSpecies_Materials_triggered();

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
    void update_reference_geometry_panel();
    void update_reference_geometry_controls();
    void apply_reference_geometry_transform();
    void create_object_list_panel();
    void update_object_list_panel();
    void update_object_list_item(const QUuid &uuid, const QString &name);
    void update_object_list_selection(const QUuid &uuid, bool reference_geometry);
    void restore_window_layout();
    void save_window_layout();
    void reset_window_layout();
    void open_unit_preferences_dialog();
    project_session::Data collect_project_data() const;
    void refresh_project_dirty_state();

    Ui::MainWindow *ui;
    QStringList m_chemkin_species_names;
    QString m_chemkin_file_path;
    QString m_project_session_file_path;
    bool m_project_dirty = false;
    bool m_project_baseline_initialized = false;
    QByteArray m_saved_project_fingerprint;
    bool m_loading_project_session = false;
    QList<MaterialConfigEntry> m_material_entries;
    QToolBar *m_chemkin_toolbar = nullptr;
    QLabel *m_chemkin_status_label = nullptr;
    QLineEdit *m_chemkin_path_edit = nullptr;
    QMenu *m_recent_projects_menu = nullptr;
    QStringList m_recent_project_paths;
    QPointer<SpeciesColorDialog> m_species_color_dialog;
    QPointer<SpeciesMaterialDialog> m_species_material_dialog;
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
    QLabel *m_reference_geometry_path = nullptr;
    QLabel *m_reference_face_origin = nullptr;
    QLabel *m_reference_face_normal = nullptr;
    QDockWidget *m_object_list_dock = nullptr;
    QListWidget *m_object_list = nullptr;
    QLineEdit *m_object_filter = nullptr;
};
#endif // MAINWINDOW_H
