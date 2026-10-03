#ifndef UNIT_H
#define UNIT_H


#pragma once

#include <AIS_Shape.hxx>
#include <SelectMgr_EntityOwner.hxx>

#include <qdebug.h>
#include <utility>
#include <QHash>
#include <QList>
#include <QVector>

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

// Display state is persisted separately from Unit geometry. Runtime-derived
// array children are rebuilt, while persistent Units keep their UUID state.
struct UnitDisplayState
{
    bool visible = true;
    bool locked = false;
};

// Runtime-derived array children receive fresh UUIDs after every rebuild.
// Persist their display state through stable array/prototype identity instead
// of the transient runtime UUID.
struct DerivedUnitDisplayState
{
    QUuid array_parent_uuid;
    QList<QUuid> prototype_chain;
    QVector<int> array_instance_path;
    QVector<QUuid> array_instance_key;
    QUuid array_layer_uuid;
    UnitDisplayState state;
};

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

struct UnitArrayOverride
{
    // New projects store property-level inheritance in this mask. The two
    // boolean fields below remain for backward compatibility with schema v4
    // sessions and are treated as legacy aliases when the mask is empty.
    quint32 override_fields = 0;
    QVector<int> instance_path;
    QVector<QUuid> instance_key;
    bool override_physical = false;
    bool override_geometry = false;
    Injector snapshot;
};

// Property-level array override scopes. A child can override only the data it
// needs instead of detaching its complete physical or geometric state.
enum UnitArrayOverrideField : quint32
{
    UnitArrayOverrideNone = 0u,
    UnitArrayOverrideMaterialSpecies = 1u << 0,
    UnitArrayOverridePhysical = 1u << 1,
    UnitArrayOverridePosition = 1u << 2,
    UnitArrayOverrideDirection = 1u << 3,
    UnitArrayOverrideParticleSize = 1u << 4,
    UnitArrayOverrideGeometry = 1u << 5
};

constexpr quint32 unit_array_geometry_override_fields()
{
    return UnitArrayOverridePosition | UnitArrayOverrideDirection |
           UnitArrayOverrideParticleSize | UnitArrayOverrideGeometry;
}

constexpr quint32 unit_array_physical_override_fields()
{
    return UnitArrayOverrideMaterialSpecies | UnitArrayOverridePhysical;
}

