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

#include <QFileDialog>
#include <QMessageBox>

#include <TopTools_HSequenceOfShape.hxx>

#include "occtwidget.h"
#include "unit.h"
#include "app_config.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class SpeciesColorDialog;
class SpeciesMaterialDialog;

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

    void on_actionRead_Base_Geometry_triggered();

    void on_actionRead_Chemkin_Files_triggered();

    void on_actionSpecies_Colors_triggered();

    void on_actionSpecies_Materials_triggered();

private:
    void closeEvent(QCloseEvent *event) override;
    QList<Unit> build_test_injector_units() const;
    bool load_chemkin_file(const QString &file_path,
                           bool show_error_message_box,
                           bool show_success_feedback);
    void restore_last_chemkin_file();
    void restore_material_table();
    void apply_material_entries(const QList<MaterialConfigEntry> &entries,
                                bool save_to_config,
                                bool show_status_feedback);
    void update_chemkin_status();

    Ui::MainWindow *ui;
    QStringList m_chemkin_species_names;
    QString m_chemkin_file_path;
    QList<MaterialConfigEntry> m_material_entries;
    QToolBar *m_chemkin_toolbar = nullptr;
    QLabel *m_chemkin_status_label = nullptr;
    QLineEdit *m_chemkin_path_edit = nullptr;
    QPointer<SpeciesColorDialog> m_species_color_dialog;
    QPointer<SpeciesMaterialDialog> m_species_material_dialog;
};
#endif // MAINWINDOW_H
