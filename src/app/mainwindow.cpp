#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "app_config.h"
#include "chemkin_io.h"
#include "dpm_file_io.h"
#include "runtime_debug.h"
#include "species_color_dialog.h"
#include "species_material_dialog.h"
#include <QFileInfo>
#include <QSizePolicy>
#include <QtMath>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
// Match injector.cpp: advanced atomizer previews are intentionally hidden for now.
constexpr bool kEnableAdvancedAtomizerPreview = false;

bool material_entries_equal(const QList<MaterialConfigEntry> &lhs,
                            const QList<MaterialConfigEntry> &rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (int i = 0; i < lhs.size(); ++i)
    {
        if (lhs[i].name != rhs[i].name || lhs[i].density != rhs[i].density)
        {
            return false;
        }
    }

    return true;
}

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
    runtime_debug::trace("MainWindow constructor begin");
    ui->setupUi(this);
    runtime_debug::trace("MainWindow ui->setupUi finished");

    QString config_error_message;
    if (!ensure_app_config_directories(&config_error_message) &&
        !config_error_message.trimmed().isEmpty())
    {
        qWarning() << config_error_message;
    }

    //tab_widget = new QTabWidget();

    //this->setCentralWidget(m_3d_widget);
    m_3d_widget = new OCCTWidget(this);
    this->setCentralWidget(m_3d_widget);
    runtime_debug::trace("MainWindow OCCTWidget created");

    create_reference_geometry_panel();
    create_object_list_panel();
    connect(ui->actionObjects, &QAction::triggered, this, [this]()
    {
        if (m_object_list_dock != nullptr)
        {
            m_object_list_dock->show();
            m_object_list_dock->raise();
        }
    });
    connect(ui->actionReference_Geometry, &QAction::triggered, this, [this]()
    {
        if (m_reference_geometry_dock != nullptr &&
            !m_3d_widget->geometry.getShape().IsNull())
        {
            m_reference_geometry_dock->show();
            m_reference_geometry_dock->raise();
        }
    });
    connect(ui->actionReset_Window_Layout, &QAction::triggered, this,
            &MainWindow::reset_window_layout);
    connect(m_3d_widget, &OCCTWidget::reference_geometry_available, this,
            [this](bool available)
    {
        if (m_reference_geometry_dock != nullptr)
        {
            m_reference_geometry_dock->setVisible(available);
        }
        if (ui->actionReference_Geometry != nullptr)
        {
            ui->actionReference_Geometry->setEnabled(available);
        }
        update_reference_geometry_panel();
        update_object_list_panel();
    });
    connect(m_3d_widget, &OCCTWidget::reference_transform_changed, this,
            [this](const QVector3D &, const QVector3D &)
    {
        update_reference_geometry_panel();
    });
    connect(m_3d_widget, &OCCTWidget::face_reference_changed, this,
            [this](bool available)
    {
        if (m_align_reference_face != nullptr)
        {
            m_align_reference_face->setEnabled(available);
        }
    });
    connect(m_3d_widget, &OCCTWidget::face_reference_info_changed, this,
            [this](const QVector3D &origin, const QVector3D &normal)
    {
        if (m_reference_face_origin == nullptr || m_reference_face_normal == nullptr)
        {
            return;
        }

        const auto format_vector = [](const QVector3D &value)
        {
            return QString("(%1, %2, %3)")
                .arg(value.x(), 0, 'f', 3)
                .arg(value.y(), 0, 'f', 3)
                .arg(value.z(), 0, 'f', 3);
        };
        m_reference_face_origin->setText(format_vector(origin));
        m_reference_face_normal->setText(format_vector(normal));
    });
    connect(m_3d_widget, &OCCTWidget::reference_geometry_lock_changed,
            this, [this](bool locked)
    {
        if (m_reference_geometry_lock != nullptr &&
            m_reference_geometry_lock->isChecked() != locked)
        {
            m_reference_geometry_lock->setChecked(locked);
        }
    });
    connect(m_3d_widget, &OCCTWidget::unit_display_list_changed,
            this, &MainWindow::update_object_list_panel);
    connect(m_3d_widget, &OCCTWidget::unit_removed, this,
            [this](const QUuid &uuid)
    {
        for (auto it = units.begin(); it != units.end(); ++it)
        {
            if (it->inj.uuid == uuid)
            {
                units.erase(it);
                break;
            }
        }
    });
    connect(m_3d_widget, &OCCTWidget::selection_changed,
            this, &MainWindow::update_object_list_selection);
    connect(ui->actionUndo_Move, &QAction::triggered, m_3d_widget,
            &OCCTWidget::undo_last_move);
    connect(ui->actionRedo_Move, &QAction::triggered, m_3d_widget,
            &OCCTWidget::redo_move);
    connect(m_3d_widget, &OCCTWidget::move_history_changed, this,
            [this](bool can_undo, bool can_redo)
    {
        ui->actionUndo_Move->setEnabled(can_undo);
        ui->actionRedo_Move->setEnabled(can_redo);
    });

    // OCCT owns editable Unit copies so that interactive handles remain stable.
    // Keep the MainWindow list synchronized whenever one of those copies changes.
    connect(m_3d_widget, &OCCTWidget::unit_data_updated, this, [this](Unit *changed_unit)
    {
        if (changed_unit == nullptr)
        {
            return;
        }

        for (Unit &stored_unit : units)
        {
            if (stored_unit.inj.uuid == changed_unit->inj.uuid)
            {
                // Do not assign a whole Unit here. Unit::operator= rebuilds
                // OpenCASCADE/OCAF runtime state, which is unsafe and
                // unnecessarily expensive while an editor update is active.
                stored_unit.type = changed_unit->type;
                stored_unit.inj.injector_data = changed_unit->inj.injector_data;
                stored_unit.inj.shape = changed_unit->inj.shape;
                update_object_list_item(stored_unit.inj.uuid,
                                        stored_unit.inj.injector_data.name);
                return;
            }
        }
    });

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
    runtime_debug::trace("MainWindow toolbar initialized");

    update_chemkin_status();
    runtime_debug::trace("MainWindow chemkin status initialized");

