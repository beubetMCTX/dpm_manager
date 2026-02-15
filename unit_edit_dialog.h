#ifndef UNIT_EDIT_DIALOG_H
#define UNIT_EDIT_DIALOG_H

#include <QDialog>

#include <AIS_Shape.hxx>
#include <QEvent>
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
};

#endif // UNIT_EDIT_DIALOG_H
