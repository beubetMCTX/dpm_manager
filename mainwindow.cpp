#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "chemkin_io.h"
#include "dpm_file_io.h"
#include <QFileInfo>
#include <QSizePolicy>
#include <QtMath>
#include <QDebug>

namespace
{
// Match injector.cpp: advanced atomizer previews are intentionally hidden for now.
constexpr bool kEnableAdvancedAtomizerPreview = false;

void configure_common_injector(Unit &unit, const QString &name, const QVector3D &pos)
{
    const QVector3D axial_dir(1.0f, 0.0f, 0.0f);

    unit.inj.injector_data.name = name;
    unit.inj.injector_data.material = "debug-liquid";
    unit.inj.injector_data.pos = pos;
    unit.inj.injector_data.pos2 = pos + QVector3D(8.0f, 0.0f, 0.0f);
    unit.inj.injector_data.vel = 100.0f * axial_dir;
    unit.inj.injector_data.vel2 = 95.0f * axial_dir;
    unit.inj.injector_data.axis = axial_dir;
    unit.inj.injector_data.atomizer_axis = axial_dir;
    unit.inj.injector_data.total_flow_rate = 0.3;
    unit.inj.injector_data.flow_rate = 0.15;
    unit.inj.injector_data.flow_rate2 = 0.15;
    unit.inj.injector_data.numpts = 6;
    unit.inj.injector_data.radius = 2.6;
    unit.inj.injector_data.inner_radius = 1.0;
    unit.inj.injector_data.cone_angle = 36.0;
    unit.inj.injector_data.atomizer_disp_angle = 24.0;
    unit.inj.injector_data.half_angle = qDegreesToRadians(18.0);
    unit.inj.injector_data.diameter = 1.2;
    unit.inj.injector_data.diameter2 = 1.0;
    unit.inj.injector_data.inner_diameter = 1.4;
    unit.inj.injector_data.outer_diameter = 3.4;
    unit.inj.injector_data.plain_length = 4.5;
    unit.inj.injector_data.sheet_const = 10.0;
    unit.inj.injector_data.lig_const = 0.6;
    unit.inj.injector_data.airbl_rel_vel = 90.0;
    unit.inj.injector_data.effer_const = 0.35;
    unit.inj.injector_data.effer_quality = 0.12;
    unit.inj.injector_data.effer_half_angle_max = qDegreesToRadians(22.0);
    unit.inj.injector_data.ff_oriface_width = 2.8;
    unit.inj.injector_data.ff_sheet_const = 3.0;
    unit.inj.injector_data.phi_start = -qDegreesToRadians(32.0);
    unit.inj.injector_data.phi_stop = qDegreesToRadians(32.0);
    unit.inj.injector_data.ff_center = pos;
    unit.inj.injector_data.ff_virtual_origin = pos + 6.0f * axial_dir;
    unit.inj.injector_data.ff_normal = QVector3D(0.0f, 1.0f, 0.0f);
    unit.inj.injector_data.volume_specification = bouning_geometry;
    unit.inj.injector_data.volume_bgeom_shapes = hexahedron;
    unit.inj.injector_data.volume_bgeom_min = pos + QVector3D(-2.5f, -2.0f, -2.0f);
    unit.inj.injector_data.volume_bgeom_max = pos + QVector3D(2.5f, 2.0f, 3.5f);
    unit.inj.injector_data.volume_bgeom_radius = 2.5;
    unit.inj.injector_data.volume_bgeom_viconeangle = qDegreesToRadians(12.0);
}

Unit make_test_unit(const QString &name,
                    Injection_Type injection_type,
                    const QVector3D &pos)
{
    Unit unit;
    configure_common_injector(unit, name, pos);
    unit.inj.injector_data.injection_type = injection_type;
    return unit;
}
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //tab_widget = new QTabWidget();

    //this->setCentralWidget(m_3d_widget);
    m_3d_widget = new OCCTWidget(this);
    this->setCentralWidget(m_3d_widget);

