#include "qUI_components.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    QUI_LineEdit double_editor;
    double_editor.set_double_mode();
    double_editor.setText(QStringLiteral("5+6"));
    if (!double_editor.commit() || double_editor.text() != QStringLiteral("11"))
    {
        qCritical() << "Double expression evaluation failed:" << double_editor.text();
        return 1;
    }

    double_editor.setText(QStringLiteral("sin(30)"));
    if (!double_editor.commit() || double_editor.text() != QStringLiteral("0.5"))
    {
        qCritical() << "Degree-based trigonometric evaluation failed:" << double_editor.text();
        return 1;
    }

    QUI_LineEdit integer_editor;
    integer_editor.set_integer_mode();
    integer_editor.setText(QStringLiteral("5+6"));
    if (!integer_editor.commit() || integer_editor.text() != QStringLiteral("11"))
    {
        qCritical() << "Integer expression evaluation failed:" << integer_editor.text();
        return 1;
    }

    integer_editor.setText(QStringLiteral("sin(30)"));
    if (integer_editor.commit())
    {
        qCritical() << "Non-integer expression was accepted as integer.";
        return 1;
    }

    int combo_value = 0;
    QUI_ComboBox combo;
    int combo_commits = 0;
    QObject::connect(&combo, &QUI_ComboBox::selection_committed,
                     [&combo_commits]() { ++combo_commits; });
    combo.add_option(QStringLiteral("A"));
    combo.add_option(QStringLiteral("B"));
    combo.bind_current_index(&combo_value);
    if (combo_commits != 0)
    {
        qCritical() << "Programmatic combo synchronization emitted a commit.";
        return 1;
    }

    bool check_value = false;
    QUI_CheckBox check_box;
    int check_commits = 0;
    QObject::connect(&check_box, &QUI_CheckBox::value_committed,
                     [&check_commits](bool) { ++check_commits; });
    check_box.bind_value(&check_value);
    if (check_commits != 0)
    {
        qCritical() << "Programmatic checkbox synchronization emitted a commit.";
        return 1;
    }

    int radio_value = 0;
    QUI_RadioGroup radio_group;
    int radio_commits = 0;
    QObject::connect(&radio_group, &QUI_RadioGroup::value_committed,
                     [&radio_commits](int) { ++radio_commits; });
    radio_group.add_option(QStringLiteral("A"));
    radio_group.add_option(QStringLiteral("B"));
    radio_group.bind_checked_id(&radio_value);
    if (radio_commits != 0)
    {
        qCritical() << "Programmatic radio synchronization emitted a commit.";
        return 1;
    }

    qInfo() << "Component smoke test passed";
    return 0;
}