#if defined(QT_DEBUG)
    runtime_debug::trace("MainWindow building debug injector units");
    units = build_test_injector_units();
    runtime_debug::trace(QString("MainWindow built %1 debug injector units").arg(units.size()));
    m_3d_widget->display_units(units);
    runtime_debug::trace("MainWindow displayed debug injector units");
    statusBar()->showMessage(
        QString("Loaded %1 debug injector units").arg(units.size()), 5000);
#endif

    restore_material_table();
    runtime_debug::trace("MainWindow material table restored");
    restore_last_chemkin_file();
    runtime_debug::trace("MainWindow chemkin file restore finished");
    restore_window_layout();
    runtime_debug::trace("MainWindow constructor end");

}

MainWindow::~MainWindow()
{
    runtime_debug::trace("MainWindow destructor begin");
    // closeEvent is not guaranteed for every application-exit path. Close
    // auxiliary windows here as a final lifetime boundary before UI teardown.
    if (m_3d_widget != nullptr)
    {
        m_3d_widget->discard_auxiliary_dialogs();
    }
    if (m_species_color_dialog != nullptr)
    {
        m_species_color_dialog->close();
    }
    if (m_species_material_dialog != nullptr)
    {
        m_species_material_dialog->close();
    }
    // Keep layout persistence reliable even when the window is destroyed
    // through an application-exit path that bypasses closeEvent.
    save_window_layout();
    delete ui;
    runtime_debug::trace("MainWindow destructor end");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    runtime_debug::trace("MainWindow closeEvent begin");

    // Close OCCT-owned editors first so their selection-clearing callbacks run
    // while the interactive context and view are still valid.
    if (m_3d_widget != nullptr)
    {
        runtime_debug::trace("Closing OCCT auxiliary dialogs from MainWindow closeEvent");
        m_3d_widget->discard_auxiliary_dialogs();
    }

    if (m_species_color_dialog != nullptr)
    {
        runtime_debug::trace("Closing SpeciesColorDialog from MainWindow closeEvent");
        m_species_color_dialog->close();
        m_species_color_dialog = nullptr;
    }

    if (m_species_material_dialog != nullptr)
    {
        runtime_debug::trace("Closing SpeciesMaterialDialog from MainWindow closeEvent");
        m_species_material_dialog->close();
        m_species_material_dialog = nullptr;
    }

    save_window_layout();

    QMainWindow::closeEvent(event);
    runtime_debug::trace("MainWindow closeEvent end");
}


