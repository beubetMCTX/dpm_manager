#include "unit_system.h"

#include <QHash>

#include <cmath>

namespace
{
const QHash<QString, Unit_Definition> &unit_table()
{
    static const QHash<QString, Unit_Definition> table = {
        {"-", {"-", Unit_Dimension::Dimensionless, 1.0, 0.0}},
        {"1", {"1", Unit_Dimension::Dimensionless, 1.0, 0.0}},

        {"m", {"m", Unit_Dimension::Length, 1.0, 0.0}},
        {"mm", {"mm", Unit_Dimension::Length, 1.0e-3, 0.0}},
        {"cm", {"cm", Unit_Dimension::Length, 1.0e-2, 0.0}},
        {"km", {"km", Unit_Dimension::Length, 1.0e3, 0.0}},

        {"rad", {"rad", Unit_Dimension::Angle, 1.0, 0.0}},
        {"deg", {"deg", Unit_Dimension::Angle, 3.14159265358979323846 / 180.0, 0.0}},

        {"m/s", {"m/s", Unit_Dimension::Velocity, 1.0, 0.0}},
        {"km/h", {"km/h", Unit_Dimension::Velocity, 1000.0 / 3600.0, 0.0}},

        {"kg", {"kg", Unit_Dimension::Mass, 1.0, 0.0}},
        {"g", {"g", Unit_Dimension::Mass, 1.0e-3, 0.0}},

        {"kg/s", {"kg/s", Unit_Dimension::MassFlow, 1.0, 0.0}},
        {"g/s", {"g/s", Unit_Dimension::MassFlow, 1.0e-3, 0.0}},

        {"s", {"s", Unit_Dimension::Time, 1.0, 0.0}},
        {"ms", {"ms", Unit_Dimension::Time, 1.0e-3, 0.0}},
        {"min", {"min", Unit_Dimension::Time, 60.0, 0.0}},

        {"Pa", {"Pa", Unit_Dimension::Pressure, 1.0, 0.0}},
        {"kPa", {"kPa", Unit_Dimension::Pressure, 1.0e3, 0.0}},
        {"bar", {"bar", Unit_Dimension::Pressure, 1.0e5, 0.0}},

        {"K", {"K", Unit_Dimension::Temperature, 1.0, 0.0}},
        {"C", {"C", Unit_Dimension::Temperature, 1.0, 273.15}},
    };
    return table;
}

Unit_Definition invalid_definition()
{
    return {QString(), Unit_Dimension::Dimensionless, 1.0, 0.0};
}

void set_result(bool *ok, bool value)
{
    if (ok != nullptr)
    {
        *ok = value;
    }
}
}

Unit_Definition UnitSystem::definition(const QString &symbol)
{
    const auto it = unit_table().constFind(symbol.trimmed());
    return it == unit_table().constEnd() ? invalid_definition() : it.value();
}

bool UnitSystem::is_supported(const QString &symbol)
{
    return unit_table().contains(symbol.trimmed());
}

bool UnitSystem::are_compatible(const QString &from_symbol, const QString &to_symbol)
{
    const Unit_Definition from = definition(from_symbol);
    const Unit_Definition to = definition(to_symbol);
    return !from.symbol.isEmpty() && !to.symbol.isEmpty() && from.dimension == to.dimension;
}

double UnitSystem::convert(double value,
                           const QString &from_symbol,
                           const QString &to_symbol,
                           bool *ok)
{
    if (!are_compatible(from_symbol, to_symbol) || !std::isfinite(value))
    {
        set_result(ok, false);
        return 0.0;
    }

    const double base_value = to_base(value, from_symbol, nullptr);
    const double converted = from_base(base_value, to_symbol, nullptr);
    set_result(ok, std::isfinite(converted));
    return converted;
}

double UnitSystem::to_base(double value, const QString &from_symbol, bool *ok)
{
    const Unit_Definition from = definition(from_symbol);
    const bool valid = !from.symbol.isEmpty() && std::isfinite(value);
    set_result(ok, valid);
    return valid ? value * from.scale_to_base + from.offset_to_base : 0.0;
}

double UnitSystem::from_base(double value, const QString &to_symbol, bool *ok)
{
    const Unit_Definition to = definition(to_symbol);
    const bool valid = !to.symbol.isEmpty() && std::isfinite(value);
    set_result(ok, valid);
    return valid ? (value - to.offset_to_base) / to.scale_to_base : 0.0;
}

QStringList UnitSystem::symbols(Unit_Dimension dimension)
{
    QStringList result;
    for (auto it = unit_table().cbegin(); it != unit_table().cend(); ++it)
    {
        if (it.value().dimension == dimension)
        {
            result.append(it.key());
        }
    }
    result.sort();
    return result;
}
