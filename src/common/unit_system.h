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

class UnitSystem
{
public:
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