void MainWindow::on_actionRead_triggered()
{
    bool ok = false;
    QString error_message;
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "选择 DPM 文件",
        ".",
        "DPM Files (*.dpm *.txt);;All Files (*.*)");
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("DPM file import canceled", 5000);
        return;
    }

    const QList<Unit> temp = read_dpm_file(file_path, &ok, &error_message, true);
    if (ok)
    {
        units.clear();
        units = temp;
        m_3d_widget->display_units(units, true);

        qDebug() << "Loaded injector count:" << units.size();
        for (int i = 0; i < units.size(); ++i)
        {
            qDebug() << "Injector" << i << ":" << units[i].inj.injector_data.name;
        }

        statusBar()->showMessage(
            QString("Loaded %1 injectors from DPM file").arg(units.size()), 5000);
    }
    else
    {
        statusBar()->showMessage(
            error_message.trimmed().isEmpty() ? "DPM file import failed" : error_message,
            8000);
    }
}



void MainWindow::on_actionRead_Base_Geometry_triggered()
{
    const bool ok = m_3d_widget->geometry.Read_Geometry_Dialog();
    if (ok)
    {
        qDebug() << "true";
        m_3d_widget->add_readed_geometry();
        statusBar()->showMessage("Base geometry loaded successfully", 5000);
    }
    else
    {
        statusBar()->showMessage("Base geometry import failed or was canceled", 5000);
    }
}

void MainWindow::on_actionRead_Chemkin_Files_triggered()
{
    const QString file_path = Read_Chemkin_File_Dialog();
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("Chemkin species import canceled", 5000);
        return;
    }

    load_chemkin_file(file_path, true, true);
}

void MainWindow::on_actionSpecies_Colors_triggered()
{
    if (m_species_color_dialog == nullptr)
    {
        m_species_color_dialog = new SpeciesColorDialog(this);
        connect(m_species_color_dialog, &QObject::destroyed, this, [this]()
        {
            m_species_color_dialog = nullptr;
        });
        connect(m_species_color_dialog, &SpeciesColorDialog::warning_message_requested, this,
                [this](const QString &message)
        {
            statusBar()->showMessage(message, 8000);
        });
    }

    m_species_color_dialog->set_chemkin_context(m_chemkin_file_path, m_chemkin_species_names);
    m_species_color_dialog->show();
    m_species_color_dialog->raise();
    m_species_color_dialog->activateWindow();
}

void MainWindow::on_actionSpecies_Materials_triggered()
{
    if (m_species_material_dialog == nullptr)
    {
        runtime_debug::trace("Creating SpeciesMaterialDialog");
        m_species_material_dialog = new SpeciesMaterialDialog(this);
        m_species_material_dialog->set_material_entries(m_material_entries);
        connect(m_species_material_dialog, &SpeciesMaterialDialog::materials_changed, this, [this]()
        {
            if (m_species_material_dialog == nullptr)
            {
                return;
            }

            apply_material_entries(m_species_material_dialog->material_entries(), true, true);
        });
        connect(m_species_material_dialog, &QObject::destroyed, this, [this]()
        {
            runtime_debug::trace("SpeciesMaterialDialog destroyed signal received in MainWindow");
            m_species_material_dialog = nullptr;
        });
    }
    else
    {
        runtime_debug::trace("Reusing existing SpeciesMaterialDialog");
    }

    m_species_material_dialog->set_material_entries(m_material_entries);
    runtime_debug::trace(
        QString("Showing SpeciesMaterialDialog %1")
            .arg(reinterpret_cast<quintptr>(m_species_material_dialog.data()), 0, 16));
    m_species_material_dialog->show();
    m_species_material_dialog->raise();
    m_species_material_dialog->activateWindow();
}

bool MainWindow::load_chemkin_file(const QString &file_path,
                                   bool show_error_message_box,
                                   bool show_success_feedback)
{
    QString error_message;
    bool ok = false;
    const QStringList species_names = read_chemkin_species_names(file_path,
                                                                 &ok,
                                                                 &error_message,
                                                                 show_error_message_box);
    if (!ok)
    {
        if (error_message.trimmed().isEmpty())
        {
            error_message = QString("Chemkin species import failed: %1").arg(file_path);
        }
        qWarning() << error_message;
        statusBar()->showMessage(error_message, 8000);
        return false;
    }

    m_chemkin_species_names = species_names;
    m_chemkin_file_path = QFileInfo(file_path).absoluteFilePath();
    m_3d_widget->set_chemkin_species_names(m_chemkin_species_names);
    if (m_species_color_dialog != nullptr)
    {
        m_species_color_dialog->set_chemkin_context(m_chemkin_file_path, m_chemkin_species_names);
    }
    update_chemkin_status();

    QString config_error_message;
    if (!save_last_chemkin_file_path(m_chemkin_file_path, &config_error_message) &&
        !config_error_message.trimmed().isEmpty())
    {
        qWarning() << config_error_message;
        statusBar()->showMessage(config_error_message, 8000);
    }

    if (show_success_feedback)
    {
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
    }

    statusBar()->showMessage(
        QString("Imported %1 species from Chemkin file").arg(m_chemkin_species_names.size()),
        5000);
    return true;
}

