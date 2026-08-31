#include "unit_edit_dialog.h"

#include <QApplication>
#include <QMetaObject>
#include <QDebug>
#include <QLabel>
#include <QList>
#include <QStringList>
#include <QTabWidget>
#include <QVector3D>

namespace
{
void initialize_test_unit(Unit &unit, Injection_Type type)
{
    Injector &injector = unit.inj.injector_data;
    injector.name = QStringLiteral("editor-smoke");
    injector.injection_type = type;
    injector.type = Droplet;
    injector.pos = QVector3D(1.0f, 2.0f, 3.0f);
    injector.pos2 = QVector3D(4.0f, 5.0f, 6.0f);
    injector.vel = QVector3D(10.0f, 0.0f, 0.0f);
    injector.vel2 = QVector3D(8.0f, 0.0f, 0.0f);
    injector.axis = QVector3D(1.0f, 0.0f, 0.0f);
    injector.atomizer_axis = QVector3D(1.0f, 0.0f, 0.0f);
    injector.ff_center = injector.pos;
    injector.ff_virtual_origin = injector.pos + QVector3D(1.0f, 0.0f, 0.0f);
    injector.ff_normal = QVector3D(0.0f, 1.0f, 0.0f);
    injector.diameter = 1.0;
    injector.diameter2 = 1.2;
    injector.inner_diameter = 0.8;
    injector.outer_diameter = 1.6;
    injector.radius = 2.0;
    injector.inner_radius = 1.5;
    injector.plain_length = 2.0;
    injector.ff_oriface_width = 1.0;
    injector.total_flow_rate = 0.1;
    injector.flow_rate = 0.05;
    injector.flow_rate2 = 0.06;
    injector.temperature = 300.0;
    injector.temperature2 = 310.0;
    injector.cone_angle = 30.0;
    injector.vel_mag = 10.0;
    injector.volume_bgeom_min = QVector3D(-1.0f, -1.0f, -1.0f);
    injector.volume_bgeom_max = QVector3D(1.0f, 1.0f, 1.0f);
    injector.volume_bgeom_radius = 1.0;
    injector.volume_bgeom_viconeangle = 0.2;
    injector.rr_disturb = true;
    injector.rr_uniform_ln_d = true;
    injector.rr_min = 1.0e-5;
    injector.rr_max = 2.0e-5;
    injector.rr_mean = 1.5e-5;
    injector.rr_spread = 1.5;
    injector.rr_numdia = 10;
}
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    const Injection_Type types[] = {
        single,
        group,
        surface,
        volume,
        cone,
        plain_oriface_atomizer,
        pressure_swirl_atomizer,
        air_blast_atomizer,
        flat_fan_atomizer,
        effervescent_atomizer,
        file_,
        condensate
    };

    for (Injection_Type type : types)
    {
        Unit unit;
        initialize_test_unit(unit, type);

        unit.inj.create_injector();
        unit_edit_dialog dialog(&unit,
                                {QStringLiteral("fuel"), QStringLiteral("oxidizer")},
                                {QStringLiteral("water")});
        dialog.refresh_from_unit_data(&unit);
    }

    Unit volume_unit;
    initialize_test_unit(volume_unit, volume);
    volume_unit.inj.injector_data.volume_specification = zone;
    volume_unit.inj.injector_data.volume_streams_spec = total_parcel_count;
    unit_edit_dialog volume_dialog(&volume_unit, {}, {});

    const QList<QUI_ComboBox *> volume_combos = volume_dialog.findChildren<QUI_ComboBox *>();
    QUI_ComboBox *specification_combo = nullptr;
    QUI_ComboBox *streams_combo = nullptr;
    for (QUI_ComboBox *combo : volume_combos)
    {
        if (combo->findText(QStringLiteral("Bounding Geometry")) >= 0)
        {
            specification_combo = combo;
        }
        if (combo->findText(QStringLiteral("Parcel Per Cell")) >= 0)
        {
            streams_combo = combo;
        }
    }

    if (specification_combo == nullptr || streams_combo == nullptr)
    {
        qCritical() << "Volume editor combos were not created.";
        return 1;
    }

