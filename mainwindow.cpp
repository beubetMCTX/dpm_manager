#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dpm_file_io.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //tab_widget = new QTabWidget();

    //this->setCentralWidget(m_3d_widget);
    m_3d_widget = new OCCTWidget(this);
    this->setCentralWidget(m_3d_widget);

     m_3d_widget->create_cube(2,2,2);
    // m_3d_widget->create_sphere(2);

    //arrow_density=10;

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
    temp=read_single_dpm_file_regex(ok);
    if(*ok)
    {
        units.clear();
        units=temp;

        qDebug() << "Loaded injector count:" << units.size();
        for (int i = 0; i < units.size(); ++i)
        {
            qDebug() << "Injector" << i << ":" << units[i].inj.injector_data.name;
        }

        statusBar()->showMessage(
            QString("Loaded %1 injectors from DPM file").arg(units.size()), 5000);
    }
}



void MainWindow::on_actionRead_Base_Geometry_triggered()
{
    bool ok=false;
    ok=m_3d_widget->geometry.Read_Geometry_Dialog();
    if(ok) qDebug()<<"true";
    m_3d_widget->add_readed_geometry();
}





// void MainWindow::on_actionNew_triggered()
// {

// }

