#ifndef UNIT_SYSTEM_H
#define UNIT_SYSTEM_H

#include <QString>
#include <QStringList>

enum class Unit_Dimension
{
    Dimensionless,
    Length,
    Angle,
    Velocity,
    Mass,
    MassFlow,
    Time,
    Pressure,
    Temperature
};

struct Unit_Definition
{
    QString symbol;
    Unit_Dimension dimension = Unit_Dimension::Dimensionless;
    double scale_to_base = 1.0;
    double offset_to_base = 0.0;
};

struct Unit_Preferences
{
    QString length = "mm";
    QString angle = "deg";
    QString velocity = "m/s";
    QString mass = "kg";
    QString mass_flow = "kg/s";
    QString time = "s";
    QString pressure = "Pa";
    QString temperature = "K";
};

class UnitSystem
{
public:
    static Unit_Preferences default_preferences();
    static bool validate_preferences(const Unit_Preferences &preferences,
                                     QString *error_message = nullptr);
    static void set_active_preferences(const Unit_Preferences &preferences);
    static Unit_Preferences active_preferences();
    static QString base_unit(Unit_Dimension dimension);
    static QString preferred_display_unit(const QString &semantic_unit);

    static Unit_Definition definition(const QString &symbol);
    static bool is_supported(const QString &symbol);
    static bool are_compatible(const QString &from_symbol, const QString &to_symbol);

    static double convert(double value,
                          const QString &from_symbol,
                          const QString &to_symbol,
                          bool *ok = nullptr);

    static double to_base(double value, const QString &from_symbol, bool *ok = nullptr);
    static double from_base(double value, const QString &to_symbol, bool *ok = nullptr);

    static QStringList symbols(Unit_Dimension dimension);
};

#endif // UNIT_SYSTEM_H
