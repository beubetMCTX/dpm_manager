#include "app_config.h"

#include <QCoreApplication>

#include <cmath>
#include <iostream>
#include <limits>

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
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QString error_message;

    const QList<MaterialConfigEntry> valid = {{"water", 998.2}, {"kerosene", 800.0}};
    if (!check(validate_material_entries(valid, &error_message),
               "valid material entries should pass validation"))
    {
        return 1;
    }

    if (!check(!validate_material_entries({{"water", 0.0}}, &error_message) &&
                   error_message.contains("positive"),
               "zero material density should fail validation") ||
        !check(!validate_material_entries({{"water", std::numeric_limits<double>::infinity()}},
                                          &error_message) &&
                   error_message.contains("finite"),
               "non-finite material density should fail validation") ||
        !check(!validate_material_entries({{"water", 1.0}, {"WATER", 2.0}},
                                          &error_message) &&
                   error_message.contains("Duplicate"),
               "case-insensitive duplicate material names should fail validation"))
    {
        return 1;
    }

    return 0;
}
