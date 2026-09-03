#ifndef UNIT_H
#define UNIT_H


#pragma once

#include <AIS_Shape.hxx>
#include <SelectMgr_EntityOwner.hxx>

#include <qdebug.h>
#include <utility>

#include "injector.h"
#include "unit_array_spec.h"

enum Unit_Type
{
    injector,
    line_spacer,
    circle_spacer,
    Assebly,
    array
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

    Handle(AIS_Shape) ais_display;

    Handle(Unit_Owner) u_owner;

    // Array children are derived runtime instances. They are intentionally
    // not copied with a leaf Unit; the owning scene rebuilds them from the
    // array definition instead of duplicating live AIS handles.
    QVector<std::shared_ptr<Unit>> child_units;
    QUuid array_parent_uuid;
    bool is_array_child = false;
    bool follows_array = true;
    bool has_array_spec = false;
    UnitArraySpec array_spec;

    Unit()
        : type(injector)
        , inj()
    {
        initialize_runtime_handles();
    }

    Unit(const Unit &other)
        : type(other.type)
        , inj(other.inj)
        , array_parent_uuid(other.array_parent_uuid)
        , is_array_child(other.is_array_child)
        , follows_array(other.follows_array)
    {
        initialize_runtime_handles();
    }

    Unit &operator=(const Unit &other)
    {
        if (this == &other)
        {
            return *this;
        }

        type = other.type;
        inj = other.inj;
        child_units.clear();
        array_parent_uuid = other.array_parent_uuid;
        is_array_child = other.is_array_child;
        follows_array = other.follows_array;
        has_array_spec = other.has_array_spec;
        array_spec = other.array_spec;
        initialize_runtime_handles();
        return *this;
    }

    Unit(Unit &&other) noexcept
        : type(other.type)
        , inj(std::move(other.inj))
        , array_parent_uuid(other.array_parent_uuid)
        , is_array_child(other.is_array_child)
        , follows_array(other.follows_array)
    {
        initialize_runtime_handles();
    }

    Unit &operator=(Unit &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        type = other.type;
        inj = std::move(other.inj);
        child_units.clear();
        array_parent_uuid = other.array_parent_uuid;
        is_array_child = other.is_array_child;
        follows_array = other.follows_array;
        has_array_spec = other.has_array_spec;
        array_spec = other.array_spec;
        initialize_runtime_handles();
        return *this;
    }

    void test(){qDebug()<<inj.injector_data.name;}

private:
    void initialize_runtime_handles()
    {
        ais_display = new AIS_Shape(inj.shape);
        u_owner = new Unit_Owner(this);
        ais_display->SetOwner(u_owner);
    }
};






#endif // UNIT_H
