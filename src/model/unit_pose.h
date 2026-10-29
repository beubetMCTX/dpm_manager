#pragma once

#include "injector.h"

#include <QVector3D>

#include <functional>

// Shared pose and direction writes. Callers emit UnitDocument signals after
// these return. Unit stays copyable and does not emit signals itself.

void translate_injector_points(Injector &injector, const QVector3D &delta);
void set_injector_direction(Injector &injector, const QVector3D &direction);
void set_injector_pitch_yaw(Injector &injector, double pitch_degrees,
                            double yaw_degrees);
void set_injector_target_hitpoint(Injector &injector, const QVector3D &target);

// Rigid edits. map_point moves positions, map_direction moves direction
// vectors. The target point moves only when move_target_hitpoint is set.
void map_injector_spatial(
    Injector &injector,
    const std::function<QVector3D(const QVector3D &)> &map_point,
    const std::function<QVector3D(const QVector3D &)> &map_direction,
    bool move_target_hitpoint);

// Keep each direction vector's length and point it along direction.
void align_injector_directions(Injector &injector, const QVector3D &direction);