    m_chemkin_toolbar = new QToolBar("Chemkin Status", this);
    m_chemkin_toolbar->setMovable(false);
    m_chemkin_toolbar->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, m_chemkin_toolbar);

    m_chemkin_status_label = new QLabel(m_chemkin_toolbar);
    m_chemkin_status_label->setMinimumWidth(170);
    m_chemkin_toolbar->addWidget(m_chemkin_status_label);

    m_chemkin_path_edit = new QLineEdit(m_chemkin_toolbar);
    m_chemkin_path_edit->setReadOnly(true);
    m_chemkin_path_edit->setClearButtonEnabled(false);
    m_chemkin_path_edit->setMinimumWidth(360);
    m_chemkin_path_edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_chemkin_toolbar->addWidget(m_chemkin_path_edit);

    update_chemkin_status();

#if defined(QT_DEBUG)
    units = build_test_injector_units();
    m_3d_widget->display_units(units);
    statusBar()->showMessage(
        QString("Loaded %1 debug injector units").arg(units.size()), 5000);
#endif

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionRead_triggered()
{
    QList<Unit> temp;
    bool *ok=new bool();
    *ok= false;
    temp=read_single_dpm_file_regex(ok);
    if(*ok)
    {
        units.clear();
        units=temp;
        m_3d_widget->display_units(units);

        qDebug() << "Loaded injector count:" << units.size();
        for (int i = 0; i < units.size(); ++i)
        {
            qDebug() << "Injector" << i << ":" << units[i].inj.injector_data.name;
        }

        statusBar()->showMessage(
            QString("Loaded %1 injectors from DPM file").arg(units.size()), 5000);
    }
}



void MainWindow::on_actionRead_Base_Geometry_triggered()
{
    bool ok=false;
    ok=m_3d_widget->geometry.Read_Geometry_Dialog();
    if(ok) qDebug()<<"true";
    m_3d_widget->add_readed_geometry();
}

void MainWindow::on_actionRead_Chemkin_Files_triggered()
{
    const QString file_path = Read_Chemkin_File_Dialog();
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("Chemkin species import canceled", 5000);
        return;
    }

    bool ok = false;
    const QStringList species_names = read_chemkin_species_names(file_path, &ok);
    if (!ok)
    {
        statusBar()->showMessage("Chemkin species import failed", 5000);
        return;
    }

    m_chemkin_species_names = species_names;
    m_chemkin_file_path = file_path;
    m_3d_widget->set_chemkin_species_names(m_chemkin_species_names);
    update_chemkin_status();

    QString preview = m_chemkin_species_names.mid(0, 12).join(", ");
    if (m_chemkin_species_names.size() > 12)
    {
        preview += ", ...";
    }

    QMessageBox::information(
        this,
        "Chemkin Species Imported",
        QString("Imported %1 species.\n\n%2")
            .arg(m_chemkin_species_names.size())
            .arg(preview));

    statusBar()->showMessage(
        QString("Imported %1 species from Chemkin file").arg(m_chemkin_species_names.size()),
        5000);
}

void MainWindow::update_chemkin_status()
{
    if (m_chemkin_status_label == nullptr)
    {
        return;
    }

    if (m_chemkin_file_path.trimmed().isEmpty())
    {
        m_chemkin_status_label->setText("Chemkin: Not Loaded");
        m_chemkin_status_label->setToolTip("No Chemkin species file loaded.");
        if (m_chemkin_path_edit != nullptr)
        {
            m_chemkin_path_edit->setText("No Chemkin file loaded.");
            m_chemkin_path_edit->setToolTip("No Chemkin species file loaded.");
            m_chemkin_path_edit->setCursorPosition(0);
        }
        return;
    }

    const QFileInfo file_info(m_chemkin_file_path);
    m_chemkin_status_label->setText(
        QString("Chemkin: %1 (%2 species)")
            .arg(file_info.fileName())
            .arg(m_chemkin_species_names.size()));
    m_chemkin_status_label->setToolTip(m_chemkin_file_path);
    if (m_chemkin_path_edit != nullptr)
    {
        m_chemkin_path_edit->setText(m_chemkin_file_path);
        m_chemkin_path_edit->setToolTip(m_chemkin_file_path);
        m_chemkin_path_edit->setCursorPosition(0);
    }
}

