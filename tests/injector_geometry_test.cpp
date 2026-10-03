#include "injector.h"

#include <QCoreApplication>
#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

Injector_OCCT make_base_injector()
{
    Injector_OCCT injector;
    injector.injector_data.pos = QVector3D(1.0f, 2.0f, 3.0f);
    injector.injector_data.pos2 = QVector3D(4.0f, 2.0f, 3.0f);
    injector.injector_data.vel = QVector3D(10.0f, 0.0f, 0.0f);
    injector.injector_data.vel2 = QVector3D(9.0f, 0.0f, 0.0f);
    injector.injector_data.axis = QVector3D(1.0f, 0.0f, 0.0f);
    injector.injector_data.atomizer_axis = QVector3D(1.0f, 0.0f, 0.0f);
    injector.injector_data.total_flow_rate = 1.0;
    injector.injector_data.radius = 2.0;
    injector.injector_data.inner_radius = 1.0;
    injector.injector_data.cone_angle = 30.0;
    injector.injector_data.numpts = 5;
    injector.injector_data.volume_bgeom_min = QVector3D(-1.0f, -1.0f, -1.0f);
    injector.injector_data.volume_bgeom_max = QVector3D(3.0f, 3.0f, 3.0f);
    injector.injector_data.volume_bgeom_radius = 1.5;
    return injector;
}

bool geometry_is_valid(Injector_OCCT &injector)
{
    if (!injector.create_injector() || injector.shape.IsNull())
    {
        return false;
    }

    return injector.shape.NbChildren() > 0;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = single;
        if (!check(geometry_is_valid(injector), "Single geometry should be created"))
        {
            return 1;
        }
    }

    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = group;
        if (!check(geometry_is_valid(injector), "Group geometry should be created"))
        {
            return 1;
        }
    }

    for (const Cone_Type cone_type : {point, hollow, ring, solid})
    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = cone;
        injector.injector_data.cone_type = cone_type;
        if (!check(geometry_is_valid(injector), "Cone geometry should be created"))
        {
            return 1;
        }
    }

    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = volume;
        if (!check(geometry_is_valid(injector), "Volume geometry should be created"))
        {
            return 1;
        }
    }

    for (const Volume_Bgeom_Shapes volume_shape :
         {sphere, cylinder, cone_, hexahedron})
    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = volume;
        injector.injector_data.volume_specification = bouning_geometry;
        injector.injector_data.volume_bgeom_shapes = volume_shape;
        if (!geometry_is_valid(injector))
        {
            std::cerr << "Volume bounding shape failed: "
                      << static_cast<int>(volume_shape)
                      << '\n';
            return 1;
        }
    }

    for (const Injection_Type atomizer_type :
         {plain_oriface_atomizer,
          pressure_swirl_atomizer,
          air_blast_atomizer,
          flat_fan_atomizer,
          effervescent_atomizer})
    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = atomizer_type;
        injector.injector_data.ff_center = injector.injector_data.pos;
        injector.injector_data.ff_virtual_origin =
            injector.injector_data.pos + QVector3D(1.0f, 0.0f, 0.0f);
        injector.injector_data.ff_normal = QVector3D(0.0f, 1.0f, 0.0f);
        if (!check(geometry_is_valid(injector),
                   "Every advanced atomizer geometry should be created"))
        {
            return 1;
        }
    }

    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = condensate;
        if (!check(geometry_is_valid(injector),
                   "Condensate geometry should be created"))
        {
            return 1;
        }
    }

    {
        Injector_OCCT injector = make_base_injector();
        injector.injector_data.injection_type = single;
        if (!check(geometry_is_valid(injector), "Initial geometry should be created"))
        {
            return 1;
        }
        const TopoDS_Compound previous_shape = injector.shape;
        injector.injector_data.vel = QVector3D();
        if (!check(!injector.create_injector(),
                   "Zero velocity should reject geometry creation") ||
            !check(!injector.shape.IsNull() &&
                       injector.shape.IsSame(previous_shape),
                   "Failed geometry creation should preserve the previous shape"))
        {
            return 1;
        }
    }

    return 0;
}