void MainWindow::restore_last_chemkin_file()
{
    QString saved_file_path;
    QString error_message;
    if (!load_last_chemkin_file_path(&saved_file_path, &error_message))
    {
        if (!error_message.trimmed().isEmpty())
        {
            qWarning() << error_message;
            statusBar()->showMessage(error_message, 8000);
        }
        return;
    }

    if (saved_file_path.trimmed().isEmpty())
    {
        return;
    }

    if (!QFileInfo::exists(saved_file_path))
    {
        const QString message = QString("Last Chemkin file was not found: %1").arg(saved_file_path);
        qWarning() << message;
        statusBar()->showMessage(message, 8000);
        return;
    }

    if (load_chemkin_file(saved_file_path, false, false))
    {
        statusBar()->showMessage(
            QString("Restored Chemkin species from %1").arg(saved_file_path),
            5000);
    }
}

void MainWindow::restore_material_table()
{
    QList<MaterialConfigEntry> entries;
    QString error_message;
    if (!load_material_table_config(&entries, &error_message))
    {
        if (!error_message.trimmed().isEmpty())
        {
            qWarning() << error_message;
            statusBar()->showMessage(error_message, 8000);
        }
        apply_material_entries({}, false, false);
        return;
    }

    apply_material_entries(entries, false, false);
}

void MainWindow::apply_material_entries(const QList<MaterialConfigEntry> &entries,
                                        bool save_to_config,
                                        bool show_status_feedback)
{
    m_material_entries = entries;
    m_3d_widget->set_material_names(material_names_from_entries(m_material_entries));

    if (m_species_material_dialog != nullptr &&
        !material_entries_equal(m_species_material_dialog->material_entries(), m_material_entries))
    {
        m_species_material_dialog->set_material_entries(m_material_entries);
    }

    if (save_to_config)
    {
        QString error_message;
        if (!save_material_table_config(m_material_entries, &error_message))
        {
            if (!error_message.trimmed().isEmpty())
            {
                qWarning() << error_message;
                statusBar()->showMessage(error_message, 8000);
            }
        }
    }

    if (show_status_feedback)
    {
        statusBar()->showMessage(
            QString("Saved %1 materials").arg(material_names_from_entries(m_material_entries).size()),
            4000);
    }
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

void MainWindow::create_reference_geometry_panel()
{
    m_reference_geometry_dock = new QDockWidget("Reference Geometry", this);
    m_reference_geometry_dock->setObjectName("referenceGeometryDock");
    m_reference_geometry_dock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_reference_geometry_dock->setMinimumWidth(250);

    auto *panel = new QWidget(m_reference_geometry_dock);
    auto *panel_layout = new QVBoxLayout(panel);
    panel_layout->setContentsMargins(10, 10, 10, 10);

    auto create_spin_box = [panel](const QString &label, QFormLayout *layout)
    {
        auto *spin_box = new QDoubleSpinBox(panel);
        spin_box->setRange(-1.0e6, 1.0e6);
        spin_box->setDecimals(4);
        spin_box->setSingleStep(0.1);
        spin_box->setKeyboardTracking(false);
        layout->addRow(label, spin_box);
        return spin_box;
    };

    auto *position_group = new QGroupBox("Position", panel);
    auto *position_layout = new QFormLayout(position_group);
    m_reference_position_x = create_spin_box("X", position_layout);
    m_reference_position_y = create_spin_box("Y", position_layout);
    m_reference_position_z = create_spin_box("Z", position_layout);
    panel_layout->addWidget(position_group);

    auto *rotation_group = new QGroupBox("Rotation (deg)", panel);
    auto *rotation_layout = new QFormLayout(rotation_group);
    m_reference_rotation_x = create_spin_box("X", rotation_layout);
    m_reference_rotation_y = create_spin_box("Y", rotation_layout);
    m_reference_rotation_z = create_spin_box("Z", rotation_layout);
    panel_layout->addWidget(rotation_group);

    auto *apply_button = new QPushButton("Apply Transform", panel);
    auto *reset_button = new QPushButton("Reset Transform", panel);
    m_align_reference_face = new QPushButton("Align View to Selected Face", panel);
    m_align_reference_face->setEnabled(false);
    m_reference_geometry_lock = new QCheckBox("Lock Reference Geometry", panel);

    auto *face_info_group = new QGroupBox("Selected Face Coordinate", panel);
    auto *face_info_layout = new QFormLayout(face_info_group);
    m_reference_face_origin = new QLabel("-", face_info_group);
    m_reference_face_normal = new QLabel("-", face_info_group);
    m_reference_face_origin->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_reference_face_normal->setTextInteractionFlags(Qt::TextSelectableByMouse);
    face_info_layout->addRow("Origin", m_reference_face_origin);
    face_info_layout->addRow("Normal", m_reference_face_normal);

    panel_layout->addWidget(apply_button);
    panel_layout->addWidget(reset_button);
    panel_layout->addWidget(m_align_reference_face);
    panel_layout->addWidget(face_info_group);
    panel_layout->addWidget(m_reference_geometry_lock);
    panel_layout->addStretch();

    m_reference_geometry_dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, m_reference_geometry_dock);
    m_reference_geometry_dock->hide();

    connect(apply_button, &QPushButton::clicked, this,
            &MainWindow::apply_reference_geometry_transform);
    connect(reset_button, &QPushButton::clicked, this, [this]()
    {
        m_3d_widget->set_reference_transform(QVector3D(0.0f, 0.0f, 0.0f),
                                              QVector3D(0.0f, 0.0f, 0.0f));
    });
    connect(m_align_reference_face, &QPushButton::clicked, m_3d_widget,
            &OCCTWidget::align_view_to_selected_face);
    connect(m_reference_geometry_lock, &QCheckBox::toggled, this, [this](bool locked)
    {
        m_3d_widget->set_reference_geometry_locked(locked);
        const bool enabled = !locked;
        m_reference_position_x->setEnabled(enabled);
        m_reference_position_y->setEnabled(enabled);
        m_reference_position_z->setEnabled(enabled);
        m_reference_rotation_x->setEnabled(enabled);
        m_reference_rotation_y->setEnabled(enabled);
        m_reference_rotation_z->setEnabled(enabled);
    });

    connect(m_reference_geometry_lock, &QCheckBox::toggled, apply_button,
            [apply_button](bool locked) { apply_button->setEnabled(!locked); });
    connect(m_reference_geometry_lock, &QCheckBox::toggled, reset_button,
            [reset_button](bool locked) { reset_button->setEnabled(!locked); });
}