    specification_combo->setCurrentIndex(1);
    QMetaObject::invokeMethod(specification_combo, "selection_committed", Qt::DirectConnection);
    streams_combo->setCurrentIndex(1);
    QMetaObject::invokeMethod(streams_combo, "selection_committed", Qt::DirectConnection);
    application.processEvents();

    if (volume_unit.inj.injector_data.volume_specification != bouning_geometry ||
        volume_unit.inj.injector_data.volume_streams_spec != parcel_per_cell)
    {
        qCritical() << "Volume editor selection was not synchronized.";
        return 1;
    }

    auto has_field_row = [](unit_edit_dialog &dialog, const QString &text)
    {
        for (QWidget *widget : dialog.findChildren<QWidget *>())
        {
            auto *row = dynamic_cast<QUI_FieldRow *>(widget);
            if (row != nullptr && row->label_text() == text)
            {
                return true;
            }
        }
        return false;
    };

    if (!has_field_row(volume_dialog, QStringLiteral("X-Center")) ||
        has_field_row(volume_dialog, QStringLiteral("Cone Angle")))
    {
        qCritical() << "Volume geometry fields were not filtered for the initial sphere branch.";
        return 1;
    }

    auto find_volume_shape_combo = [&volume_dialog]()
    {
        for (QUI_ComboBox *combo : volume_dialog.findChildren<QUI_ComboBox *>())
        {
            if (combo->findText(QStringLiteral("Hexahedron")) >= 0)
            {
                return combo;
            }
        }
        return static_cast<QUI_ComboBox *>(nullptr);
    };

    QUI_ComboBox *shape_combo = find_volume_shape_combo();
    if (shape_combo == nullptr)
    {
        qCritical() << "Volume bounding-shape combo was not created.";
        return 1;
    }
    shape_combo->setCurrentIndex(shape_combo->findText(QStringLiteral("Hexahedron")));
    QMetaObject::invokeMethod(shape_combo, "selection_committed", Qt::DirectConnection);
    application.processEvents();
    if (!has_field_row(volume_dialog, QStringLiteral("X-Min")) ||
        has_field_row(volume_dialog, QStringLiteral("Radius")) ||
        has_field_row(volume_dialog, QStringLiteral("Cone Angle")))
    {
        qCritical() << "Volume hexahedron fields were not filtered correctly.";
        return 1;
    }

    shape_combo = find_volume_shape_combo();
    if (shape_combo == nullptr)
    {
        qCritical() << "Volume bounding-shape combo disappeared after rebuild.";
        return 1;
    }
    shape_combo->setCurrentIndex(shape_combo->findText(QStringLiteral("Cone")));
    QMetaObject::invokeMethod(shape_combo, "selection_committed", Qt::DirectConnection);
    application.processEvents();
    if (!has_field_row(volume_dialog, QStringLiteral("Radius")) ||
        !has_field_row(volume_dialog, QStringLiteral("Cone Angle")))
    {
        qCritical() << "Volume cone-specific fields were not rebuilt.";
        return 1;
    }

