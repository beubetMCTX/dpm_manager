#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dpm_file_io.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);





}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionRead_triggered()
{
    QList<Unit> temp;
    bool *ok=new bool();
    *ok= false;
    temp=read_single_dpm_file(ok);
    qDebug()<<"4";
    if(*ok)
    {
        units.clear();
        units=temp;
        //qDebug()<<"5";
        //qDebug()<<units[1].inj.temperature;
    }
}

