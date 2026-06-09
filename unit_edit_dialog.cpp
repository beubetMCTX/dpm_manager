#include "unit_edit_dialog.h"
#include "ui_unit_edit_dialog.h"

unit_edit_dialog::unit_edit_dialog(Unit* control_unit,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::unit_edit_dialog)
{
    ui->setupUi(this);





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
    int i=0;
    return true;
}



// template<typename T>
// Unit_LineEdit<T>::Unit_LineEdit(T* data, bool autoUpdate, QWidget* parent): QLineEdit(parent), m_data(data)
// {
//     // 确保数据指针有效
//     assert(data != nullptr);

//     // 根据类型初始化文本框显示
//     if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>)
//     {
//         setText(QString::number(*data));
//     }
//     else if constexpr (std::is_same_v<T, QString>)
//     {
//         setText(*data);
//     }
//     else
//     {
//         // 静态断言限制支持的类型
//         static_assert(std::is_same_v<T, int> || std::is_same_v<T, double> || std::is_same_v<T, QString>,
//                       "ValidatingLineEdit only supports int, double, and QString");
//     }

//     // 连接回车信号到验证槽函数
//     connect(this, &QLineEdit::returnPressed, this, &Unit_LineEdit<T>::on_return_pressed);
// }

// template<typename T>
// void Unit_LineEdit<T>::on_return_pressed()
// {
//     QString inputText = text().trimmed(); // 获取输入并去除空白
//     if (valid(inputText))
//     {
//         // 验证成功，发射数据变化信号
//         if(auto_update) emit dataChanged();
//     }
//     else
//     {
//         // 验证失败，恢复为原数据值
//         if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>)
//         {
//             setText(QString::number(*m_data));
//         }
//         else if constexpr (std::is_same_v<T, QString>)
//         {
//             setText(*m_data);
//         }
//         // 可选：可以添加错误提示，例如设置样式或显示消息
//     }
// }

// template<typename T>
// bool Unit_LineEdit<T>::valid(const QString &text)
// {
//     bool valid = false;
//     if constexpr (std::is_same_v<T, int>)
//     {
//         int value = text.toInt(&valid);
//         if (valid)
//         {
//             *m_data = value;
//         }
//     }
//     else if constexpr (std::is_same_v<T, double>)
//     {
//         double value = text.toDouble(&valid);
//         if (valid)
//         {
//             *m_data = value;
//         }
//     }
//     else if constexpr (std::is_same_v<T, QString>)
//     {
//         // 对于QString，默认接受任何非空输入（可根据需求调整验证逻辑）
//         if (!text.isEmpty())
//         {
//             *m_data = text;
//             valid = true;
//         }
//         else
//         {
//             valid = false; // 空字符串视为无效
//         }
//     }
//     return valid;
// }