    Unit zone_unit;
    initialize_test_unit(zone_unit, volume);
    zone_unit.inj.injector_data.volume_zones = {1, 2};
    unit_edit_dialog zone_dialog(&zone_unit, {}, {});
    QUI_FieldRow *zones_row = nullptr;
    for (QWidget *widget : zone_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row != nullptr && row->label_text() == QStringLiteral("Volume Zones"))
        {
            zones_row = row;
            break;
        }
    }

    if (zones_row == nullptr)
    {
        qCritical() << "Volume zone field was not created.";
        return 1;
    }
    zones_row->primary_editor()->setText(QStringLiteral("1, 3, 5"));
    if (!zones_row->primary_editor()->commit() ||
        zone_unit.inj.injector_data.volume_zones != QVector<int>({1, 3, 5}))
    {
        qCritical() << "Volume zone field was not synchronized.";
        return 1;
    }

    Unit surface_unit;
    initialize_test_unit(surface_unit, surface);
    surface_unit.inj.injector_data.surfaces = {10, 11};
    surface_unit.inj.injector_data.boundary = {20};
    unit_edit_dialog surface_dialog(&surface_unit, {}, {});
    QUI_FieldRow *surface_ids_row = nullptr;
    QUI_FieldRow *boundary_ids_row = nullptr;
    for (QWidget *widget : surface_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row == nullptr)
        {
            continue;
        }
        if (row->label_text() == QStringLiteral("Surface Zone IDs"))
        {
            surface_ids_row = row;
        }
        else if (row->label_text() == QStringLiteral("Boundary IDs"))
        {
            boundary_ids_row = row;
        }
    }

    if (surface_ids_row == nullptr || boundary_ids_row == nullptr)
    {
        qCritical() << "Surface zone fields were not created.";
        return 1;
    }
    surface_ids_row->primary_editor()->setText(QStringLiteral("3; 4 5"));
    boundary_ids_row->primary_editor()->setText(QStringLiteral("7, 8"));
    if (!surface_ids_row->primary_editor()->commit() ||
        !boundary_ids_row->primary_editor()->commit() ||
        surface_unit.inj.injector_data.surfaces != QVector<int>({3, 4, 5}) ||
        surface_unit.inj.injector_data.boundary != QVector<int>({7, 8}))
    {
        qCritical() << "Surface zone fields were not synchronized.";
        return 1;
    }

    Unit file_unit;
    initialize_test_unit(file_unit, file_);
    file_unit.inj.injector_data.dpm_fname = QStringLiteral("old.dpm");
    unit_edit_dialog file_dialog(&file_unit, {}, {});
    QUI_FieldRow *dpm_file_row = nullptr;
    for (QWidget *widget : file_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row != nullptr && row->label_text() == QStringLiteral("DPM File"))
        {
            dpm_file_row = row;
            break;
        }
    }
    if (dpm_file_row == nullptr)
    {
        qCritical() << "File injection source field was not created.";
        return 1;
    }
    dpm_file_row->primary_editor()->setText(QStringLiteral("new.dpm"));
    if (!dpm_file_row->primary_editor()->commit() ||
        file_unit.inj.injector_data.dpm_fname != QStringLiteral("new.dpm"))
    {
        qCritical() << "File injection source field was not synchronized.";
        return 1;
    }

    auto has_editor_label = [](unit_edit_dialog &dialog, const QString &text)
    {
        for (QLabel *label : dialog.findChildren<QLabel *>())
        {
            if (label->text() == text || label->text().startsWith(text + QStringLiteral(" ")))
            {
                return true;
            }
        }
        return false;
    };

    Unit conditional_unit;
    initialize_test_unit(conditional_unit, single);
    conditional_unit.inj.injector_data.type = Droplet;
    conditional_unit.inj.injector_data.stochastic = false;
    conditional_unit.inj.injector_data.cloud = false;
    conditional_unit.inj.injector_data.rotation_on = false;
    conditional_unit.inj.injector_data.drag_law = spherical;
    conditional_unit.inj.injector_data.parcel_model = standard;
    conditional_unit.inj.injector_data.evaporating_liquid = false;
    unit_edit_dialog conditional_dialog(&conditional_unit, {}, {});
    if (has_editor_label(conditional_dialog, QStringLiteral("Random Eddy")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Cloud Minimum Diameter")) ||
        has_editor_label(conditional_dialog, QStringLiteral("X-Angular Velocity")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Shape Factor")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Evaporating Material")))
    {
        qCritical() << "Conditional model fields were visible before their prerequisites were enabled.";
        return 1;
    }

    conditional_unit.inj.injector_data.stochastic = true;
    conditional_unit.inj.injector_data.cloud = true;
    conditional_unit.inj.injector_data.rotation_on = true;
    conditional_unit.inj.injector_data.drag_law = nonspherical;
    conditional_unit.inj.injector_data.evaporating_liquid = true;
    conditional_dialog.refresh_from_unit_data(&conditional_unit);
    if (!has_editor_label(conditional_dialog, QStringLiteral("Random Eddy")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("Cloud Minimum Diameter")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("X-Angular Velocity")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("Shape Factor")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("Evaporating Material")))
    {
        QStringList labels;
        for (QLabel *label : conditional_dialog.findChildren<QLabel *>())
        {
            labels.push_back(label->text());
        }
        qCritical() << "Conditional model fields were not rebuilt after prerequisites changed."
                    << labels;
        return 1;
    }

    conditional_unit.inj.injector_data.type = Massless;
    conditional_dialog.refresh_from_unit_data(&conditional_unit);
    if (conditional_unit.inj.injector_data.rr_disturb ||
        has_editor_label(conditional_dialog, QStringLiteral("RR Min Diameter")))
    {
        qCritical() << "Non-droplet particle type retained diameter-distribution controls.";
        return 1;
    }
    for (QTabWidget *tabs : conditional_dialog.findChildren<QTabWidget *>())
    {
        QWidget *wet_page = conditional_dialog.findChild<QWidget *>(
            QStringLiteral("tab_wet_combustion"));
        const int wet_index = wet_page != nullptr ? tabs->indexOf(wet_page) : -1;
        if (wet_index >= 0 && tabs->isTabVisible(wet_index))
        {
            qCritical() << "Wet Combustion tab remained visible for a massless particle.";
            return 1;
        }
    }

    if (has_editor_label(conditional_dialog, QStringLiteral("Drag Law")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Rotation")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Brownian Motion")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Vapor Pressure")) ||
        has_editor_label(conditional_dialog, QStringLiteral("SECO Breakup")))
    {
        qCritical() << "Massless particle retained inertial or liquid-only physical models.";
        return 1;
    }

    conditional_unit.inj.injector_data.type = Droplet;
    conditional_unit.inj.injector_data.injection_type = single;
    conditional_dialog.refresh_from_unit_data(&conditional_unit);
    if (!has_editor_label(conditional_dialog, QStringLiteral("Drag Law")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("Vapor Pressure")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("SECO Breakup")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Atomizer Staggering")) ||
        !has_editor_label(conditional_dialog, QStringLiteral("Standard Injection Staggering")))
    {
        qCritical() << "Droplet physical-model or standard-stagger fields were not filtered correctly.";
        return 1;
    }

    conditional_unit.inj.injector_data.injection_type = pressure_swirl_atomizer;
    conditional_dialog.refresh_from_unit_data(&conditional_unit);
    if (!has_editor_label(conditional_dialog, QStringLiteral("Atomizer Staggering")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Standard Injection Staggering")))
    {
        qCritical() << "Atomizer staggering fields were not filtered correctly.";
        return 1;
    }

    conditional_unit.inj.injector_data.parcel_model = const_mass;
    conditional_dialog.refresh_from_unit_data(&conditional_unit);
    if (!has_editor_label(conditional_dialog, QStringLiteral("Parcel Mass")) ||
        has_editor_label(conditional_dialog, QStringLiteral("Parcel Number")))
    {
        qCritical() << "Parcel model-specific fields were not synchronized.";
        return 1;
    }

    Unit injection_switch_unit;
    initialize_test_unit(injection_switch_unit, single);
    unit_edit_dialog injection_switch_dialog(&injection_switch_unit, {}, {});
    QUI_ComboBox *injection_type_combo = nullptr;
    for (QUI_ComboBox *combo : injection_switch_dialog.findChildren<QUI_ComboBox *>())
    {
        if (combo->findText(QStringLiteral("Surface")) >= 0)
        {
            injection_type_combo = combo;
            break;
        }
    }
    if (injection_type_combo == nullptr)
    {
        qCritical() << "Injection type combo was not created.";
        return 1;
    }
    injection_type_combo->setCurrentIndex(injection_type_combo->findText(QStringLiteral("Surface")));
    QMetaObject::invokeMethod(injection_type_combo, "selection_committed", Qt::DirectConnection);
    application.processEvents();
    if (!has_editor_label(injection_switch_dialog, QStringLiteral("Scale By Area")))
    {
        qCritical() << "Injection-type-specific model fields were not rebuilt.";
        return 1;
    }

    Unit model_unit;
    initialize_test_unit(model_unit, single);
    unit_edit_dialog model_dialog(&model_unit, {}, {});
    QUI_FieldRow *parcel_number_row = nullptr;
    for (QWidget *widget : model_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row == nullptr)
        {
            continue;
        }
        if (row->label_text() == QStringLiteral("Parcel Number"))
        {
            parcel_number_row = row;
            break;
        }
    }

    if (parcel_number_row == nullptr)
    {
        qCritical() << "Parcel model fields were not created.";
        return 1;
    }

    parcel_number_row->primary_editor()->setText(QStringLiteral("5+6"));
    if (!parcel_number_row->primary_editor()->commit() ||
        model_unit.inj.injector_data.parcel_number != 11)
    {
        qCritical() << "Parcel model field was not synchronized."
                    << model_unit.inj.injector_data.parcel_number;
        return 1;
    }

    Unit tabulated_unit;
    initialize_test_unit(tabulated_unit, cone);
    tabulated_unit.inj.injector_data.tabulated_diam_dist = true;
    tabulated_unit.inj.injector_data.rr_disturb = false;
    unit_edit_dialog tabulated_dialog(&tabulated_unit, {}, {});
    bool cone_has_fixed_distribution = false;
    bool cone_has_uniform_distribution = false;
    for (QUI_ComboBox *combo : tabulated_dialog.findChildren<QUI_ComboBox *>())
    {
        cone_has_fixed_distribution = cone_has_fixed_distribution ||
                                      combo->findText(QStringLiteral("fixed")) >= 0;
        cone_has_uniform_distribution = cone_has_uniform_distribution ||
                                       combo->findText(QStringLiteral("uniform")) >= 0;
    }
    if (!cone_has_fixed_distribution || cone_has_uniform_distribution)
    {
        qCritical() << "Cone diameter-distribution options were not filtered correctly.";
        return 1;
    }
    QUI_FieldRow *table_name_row = nullptr;
    for (QWidget *widget : tabulated_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row != nullptr && row->label_text() == QStringLiteral("Table Name"))
        {
            table_name_row = row;
            break;
        }
    }

    if (table_name_row == nullptr)
    {
        qCritical() << "Tabulated diameter fields were not created.";
        return 1;
    }

    tabulated_unit.inj.injector_data.tabulated_diam_table_name = QStringLiteral("diameters");
    tabulated_dialog.refresh_from_unit_data(&tabulated_unit);
    if (table_name_row->primary_editor()->text() != QStringLiteral("diameters"))
    {
        qCritical() << "Tabulated string field was not synchronized.";
        return 1;
    }

    Unit geometry_unit;
    initialize_test_unit(geometry_unit, cone);
    geometry_unit.inj.injector_data.cone_type = solid;
    geometry_unit.inj.create_injector();
    unit_edit_dialog geometry_dialog(&geometry_unit, {}, {});
    QUI_FieldRow *cone_angle_row = nullptr;
    for (QWidget *widget : geometry_dialog.findChildren<QWidget *>())
    {
        auto *row = dynamic_cast<QUI_FieldRow *>(widget);
        if (row != nullptr && row->label_text() == QStringLiteral("Cone Angle"))
        {
            cone_angle_row = row;
            break;
        }
    }

    if (cone_angle_row == nullptr)
    {
        qCritical() << "Cone geometry field was not created.";
        return 1;
    }
    cone_angle_row->primary_editor()->setText(QStringLiteral("45"));
    if (!cone_angle_row->primary_editor()->commit() ||
        geometry_unit.inj.injector_data.cone_angle != 45.0)
    {
        qCritical() << "Cone angle was not written to the model.";
        return 1;
    }
    if (!geometry_unit.inj.create_injector() || geometry_unit.inj.shape.IsNull())
    {
        qCritical() << "Geometry was not rebuilt after editing cone angle.";
        return 1;
    }
    geometry_unit.inj.injector_data.cone_angle = 60.0;
    geometry_dialog.refresh_from_unit_data(&geometry_unit);
    if (cone_angle_row->primary_editor()->text() != QStringLiteral("60"))
    {
        qCritical() << "External cone-angle update was not reflected in the dialog.";
        return 1;
    }

    qInfo() << "Unit editor smoke test passed";
    return 0;
}
