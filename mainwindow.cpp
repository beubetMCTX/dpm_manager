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
    Unit temp;
    bool *ok=new bool();
    *ok= false;
    temp=read_single_dpm_file(ok);
    if(*ok)
    {
        units.clear();
        units.shrink_to_fit();
        units.push_back(temp);
    }
}

