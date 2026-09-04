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

QString base_unit_for(Unit_Dimension dimension)
{
    switch (dimension)
    {
    case Unit_Dimension::Length:
        return "m";
    case Unit_Dimension::Angle:
        return "rad";
    case Unit_Dimension::Velocity:
        return "m/s";
    case Unit_Dimension::Mass:
        return "kg";
    case Unit_Dimension::MassFlow:
        return "kg/s";
    case Unit_Dimension::Time:
        return "s";
    case Unit_Dimension::Pressure:
        return "Pa";
    case Unit_Dimension::Temperature:
        return "K";
    case Unit_Dimension::Dimensionless:
    default:
        return "-";
    }
}

bool validate_preference(const QString &value,
                         Unit_Dimension dimension,
                         const char *name,
                         QString *error_message)
{
    if (!UnitSystem::are_compatible(value, base_unit_for(dimension)))
    {
        if (error_message != nullptr)
        {
            *error_message = QString("Invalid %1 display unit: %2.")
                                 .arg(QString::fromLatin1(name), value);
        }
        return false;
    }
    return true;
}

Unit_Preferences &active_preferences_storage()
{
    static Unit_Preferences preferences = UnitSystem::default_preferences();
    return preferences;
}
}

Unit_Preferences UnitSystem::default_preferences()
{
    return {};
}

bool UnitSystem::validate_preferences(const Unit_Preferences &preferences,
                                      QString *error_message)
{
    if (error_message != nullptr)
    {
        error_message->clear();
    }

    const bool units_valid = validate_preference(preferences.length, Unit_Dimension::Length, "length", error_message) &&
           validate_preference(preferences.angle, Unit_Dimension::Angle, "angle", error_message) &&
           validate_preference(preferences.velocity, Unit_Dimension::Velocity, "velocity", error_message) &&
           validate_preference(preferences.mass, Unit_Dimension::Mass, "mass", error_message) &&
           validate_preference(preferences.mass_flow, Unit_Dimension::MassFlow, "mass_flow", error_message) &&
           validate_preference(preferences.time, Unit_Dimension::Time, "time", error_message) &&
           validate_preference(preferences.pressure, Unit_Dimension::Pressure, "pressure", error_message) &&
           validate_preference(preferences.temperature, Unit_Dimension::Temperature, "temperature", error_message);
    if (!units_valid)
    {
        return false;
    }
    if (!std::isfinite(preferences.injector_transparency) ||
        preferences.injector_transparency < 0.0 || preferences.injector_transparency > 1.0 ||
        !std::isfinite(preferences.reference_geometry_transparency) ||
        preferences.reference_geometry_transparency < 0.0 || preferences.reference_geometry_transparency > 1.0)
    {
        if (error_message != nullptr)
        {
            *error_message = "Transparency values must be between 0 and 1.";
        }
        return false;
    }
    return true;
}

void UnitSystem::set_active_preferences(const Unit_Preferences &preferences)
{
    if (validate_preferences(preferences, nullptr))
    {
        active_preferences_storage() = preferences;
    }
}

Unit_Preferences UnitSystem::active_preferences()
{
    return active_preferences_storage();
}

QString UnitSystem::base_unit(Unit_Dimension dimension)
{
    return base_unit_for(dimension);
}

QString UnitSystem::preferred_display_unit(const QString &semantic_unit)
{
    const Unit_Definition definition_value = definition(semantic_unit);
    if (definition_value.symbol.isEmpty())
    {
        return semantic_unit.trimmed();
    }

    const Unit_Preferences preferences = active_preferences_storage();
    switch (definition_value.dimension)
    {
    case Unit_Dimension::Length:
        return preferences.length;
    case Unit_Dimension::Angle:
        return preferences.angle;
    case Unit_Dimension::Velocity:
        return preferences.velocity;
    case Unit_Dimension::Mass:
        return preferences.mass;
    case Unit_Dimension::MassFlow:
        return preferences.mass_flow;
    case Unit_Dimension::Time:
        return preferences.time;
    case Unit_Dimension::Pressure:
        return preferences.pressure;
    case Unit_Dimension::Temperature:
        return preferences.temperature;
    case Unit_Dimension::Dimensionless:
    default:
        return semantic_unit.trimmed();
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

    bool base_ok = false;
    const double base_value = to_base(value, from_symbol, &base_ok);
    if (!base_ok)
    {
        set_result(ok, false);
        return 0.0;
    }

    bool converted_ok = false;
    const double converted = from_base(base_value, to_symbol, &converted_ok);
    set_result(ok, converted_ok);
    return converted_ok ? converted : 0.0;
}

double UnitSystem::to_base(double value, const QString &from_symbol, bool *ok)
{
    const Unit_Definition from = definition(from_symbol);
    const bool input_valid = !from.symbol.isEmpty() && std::isfinite(value);
    const double converted = input_valid
        ? value * from.scale_to_base + from.offset_to_base
        : 0.0;
    const bool valid = input_valid && std::isfinite(converted);
    set_result(ok, valid);
    return valid ? converted : 0.0;
}

double UnitSystem::from_base(double value, const QString &to_symbol, bool *ok)
{
    const Unit_Definition to = definition(to_symbol);
    const bool input_valid = !to.symbol.isEmpty() && std::isfinite(value);
    const double converted = input_valid
        ? (value - to.offset_to_base) / to.scale_to_base
        : 0.0;
    const bool valid = input_valid && std::isfinite(converted);
    set_result(ok, valid);
    return valid ? converted : 0.0;
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
