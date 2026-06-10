#include "unit_edit_dialog.h"
#include "ui_unit_edit_dialog.h"

unit_edit_dialog::unit_edit_dialog(Unit* control_unit,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::unit_edit_dialog)
    , control_unit(control_unit)
{
    ui->setupUi(this);
    initialize();

    // for(auto i =0;i<ui->verticalLayout_number_of_stream->count();i++)
    // {
    //     QWidget *w = ui->verticalLayout_number_of_stream->itemAt(i)->widget();
    //     if(w != nullptr){
    //         w->setVisible(false);
    //     }
    // }


    //QLabel *label = new QLabel("这是一个简单的对话框", this);
    //ui->verticalLayout->addWidget(label);
    //ui->frame_layout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //ui->horizontalLayout_partical_type->setParent(ui->groupBox_partical_type);



}

unit_edit_dialog::~unit_edit_dialog()
{
    delete ui;
}

inline bool unit_edit_dialog::initialize()
{
    if (control_unit == nullptr)
    {
        return false;
    }

    setWindowTitle(QString("Unit Editor - %1").arg(control_unit->inj.injector_data.name));
    return true;
}
