#ifndef UNIT_EDIT_DIALOG_H
#define UNIT_EDIT_DIALOG_H

#include <QDialog>

#include <AIS_Shape.hxx>
#include <QEvent>
#include <qlineedit.h>
#include "unit.h"

namespace Ui {
class unit_edit_dialog;
}

class unit_edit_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit unit_edit_dialog(Unit* control_unit,QWidget *parent = nullptr);

    ~unit_edit_dialog();

protected:
    // 可以重写 resizeEvent！
    void resizeEvent(QResizeEvent *event) override
    {
        // 先调用基类实现
        QDialog::resizeEvent(event);
    }

private slots:


private:
    Ui::unit_edit_dialog *ui;
    Unit* control_unit;

    inline bool initialize();
};



// template<typename T>
// class Unit_LineEdit : public QLineEdit
// {
//    //Q_OBJECT

// public:
//     explicit Unit_LineEdit(T* data, bool autoUpdate=false , QWidget* parent = nullptr);

// signals:
//     void unit_data_changed();

// private slots:
//     void on_return_pressed();

// private:
//     T* m_data;

//     bool auto_update;


// private:
//     bool valid(const QString& text);
// };




#endif // UNIT_EDIT_DIALOG_H
