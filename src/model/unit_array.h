#ifndef UNIT_ARRAY_H
#define UNIT_ARRAY_H

#include "unit.h"
#include "unit_array_spec.h"

#include <QList>
#include <QHash>
#include <QVector3D>

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

#endif // UNIT_ARRAY_H