void MainWindow::update_reference_geometry_panel()
{
    if (m_3d_widget == nullptr || m_reference_position_x == nullptr)
    {
        return;
    }

    const QVector3D position = m_3d_widget->reference_position();
    const QVector3D rotation = m_3d_widget->reference_rotation();
    const QSignalBlocker position_x_blocker(m_reference_position_x);
    const QSignalBlocker position_y_blocker(m_reference_position_y);
    const QSignalBlocker position_z_blocker(m_reference_position_z);
    const QSignalBlocker rotation_x_blocker(m_reference_rotation_x);
    const QSignalBlocker rotation_y_blocker(m_reference_rotation_y);
    const QSignalBlocker rotation_z_blocker(m_reference_rotation_z);
    m_reference_position_x->setValue(position.x());
    m_reference_position_y->setValue(position.y());
    m_reference_position_z->setValue(position.z());
    m_reference_rotation_x->setValue(rotation.x());
    m_reference_rotation_y->setValue(rotation.y());
    m_reference_rotation_z->setValue(rotation.z());
}

void MainWindow::restore_window_layout()
{
    QByteArray saved_geometry;
    QByteArray saved_state;
    QString error_message;
    if (!load_main_window_state(&saved_geometry, &saved_state, &error_message))
    {
        if (!error_message.trimmed().isEmpty())
        {
            qWarning() << error_message;
        }
        return;
    }

    if (!saved_geometry.isEmpty())
    {
        restoreGeometry(saved_geometry);
    }
    if (!saved_state.isEmpty())
    {
        restoreState(saved_state);
    }

    if (m_reference_geometry_dock != nullptr &&
        m_3d_widget != nullptr && m_3d_widget->geometry.getShape().IsNull())
    {
        m_reference_geometry_dock->hide();
    }
}

void MainWindow::save_window_layout()
{
    QString error_message;
    if (!save_main_window_state(saveGeometry(), saveState(), &error_message) &&
        !error_message.trimmed().isEmpty())
    {
        qWarning() << error_message;
    }
}

