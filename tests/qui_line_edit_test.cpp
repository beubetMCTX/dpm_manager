#include "qUI_components.h"

#include <QApplication>

#include <cmath>
#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

bool close_enough(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-12;
}
}

int main(int argc, char **argv)
{
    QApplication application(argc, argv);

    double double_value = 0.0;
    QUI_LineEdit double_editor;
    double_editor.bind_value(&double_value);

    double_editor.setText("5+6");
    if (!check(double_editor.commit(), "5+6 should be accepted") ||
        !check(close_enough(double_value, 11.0), "5+6 should produce 11") ||
        !check(double_editor.text() == "11", "accepted expression should be normalized"))
    {
        return 1;
    }

    double_editor.setText("sin(30)");
    if (!check(double_editor.commit(), "sin(30) should be accepted") ||
        !check(close_enough(double_value, 0.5), "sin(30) should use degrees"))
    {
        return 1;
    }

    const double value_before_invalid = double_value;
    double_editor.setText("5+");
    if (!check(!double_editor.commit(), "invalid expression should be rejected") ||
        !check(close_enough(double_value, value_before_invalid),
               "rejected expression must not change bound value") ||
        !check(double_editor.text() == "0.5", "rejected expression should restore last valid text"))
    {
        return 1;
    }

    int integer_value = 0;
    QUI_LineEdit integer_editor;
    integer_editor.bind_value(&integer_value);
    integer_editor.setText("5+6");
    if (!check(integer_editor.commit(), "integer expression should be accepted") ||
        !check(integer_value == 11, "integer expression should produce 11"))
    {
        return 1;
    }

    integer_editor.setText("5.5");
    if (!check(!integer_editor.commit(), "non-integer result should be rejected") ||
        !check(integer_value == 11, "rejected non-integer must not change integer value"))
    {
        return 1;
    }

    double storage_length = 0.025;
    QUI_LineEdit length_editor;
    length_editor.bind_value(&storage_length);
    if (!check(length_editor.set_unit_conversion("mm", "m"),
               "mm to m conversion should be accepted") ||
        !check(length_editor.text() == "25", "storage value should display in mm"))
    {
        return 1;
    }

    length_editor.setText("30");
    if (!check(length_editor.commit(), "display-unit value should be accepted") ||
        !check(close_enough(storage_length, 0.03),
               "30 mm should be stored as 0.03 m"))
    {
        return 1;
    }

    if (!check(!length_editor.set_unit_conversion("m", "rad"),
               "incompatible unit conversion should be rejected") ||
        !check(length_editor.display_unit() == "" && length_editor.storage_unit() == "",
               "rejected conversion should clear conversion state"))
    {
        return 1;
    }

    return 0;
}
