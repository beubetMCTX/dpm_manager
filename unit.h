#ifndef UNIT_H
#define UNIT_H
#pragma once

#include <AIS_Shape.hxx>


#include "injector.h"

enum Unit_Type
{
    injector,
    line_spacer,
    circle_spacer,
    Assebly
};

class Unit
{
public:

    Injector_OCCT inj;

    Handle(AIS_Shape) ais_display=new AIS_Shape(inj.shape);

    Unit()
    {

    }

    ~Unit()
    {

    }
};

#endif // UNIT_H