void MainWindow::reset_window_layout()
{
    resize(1280, 720);

    if (m_object_list_dock != nullptr)
    {
        addDockWidget(Qt::LeftDockWidgetArea, m_object_list_dock);
        m_object_list_dock->show();
    }

    if (m_reference_geometry_dock != nullptr)
    {
        addDockWidget(Qt::RightDockWidgetArea, m_reference_geometry_dock);
        m_reference_geometry_dock->setVisible(
            m_3d_widget != nullptr &&
            !m_3d_widget->geometry.getShape().IsNull());
    }

    save_window_layout();
}

void MainWindow::create_object_list_panel()
{
    m_object_list_dock = new QDockWidget("Objects", this);
    m_object_list_dock->setObjectName("objectListDock");
    m_object_list_dock->setAllowedAreas(Qt::LeftDockWidgetArea |
                                        Qt::RightDockWidgetArea);
    m_object_list_dock->setMinimumWidth(220);

    auto *panel = new QWidget(m_object_list_dock);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 8, 8, 8);

    m_object_filter = new QLineEdit(panel);
    m_object_filter->setPlaceholderText("Filter objects...");
    m_object_filter->setClearButtonEnabled(true);
    layout->addWidget(m_object_filter);

    auto *view_controls = new QWidget(panel);
    auto *view_controls_layout = new QHBoxLayout(view_controls);
    view_controls_layout->setContentsMargins(0, 0, 0, 0);
    auto *fit_all_button = new QPushButton("Fit All", view_controls);
    auto *clear_selection_button = new QPushButton("Clear Selection", view_controls);
    auto *show_all_button = new QPushButton("Show All", view_controls);
    auto *hide_all_button = new QPushButton("Hide All", view_controls);
    view_controls_layout->addWidget(fit_all_button);
    view_controls_layout->addWidget(clear_selection_button);
    view_controls_layout->addWidget(show_all_button);
    view_controls_layout->addWidget(hide_all_button);
    layout->addWidget(view_controls);

    m_object_list = new QListWidget(panel);
    m_object_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_object_list->setAlternatingRowColors(true);
    layout->addWidget(m_object_list);

    m_object_list_dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, m_object_list_dock);

    connect(fit_all_button, &QPushButton::clicked, m_3d_widget,
            &OCCTWidget::fit_all_view);
    connect(clear_selection_button, &QPushButton::clicked, m_3d_widget,
            &OCCTWidget::clear_selection);
    connect(show_all_button, &QPushButton::clicked, this, [this]()
    {
        m_3d_widget->set_all_units_visible(true);
        update_object_list_panel();
    });
    connect(hide_all_button, &QPushButton::clicked, this, [this]()
    {
        m_3d_widget->set_all_units_visible(false);
        update_object_list_panel();
    });

    connect(m_object_list, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const QString object_id = item->data(Qt::UserRole).toString();
        if (object_id == QStringLiteral("reference"))
        {
            m_3d_widget->select_reference_geometry();
            return;
        }

        const QUuid uuid(object_id);
        if (uuid.isNull() || !m_3d_widget->select_unit_by_uuid(uuid))
        {
            update_object_list_panel();
        }
    });
    connect(m_object_list, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const QUuid uuid(item->data(Qt::UserRole).toString());
        if (!uuid.isNull())
        {
            m_3d_widget->edit_unit_by_uuid(uuid);
        }
    });
    connect(m_object_list, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *item)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const bool visible = item->checkState() == Qt::Checked;
        const QString object_id = item->data(Qt::UserRole).toString();
        if (object_id == QStringLiteral("reference"))
        {
            m_3d_widget->set_reference_geometry_visible(visible);
            return;
        }

        const QUuid uuid(object_id);
        if (!uuid.isNull())
        {
            m_3d_widget->set_unit_visible(uuid, visible);
            }
    });
    connect(m_object_filter, &QLineEdit::textChanged, this,
            [this](const QString &text)
    {
        if (m_object_list == nullptr)
        {
            return;
        }

        const QString filter = text.trimmed();
        for (int row = 0; row < m_object_list->count(); ++row)
        {
            QListWidgetItem *item = m_object_list->item(row);
            const bool matches = filter.isEmpty() ||
                                 item->text().contains(filter, Qt::CaseInsensitive) ||
                                 item->data(Qt::UserRole).toString()
                                     .contains(filter, Qt::CaseInsensitive);
            item->setHidden(!matches);
        }
    });
    m_object_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_object_list, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint &position)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QListWidgetItem *item = m_object_list->itemAt(position);
        if (item == nullptr)
        {
            return;
        }

        const QString object_id = item->data(Qt::UserRole).toString();
        if (object_id == QStringLiteral("reference"))
        {
            QMenu menu(m_object_list);
            QAction *fit_all_action = menu.addAction("Fit All");
            QAction *clear_face_action = menu.addAction("Clear Selected Face");
            QAction *align_face_action = menu.addAction("Align View to Selected Face");
            align_face_action->setEnabled(m_align_reference_face != nullptr &&
                                           m_align_reference_face->isEnabled());
            menu.addSeparator();
            QAction *lock_action = menu.addAction(
                m_3d_widget->reference_geometry_locked()
                    ? "Unlock Reference Geometry"
                    : "Lock Reference Geometry");
            QAction *chosen_action = menu.exec(
                m_object_list->viewport()->mapToGlobal(position));
            if (chosen_action == fit_all_action)
            {
                m_3d_widget->fit_all_view();
            }
            else if (chosen_action == clear_face_action)
            {
                m_3d_widget->clear_selection();
            }
            else if (chosen_action == align_face_action)
            {
                m_3d_widget->align_view_to_selected_face();
            }
            else if (chosen_action == lock_action)
            {
                m_3d_widget->set_reference_geometry_locked(
                    !m_3d_widget->reference_geometry_locked());
            }
            return;
        }

        const QUuid uuid(object_id);
        if (uuid.isNull() || !m_3d_widget->unit_hash.contains(uuid))
        {
            return;
        }

        QMenu menu(m_object_list);
        QAction *edit_action = menu.addAction("Edit");
        QAction *copy_action = menu.addAction("Copy");
        QAction *paste_action = menu.addAction("Paste to replace");
        paste_action->setEnabled(m_3d_widget->has_copied_unit());
        QAction *rename_action = menu.addAction("Rename");
        QAction *lock_action = menu.addAction(
            m_3d_widget->unit_locked(uuid) ? "Unlock Movement"
                                            : "Lock Movement");
        menu.addSeparator();
        QAction *delete_action = menu.addAction("Delete");
        QAction *chosen_action = menu.exec(m_object_list->viewport()->mapToGlobal(position));
        if (chosen_action == edit_action)
        {
            m_3d_widget->edit_unit_by_uuid(uuid);
        }
        else if (chosen_action == copy_action)
        {
            m_3d_widget->copy_unit_by_uuid(uuid);
        }
        else if (chosen_action == paste_action)
        {
            m_3d_widget->paste_unit_by_uuid(uuid);
        }
        else if (chosen_action == rename_action)
        {
            const QString old_name = m_3d_widget->unit_hash.value(uuid)
                                         ->inj.injector_data.name;
            bool accepted = false;
            const QString new_name = QInputDialog::getText(
                this, "Rename Injector", "Name:", QLineEdit::Normal,
                old_name, &accepted);
            if (accepted)
            {
                m_3d_widget->set_unit_name(uuid, new_name);
            }
        }
        else if (chosen_action == lock_action)
        {
            m_3d_widget->set_unit_locked(
                uuid, !m_3d_widget->unit_locked(uuid));
            update_object_list_item(
                uuid, m_3d_widget->unit_hash.value(uuid)
                           ->inj.injector_data.name);
        }
        else if (chosen_action == delete_action)
        {
            const QString name = m_3d_widget->unit_hash.value(uuid)
                                     ->inj.injector_data.name;
            const auto answer = QMessageBox::question(
                this, "Delete Injector",
                QString("Delete injector \"%1\"?").arg(name),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes)
            {
                m_3d_widget->remove_unit_by_uuid(uuid);
            }
        }
    });

    update_object_list_panel();
}

