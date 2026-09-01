#include "unit_system.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool close_enough(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-12;
}

bool expect(bool condition, const char *message)
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
    (void)argc;
    (void)argv;

    bool ok = false;
    if (!expect(UnitSystem::is_supported("mm"), "mm should be supported") ||
        !expect(UnitSystem::are_compatible("mm", "m"), "length units should be compatible") ||
        !expect(!UnitSystem::are_compatible("mm", "rad"), "length and angle should be incompatible") ||
        !expect(close_enough(UnitSystem::convert(25.0, "mm", "m", &ok), 0.025) && ok,
                "25 mm should convert to 0.025 m") ||
        !expect(close_enough(UnitSystem::convert(180.0, "deg", "rad", &ok),
                             3.14159265358979323846) && ok,
                "180 deg should convert to pi rad") ||
        !expect(close_enough(UnitSystem::convert(20.0, "C", "K", &ok), 293.15) && ok,
                "20 C should convert to 293.15 K") ||
        !expect(close_enough(UnitSystem::convert(1.0, "bar", "Pa", &ok), 100000.0) && ok,
                "1 bar should convert to 100000 Pa") ||
        !expect(UnitSystem::convert(1.0, "m", "rad", &ok) == 0.0 && !ok,
                "incompatible conversion should fail") ||
        !expect(UnitSystem::convert(std::numeric_limits<double>::infinity(), "m", "mm", &ok) == 0.0 && !ok,
                "non-finite conversion should fail"))
    {
        return 1;
    }

    return 0;
}