QList<Unit> MainWindow::build_test_injector_units() const
{
    QList<Unit> result;

    auto finalize_unit = [&](Unit &unit)
    {
        if (!unit.inj.create_injector())
        {
            qDebug() << "Failed to build test injector:" << unit.inj.injector_data.name;
        }
        result.append(unit);
    };

    {
        Unit unit = make_test_unit("single_demo", single, QVector3D(0.0f, 0.0f, 0.0f));
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("group_demo", group, QVector3D(20.0f, 0.0f, 0.0f));
        unit.inj.injector_data.pos2 = unit.inj.injector_data.pos + QVector3D(10.0f, 0.0f, 0.0f);
        unit.inj.injector_data.vel = QVector3D(90.0f, 0.0f, 0.0f);
        unit.inj.injector_data.vel2 = QVector3D(80.0f, 0.0f, 0.0f);
        unit.inj.injector_data.numpts = 7;
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("volume_box_demo", volume, QVector3D(40.0f, 0.0f, 0.0f));
        unit.inj.injector_data.volume_specification = bouning_geometry;
        unit.inj.injector_data.volume_bgeom_shapes = hexahedron;
        unit.inj.injector_data.volume_bgeom_min = unit.inj.injector_data.pos + QVector3D(-3.0f, -2.0f, -2.0f);
        unit.inj.injector_data.volume_bgeom_max = unit.inj.injector_data.pos + QVector3D(3.0f, 2.0f, 4.0f);
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("volume_cyl_demo", volume, QVector3D(60.0f, 0.0f, 0.0f));
        unit.inj.injector_data.volume_specification = bouning_geometry;
        unit.inj.injector_data.volume_bgeom_shapes = cylinder;
        unit.inj.injector_data.volume_bgeom_min = unit.inj.injector_data.pos + QVector3D(-4.0f, 0.0f, 0.0f);
        unit.inj.injector_data.volume_bgeom_max = unit.inj.injector_data.pos + QVector3D(4.0f, 0.0f, 0.0f);
        unit.inj.injector_data.volume_bgeom_radius = 2.2;
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("volume_sphere_demo", volume, QVector3D(80.0f, 0.0f, 0.0f));
        unit.inj.injector_data.volume_specification = bouning_geometry;
        unit.inj.injector_data.volume_bgeom_shapes = sphere;
        unit.inj.injector_data.volume_bgeom_radius = 2.8;
        unit.inj.injector_data.volume_bgeom_min = unit.inj.injector_data.pos + QVector3D(-2.8f, -2.8f, -2.8f);
        unit.inj.injector_data.volume_bgeom_max = unit.inj.injector_data.pos + QVector3D(2.8f, 2.8f, 2.8f);
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("volume_cone_demo", volume, QVector3D(100.0f, 0.0f, 0.0f));
        unit.inj.injector_data.volume_specification = bouning_geometry;
        unit.inj.injector_data.volume_bgeom_shapes = cone_;
        unit.inj.injector_data.volume_bgeom_min = unit.inj.injector_data.pos + QVector3D(-4.5f, 0.0f, 0.0f);
        unit.inj.injector_data.volume_bgeom_max = unit.inj.injector_data.pos + QVector3D(4.5f, 0.0f, 0.0f);
        unit.inj.injector_data.volume_bgeom_radius = 2.5;
        unit.inj.injector_data.volume_bgeom_viconeangle = qDegreesToRadians(18.0);
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("cone_point_demo", cone, QVector3D(0.0f, 22.0f, 0.0f));
        unit.inj.injector_data.cone_type = point;
        unit.inj.injector_data.cone_angle = 26.0;
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("cone_hollow_demo", cone, QVector3D(20.0f, 22.0f, 0.0f));
        unit.inj.injector_data.cone_type = hollow;
        unit.inj.injector_data.cone_angle = 34.0;
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("cone_ring_demo", cone, QVector3D(40.0f, 22.0f, 0.0f));
        unit.inj.injector_data.cone_type = ring;
        unit.inj.injector_data.radius = 2.8;
        unit.inj.injector_data.inner_radius = 1.4;
        unit.inj.injector_data.cone_angle = 30.0;
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("cone_solid_demo", cone, QVector3D(60.0f, 22.0f, 0.0f));
        unit.inj.injector_data.cone_type = solid;
        unit.inj.injector_data.radius = 2.5;
        unit.inj.injector_data.cone_angle = 22.0;
        finalize_unit(unit);
    }

    if (kEnableAdvancedAtomizerPreview)
    {
        Unit unit = make_test_unit("plain_orifice_demo", plain_oriface_atomizer, QVector3D(0.0f, 44.0f, 0.0f));
        unit.inj.injector_data.plain_length = 5.5;
        unit.inj.injector_data.diameter = 1.1;
        unit.inj.injector_data.outer_diameter = 3.0;
        unit.inj.injector_data.half_angle = qDegreesToRadians(10.0);
        finalize_unit(unit);
    }

    if (kEnableAdvancedAtomizerPreview)
    {
        Unit unit = make_test_unit("pressure_swirl_demo", pressure_swirl_atomizer, QVector3D(20.0f, 44.0f, 0.0f));
        unit.inj.injector_data.inner_diameter = 1.2;
        unit.inj.injector_data.outer_diameter = 3.6;
        unit.inj.injector_data.sheet_const = 12.0;
        unit.inj.injector_data.lig_const = 0.5;
        unit.inj.injector_data.half_angle = qDegreesToRadians(18.0);
        finalize_unit(unit);
    }

    if (kEnableAdvancedAtomizerPreview)
    {
        Unit unit = make_test_unit("air_blast_demo", air_blast_atomizer, QVector3D(40.0f, 44.0f, 0.0f));
        unit.inj.injector_data.inner_diameter = 1.6;
        unit.inj.injector_data.outer_diameter = 4.0;
        unit.inj.injector_data.airbl_rel_vel = 120.0;
        unit.inj.injector_data.half_angle = qDegreesToRadians(14.0);
        finalize_unit(unit);
    }

    if (kEnableAdvancedAtomizerPreview)
    {
        Unit unit = make_test_unit("flat_fan_demo", flat_fan_atomizer, QVector3D(60.0f, 44.0f, 0.0f));
        unit.inj.injector_data.ff_oriface_width = 3.6;
        unit.inj.injector_data.ff_sheet_const = 3.5;
        unit.inj.injector_data.ff_normal = QVector3D(0.0f, 0.0f, 1.0f);
        unit.inj.injector_data.phi_start = -qDegreesToRadians(28.0);
        unit.inj.injector_data.phi_stop = qDegreesToRadians(28.0);
        finalize_unit(unit);
    }

    if (kEnableAdvancedAtomizerPreview)
    {
        Unit unit = make_test_unit("effervescent_demo", effervescent_atomizer, QVector3D(20.0f, 66.0f, 0.0f));
        unit.inj.injector_data.outer_diameter = 3.8;
        unit.inj.injector_data.effer_const = 0.45;
        unit.inj.injector_data.effer_quality = 0.18;
        unit.inj.injector_data.effer_half_angle_max = qDegreesToRadians(24.0);
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("condensate_demo", condensate, QVector3D(40.0f, 66.0f, 0.0f));
        unit.inj.injector_data.radius = 2.0;
        unit.inj.injector_data.vel = QVector3D(60.0f, 0.0f, 0.0f);
        finalize_unit(unit);
    }

    return result;
}





// void MainWindow::on_actionNew_triggered()
// {

// }

