#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qvector.h>
#include <QList>

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

public:


private slots:
    void on_actionRead_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
