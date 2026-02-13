#ifndef UNIT_H
#define UNIT_H

#pragma once

#include <AIS_Shape.hxx>

#include <qdebug.h>


#include "injector.h"

enum Unit_Type
{
    injector,
    line_spacer,
    circle_spacer,
    Assebly
};

// class Unit;

// class AIS_UnitShape : public AIS_Shape
// {
// public:
//     DEFINE_STANDARD_RTTI_INLINE(AIS_UnitShape, AIS_Shape)


//     AIS_UnitShape(const TopoDS_Shape& the_shape, Unit* _unit)
//         : AIS_Shape(the_shape) {set_unitptr(_unit);}

//     Unit* get_unitptr() const { return m_unit; }

//     // 设置注射器指针
//     void set_unitptr(Unit* the_unit) { m_unit=the_unit;}


// private:
//     Unit* m_unit;
// };


class Unit
{
public:

    Unit_Type type=injector;

    Injector_OCCT inj;

    Handle(AIS_Shape) ais_display=new AIS_Shape(inj.shape);

    Unit()
    {

    }

    ~Unit()
    {

    }

    void test(){qDebug()<<inj.injector_data.name;}
};




#endif // UNIT_H