void MainWindow::update_object_list_panel()
{
    if (m_object_list == nullptr || m_3d_widget == nullptr)
    {
        return;
    }

    const QString selected_object_id = m_object_list->currentItem() == nullptr
                                           ? QString()
                                           : m_object_list->currentItem()
                                                 ->data(Qt::UserRole).toString();
    const QSignalBlocker blocker(m_object_list);
    m_object_list->clear();

    if (!m_3d_widget->geometry.getShape().IsNull())
    {
        auto *reference_item = new QListWidgetItem("Reference Geometry", m_object_list);
        reference_item->setData(Qt::UserRole, QStringLiteral("reference"));
        reference_item->setFlags(reference_item->flags() | Qt::ItemIsUserCheckable);
        reference_item->setCheckState(m_3d_widget->reference_geometry_visible()
                                          ? Qt::Checked
                                          : Qt::Unchecked);
    }

    QList<QUuid> unit_ids = m_3d_widget->unit_hash.keys();
    std::sort(unit_ids.begin(), unit_ids.end(), [this](const QUuid &lhs,
                                                       const QUuid &rhs)
    {
        const auto left = m_3d_widget->unit_hash.value(lhs);
        const auto right = m_3d_widget->unit_hash.value(rhs);
        const QString left_name = left == nullptr
                                      ? QString()
                                      : left->inj.injector_data.name.trimmed();
        const QString right_name = right == nullptr
                                       ? QString()
                                       : right->inj.injector_data.name.trimmed();
        const int name_comparison = QString::compare(left_name, right_name,
                                                     Qt::CaseInsensitive);
        if (name_comparison != 0)
        {
            return name_comparison < 0;
        }
        return lhs.toString() < rhs.toString();
    });

    for (const QUuid &unit_id : unit_ids)
    {
        const auto it = m_3d_widget->unit_hash.constFind(unit_id);
        if (it.value() == nullptr)
        {
            continue;
        }

        QString name = it.value()->inj.injector_data.name.trimmed();
        if (name.isEmpty())
        {
            name = it.key().toString(QUuid::WithoutBraces);
        }

        if (m_3d_widget->unit_locked(unit_id))
        {
            name = "[Locked] " + name;
        }
        auto *unit_item = new QListWidgetItem(name, m_object_list);
        unit_item->setData(Qt::UserRole,
                           unit_id.toString(QUuid::WithoutBraces));
        unit_item->setToolTip(unit_id.toString(QUuid::WithoutBraces));
        unit_item->setFlags(unit_item->flags() | Qt::ItemIsUserCheckable);
        unit_item->setCheckState(m_3d_widget->unit_visible(unit_id)
                                     ? Qt::Checked
                                     : Qt::Unchecked);
    }

    if (m_object_filter != nullptr)
    {
        const QString filter = m_object_filter->text().trimmed();
        for (int row = 0; row < m_object_list->count(); ++row)
        {
            QListWidgetItem *item = m_object_list->item(row);
            item->setHidden(!filter.isEmpty() &&
                            !item->text().contains(filter, Qt::CaseInsensitive) &&
                            !item->data(Qt::UserRole).toString()
                                 .contains(filter, Qt::CaseInsensitive));
        }
    }

    if (!selected_object_id.isEmpty())
    {
        for (int row = 0; row < m_object_list->count(); ++row)
        {
            QListWidgetItem *item = m_object_list->item(row);
            if (item->data(Qt::UserRole).toString() == selected_object_id &&
                !item->isHidden())
            {
                item->setSelected(true);
                m_object_list->scrollToItem(item);
                break;
            }
        }
    }
}

