#pragma once

#include "injector.h"

#include <QVector3D>

// Shared pose and direction writes. Callers emit UnitDocument signals after
// these return. Unit stays copyable and does not emit signals itself.

void translate_injector_points(Injector &injector, const QVector3D &delta);
void set_injector_direction(Injector &injector, const QVector3D &direction);
void set_injector_pitch_yaw(Injector &injector, double pitch_degrees,
                            double yaw_degrees);
void set_injector_target_hitpoint(Injector &injector, const QVector3D &target);
