#ifndef UNIT_ARRAY_H
#define UNIT_ARRAY_H

#include "unit.h"
#include "unit_array_spec.h"

#include <QList>
#include <QHash>
#include <QVector3D>

// Target mode keeps the configured point. Other modes follow the copy unless
// the scope is world or reference-local.
bool target_hitpoint_follows_copy(const Injector &injector);

// Reference-local values are read in the pattern frame. Other scopes return
// the hitpoint already stored on the injector.
QVector3D resolved_aim_point(const Injector &injector, const UnitArraySpec &spec);
QVector3D resolved_aim_point(const Injector &injector, const UnitFillSpec &spec);

// Expands one leaf Unit into independent display-ready child Units.
// The source remains unchanged and every child receives a fresh UUID.
QList<Unit> expand_unit_array(const Unit &source, const UnitArraySpec &spec);

// Expands one or more seed Units over a square or hexagonal point layout.
// Seeds are assigned by the optional deterministic source-weight cycle.
QList<Unit> expand_unit_fill(const QList<Unit> &sources, const UnitFillSpec &spec);

// Clones a persistent Assembly/Unit subtree with fresh UUIDs. Runtime
// derived array children are omitted and can be rebuilt from their metadata.
std::shared_ptr<Unit> clone_unit_tree(const Unit &source,
                                      QHash<QUuid, QUuid> &uuid_map,
                                      bool mark_as_derived = false);

// Expands a persistent Unit tree as composite array instances. The tree is
// cloned once per linear or rotational placement and transformed as a whole.
QList<std::shared_ptr<Unit>> expand_unit_tree_array(const Unit &source,
                                                    const UnitArraySpec &spec);

// Expands mixed leaf or composite sources over a square/hexagonal layout,
// preserving the complete source tree for every placement.
QList<std::shared_ptr<Unit>> expand_unit_tree_fill(
    const QList<std::shared_ptr<Unit>> &sources, const UnitFillSpec &spec);

// Applies one rigid transform to every injector in a persistent Unit tree.
void transform_unit_tree(Unit &root, const QVector3D &pivot,
                         const QVector3D &axis, float angle_radians,
                         const QVector3D &translation = QVector3D());

// Normalize stable identities while preserving existing IDs when a caller
// changes only count/rows/columns. These functions are also used to migrate
// legacy sessions that contain only numeric instance paths.
void initialize_new_array_spec_identity(UnitArraySpec &spec);
void reconcile_array_spec_identity(UnitArraySpec &spec,
                                   const UnitArraySpec &previous);
void ensure_array_spec_identity(UnitArraySpec &spec);
void initialize_new_fill_spec_identity(UnitFillSpec &spec);
void reconcile_fill_spec_identity(UnitFillSpec &spec,
                                  const UnitFillSpec &previous);
void ensure_fill_spec_identity(UnitFillSpec &spec);

// New array layers should not inherit the hardcoded world +X default.
// Direction is a stable offset perpendicular to the injector, and the
// plane normal follows the injector spray axis.
void array_frame_from_injector(const Injector &injector,
                               QVector3D *origin,
                               QVector3D *direction,
                               QVector3D *plane_normal);

// Preview geometry for the array frame. Loop vectors run from the center to
// the curve; they are not unit vectors.
enum class ArrayGuideKind
{
    None,
    Segment,
    Loop,
    Plane
};

struct ArrayPatternGuide
{
    ArrayGuideKind kind = ArrayGuideKind::None;
    QVector3D origin;
    QVector3D normal;
    QVector3D major;
    QVector3D minor;
    float length = 0.0f;
    float half_extent = 0.0f;
};

ArrayPatternGuide array_pattern_guide(const QVector3D &source_position,
                                      const UnitArraySpec &spec);

// Yaw every injector in the instance around pivot without moving that pivot
// or the array frame. Identity headings are ignored.
void apply_array_heading_about_pivot(Unit &root, const QVector3D &pivot,
                                     const QQuaternion &heading);
void apply_array_heading(Unit &root, const QQuaternion &heading);

inline bool array_heading_is_identity(const QQuaternion &heading)
{
    return heading.vector().lengthSquared() <= 1.0e-12f;
}

#endif // UNIT_ARRAY_H