inline quint32 effective_unit_array_override_fields(
    const UnitArrayOverride &override_state)
{
    if (override_state.override_fields != UnitArrayOverrideNone)
    {
        return override_state.override_fields;
    }

    quint32 fields = UnitArrayOverrideNone;
    if (override_state.override_physical)
    {
        fields |= UnitArrayOverridePhysical;
    }
    if (override_state.override_geometry)
    {
        fields |= UnitArrayOverrideGeometry;
    }
    return fields;
}

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
    // Persistent nested sources can keep their own Array/Fill definition while
    // optionally following the placement transform of their parent array.
    bool follows_parent_transform = false;
    QUuid prototype_uuid;
    QList<QUuid> prototype_chain;
    // Stable placement path within the owning Array/Fill source. Runtime
    // UUIDs are regenerated during rebuilds. The UUID path is authoritative;
    // the integer path remains for backward-compatible project files.
    QVector<int> array_instance_path;
    QVector<QUuid> array_instance_key;
    QUuid array_layer_uuid;
    QList<UnitArrayOverride> array_overrides;
    bool has_array_spec = false;
    UnitArraySpec array_spec;
    // Ordered array layers. array_spec remains the latest layer for
    // backward-compatible callers and project files.
    QList<UnitArraySpec> array_specs;
    // Runtime-only layer marker used by the object tree. Zero means that the
    // Unit is not a generated array-layer root.
    int array_layer = 0;
    bool has_fill_spec = false;
    UnitFillSpec fill_spec;
    QList<QUuid> fill_source_uuids;
    QUuid assembly_parent_uuid;
    QList<QUuid> assembly_child_uuids;
    // Transform relative to the owning Assembly. World-space injector data
    // remains the runtime representation for backward compatibility.
    QVector3D assembly_local_position;
    QVector3D assembly_local_rotation;

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
        , follows_parent_transform(other.follows_parent_transform)
        , prototype_uuid(other.prototype_uuid)
        , prototype_chain(other.prototype_chain)
        , array_instance_path(other.array_instance_path)
        , array_instance_key(other.array_instance_key)
        , array_layer_uuid(other.array_layer_uuid)
        , array_overrides(other.array_overrides)
        , has_array_spec(other.has_array_spec)
        , array_spec(other.array_spec)
        , array_specs(other.array_specs)
        , array_layer(other.array_layer)
        , has_fill_spec(other.has_fill_spec)
        , fill_spec(other.fill_spec)
        , fill_source_uuids(other.fill_source_uuids)
        , assembly_parent_uuid(other.assembly_parent_uuid)
        , assembly_child_uuids(other.assembly_child_uuids)
        , assembly_local_position(other.assembly_local_position)
        , assembly_local_rotation(other.assembly_local_rotation)
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
        follows_parent_transform = other.follows_parent_transform;
        prototype_uuid = other.prototype_uuid;
        prototype_chain = other.prototype_chain;
        array_instance_path = other.array_instance_path;
        array_instance_key = other.array_instance_key;
        array_layer_uuid = other.array_layer_uuid;
        array_overrides = other.array_overrides;
        has_array_spec = other.has_array_spec;
        array_spec = other.array_spec;
        array_specs = other.array_specs;
        array_layer = other.array_layer;
        has_fill_spec = other.has_fill_spec;
        fill_spec = other.fill_spec;
        fill_source_uuids = other.fill_source_uuids;
        assembly_parent_uuid = other.assembly_parent_uuid;
        assembly_child_uuids = other.assembly_child_uuids;
        assembly_local_position = other.assembly_local_position;
        assembly_local_rotation = other.assembly_local_rotation;
        initialize_runtime_handles();
        return *this;
    }

    Unit(Unit &&other) noexcept
        : type(other.type)
        , inj(std::move(other.inj))
        , array_parent_uuid(other.array_parent_uuid)
        , is_array_child(other.is_array_child)
        , follows_array(other.follows_array)
        , follows_parent_transform(other.follows_parent_transform)
        , prototype_uuid(other.prototype_uuid)
        , prototype_chain(other.prototype_chain)
        , array_instance_path(std::move(other.array_instance_path))
        , array_instance_key(std::move(other.array_instance_key))
        , array_layer_uuid(other.array_layer_uuid)
        , array_overrides(std::move(other.array_overrides))
        , has_array_spec(other.has_array_spec)
        , array_spec(std::move(other.array_spec))
        , array_specs(std::move(other.array_specs))
        , array_layer(other.array_layer)
        , has_fill_spec(other.has_fill_spec)
        , fill_spec(std::move(other.fill_spec))
        , fill_source_uuids(std::move(other.fill_source_uuids))
        , assembly_parent_uuid(other.assembly_parent_uuid)
        , assembly_child_uuids(std::move(other.assembly_child_uuids))
        , assembly_local_position(other.assembly_local_position)
        , assembly_local_rotation(other.assembly_local_rotation)
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
        follows_parent_transform = other.follows_parent_transform;
        prototype_uuid = other.prototype_uuid;
        prototype_chain = other.prototype_chain;
        array_instance_path = std::move(other.array_instance_path);
        array_instance_key = std::move(other.array_instance_key);
        array_layer_uuid = other.array_layer_uuid;
        array_overrides = std::move(other.array_overrides);
        has_array_spec = other.has_array_spec;
        array_spec = other.array_spec;
        array_specs = std::move(other.array_specs);
        array_layer = other.array_layer;
        has_fill_spec = other.has_fill_spec;
        fill_spec = other.fill_spec;
        fill_source_uuids = other.fill_source_uuids;
        assembly_parent_uuid = other.assembly_parent_uuid;
        assembly_child_uuids = std::move(other.assembly_child_uuids);
        assembly_local_position = other.assembly_local_position;
        assembly_local_rotation = other.assembly_local_rotation;
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