void MainWindow::update_object_list_selection(const QUuid &uuid,
                                               bool reference_geometry)
{
    if (m_object_list == nullptr)
    {
        return;
    }

    const QSignalBlocker blocker(m_object_list);
    m_object_list->clearSelection();
    const QString object_id = reference_geometry
                                  ? QStringLiteral("reference")
                                  : uuid.toString(QUuid::WithoutBraces);
    for (int row = 0; row < m_object_list->count(); ++row)
    {
        QListWidgetItem *item = m_object_list->item(row);
        if (item->data(Qt::UserRole).toString() == object_id)
        {
            item->setSelected(true);
            m_object_list->scrollToItem(item);
            break;
        }
    }
}

void MainWindow::update_object_list_item(const QUuid &uuid, const QString &name)
{
    if (m_object_list == nullptr || uuid.isNull())
    {
        return;
    }

    QString display_name = name.trimmed();
    if (display_name.isEmpty())
    {
        display_name = uuid.toString(QUuid::WithoutBraces);
    }
    if (m_3d_widget != nullptr && m_3d_widget->unit_locked(uuid))
    {
        display_name = "[Locked] " + display_name;
    }

    for (int row = 0; row < m_object_list->count(); ++row)
    {
        QListWidgetItem *item = m_object_list->item(row);
        if (item->data(Qt::UserRole).toString() ==
            uuid.toString(QUuid::WithoutBraces))
        {
            item->setText(display_name);
            return;
        }
    }
}

void MainWindow::apply_reference_geometry_transform()
{
    if (m_3d_widget == nullptr)
    {
        return;
    }

    m_3d_widget->set_reference_transform(
        QVector3D(static_cast<float>(m_reference_position_x->value()),
                  static_cast<float>(m_reference_position_y->value()),
                  static_cast<float>(m_reference_position_z->value())),
        QVector3D(static_cast<float>(m_reference_rotation_x->value()),
                  static_cast<float>(m_reference_rotation_y->value()),
                  static_cast<float>(m_reference_rotation_z->value())));
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

