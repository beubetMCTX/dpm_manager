#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <qvector.h>
#include <QList>
#include <QLabel>
#include <QStringList>
#include <QToolBar>

#include <QFileDialog>
#include <QMessageBox>

#include <TopTools_HSequenceOfShape.hxx>

#include "occtwidget.h"
#include "unit.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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

private:
    QList<Unit> build_test_injector_units() const;
    void update_chemkin_status();

    Ui::MainWindow *ui;
    QStringList m_chemkin_species_names;
    QString m_chemkin_file_path;
    QToolBar *m_chemkin_toolbar = nullptr;
    QLabel *m_chemkin_status_label = nullptr;
    QLineEdit *m_chemkin_path_edit = nullptr;
};
#endif // MAINWINDOW_H
