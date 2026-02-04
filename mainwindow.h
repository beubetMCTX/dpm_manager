#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qvector.h>
#include <QList>

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


    QList<Unit> units;

public:


private slots:
    void on_actionRead_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
