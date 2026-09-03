#ifndef UNIT_ARRAY_H
#define UNIT_ARRAY_H

#include "unit.h"
#include "unit_array_spec.h"

#include <QList>
#include <QVector3D>

// Expands one leaf Unit into independent display-ready child Units.
// The source remains unchanged and every child receives a fresh UUID.
QList<Unit> expand_unit_array(const Unit &source, const UnitArraySpec &spec);

// Expands one or more seed Units over a square or hexagonal point layout.
// Seeds are assigned round-robin, allowing a simple deterministic mixture.
QList<Unit> expand_unit_fill(const QList<Unit> &sources, const UnitFillSpec &spec);

#endif // UNIT_ARRAY_H
