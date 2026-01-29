#ifndef UNIT_H
#define UNIT_H
#pragma once


#include "injector.h"

enum Unit_Type
{
    injector,
    line_spacer,
    circle_spacer,
};

class Unit
{
public:
    Injector inj;

    Unit()
    {

    }

    ~Unit()
    {

    }
};

#endif // UNIT_H
