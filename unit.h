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
    Injector *inj;

    Unit()
    {
        inj=new Injector();
    }

    ~Unit()
    {
        delete(inj);
    }
};

#endif // UNIT_H
