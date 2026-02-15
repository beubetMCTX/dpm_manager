#ifndef UNIT_H
#define UNIT_H


#pragma once

#include <AIS_Shape.hxx>
#include <SelectMgr_EntityOwner.hxx>

#include <qdebug.h>

#include "injector.h"

enum Unit_Type
{
    injector,
    line_spacer,
    circle_spacer,
    Assebly
};

class Unit;

class Unit_Owner : public SelectMgr_EntityOwner
{
    DEFINE_STANDARD_RTTI_INLINE(Unit_Owner, SelectMgr_EntityOwner)

public:
    // 构造函数
    Unit_Owner(Unit* the_unit,Standard_Integer thePriority = 0)
        : SelectMgr_EntityOwner(thePriority)
    {
        m_Unit=the_unit;
    }

    // 获取存储的Injector指针
    Unit* get_unit() const { return m_Unit; }

    // 检查是否有效
    Standard_Boolean IsValid() const
    {
        return (m_Unit != nullptr);
    }

    void set_unit(Unit* the_unit){m_Unit=the_unit;}

    // 对象销毁时的清理（重要！）
    virtual ~Unit_Owner()
    {
        // 注意：这里不要删除myInjector，它由外部管理
    }

private:
    Unit* m_Unit=nullptr;

};

class Unit
{
public:

    Unit_Type type=injector;

    Injector_OCCT inj;

    Handle(AIS_Shape) ais_display=new AIS_Shape(inj.shape);

    Handle(Unit_Owner) u_owner=new Unit_Owner(this);

    Unit()
    {

    }

    ~Unit()
    {

    }

    void test(){qDebug()<<inj.injector_data.name;}
};






#endif // UNIT_H
