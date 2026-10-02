#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "app_config.h"
#include "chemkin_io.h"
#include "dpm_file_io.h"
#include "project_session.h"
#include "runtime_debug.h"
#include "species_color_dialog.h"
#include "species_material_dialog.h"
#include "unit_preferences_dialog.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSysInfo>
#include <QTextStream>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QDialogButtonBox>
#include <QSizePolicy>
#include <QtMath>
#include <QDebug>
#include <QApplication>
#include <QButtonGroup>
#include <QActionGroup>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QComboBox>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QTimer>
#include <QToolBar>
#include <QStyle>
#include <QSettings>
#include <QMap>
#include <QBrush>
#include <algorithm>
#include <functional>

namespace
{
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

QList<QTreeWidgetItem *> ObjectTreeWidget::all_items() const
{
    QList<QTreeWidgetItem *> result;
    std::function<void(QTreeWidgetItem *)> append_children;
    append_children = [&result, &append_children](QTreeWidgetItem *parent)
    {
        if (parent == nullptr)
        {
            return;
        }
        result.append(parent);
        for (int index = 0; index < parent->childCount(); ++index)
        {
            append_children(parent->child(index));
        }
    };

    for (int index = 0; index < topLevelItemCount(); ++index)
    {
        append_children(topLevelItem(index));
    }
    return result;
}

QTreeWidgetItem *ObjectTreeWidget::item(int index) const
{
    const QList<QTreeWidgetItem *> items = all_items();
    return index >= 0 && index < items.size() ? items.at(index) : nullptr;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    runtime_debug::trace("MainWindow constructor begin");
    ui->setupUi(this);
    runtime_debug::trace("MainWindow ui->setupUi finished");
    update_project_session_title();

    m_recent_projects_menu = ui->menureaddile->addMenu("Recent Projects");
    restore_recent_projects();

    QString config_error_message;
    if (!ensure_app_config_directories(&config_error_message) &&
        !config_error_message.trimmed().isEmpty())
    {
        qWarning() << config_error_message;
    }

    connect(ui->actionUnit_of_Measurement, &QAction::triggered, this,
            &MainWindow::open_unit_preferences_dialog);
    ui->menuSettings->removeAction(ui->actionLanguage);
    QMenu *language_menu = ui->menuSettings->addMenu(tr("Language"));
    QAction *english_action = language_menu->addAction(tr("English"));
    QAction *chinese_action = language_menu->addAction(tr("Simplified Chinese"));
    const auto select_language = [this](const QString &locale)
    {
        QSettings settings;
        settings.setValue("language/locale", locale);
        QMessageBox::information(
            this, tr("Language"),
            tr("Language changes will take effect after restart."));
    };
    connect(english_action, &QAction::triggered, this,
            [select_language]() { select_language("en_US"); });
    connect(chinese_action, &QAction::triggered, this,
            [select_language]() { select_language("zh_CN"); });

    Unit_Preferences unit_preferences;
    QString unit_preferences_error;
    if (load_unit_preferences(&unit_preferences, &unit_preferences_error))
    {
        UnitSystem::set_active_preferences(unit_preferences);
    }
    else if (!unit_preferences_error.trimmed().isEmpty())
    {
        qWarning() << unit_preferences_error;
    }

    //tab_widget = new QTabWidget();

    //this->setCentralWidget(m_3d_widget);
    m_3d_widget = new OCCTWidget(this);
    this->setCentralWidget(m_3d_widget);
    m_3d_widget->installEventFilter(this);
    m_3d_widget->apply_visual_preferences(UnitSystem::active_preferences());
    runtime_debug::trace("MainWindow OCCTWidget created");

    // QApplication::aboutToQuit also covers quit paths that bypass closeEvent.
    connect(qApp, &QCoreApplication::aboutToQuit, this,
            &MainWindow::close_auxiliary_windows_for_shutdown);

    create_reference_geometry_panel();
    create_object_list_panel();
    create_array_editor_panel();
    connect(ui->actionObjects, &QAction::toggled, this, [this](bool visible)
    {
        if (m_object_list_dock != nullptr)
        {
            m_object_list_dock->setVisible(visible);
            if (visible)
            {
                m_object_list_dock->raise();
            }
        }
    });
    connect(m_object_list_dock, &QDockWidget::visibilityChanged, this,
            [this](bool visible)
    {
        const QSignalBlocker blocker(ui->actionObjects);
        ui->actionObjects->setChecked(visible);
    });
    connect(ui->actionReference_Geometry, &QAction::toggled, this,
            [this](bool visible)
    {
        if (m_reference_geometry_dock != nullptr &&
            !m_3d_widget->geometry.getShape().IsNull())
        {
            m_reference_geometry_dock->setVisible(visible);
            if (visible)
            {
                m_reference_geometry_dock->raise();
            }
        }
    });
    connect(m_reference_geometry_dock, &QDockWidget::visibilityChanged, this,
            [this](bool visible)
    {
        const QSignalBlocker blocker(ui->actionReference_Geometry);
        ui->actionReference_Geometry->setChecked(visible);
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
            const QSignalBlocker blocker(ui->actionReference_Geometry);
            ui->actionReference_Geometry->setChecked(available &&
                                                       m_reference_geometry_dock != nullptr &&
                                                       m_reference_geometry_dock->isVisible());
        }
        update_reference_geometry_panel();
        update_object_list_panel();
    });
    connect(m_3d_widget, &OCCTWidget::reference_transform_changed, this,
            [this](const QVector3D &, const QVector3D &)
    {
        update_reference_geometry_panel();
        mark_project_dirty();
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
        update_reference_geometry_controls();
        update_object_list_panel();
    });
    connect(m_3d_widget, &OCCTWidget::unit_display_list_changed,
            this, [this]()
    {
        sync_persistent_units_from_occt();
        update_object_list_panel();
    });
    connect(m_3d_widget, &OCCTWidget::unit_lock_changed,
            this, [this](const QUuid &uuid, bool)
    {
        if (m_3d_widget == nullptr || !m_3d_widget->unit_hash.contains(uuid))
        {
            return;
        }

        const std::shared_ptr<Unit> unit = m_3d_widget->unit_hash.value(uuid);
        if (unit != nullptr)
        {
            update_object_list_item(uuid, unit->inj.injector_data.name);
        }
    });
    connect(m_3d_widget, &OCCTWidget::unit_removed, this,
            [this](const QUuid &uuid)
    {
        Q_UNUSED(uuid);
        sync_persistent_units_from_occt();
        mark_project_dirty();
    });
    connect(m_3d_widget, &OCCTWidget::selection_changed,
            this, &MainWindow::update_object_list_selection);
    connect(m_3d_widget, &OCCTWidget::unit_selection_changed, this,
            [this](const QList<QUuid> &uuids)
    {
        if (m_object_list == nullptr)
        {
            return;
        }
        const QSignalBlocker blocker(m_object_list);
        m_object_list->clearSelection();
        QTreeWidgetItem *current = nullptr;
        for (int row = 0; row < m_object_list->count(); ++row)
        {
            QTreeWidgetItem *item = m_object_list->item(row);
            if (uuids.contains(QUuid(item->data(0, Qt::UserRole).toString())))
            {
                item->setSelected(true);
                if (current == nullptr) current = item;
            }
        }
        if (current != nullptr)
        {
            m_object_list->setCurrentItem(current, QItemSelectionModel::NoUpdate);
            m_object_list->scrollToItem(current);
        }
        update_unit_position_controls();
    });
    connect(ui->actionUndo_Move, &QAction::triggered, m_3d_widget,
            &OCCTWidget::undo_last_move);
    connect(ui->actionRedo_Move, &QAction::triggered, m_3d_widget,
            &OCCTWidget::redo_move);
    connect(ui->actionUndo_Edit, &QAction::triggered, m_3d_widget,
            &OCCTWidget::undo_last_edit);
    connect(ui->actionRedo_Edit, &QAction::triggered, m_3d_widget,
            &OCCTWidget::redo_edit);
    connect(ui->actionUndo_Delete, &QAction::triggered, m_3d_widget,
            &OCCTWidget::undo_last_delete);
    connect(ui->actionRedo_Delete, &QAction::triggered, m_3d_widget,
            &OCCTWidget::redo_delete);
    connect(ui->actionUndo_Reference_Transform, &QAction::triggered,
            m_3d_widget, &OCCTWidget::undo_reference_transform);
    connect(ui->actionRedo_Reference_Transform, &QAction::triggered,
            m_3d_widget, &OCCTWidget::redo_reference_transform);
    connect(m_3d_widget, &OCCTWidget::move_history_changed, this,
            [this](bool can_undo, bool can_redo)
    {
        ui->actionUndo_Move->setEnabled(can_undo);
        ui->actionRedo_Move->setEnabled(can_redo);
    });
    connect(m_3d_widget, &OCCTWidget::edit_history_changed, this,
            [this](bool can_undo, bool can_redo)
    {
        ui->actionUndo_Edit->setEnabled(can_undo);
        ui->actionRedo_Edit->setEnabled(can_redo);
    });
    connect(m_3d_widget, &OCCTWidget::delete_history_changed, this,
            [this](bool can_undo, bool can_redo)
    {
        ui->actionUndo_Delete->setEnabled(can_undo);
        ui->actionRedo_Delete->setEnabled(can_redo);
    });
    connect(m_3d_widget, &OCCTWidget::unit_added, this,
            [this](Unit *added_unit)
    {
        if (added_unit == nullptr)
        {
            return;
        }
        sync_persistent_units_from_occt();
        mark_project_dirty();
    });
    connect(m_3d_widget, &OCCTWidget::reference_transform_history_changed,
            this, [this](bool can_undo, bool can_redo)
    {
        ui->actionUndo_Reference_Transform->setEnabled(can_undo);
        ui->actionRedo_Reference_Transform->setEnabled(can_redo);
        if (!m_loading_project_session && (can_undo || can_redo))
        {
            mark_project_dirty();
            save_reference_geometry_state();
        }
    });

    // OCCT owns editable Unit copies so that interactive handles remain stable.
    // Keep the MainWindow list synchronized whenever one of those copies changes.
    connect(m_3d_widget, &OCCTWidget::unit_data_updated,
            this, &MainWindow::sync_unit_from_occt);
    connect(m_3d_widget, &OCCTWidget::unit_position_updated,
            this, &MainWindow::sync_unit_position_from_occt);
    connect(m_3d_widget, &OCCTWidget::unit_geometry_refresh_failed,
            this, [this](const QUuid &, const QString &message)
    {
        if (!message.trimmed().isEmpty())
        {
            statusBar()->showMessage(message, 8000);
        }
    });

    m_chemkin_toolbar = new QToolBar("Chemkin Status", this);
    m_chemkin_toolbar->setObjectName("chemkinStatusToolbar");
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

    runtime_debug::trace("MainWindow building default injector preview units");
    units = build_test_injector_units();
    runtime_debug::trace(QString("MainWindow built %1 default injector preview units").arg(units.size()));
    m_3d_widget->display_units(units);
    runtime_debug::trace("MainWindow displayed default injector preview units");
    statusBar()->showMessage(
        QString("Loaded %1 default injector preview units").arg(units.size()), 5000);

    restore_material_table();
    runtime_debug::trace("MainWindow material table restored");
    // Keep the default preview scene clean. Reference geometry is restored
    // only when an explicit project session is opened.
    runtime_debug::trace("MainWindow skipped global reference geometry restore");
    restore_last_chemkin_file();
    runtime_debug::trace("MainWindow chemkin file restore finished");
    restore_window_layout();
    runtime_debug::trace("MainWindow constructor end");

}

void MainWindow::open_unit_preferences_dialog()
{
    UnitPreferencesDialog dialog(UnitSystem::active_preferences(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const Unit_Preferences preferences = dialog.preferences();
    QString error_message;
    if (!save_unit_preferences(preferences, &error_message))
    {
        QMessageBox::warning(this, "Display Units", error_message);
        return;
    }

    UnitSystem::set_active_preferences(preferences);
    if (m_3d_widget != nullptr)
    {
        m_3d_widget->apply_visual_preferences(preferences);
        m_3d_widget->refresh_open_unit_editors();
        update_unit_position_controls();
    }
    statusBar()->showMessage("Display units updated", 3000);
}

MainWindow::~MainWindow()
{
    runtime_debug::trace("MainWindow destructor begin");
    close_auxiliary_windows_for_shutdown();
    // Keep layout persistence reliable even when the window is destroyed
    // through an application-exit path that bypasses closeEvent.
    save_window_layout();
    save_reference_geometry_state();
    delete ui;
    runtime_debug::trace("MainWindow destructor end");
}

void MainWindow::close_auxiliary_windows_for_shutdown()
{
    if (m_auxiliary_shutdown_started)
    {
        return;
    }
    m_auxiliary_shutdown_started = true;

    // Close OCCT-owned editors before the context or view starts teardown.
    if (m_3d_widget != nullptr)
    {
        m_3d_widget->discard_auxiliary_dialogs();
    }

    // These dialogs are reusable and parent-owned, so hide them here and let
    // normal QObject ownership perform the final destruction.
    if (m_species_color_dialog != nullptr)
    {
        m_species_color_dialog->close();
    }
    if (m_species_material_dialog != nullptr)
    {
        m_species_material_dialog->close();
    }
}

void MainWindow::sync_unit_from_occt(Unit *changed_unit)
{
    sync_unit_from_occt_impl(changed_unit, true);
    update_unit_position_controls();
}

void MainWindow::set_unit_editor_case_context(const Unit_Edit_Case_Context &context)
{
    if (m_3d_widget != nullptr)
    {
        m_3d_widget->set_unit_editor_case_context(context);
    }
}

void MainWindow::sync_unit_position_from_occt(Unit *changed_unit)
{
    sync_unit_from_occt_impl(changed_unit, false);
    update_unit_position_controls();
}

void MainWindow::sync_persistent_units_from_occt()
{
    if (m_3d_widget == nullptr)
    {
        return;
    }

    const auto copy_persistent_state = [](Unit &target, const Unit &source)
    {
        target.type = source.type;
        target.inj.uuid = source.inj.uuid;
        target.inj.injector_data = source.inj.injector_data;
        target.inj.shape = source.inj.shape;
        target.array_parent_uuid = source.array_parent_uuid;
        target.is_array_child = source.is_array_child;
        target.follows_array = source.follows_array;
        target.prototype_uuid = source.prototype_uuid;
        target.prototype_chain = source.prototype_chain;
        target.has_array_spec = source.has_array_spec;
        target.array_spec = source.array_spec;
        target.array_specs = source.array_specs;
        target.has_fill_spec = source.has_fill_spec;
        target.fill_spec = source.fill_spec;
        target.fill_source_uuids = source.fill_source_uuids;
        target.assembly_parent_uuid = source.assembly_parent_uuid;
        target.assembly_child_uuids = source.assembly_child_uuids;
        target.assembly_local_position = source.assembly_local_position;
        target.assembly_local_rotation = source.assembly_local_rotation;
        target.child_units.clear();
    };

    QSet<QUuid> persistent_ids;
    for (auto it = m_3d_widget->unit_hash.constBegin();
         it != m_3d_widget->unit_hash.constEnd(); ++it)
    {
        const std::shared_ptr<Unit> runtime_unit = it.value();
        if (runtime_unit == nullptr ||
            (runtime_unit->is_array_child && runtime_unit->follows_array))
        {
            continue;
        }

        persistent_ids.insert(runtime_unit->inj.uuid);
        auto stored_it = std::find_if(
            units.begin(), units.end(),
            [&runtime_unit](const Unit &stored_unit)
            {
                return stored_unit.inj.uuid == runtime_unit->inj.uuid;
            });
        if (stored_it == units.end())
        {
            Unit stored_unit;
            copy_persistent_state(stored_unit, *runtime_unit);
            units.append(std::move(stored_unit));
        }
        else
        {
            copy_persistent_state(*stored_it, *runtime_unit);
        }
    }

    for (int index = units.size() - 1; index >= 0; --index)
    {
        if (!persistent_ids.contains(units.at(index).inj.uuid))
        {
            units.removeAt(index);
        }
    }
}

void MainWindow::sync_unit_from_occt_impl(Unit *changed_unit, bool recompute_dirty)
{
    if (changed_unit == nullptr)
    {
        return;
    }

    sync_persistent_units_from_occt();
    update_object_list_item(changed_unit->inj.uuid,
                            changed_unit->inj.injector_data.name);
    if (recompute_dirty)
    {
        mark_project_dirty();
    }
    else if (!m_loading_project_session && !m_project_dirty)
    {
        // Drag events are frequent. Avoid recalculating the complete
        // project fingerprint for every mouse-move event.
        m_project_dirty = true;
        update_project_session_title();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    runtime_debug::trace("MainWindow closeEvent begin");

    if (!confirm_project_change("closing"))
    {
        event->ignore();
        return;
    }

    runtime_debug::trace("Closing auxiliary windows from MainWindow closeEvent");
    close_auxiliary_windows_for_shutdown();

    save_window_layout();
    save_reference_geometry_state();

    QMainWindow::closeEvent(event);
    runtime_debug::trace("MainWindow closeEvent end");
}

int MainWindow::assign_species_to_unassigned_units()
{
    if (m_3d_widget == nullptr || m_chemkin_species_names.isEmpty())
    {
        return 0;
    }

    int assigned_count = 0;
    for (const Unit &unit : units)
    {
        if (unit.type == Assebly)
        {
            continue;
        }

        const QString &current_species = unit.inj.injector_data.type == Droplet
            ? unit.inj.injector_data.evaporating_species
            : unit.inj.injector_data.material;
        if (!current_species.trimmed().isEmpty())
        {
            continue;
        }

        const QString &species = m_chemkin_species_names.at(
            QRandomGenerator::global()->bounded(m_chemkin_species_names.size()));
        if (m_3d_widget->set_species_for_units_by_uuid(
                {unit.inj.uuid}, species) > 0)
        {
            ++assigned_count;
        }
    }
    return assigned_count;
}


void MainWindow::on_actionRead_triggered()
{
    bool ok = false;
    QString error_message;
    QStringList warning_messages;
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

    const QList<Unit> temp = read_dpm_file(file_path,
                                           &ok,
                                           &error_message,
                                           true,
                                           &warning_messages);
    if (ok)
    {
        if (!confirm_project_change("importing a DPM file", true))
        {
            return;
        }

        units.clear();
        units = temp;
        m_3d_widget->display_units(units, true);
        const int assigned_species_count = assign_species_to_unassigned_units();
        m_project_session_file_path.clear();
        m_project_baseline_initialized = false;
        m_saved_project_fingerprint.clear();
        update_project_session_title();
        mark_project_dirty();

        qDebug() << "Loaded injector count:" << units.size();
        for (int i = 0; i < units.size(); ++i)
        {
            qDebug() << "Injector" << i << ":" << units[i].inj.injector_data.name;
        }

        statusBar()->showMessage(
            QString("Loaded %1 injectors from DPM file").arg(units.size()), 5000);
        if (assigned_species_count > 0)
        {
            statusBar()->showMessage(
                QString("Automatically assigned Species to %1 injector(s)")
                    .arg(assigned_species_count),
                5000);
        }
        if (!warning_messages.isEmpty())
        {
            for (const QString &warning : warning_messages)
            {
                qWarning() << warning;
            }
            statusBar()->showMessage(
                QString("DPM imported with %1 unsupported field warning(s)")
                    .arg(warning_messages.size()),
                8000);
        }
    }
    else
    {
        statusBar()->showMessage(
            error_message.trimmed().isEmpty() ? "DPM file import failed" : error_message,
            8000);
    }
}

void MainWindow::on_actionSave_DPM_triggered()
{
    QString preflight_error;
    if (!validate_dpm_units(units, &preflight_error))
    {
        const QString message = preflight_error.trimmed().isEmpty()
            ? "DPM export preflight failed."
            : preflight_error;
        QMessageBox::warning(this, "DPM Export Preflight", message);
        statusBar()->showMessage(message, 5000);
        return;
    }

    QString file_path = QFileDialog::getSaveFileName(
        this,
        "Save DPM File",
        ".",
        "DPM Files (*.dpm);;Text Files (*.txt);;All Files (*.*)");
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("DPM export canceled", 5000);
        return;
    }

    QString error_message;
    if (!write_dpm_file(file_path, units, &error_message))
    {
        const QString message = error_message.trimmed().isEmpty()
            ? "DPM export failed."
            : error_message;
        QMessageBox::critical(this, "DPM Export Error", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    statusBar()->showMessage(QString("DPM file saved: %1").arg(file_path), 8000);
}

void MainWindow::on_actionOpen_Project_triggered()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        "Open Project Session",
        ".",
        "DPM Manager Project (*.dpmpj *.dpmproj);;All Files (*.*)");
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("Project session open canceled", 5000);
        return;
    }

    if (!confirm_project_change("opening another project", true))
    {
        return;
    }

    load_project_session(file_path);
}

bool MainWindow::confirm_project_change(const QString &action_description,
                                        bool restore_saved_project_on_discard)
{
    if (!m_project_dirty)
    {
        return true;
    }

    const QString discard_hint = restore_saved_project_on_discard
        ? (m_project_session_file_path.trimmed().isEmpty()
               ? " Discard will abandon the current unsaved temporary project."
               : " Discard will restore the last saved project before continuing.")
        : QString();
    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this,
        "Unsaved Project Changes",
        QString("The current project has unsaved changes. Save before %1?%2")
            .arg(action_description, discard_hint),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (answer == QMessageBox::Cancel)
    {
        return false;
    }

    if (answer == QMessageBox::Save)
    {
        return save_current_project_session();
    }

    if (answer == QMessageBox::Discard &&
        restore_saved_project_on_discard &&
        !m_project_session_file_path.trimmed().isEmpty())
    {
        return load_project_session(m_project_session_file_path);
    }

    return true;
}

void MainWindow::on_actionSave_Project_triggered()
{
    save_current_project_session();
}

void MainWindow::on_actionSave_Project_As_triggered()
{
    save_project_session_as();
}

void MainWindow::on_actionValidate_Project_triggered()
{
    const project_session::Data data = collect_project_data();
    QString error_message;
    if (!validate_dpm_units(data.units, &error_message))
    {
        const QString message = error_message.trimmed().isEmpty()
            ? "Project validation failed for one or more DPM units."
            : error_message;
        QMessageBox::warning(this, "Project Validation", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    QStringList external_file_errors;
    if (!data.chemkin_file_path.trimmed().isEmpty())
    {
        const QFileInfo chemkin_info(data.chemkin_file_path);
        if (!chemkin_info.exists() || !chemkin_info.isFile())
        {
            external_file_errors.append(
                QString("Chemkin file is missing: %1").arg(data.chemkin_file_path));
        }
    }
    if (!data.reference_geometry.file_path.trimmed().isEmpty())
    {
        const QFileInfo geometry_info(data.reference_geometry.file_path);
        if (!geometry_info.exists() || !geometry_info.isFile())
        {
            external_file_errors.append(
                QString("Reference geometry file is missing: %1")
                    .arg(data.reference_geometry.file_path));
        }
    }
    if (!external_file_errors.isEmpty())
    {
        const QString message = QString("Project validation found %1 external file problem(s):\n- %2")
                                    .arg(external_file_errors.size())
                                    .arg(external_file_errors.join("\n- "));
        QMessageBox::warning(this, "Project Validation", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    if (!project_session::validate(data, &error_message) ||
        !project_session::validate_references(data,
                                               m_chemkin_species_names,
                                               &error_message))
    {
        const QString message = error_message.trimmed().isEmpty()
            ? "Project validation failed."
            : error_message;
        QMessageBox::warning(this, "Project Validation", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    QMessageBox::information(this,
                             "Project Validation",
                             QString("Project validation passed.\n\n%1 unit(s), %2 material(s), %3 species color(s).")
                                 .arg(data.units.size())
                                 .arg(data.materials.size())
                                 .arg(data.species_colors.size()));
    statusBar()->showMessage("Project validation passed", 5000);
}

void MainWindow::on_actionExport_Diagnostics_triggered()
{
    const QString file_path = QFileDialog::getSaveFileName(
        this,
        "Export Diagnostics",
        ".",
        "Diagnostic Reports (*.txt);;All Files (*.*)");
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("Diagnostic export canceled", 5000);
        return;
    }

    const project_session::Data data = collect_project_data();
    QSaveFile output(file_path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        const QString message = QString("Unable to write diagnostic report: %1")
                                    .arg(file_path);
        QMessageBox::critical(this, "Diagnostic Export Error", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    QTextStream stream(&output);
    stream << "DPM Manager Diagnostic Report\n"
           << "Generated: " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
           << "\n"
           << "Qt: " << QT_VERSION_STR << "\n"
           << "Platform: " << QSysInfo::prettyProductName() << "\n"
           << "Application directory: " << QCoreApplication::applicationDirPath() << "\n"
           << "Project session: " << m_project_session_file_path << "\n"
           << "Project dirty: " << (m_project_dirty ? "yes" : "no") << "\n"
           << "Units: " << data.units.size() << "\n"
           << "Materials: " << data.materials.size() << "\n"
           << "Chemkin file: " << data.chemkin_file_path << "\n"
           << "Chemkin species: " << m_chemkin_species_names.size() << "\n"
           << "Reference geometry: "
           << (data.reference_geometry.file_path.trimmed().isEmpty() ? "none" :
               data.reference_geometry.file_path)
           << "\n"
           << "Runtime log: " << runtime_debug::current_log_file_path() << "\n\n"
           << "Runtime log contents\n"
           << "====================\n";

    QFile log_file(runtime_debug::current_log_file_path());
    if (log_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        stream << log_file.readAll();
    }
    else
    {
        stream << "Unable to read the current runtime log.\n";
    }

    if (!output.commit())
    {
        const QString message = QString("Unable to finalize diagnostic report: %1")
                                    .arg(file_path);
        QMessageBox::critical(this, "Diagnostic Export Error", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    statusBar()->showMessage(QString("Diagnostics exported: %1").arg(file_path), 8000);
}

void MainWindow::on_actionOpen_Config_Folder_triggered()
{
    QString error_message;
    if (!ensure_app_config_directories(&error_message))
    {
        const QString message = error_message.trimmed().isEmpty()
            ? "Unable to create the application config folder."
            : error_message;
        QMessageBox::warning(this, "Open Config Folder", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    const QString config_path = app_config_directory_path();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(config_path)))
    {
        const QString message = QString("Unable to open config folder: %1").arg(config_path);
        QMessageBox::warning(this, "Open Config Folder", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    statusBar()->showMessage(QString("Config folder: %1").arg(config_path), 8000);
}

void MainWindow::on_actionOpen_Logs_Folder_triggered()
{
    const QString logs_path = runtime_debug::log_directory_path();
    if (!QDir().mkpath(logs_path))
    {
        const QString message = QString("Unable to create logs folder: %1").arg(logs_path);
        QMessageBox::warning(this, "Open Logs Folder", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(logs_path)))
    {
        const QString message = QString("Unable to open logs folder: %1").arg(logs_path);
        QMessageBox::warning(this, "Open Logs Folder", message);
        statusBar()->showMessage(message, 8000);
        return;
    }

    statusBar()->showMessage(QString("Logs folder: %1").arg(logs_path), 8000);
}

bool MainWindow::save_current_project_session()
{
    if (!m_project_session_file_path.trimmed().isEmpty())
    {
        return save_project_session(m_project_session_file_path);
    }

    return save_project_session_as();
}

bool MainWindow::save_project_session_as()
{
    QString file_path = QFileDialog::getSaveFileName(
        this,
        "Save Project Session",
        ".",
        "DPM Manager Project (*.dpmpj);;Legacy Project (*.dpmproj);;All Files (*.*)");
    if (file_path.trimmed().isEmpty())
    {
        statusBar()->showMessage("Project session save canceled", 5000);
        return false;
    }

    if (QFileInfo(file_path).suffix().isEmpty())
    {
        file_path += ".dpmpj";
    }

    return save_project_session(file_path);
}

bool MainWindow::save_project_session(const QString &file_path)
{
    const project_session::Data data = collect_project_data();

    QString error_message;
    if (!project_session::validate_references(data,
                                               m_chemkin_species_names,
                                               &error_message))
    {
        QMessageBox::warning(this, "Project Reference Preflight", error_message);
        statusBar()->showMessage(error_message, 8000);
        return false;
    }
    if (!project_session::save(file_path, data, &error_message))
    {
        QMessageBox::critical(this, "Project Session Error", error_message);
        statusBar()->showMessage(error_message, 8000);
        return false;
    }

    m_project_session_file_path = QFileInfo(file_path).absoluteFilePath();
    remember_project_path(m_project_session_file_path);
    m_saved_project_fingerprint = project_session::fingerprint(data);
    m_project_baseline_initialized = true;
    m_project_dirty = false;
    update_project_session_title();
    statusBar()->showMessage(QString("Project session saved: %1").arg(file_path), 8000);
    return true;
}

bool MainWindow::load_project_session(const QString &file_path)
{
    project_session::Data data;
    QStringList project_species_names;
    QString error_message;
    if (!project_session::load(file_path, &data, &error_message))
    {
        QMessageBox::critical(this, "Project Session Error", error_message);
        statusBar()->showMessage(error_message, 8000);
        return false;
    }

    if (!data.chemkin_file_path.trimmed().isEmpty())
    {
        bool chemkin_ok = false;
        QString chemkin_error;
        project_species_names = read_chemkin_species_names(data.chemkin_file_path,
                                                           &chemkin_ok,
                                                           &chemkin_error,
                                                           false);
        if (!chemkin_ok)
        {
            const QString message = chemkin_error.trimmed().isEmpty()
                ? QString("Project Chemkin file cannot be read: %1").arg(data.chemkin_file_path)
                : chemkin_error;
            QMessageBox::critical(this, "Project Session Error", message);
            statusBar()->showMessage(message, 8000);
            return false;
        }
    }

    if (!project_session::validate_references(data,
                                               project_species_names,
                                               &error_message))
    {
        QMessageBox::critical(this, "Project Session Error", error_message);
        statusBar()->showMessage(error_message, 8000);
        return false;
    }

    if (!data.reference_geometry.file_path.trimmed().isEmpty())
    {
        const QFileInfo geometry_info(data.reference_geometry.file_path);
        if (!geometry_info.exists() || !geometry_info.isFile())
        {
            const QString message = QString("Project reference geometry was not found: %1")
                                        .arg(data.reference_geometry.file_path);
            QMessageBox::critical(this, "Project Session Error", message);
            statusBar()->showMessage(message, 8000);
            return false;
        }
    }

    Base_Geom_Read loaded_geometry;
    const bool has_reference_geometry =
        data.reference_geometry.kind != QStringLiteral("file") ||
        !data.reference_geometry.file_path.trimmed().isEmpty();
    if (has_reference_geometry && data.reference_geometry.kind == QStringLiteral("file"))
    {
        QString geometry_path = QFileInfo(data.reference_geometry.file_path).absoluteFilePath();
        if (!loaded_geometry.readFile(geometry_path))
        {
            const QString message = loaded_geometry.last_error_message().trimmed().isEmpty()
                ? QString("Unable to read project reference geometry: %1")
                      .arg(data.reference_geometry.file_path)
                : loaded_geometry.last_error_message();
            QMessageBox::critical(this, "Project Session Error", message);
            statusBar()->showMessage(message, 8000);
            return false;
        }
    }

    m_loading_project_session = true;
    if (data.has_unit_preferences)
    {
        UnitSystem::set_active_preferences(data.unit_preferences);
    }
    m_3d_widget->discard_auxiliary_dialogs();
    units = data.units;
    m_3d_widget->display_units(units, true);

    if (data.chemkin_file_path.trimmed().isEmpty())
    {
        m_chemkin_file_path.clear();
        m_chemkin_species_names.clear();
        m_3d_widget->set_chemkin_species_names({});
        m_3d_widget->set_species_colors({});
        update_chemkin_status();
        QString clear_error;
        if (!clear_last_chemkin_file_path(&clear_error) && !clear_error.trimmed().isEmpty())
        {
            qWarning() << clear_error;
        }
    }
    else if (!load_chemkin_file(data.chemkin_file_path, false, false))
    {
        m_loading_project_session = false;
        return false;
    }

    if (!data.species_colors.isEmpty() && !m_chemkin_species_names.isEmpty())
    {
        QString color_error;
        if (!save_species_color_config(m_chemkin_file_path,
                                       m_chemkin_species_names,
                                       data.species_colors,
                                       &color_error) &&
            !color_error.trimmed().isEmpty())
        {
            qWarning() << color_error;
        }
        if (m_species_color_dialog != nullptr)
        {
            m_species_color_dialog->set_chemkin_context(m_chemkin_file_path,
                                                        m_chemkin_species_names);
            m_species_color_dialog->set_species_colors(data.species_colors);
        }
    }
    m_3d_widget->set_species_colors(data.species_colors);

    apply_material_entries(data.materials, true, false);

    m_3d_widget->clear_reference_geometry();
    if (has_reference_geometry && data.reference_geometry.kind == QStringLiteral("file"))
    {
        m_3d_widget->geometry.adopt_loaded_geometry(loaded_geometry);
        m_3d_widget->add_readed_geometry();
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }
    else if (has_reference_geometry &&
             data.reference_geometry.kind == QStringLiteral("datum_plane"))
    {
        m_3d_widget->create_reference_datum_plane(
            data.reference_geometry.construction_size,
            data.reference_geometry.construction_thickness,
            data.reference_geometry.construction_direction);
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }
    else if (has_reference_geometry &&
             data.reference_geometry.kind == QStringLiteral("section_plane"))
    {
        m_3d_widget->create_reference_section_plane(
            data.reference_geometry.construction_size,
            data.reference_geometry.construction_thickness,
            data.reference_geometry.construction_direction);
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }
    else if (has_reference_geometry &&
             data.reference_geometry.kind == QStringLiteral("datum_axis"))
    {
        m_3d_widget->create_reference_datum_axis(
            data.reference_geometry.construction_size,
            data.reference_geometry.construction_radius,
            data.reference_geometry.construction_direction);
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }
    else if (has_reference_geometry &&
             data.reference_geometry.kind == QStringLiteral("datum_origin"))
    {
        m_3d_widget->create_reference_datum_origin(
            data.reference_geometry.construction_radius);
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }
    else if (has_reference_geometry &&
             data.reference_geometry.kind == QStringLiteral("alignment_frame"))
    {
        m_3d_widget->create_reference_alignment_frame(
            data.reference_geometry.construction_size,
            data.reference_geometry.construction_direction);
        m_3d_widget->set_reference_transform(data.reference_geometry.position,
                                              data.reference_geometry.rotation);
        m_3d_widget->set_reference_geometry_locked(data.reference_geometry.locked);
        m_3d_widget->set_reference_geometry_visible(data.reference_geometry.visible);
    }

    update_object_list_panel();
    update_reference_geometry_panel();
    m_project_session_file_path = QFileInfo(file_path).absoluteFilePath();
    remember_project_path(m_project_session_file_path);
    m_saved_project_fingerprint = project_session::fingerprint(data);
    m_project_baseline_initialized = true;
    m_project_dirty = false;
    m_loading_project_session = false;
    update_project_session_title();
    statusBar()->showMessage(QString("Project session loaded: %1").arg(file_path), 8000);
    return true;
}

void MainWindow::restore_recent_projects()
{
    QString error_message;
    if (!load_recent_project_paths(&m_recent_project_paths, &error_message) &&
        !error_message.trimmed().isEmpty())
    {
        qWarning() << error_message;
    }
    update_recent_projects_menu();
}

void MainWindow::remember_project_path(const QString &file_path)
{
    const QFileInfo file_info(file_path);
    if (!file_info.exists() || !file_info.isFile())
    {
        return;
    }

    const QString absolute_path = file_info.absoluteFilePath();
    for (int index = m_recent_project_paths.size() - 1; index >= 0; --index)
    {
        if (m_recent_project_paths.at(index).compare(absolute_path,
                                                     Qt::CaseInsensitive) == 0)
        {
            m_recent_project_paths.removeAt(index);
        }
    }
    m_recent_project_paths.prepend(absolute_path);
    while (m_recent_project_paths.size() > 10)
    {
        m_recent_project_paths.removeLast();
    }

    QString error_message;
    if (!save_recent_project_paths(m_recent_project_paths, &error_message) &&
        !error_message.trimmed().isEmpty())
    {
        qWarning() << error_message;
    }
    update_recent_projects_menu();
}

void MainWindow::update_recent_projects_menu()
{
    if (m_recent_projects_menu == nullptr)
    {
        return;
    }

    const QStringList previous_paths = m_recent_project_paths;
    m_recent_projects_menu->clear();
    QStringList existing_paths;
    for (const QString &path : m_recent_project_paths)
    {
        const QFileInfo file_info(path);
        if (file_info.exists() && file_info.isFile() &&
            !existing_paths.contains(file_info.absoluteFilePath(), Qt::CaseInsensitive))
        {
            existing_paths.append(file_info.absoluteFilePath());
        }
    }
    m_recent_project_paths = existing_paths;
    if (m_recent_project_paths != previous_paths)
    {
        QString save_error;
        if (!save_recent_project_paths(m_recent_project_paths, &save_error) &&
            !save_error.trimmed().isEmpty())
        {
            qWarning() << save_error;
        }
    }

    if (m_recent_project_paths.isEmpty())
    {
        QAction *empty_action = m_recent_projects_menu->addAction("No Recent Projects");
        empty_action->setEnabled(false);
        return;
    }

    for (const QString &path : m_recent_project_paths)
    {
        QAction *action = m_recent_projects_menu->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        action->setData(path);
        connect(action, &QAction::triggered, this, [this, action]()
        {
            const QString path = action->data().toString();
            const QFileInfo file_info(path);
            if (!file_info.exists() || !file_info.isFile())
            {
                for (int index = m_recent_project_paths.size() - 1; index >= 0; --index)
                {
                    if (m_recent_project_paths.at(index).compare(
                            path, Qt::CaseInsensitive) == 0)
                    {
                        m_recent_project_paths.removeAt(index);
                    }
                }
                QString save_error;
                save_recent_project_paths(m_recent_project_paths, &save_error);
                update_recent_projects_menu();
                statusBar()->showMessage(
                    QString("Recent project was not found: %1").arg(path), 8000);
                return;
            }
            if (!load_project_session(path))
            {
                return;
            }
            remember_project_path(path);
        });
    }
}

void MainWindow::mark_project_dirty()
{
    if (m_loading_project_session)
    {
        return;
    }

    refresh_project_dirty_state();
}

project_session::Data MainWindow::collect_project_data() const
{
    project_session::Data data;
    data.units = units;
    data.chemkin_file_path = m_chemkin_file_path;
    data.materials = m_material_entries;
    data.unit_preferences = UnitSystem::active_preferences();
    data.has_unit_preferences = true;
    if (m_species_color_dialog != nullptr)
    {
        data.species_colors = m_species_color_dialog->species_colors();
    }
    else if (!m_chemkin_file_path.trimmed().isEmpty())
    {
        load_species_color_config(m_chemkin_file_path,
                                  m_chemkin_species_names,
                                  &data.species_colors,
                                  nullptr);
    }
    if (m_3d_widget != nullptr && !m_3d_widget->geometry.getShape().IsNull())
    {
        data.reference_geometry.kind = m_3d_widget->reference_geometry_kind();
        data.reference_geometry.file_path = m_3d_widget->geometry.file_path();
        if (data.reference_geometry.kind != QStringLiteral("file"))
        {
            data.reference_geometry.file_path.clear();
            data.reference_geometry.construction_size =
                m_3d_widget->reference_construction_size();
            data.reference_geometry.construction_thickness =
                m_3d_widget->reference_construction_thickness();
            data.reference_geometry.construction_radius =
                m_3d_widget->reference_construction_radius();
            data.reference_geometry.construction_direction =
                m_3d_widget->reference_construction_direction();
        }
        data.reference_geometry.position = m_3d_widget->reference_position();
        data.reference_geometry.rotation = m_3d_widget->reference_rotation();
        data.reference_geometry.locked = m_3d_widget->reference_geometry_locked();
        data.reference_geometry.visible = m_3d_widget->reference_geometry_visible();
        data.reference_geometry.section_clipping =
            m_3d_widget->section_plane_clipping_enabled();
    }
    return data;
}

void MainWindow::refresh_project_dirty_state()
{
    const bool dirty = !m_project_baseline_initialized ||
                       project_session::fingerprint(collect_project_data()) !=
                           m_saved_project_fingerprint;
    if (m_project_dirty == dirty)
    {
        return;
    }

    m_project_dirty = dirty;
    update_project_session_title();
}

void MainWindow::update_project_session_title()
{
    const QString project_name = m_project_session_file_path.trimmed().isEmpty()
        ? QString("DPM Manager")
        : QString("DPM Manager - %1")
              .arg(QFileInfo(m_project_session_file_path).fileName());
    setWindowTitle(project_name + (m_project_dirty ? " *" : QString()));
}



void MainWindow::on_actionRead_Base_Geometry_triggered()
{
    // Parse into a temporary reader so a failed import cannot destroy the
    // currently displayed reference geometry.
    Base_Geom_Read loaded_geometry;
    const bool ok = loaded_geometry.Read_Geometry_Dialog();
    if (ok)
    {
        qDebug() << "true";
        m_3d_widget->geometry.adopt_loaded_geometry(loaded_geometry);
        m_3d_widget->add_readed_geometry();
        mark_project_dirty();
        save_reference_geometry_state();
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

    if (load_chemkin_file(file_path, true, true))
    {
        mark_project_dirty();
    }
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
        connect(m_species_color_dialog, &SpeciesColorDialog::species_colors_changed,
                this, [this]()
        {
            m_3d_widget->set_species_colors(
                m_species_color_dialog != nullptr
                    ? m_species_color_dialog->species_colors()
                    : QHash<QString, QColor>());
            mark_project_dirty();
        });
    }

    m_species_color_dialog->set_chemkin_context(m_chemkin_file_path, m_chemkin_species_names);
    m_3d_widget->set_species_colors(m_species_color_dialog->species_colors());
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
    QHash<QString, QColor> loaded_species_colors;
    QString color_config_error;
    if (!load_species_color_config(m_chemkin_file_path,
                                   m_chemkin_species_names,
                                   &loaded_species_colors,
                                   &color_config_error) &&
        !color_config_error.trimmed().isEmpty())
    {
        qWarning() << color_config_error;
    }
    m_3d_widget->set_species_colors(loaded_species_colors);
    const int assigned_species_count = assign_species_to_unassigned_units();
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
    if (assigned_species_count > 0)
    {
        statusBar()->showMessage(
            QString("Automatically assigned Species to %1 injector(s)")
                .arg(assigned_species_count),
            5000);
    }
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
        QString clear_error;
        if (!clear_last_chemkin_file_path(&clear_error) && !clear_error.trimmed().isEmpty())
        {
            qWarning() << clear_error;
        }
        return;
    }

    if (load_chemkin_file(saved_file_path, false, false))
    {
        statusBar()->showMessage(
            QString("Restored Chemkin species from %1").arg(saved_file_path),
            5000);
    }
    else
    {
        QString clear_error;
        if (!clear_last_chemkin_file_path(&clear_error) && !clear_error.trimmed().isEmpty())
        {
            qWarning() << clear_error;
        }
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

void MainWindow::restore_reference_geometry()
{
    ReferenceGeometryConfig config;
    QString error_message;
    if (!load_reference_geometry_config(&config, &error_message))
    {
        if (!error_message.trimmed().isEmpty())
        {
            qWarning() << error_message;
            statusBar()->showMessage(error_message, 8000);
        }
        return;
    }

    if (config.kind == QStringLiteral("datum_plane"))
    {
        m_3d_widget->create_reference_datum_plane(
            config.construction_size, config.construction_thickness,
            config.construction_direction);
        m_3d_widget->set_reference_transform(config.position, config.rotation);
        m_3d_widget->set_reference_geometry_locked(config.locked);
        m_3d_widget->set_reference_geometry_visible(config.visible);
        update_reference_geometry_panel();
        return;
    }
    if (config.kind == QStringLiteral("section_plane"))
    {
        m_3d_widget->create_reference_section_plane(
            config.construction_size, config.construction_thickness,
            config.construction_direction);
        m_3d_widget->set_reference_transform(config.position, config.rotation);
        m_3d_widget->set_reference_geometry_locked(config.locked);
        m_3d_widget->set_reference_geometry_visible(config.visible);
        m_3d_widget->set_section_plane_clipping(config.section_clipping);
        update_reference_geometry_panel();
        return;
    }
    if (config.kind == QStringLiteral("datum_axis"))
    {
        m_3d_widget->create_reference_datum_axis(
            config.construction_size, config.construction_radius,
            config.construction_direction);
        m_3d_widget->set_reference_transform(config.position, config.rotation);
        m_3d_widget->set_reference_geometry_locked(config.locked);
        m_3d_widget->set_reference_geometry_visible(config.visible);
        update_reference_geometry_panel();
        return;
    }
    if (config.kind == QStringLiteral("datum_origin"))
    {
        m_3d_widget->create_reference_datum_origin(config.construction_radius);
        m_3d_widget->set_reference_transform(config.position, config.rotation);
        m_3d_widget->set_reference_geometry_locked(config.locked);
        m_3d_widget->set_reference_geometry_visible(config.visible);
        update_reference_geometry_panel();
        return;
    }
    if (config.kind == QStringLiteral("alignment_frame"))
    {
        m_3d_widget->create_reference_alignment_frame(
            config.construction_size, config.construction_direction);
        m_3d_widget->set_reference_transform(config.position, config.rotation);
        m_3d_widget->set_reference_geometry_locked(config.locked);
        m_3d_widget->set_reference_geometry_visible(config.visible);
        update_reference_geometry_panel();
        return;
    }
    const QFileInfo file_info(config.file_path);
    if (!file_info.exists() || !file_info.isFile())
    {
        const QString message = QString("Saved reference geometry was not found: %1")
                                    .arg(config.file_path);
        qWarning() << message;
        statusBar()->showMessage(message, 8000);
        QString clear_error;
        if (!save_reference_geometry_config(ReferenceGeometryConfig(), &clear_error) &&
            !clear_error.trimmed().isEmpty())
        {
            qWarning() << clear_error;
        }
        return;
    }

    QString file_path = file_info.absoluteFilePath();
    if (!m_3d_widget->geometry.readFile(file_path))
    {
        const QString message = m_3d_widget->geometry.last_error_message().trimmed().isEmpty()
            ? QString("Unable to restore reference geometry: %1").arg(config.file_path)
            : m_3d_widget->geometry.last_error_message();
        qWarning() << message;
        statusBar()->showMessage(message, 8000);
        QString clear_error;
        if (!save_reference_geometry_config(ReferenceGeometryConfig(), &clear_error) &&
            !clear_error.trimmed().isEmpty())
        {
            qWarning() << clear_error;
        }
        return;
    }

    m_3d_widget->add_readed_geometry();
    m_3d_widget->set_reference_transform(config.position, config.rotation);
    m_3d_widget->set_reference_geometry_locked(config.locked);
    m_3d_widget->set_reference_geometry_visible(config.visible);
    update_reference_geometry_panel();
    statusBar()->showMessage(
        QString("Restored reference geometry from %1").arg(config.file_path), 5000);
}

void MainWindow::save_reference_geometry_state()
{
    ReferenceGeometryConfig config;
    if (m_3d_widget != nullptr && !m_3d_widget->geometry.getShape().IsNull())
    {
        config.kind = m_3d_widget->reference_geometry_kind();
        config.file_path = m_3d_widget->geometry.file_path();
        if (config.kind != QStringLiteral("file"))
        {
            config.file_path.clear();
        }
        config.position = m_3d_widget->reference_position();
        config.rotation = m_3d_widget->reference_rotation();
        config.locked = m_3d_widget->reference_geometry_locked();
        config.visible = m_3d_widget->reference_geometry_visible();
        config.section_clipping = m_3d_widget->section_plane_clipping_enabled();
    }

    QString error_message;
    if (!save_reference_geometry_config(config, &error_message) &&
        !error_message.trimmed().isEmpty())
    {
        qWarning() << error_message;
    }
}

void MainWindow::apply_material_entries(const QList<MaterialConfigEntry> &entries,
                                        bool save_to_config,
                                        bool show_status_feedback)
{
    m_material_entries = entries;
    if (m_species_material_dialog != nullptr &&
        !material_entries_equal(m_species_material_dialog->material_entries(), m_material_entries))
    {
        m_species_material_dialog->set_material_entries(m_material_entries);
    }

    if (save_to_config)
    {
        mark_project_dirty();
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

    m_apply_reference_transform = new QPushButton("Apply Transform", panel);
    m_reset_reference_transform = new QPushButton("Reset Transform", panel);
    m_align_reference_face = new QPushButton("Align View to Selected Face", panel);
    m_clear_reference_geometry = new QPushButton("Clear Reference Geometry", panel);
    m_create_datum_plane = new QPushButton("Create Datum Plane", panel);
    m_create_datum_axis = new QPushButton("Create Datum Axis", panel);
    m_create_datum_origin = new QPushButton("Create Datum Origin", panel);
    m_create_section_plane = new QPushButton("Create Section Plane", panel);
    m_toggle_section_clipping = new QPushButton("Enable Section Clipping", panel);
    m_toggle_section_clipping->setCheckable(true);
    m_create_alignment_frame = new QPushButton("Create Alignment Frame", panel);
    m_align_reference_face->setEnabled(false);
    m_reference_geometry_lock = new QCheckBox("Lock Reference Geometry", panel);

    auto *source_group = new QGroupBox("Source", panel);
    auto *source_layout = new QFormLayout(source_group);
    m_reference_geometry_path = new QLabel("-", source_group);
    m_reference_geometry_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_reference_geometry_path->setWordWrap(true);
    source_layout->addRow("File", m_reference_geometry_path);

    auto *face_info_group = new QGroupBox("Selected Face Coordinate", panel);
    auto *face_info_layout = new QFormLayout(face_info_group);
    m_reference_face_origin = new QLabel("-", face_info_group);
    m_reference_face_normal = new QLabel("-", face_info_group);
    m_reference_face_origin->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_reference_face_normal->setTextInteractionFlags(Qt::TextSelectableByMouse);
    face_info_layout->addRow("Origin", m_reference_face_origin);
    face_info_layout->addRow("Normal", m_reference_face_normal);

    panel_layout->addWidget(m_apply_reference_transform);
    panel_layout->addWidget(m_reset_reference_transform);
    panel_layout->addWidget(m_align_reference_face);
    panel_layout->addWidget(m_clear_reference_geometry);
    panel_layout->addWidget(m_create_datum_plane);
    panel_layout->addWidget(m_create_datum_axis);
    panel_layout->addWidget(m_create_datum_origin);
    panel_layout->addWidget(m_create_section_plane);
    panel_layout->addWidget(m_toggle_section_clipping);
    panel_layout->addWidget(m_create_alignment_frame);
    panel_layout->addWidget(source_group);
    panel_layout->addWidget(face_info_group);
    panel_layout->addWidget(m_reference_geometry_lock);
    panel_layout->addStretch();

    m_reference_geometry_dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, m_reference_geometry_dock);
    m_reference_geometry_dock->hide();

    connect(m_apply_reference_transform, &QPushButton::clicked, this,
            &MainWindow::apply_reference_geometry_transform);
    connect(m_reset_reference_transform, &QPushButton::clicked, this, [this]()
    {
        m_3d_widget->begin_reference_transform_transaction();
        m_3d_widget->set_reference_transform(QVector3D(0.0f, 0.0f, 0.0f),
                                              QVector3D(0.0f, 0.0f, 0.0f));
        m_3d_widget->finish_reference_transform_transaction();
        mark_project_dirty();
        save_reference_geometry_state();
    });
    connect(m_align_reference_face, &QPushButton::clicked, m_3d_widget,
            &OCCTWidget::align_view_to_selected_face);
    connect(m_clear_reference_geometry, &QPushButton::clicked, this, [this]()
    {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            "Clear Reference Geometry",
            "Remove the currently loaded reference geometry?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return;
        }

        m_3d_widget->clear_reference_geometry();
        mark_project_dirty();
        save_reference_geometry_state();
        statusBar()->showMessage("Reference geometry cleared", 5000);
    });
    connect(m_create_datum_plane, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget->create_reference_datum_plane())
        {
            update_reference_geometry_panel();
            statusBar()->showMessage("Datum plane created", 5000);
        }
    });
    connect(m_create_datum_axis, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget->create_reference_datum_axis())
        {
            update_reference_geometry_panel();
            statusBar()->showMessage("Datum axis created", 5000);
        }
    });
    connect(m_create_datum_origin, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget->create_reference_datum_origin())
        {
            update_reference_geometry_panel();
            mark_project_dirty();
        }
    });
    connect(m_create_section_plane, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget->create_reference_section_plane())
        {
            update_reference_geometry_panel();
            mark_project_dirty();
        }
    });
    connect(m_toggle_section_clipping, &QPushButton::toggled, this, [this](bool enabled)
    {
        if (m_3d_widget->set_section_plane_clipping(enabled))
        {
            mark_project_dirty();
            statusBar()->showMessage(enabled ? "Section clipping enabled"
                                             : "Section clipping disabled", 5000);
        }
        else
        {
            const QSignalBlocker blocker(m_toggle_section_clipping);
            m_toggle_section_clipping->setChecked(
                m_3d_widget->section_plane_clipping_enabled());
        }
    });
    connect(m_create_alignment_frame, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget->create_reference_alignment_frame())
        {
            update_reference_geometry_panel();
            mark_project_dirty();
        }
    });
    connect(m_reference_geometry_lock, &QCheckBox::toggled, this, [this](bool locked)
    {
        m_3d_widget->set_reference_geometry_locked(locked);
        mark_project_dirty();
        save_reference_geometry_state();
        const bool enabled = !locked;
        m_reference_position_x->setEnabled(enabled);
        m_reference_position_y->setEnabled(enabled);
        m_reference_position_z->setEnabled(enabled);
        m_reference_rotation_x->setEnabled(enabled);
        m_reference_rotation_y->setEnabled(enabled);
        m_reference_rotation_z->setEnabled(enabled);
        update_reference_geometry_controls();
    });

    update_reference_geometry_controls();
}

void MainWindow::update_reference_geometry_controls()
{
    if (m_3d_widget == nullptr || m_reference_geometry_lock == nullptr)
    {
        return;
    }

    const bool available = !m_3d_widget->geometry.getShape().IsNull();
    if (!available && m_reference_geometry_lock->isChecked())
    {
        const QSignalBlocker blocker(m_reference_geometry_lock);
        m_reference_geometry_lock->setChecked(false);
        m_3d_widget->set_reference_geometry_locked(false);
    }

    const bool editable = available && !m_3d_widget->reference_geometry_locked();
    if (m_reference_position_x != nullptr)
    {
        m_reference_position_x->setEnabled(editable);
    }
    if (m_reference_position_y != nullptr)
    {
        m_reference_position_y->setEnabled(editable);
    }
    if (m_reference_position_z != nullptr)
    {
        m_reference_position_z->setEnabled(editable);
    }
    if (m_reference_rotation_x != nullptr)
    {
        m_reference_rotation_x->setEnabled(editable);
    }
    if (m_reference_rotation_y != nullptr)
    {
        m_reference_rotation_y->setEnabled(editable);
    }
    if (m_reference_rotation_z != nullptr)
    {
        m_reference_rotation_z->setEnabled(editable);
    }
    if (m_apply_reference_transform != nullptr)
    {
        m_apply_reference_transform->setEnabled(editable);
    }
    if (m_reset_reference_transform != nullptr)
    {
        m_reset_reference_transform->setEnabled(editable);
    }
    if (m_clear_reference_geometry != nullptr)
    {
        m_clear_reference_geometry->setEnabled(available);
    }
    if (m_toggle_section_clipping != nullptr)
    {
        const bool is_section_plane =
            m_3d_widget->reference_geometry_kind() == QStringLiteral("section_plane");
        const QSignalBlocker blocker(m_toggle_section_clipping);
        m_toggle_section_clipping->setEnabled(is_section_plane && editable);
        m_toggle_section_clipping->setChecked(
            is_section_plane && m_3d_widget->section_plane_clipping_enabled());
    }
    m_reference_geometry_lock->setEnabled(available);
}

void MainWindow::update_reference_geometry_panel()
{
    if (m_3d_widget == nullptr || m_reference_position_x == nullptr)
    {
        return;
    }

    const QVector3D position = m_3d_widget->reference_position();
    const QVector3D rotation = m_3d_widget->reference_rotation();
    if (m_reference_geometry_path != nullptr)
    {
        const QString path = m_3d_widget->geometry.file_path();
        m_reference_geometry_path->setText(path.trimmed().isEmpty() ? "-" : path);
        m_reference_geometry_path->setToolTip(path);
    }
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
    update_reference_geometry_controls();
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

    // Older saved layouts may still contain this toolbar as a docked toolbar.
    // Keep it as one overlay child of the main window instead.
    if (m_viewport_interaction_toolbar != nullptr)
    {
        removeToolBar(m_viewport_interaction_toolbar);
        m_viewport_interaction_toolbar->setParent(this);
        m_viewport_interaction_toolbar->show();
        QTimer::singleShot(0, this,
                           &MainWindow::position_viewport_interaction_toolbar);
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

void MainWindow::create_array_editor_panel()
{
    m_array_editor_dock = new QDockWidget(tr("Array Editor"), this);
    m_array_editor_dock->setObjectName(QStringLiteral("arrayEditorDock"));
    m_array_editor_dock->setAllowedAreas(Qt::RightDockWidgetArea);
    m_array_editor_dock->setMinimumWidth(330);

    auto *scroll_area = new QScrollArea(m_array_editor_dock);
    scroll_area->setWidgetResizable(true);
    scroll_area->setFrameShape(QFrame::NoFrame);

    auto *panel = new QWidget(scroll_area);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *source_group = new QGroupBox(tr("Array Source"), panel);
    auto *source_layout = new QVBoxLayout(source_group);
    m_array_editor_source_label = new QLabel(tr("Source: <none>"), source_group);
    m_array_editor_source_label->setWordWrap(true);
    source_layout->addWidget(m_array_editor_source_label);
    layout->addWidget(source_group);

    auto *pattern_group = new QGroupBox(tr("Pattern"), panel);
    auto *pattern_layout = new QFormLayout(pattern_group);
    auto *type = new QComboBox(pattern_group);
    type->addItems({tr("Linear"), tr("Rotational"), tr("Mirror"), tr("Elliptical")});
    auto *count = new QSpinBox(pattern_group);
    count->setRange(1, 100000);
    count->setValue(4);
    pattern_layout->addRow(tr("Array type"), type);
    pattern_layout->addRow(tr("Children"), count);
    layout->addWidget(pattern_group);

    auto make_double_spin = [panel](double value, int decimals = 4)
    {
        auto *spin = new QDoubleSpinBox(panel);
        spin->setRange(-1.0e6, 1.0e6);
        spin->setDecimals(decimals);
        spin->setValue(value);
        return spin;
    };

    auto *parameter_stack = new QStackedWidget(panel);
    auto *linear_page = new QWidget(parameter_stack);
    auto *linear_form = new QFormLayout(linear_page);
    auto *linear_spacing = make_double_spin(0.004, 6);
    linear_form->addRow(tr("Spacing"), linear_spacing);

    auto *rotational_page = new QWidget(parameter_stack);
    auto *rotational_form = new QFormLayout(rotational_page);
    auto *rotational_angle = make_double_spin(360.0, 3);
    auto *rotational_spacing = make_double_spin(0.0, 6);
    rotational_form->addRow(tr("Total angle"), rotational_angle);
    rotational_form->addRow(tr("Axial spacing"), rotational_spacing);

    auto *mirror_page = new QWidget(parameter_stack);
    auto *mirror_form = new QFormLayout(mirror_page);
    mirror_form->addRow(new QLabel(
        tr("Mirror uses the plane normal defined below."), mirror_page));

    auto *elliptical_page = new QWidget(parameter_stack);
    auto *elliptical_form = new QFormLayout(elliptical_page);
    auto *major_radius = make_double_spin(0.010, 6);
    auto *minor_radius = make_double_spin(0.005, 6);
    auto *elliptical_angle = make_double_spin(360.0, 3);
    elliptical_form->addRow(tr("Major radius"), major_radius);
    elliptical_form->addRow(tr("Minor radius"), minor_radius);
    elliptical_form->addRow(tr("Total angle"), elliptical_angle);

    parameter_stack->addWidget(linear_page);
    parameter_stack->addWidget(rotational_page);
    parameter_stack->addWidget(mirror_page);
    parameter_stack->addWidget(elliptical_page);

    auto *parameters_group = new QGroupBox(tr("Pattern Parameters"), panel);
    auto *parameters_layout = new QVBoxLayout(parameters_group);
    parameters_layout->addWidget(parameter_stack);
    layout->addWidget(parameters_group);

    auto *frame_group = new QGroupBox(tr("Coordinate Frame"), panel);
    auto *frame_layout = new QFormLayout(frame_group);
    auto *frame_mode = new QComboBox(frame_group);
    frame_mode->addItems({tr("World / custom vectors"),
                          tr("Selected face / reference geometry")});
    frame_layout->addRow(tr("Reference"), frame_mode);

    auto *origin_x = make_double_spin(0.0);
    auto *origin_y = make_double_spin(0.0);
    auto *origin_z = make_double_spin(0.0);
    // Start linear previews on a visible transverse axis instead of stacking
    // every child along the injector's usual X direction.
    auto *direction_x = make_double_spin(0.0);
    auto *direction_y = make_double_spin(1.0);
    auto *direction_z = make_double_spin(0.0);
    auto *normal_x = make_double_spin(1.0);
    auto *normal_y = make_double_spin(0.0);
    auto *normal_z = make_double_spin(0.0);
    frame_layout->addRow(tr("Origin X"), origin_x);
    frame_layout->addRow(tr("Origin Y"), origin_y);
    frame_layout->addRow(tr("Origin Z"), origin_z);
    frame_layout->addRow(tr("Direction X"), direction_x);
    frame_layout->addRow(tr("Direction Y"), direction_y);
    frame_layout->addRow(tr("Direction Z"), direction_z);
    frame_layout->addRow(tr("Plane normal X"), normal_x);
    frame_layout->addRow(tr("Plane normal Y"), normal_y);
    frame_layout->addRow(tr("Plane normal Z"), normal_z);
    layout->addWidget(frame_group);

    auto *hint = new QLabel(
        tr("This is the first dockable layout. The array geometry is rebuilt\n"
           "only after Apply is pressed."), panel);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Apply | QDialogButtonBox::Close, panel);
    layout->addWidget(buttons);
    layout->addStretch();

    connect(type, qOverload<int>(&QComboBox::currentIndexChanged),
            parameter_stack, &QStackedWidget::setCurrentIndex);
    connect(type, qOverload<int>(&QComboBox::currentIndexChanged),
            count, [count](int index)
    {
        const bool mirror = index == 2;
        count->setEnabled(!mirror);
        if (mirror)
        {
            count->setValue(2);
        }
    });
    connect(frame_mode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this, origin_x, origin_y, origin_z, direction_x,
                   direction_y, direction_z, normal_x, normal_y,
                   normal_z](int index)
    {
        const bool custom = index == 0;
        for (QDoubleSpinBox *spin : {origin_x, origin_y, origin_z,
                                     direction_x, direction_y, direction_z,
                                     normal_x, normal_y, normal_z})
        {
            spin->setEnabled(custom);
        }
        if (!custom && m_3d_widget != nullptr)
        {
            QVector3D origin;
            QVector3D reference_x;
            QVector3D reference_z;
            if (m_3d_widget->reference_frame(&origin, &reference_x, &reference_z))
            {
                origin_x->setValue(origin.x());
                origin_y->setValue(origin.y());
                origin_z->setValue(origin.z());
                direction_x->setValue(reference_x.x());
                direction_y->setValue(reference_x.y());
                direction_z->setValue(reference_x.z());
                normal_x->setValue(reference_z.x());
                normal_y->setValue(reference_z.y());
                normal_z->setValue(reference_z.z());
            }
        }
    });

    auto build_array_spec = [this, type, count, linear_spacing,
                             rotational_angle, rotational_spacing,
                             major_radius, minor_radius, elliptical_angle,
                             frame_mode, origin_x, origin_y, origin_z,
                             direction_x, direction_y, direction_z,
                             normal_x, normal_y, normal_z](UnitArraySpec *output,
                                                            bool show_warning)
    {
        if (output == nullptr || m_3d_widget == nullptr ||
            m_array_editor_source_uuid.isNull())
        {
            return false;
        }

        const auto source = m_3d_widget->unit_hash.value(m_array_editor_source_uuid);
        if (source == nullptr)
        {
            return false;
        }

        UnitArraySpec spec;
        spec.count = count->value();
        switch (type->currentIndex())
        {
        case 0:
            spec.type = UnitArrayType::Linear;
            spec.spacing = static_cast<float>(linear_spacing->value());
            break;
        case 1:
            spec.type = UnitArrayType::Rotational;
            spec.angle_degrees = static_cast<float>(rotational_angle->value());
            spec.spacing = static_cast<float>(rotational_spacing->value());
            break;
        case 2:
            spec.type = UnitArrayType::Mirror;
            spec.count = 2;
            break;
        default:
            spec.type = UnitArrayType::Elliptical;
            spec.major_radius = static_cast<float>(major_radius->value());
            spec.minor_radius = static_cast<float>(minor_radius->value());
            spec.angle_degrees = static_cast<float>(elliptical_angle->value());
            break;
        }

        if (frame_mode->currentIndex() == 1)
        {
            QVector3D origin;
            QVector3D reference_x;
            QVector3D reference_z;
            if (!m_3d_widget->reference_frame(&origin, &reference_x, &reference_z))
            {
                if (show_warning)
                {
                    QMessageBox::warning(
                        this, tr("Array Editor"),
                        tr("No usable reference coordinate frame is available."));
                }
                return false;
            }
            spec.use_reference_geometry = true;
            spec.origin = origin;
            spec.direction = reference_x;
            spec.plane_normal = reference_z;
        }
        else
        {
            spec.origin = QVector3D(
                static_cast<float>(origin_x->value()),
                static_cast<float>(origin_y->value()),
                static_cast<float>(origin_z->value()));
            if (spec.type == UnitArrayType::Linear)
            {
                spec.origin = source->inj.injector_data.pos;
            }
            spec.direction = QVector3D(
                static_cast<float>(direction_x->value()),
                static_cast<float>(direction_y->value()),
                static_cast<float>(direction_z->value()));
            spec.plane_normal = QVector3D(
                static_cast<float>(normal_x->value()),
                static_cast<float>(normal_y->value()),
                static_cast<float>(normal_z->value()));
        }

        *output = spec;
        return true;
    };

    auto refresh_array_preview = [this, build_array_spec]()
    {
        UnitArraySpec spec;
        if (build_array_spec(&spec, false))
        {
            m_3d_widget->update_array_preview(
                m_array_editor_source_uuid, spec);
        }
        else if (m_3d_widget != nullptr)
        {
            m_3d_widget->clear_array_preview();
        }
    };

    connect(type, qOverload<int>(&QComboBox::currentIndexChanged),
            panel, [refresh_array_preview](int)
    {
        refresh_array_preview();
    });
    connect(frame_mode, qOverload<int>(&QComboBox::currentIndexChanged),
            panel, [refresh_array_preview](int)
    {
        refresh_array_preview();
    });
    connect(count, qOverload<int>(&QSpinBox::valueChanged),
            panel, [refresh_array_preview](int)
    {
        refresh_array_preview();
    });
    for (QDoubleSpinBox *spin : {linear_spacing, rotational_angle,
                                 rotational_spacing, major_radius,
                                 minor_radius, elliptical_angle, origin_x,
                                 origin_y, origin_z, direction_x, direction_y,
                                 direction_z, normal_x, normal_y, normal_z})
    {
        connect(spin, qOverload<double>(&QDoubleSpinBox::valueChanged),
                panel, [refresh_array_preview](double)
        {
            refresh_array_preview();
        });
    }

    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked,
            this, [this]()
    {
        if (m_3d_widget != nullptr)
        {
            m_3d_widget->clear_array_preview();
        }
        if (m_array_editor_dock != nullptr)
        {
            m_array_editor_dock->hide();
        }
    });
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, [this, build_array_spec]()
    {
        UnitArraySpec spec;
        if (!build_array_spec(&spec, true))
        {
            return;
        }

        m_3d_widget->clear_array_preview();
        const int created = m_3d_widget->create_unit_array(
            m_array_editor_source_uuid, spec);
        if (created <= 0)
        {
            QMessageBox::warning(this, tr("Array Editor"),
                                 tr("The array could not be created."));
            return;
        }
        mark_project_dirty();
        update_object_list_panel();
        statusBar()->showMessage(
            tr("Created %1 array child units").arg(created), 5000);
    });

    connect(m_array_editor_dock, &QDockWidget::visibilityChanged,
            this, [this](bool visible)
    {
        if (!visible && m_3d_widget != nullptr)
        {
            m_3d_widget->clear_array_preview();
        }
    });

    scroll_area->setWidget(panel);
    m_array_editor_dock->setWidget(scroll_area);
    addDockWidget(Qt::RightDockWidgetArea, m_array_editor_dock);
    m_array_editor_dock->hide();
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

void MainWindow::position_viewport_interaction_toolbar()
{
    if (m_viewport_interaction_toolbar == nullptr || m_3d_widget == nullptr)
    {
        return;
    }

    m_viewport_interaction_toolbar->move(
        m_3d_widget->mapTo(this, QPoint(8, 8)));
    m_viewport_interaction_toolbar->raise();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_3d_widget && event != nullptr &&
        (event->type() == QEvent::Move || event->type() == QEvent::Resize))
    {
        position_viewport_interaction_toolbar();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event != nullptr && event->type() == QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, [this]()
        {
            if (m_viewport_interaction_toolbar != nullptr)
            {
                removeToolBar(m_viewport_interaction_toolbar);
                m_viewport_interaction_toolbar->setParent(this);
                m_viewport_interaction_toolbar->show();
            }
            position_viewport_interaction_toolbar();
        });
    }
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
    auto *fit_selected_button = new QPushButton("Fit Selected", view_controls);
    auto *clear_selection_button = new QPushButton("Clear Selection", view_controls);
    auto *show_all_button = new QPushButton("Show All", view_controls);
    auto *hide_all_button = new QPushButton("Hide All", view_controls);
    view_controls_layout->addWidget(fit_all_button);
    view_controls_layout->addWidget(fit_selected_button);
    view_controls_layout->addWidget(clear_selection_button);
    view_controls_layout->addWidget(show_all_button);
    view_controls_layout->addWidget(hide_all_button);
    layout->addWidget(view_controls);
    view_controls->hide();

    auto *batch_controls = new QWidget(panel);
    auto *batch_controls_layout = new QHBoxLayout(batch_controls);
    batch_controls_layout->setContentsMargins(0, 0, 0, 0);
    auto *show_selected_button = new QPushButton("Show Selected", batch_controls);
    auto *hide_selected_button = new QPushButton("Hide Selected", batch_controls);
    auto *lock_selected_button = new QPushButton("Lock Selected", batch_controls);
    auto *unlock_selected_button = new QPushButton("Unlock Selected", batch_controls);
    batch_controls_layout->addWidget(show_selected_button);
    batch_controls_layout->addWidget(hide_selected_button);
    batch_controls_layout->addWidget(lock_selected_button);
    batch_controls_layout->addWidget(unlock_selected_button);
    layout->addWidget(batch_controls);
    batch_controls->hide();
    auto *delete_selected_button = new QPushButton("Delete Selected", panel);
    layout->addWidget(delete_selected_button);
    delete_selected_button->hide();
    auto *paste_selected_button = new QPushButton("Paste to Selected", panel);
    layout->addWidget(paste_selected_button);
    paste_selected_button->hide();
    auto *selection_mode_button = new QPushButton("Selection Mode", panel);
    auto *translate_selected_button = new QPushButton("Translation Mode", panel);
    auto *rotate_selected_button = new QPushButton("Rotation Mode", panel);
    layout->addWidget(selection_mode_button);
    layout->addWidget(translate_selected_button);
    layout->addWidget(rotate_selected_button);
    // Transform modes belong to the viewport toolbar; keep the old widgets
    // hidden for compatibility with the existing panel wiring.
    selection_mode_button->hide();
    translate_selected_button->hide();
    rotate_selected_button->hide();
    selection_mode_button->setCheckable(true);
    translate_selected_button->setCheckable(true);
    rotate_selected_button->setCheckable(true);
    auto *interaction_mode_group = new QButtonGroup(panel);
    interaction_mode_group->setExclusive(true);
    interaction_mode_group->addButton(selection_mode_button);
    interaction_mode_group->addButton(translate_selected_button);
    interaction_mode_group->addButton(rotate_selected_button);
    selection_mode_button->setChecked(true);
    connect(m_3d_widget, &OCCTWidget::interaction_mode_changed,
            panel, [selection_mode_button, translate_selected_button,
                    rotate_selected_button](int mode)
    {
        selection_mode_button->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Selection));
        translate_selected_button->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Translation));
        rotate_selected_button->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Rotation));
    });

    // Keep the viewport tools overlaid on the OCCT view instead of consuming
    // space in the main window's top toolbar area.
    m_viewport_interaction_toolbar = new QToolBar(tr("Viewport Tools"), this);
    auto *interaction_toolbar = m_viewport_interaction_toolbar;
    interaction_toolbar->setObjectName("viewportInteractionToolbar");
    interaction_toolbar->setMovable(false);
    interaction_toolbar->setFloatable(false);
    interaction_toolbar->setAllowedAreas(Qt::NoToolBarArea);
    interaction_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    interaction_toolbar->setIconSize(QSize(22, 22));
    interaction_toolbar->setContentsMargins(2, 1, 2, 1);
    interaction_toolbar->setFixedHeight(30);
    interaction_toolbar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    interaction_toolbar->setStyleSheet(
        "QToolBar#viewportInteractionToolbar {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 0px;"
        "  spacing: 0px;"
        "}"
        "QToolBar#viewportInteractionToolbar QToolButton {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 1px;"
        "  margin: 0px;"
        "}"
        "QToolBar#viewportInteractionToolbar QToolButton:hover {"
        "  background: rgba(255, 255, 255, 32);"
        "}"
        "QToolBar#viewportInteractionToolbar QToolButton:checked {"
        "  background: rgba(150, 220, 205, 64);"
        "}"
        "QToolBar#viewportInteractionToolbar::separator {"
        "  width: 1px;"
        "  background: rgba(235, 240, 245, 80);"
        "  margin: 5px 2px;"
        "}");
    interaction_toolbar->setAttribute(Qt::WA_TranslucentBackground);
    interaction_toolbar->setAutoFillBackground(false);
    interaction_toolbar->setAttribute(Qt::WA_NativeWindow);
    position_viewport_interaction_toolbar();
    interaction_toolbar->show();
    interaction_toolbar->raise();
    QTimer::singleShot(0, this, &MainWindow::position_viewport_interaction_toolbar);
    auto *toolbar_selection = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-select.svg")),
        tr("Select"), interaction_toolbar);
    auto *toolbar_translation = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-translate.svg")),
        tr("Translate"), interaction_toolbar);
    auto *toolbar_rotation = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-rotate.svg")),
        tr("Rotate"), interaction_toolbar);
    toolbar_selection->setToolTip(tr("Select"));
    toolbar_translation->setToolTip(tr("Translate"));
    toolbar_rotation->setToolTip(tr("Rotate"));
    for (QAction *action : {toolbar_selection, toolbar_translation, toolbar_rotation})
    {
        action->setCheckable(true);
        interaction_toolbar->addAction(action);
    }
    interaction_toolbar->addSeparator();
    auto *array_tools_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-array.svg")),
        tr("Array Tools..."), interaction_toolbar);
    auto *fill_tools_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-fill.svg")),
        tr("Fill Tools..."), interaction_toolbar);
    auto *test_set_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-injector.svg")),
        tr("Load Test Injector Set"), interaction_toolbar);
    auto *reference_tools_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-reference-tool.svg")),
        tr("Reference Tools"), interaction_toolbar);
    auto *new_injector_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-new-injector.svg")),
        tr("New Injector"), interaction_toolbar);
    auto *assembly_tools_action = new QAction(
        QIcon(QStringLiteral(":/ui/icons/icon-assembly.svg")),
        tr("Create Assembly"), interaction_toolbar);
    for (QAction *action : {array_tools_action, fill_tools_action, test_set_action,
                            reference_tools_action, new_injector_action,
                            assembly_tools_action})
    {
        action->setToolTip(action->text());
    }
    interaction_toolbar->addAction(new_injector_action);
    interaction_toolbar->addAction(assembly_tools_action);
    interaction_toolbar->addAction(array_tools_action);
    interaction_toolbar->addAction(fill_tools_action);
    interaction_toolbar->addAction(test_set_action);
    interaction_toolbar->addAction(reference_tools_action);
    interaction_toolbar->adjustSize();
    auto *toolbar_mode_group = new QActionGroup(interaction_toolbar);
    toolbar_mode_group->setExclusive(true);
    toolbar_mode_group->addAction(toolbar_selection);
    toolbar_mode_group->addAction(toolbar_translation);
    toolbar_mode_group->addAction(toolbar_rotation);
    toolbar_selection->setChecked(true);
    connect(toolbar_selection, &QAction::triggered, this, [this]()
    {
        m_3d_widget->set_interaction_mode(OCCTWidget::Interaction_Mode::Selection);
    });
    connect(toolbar_translation, &QAction::triggered, this, [this]()
    {
        m_3d_widget->set_interaction_mode(OCCTWidget::Interaction_Mode::Translation);
    });
    connect(toolbar_rotation, &QAction::triggered, this, [this]()
    {
        m_3d_widget->set_interaction_mode(OCCTWidget::Interaction_Mode::Rotation);
    });
    connect(array_tools_action, &QAction::triggered, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr ||
            m_array_editor_dock == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr ||
            item->data(0, Qt::UserRole + 1).toString() != QStringLiteral("unit"))
        {
            statusBar()->showMessage(tr("Select an injector or Assembly first"), 4000);
            return;
        }

        m_3d_widget->clear_array_preview();
        m_array_editor_source_uuid = QUuid(item->data(0, Qt::UserRole).toString());
        const auto source = m_3d_widget->unit_hash.value(m_array_editor_source_uuid);
        if (m_array_editor_source_label != nullptr && source != nullptr)
        {
            m_array_editor_source_label->setText(
                tr("Source: %1").arg(source->inj.injector_data.name));
        }
        m_array_editor_dock->show();
        m_array_editor_dock->raise();
    });
    connect(fill_tools_action, &QAction::triggered, this, [this]()
    {
        if (m_object_list == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr ||
            item->data(0, Qt::UserRole + 1).toString() != QStringLiteral("unit"))
        {
            statusBar()->showMessage(tr("Select an injector or Assembly first"), 4000);
            return;
        }
        const QPoint item_position = m_object_list->visualItemRect(item).center();
        if (!item_position.isNull())
        {
            emit m_object_list->customContextMenuRequested(item_position);
        }
    });
    connect(test_set_action, &QAction::triggered, this, [this]()
    {
        units = build_test_injector_units();
        m_3d_widget->display_units(units);
        update_object_list_panel();
        statusBar()->showMessage(
            QString("Loaded %1 test injector units").arg(units.size()), 5000);
    });
    connect(reference_tools_action, &QAction::triggered, this, [this]()
    {
        if (m_reference_geometry_dock != nullptr)
        {
            m_reference_geometry_dock->show();
            m_reference_geometry_dock->raise();
        }
    });
    connect(new_injector_action, &QAction::triggered, this, [this]()
    {
        if (m_3d_widget == nullptr)
        {
            return;
        }

        QVector3D origin(0.0f, 0.0f, 0.0f);
        QVector3D reference_x;
        QVector3D reference_z;
        const bool has_reference = m_3d_widget->has_selected_face() &&
            m_3d_widget->reference_frame(&origin, &reference_x, &reference_z);

        Unit unit;
        constexpr float scale = 1.0e-3f;
        configure_common_injector(unit, "Injector", origin / scale);
        unit.inj.injector_data.material = m_chemkin_species_names.isEmpty()
            ? QString()
            : m_chemkin_species_names.first();
        if (has_reference)
        {
            unit.inj.injector_data.vel = 100.0f * reference_z;
            unit.inj.injector_data.vel2 = 100.0f * reference_z;
            unit.inj.injector_data.axis = reference_z;
            unit.inj.injector_data.atomizer_axis = reference_z;
        }

        Injector &injector = unit.inj.injector_data;
        for (QVector3D *value : {&injector.pos, &injector.pos2,
                                 &injector.ff_center, &injector.ff_virtual_origin,
                                 &injector.volume_bgeom_min, &injector.volume_bgeom_max})
        {
            *value *= scale;
        }
        for (double *value : {&injector.diameter, &injector.diameter2,
                              &injector.inner_diameter, &injector.outer_diameter,
                              &injector.radius, &injector.inner_radius,
                              &injector.volume_bgeom_radius, &injector.plain_length,
                              &injector.ff_oriface_width, &injector.stagger_radius})
        {
            *value *= scale;
        }
        if (!unit.inj.create_injector())
        {
            statusBar()->showMessage(tr("Unable to create default injector"), 5000);
            return;
        }
        m_3d_widget->display_units({unit}, false);
        update_object_list_panel();
        mark_project_dirty();
        statusBar()->showMessage(tr("Created a new injector"), 3000);
    });
    connect(assembly_tools_action, &QAction::triggered, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr ||
                item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }
        if (selected_units.size() < 2)
        {
            statusBar()->showMessage(
                tr("Select at least two units to create an Assembly"), 4000);
            return;
        }
        if (m_3d_widget->create_assembly(selected_units))
        {
            update_object_list_panel();
            mark_project_dirty();
            statusBar()->showMessage(
                QString("Created Assembly with %1 member Unit(s)")
                    .arg(selected_units.size()),
                4000);
        }
    });
    connect(m_3d_widget, &OCCTWidget::interaction_mode_changed,
            this, [toolbar_selection, toolbar_translation, toolbar_rotation,
                   interaction_toolbar](int mode)
    {
        toolbar_selection->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Selection));
        toolbar_translation->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Translation));
        toolbar_rotation->setChecked(
            mode == static_cast<int>(OCCTWidget::Interaction_Mode::Rotation));
        interaction_toolbar->raise();
    });
    auto *assembly_selected_button = new QPushButton("Create Assembly From Selected", panel);
    layout->addWidget(assembly_selected_button);
    auto *detach_assembly_button = new QPushButton("Detach Selected From Assembly", panel);
    layout->addWidget(detach_assembly_button);
    auto *dissolve_assembly_button = new QPushButton("Dissolve Selected Assembly", panel);
    layout->addWidget(dissolve_assembly_button);
    auto *material_selected_button = new QPushButton("Set Material Selected", panel);
    layout->addWidget(material_selected_button);

    auto *view_selector = new QComboBox(panel);
    view_selector->setObjectName("standardViewSelector");
    view_selector->addItem("Top", static_cast<int>(V3d_Zpos));
    view_selector->addItem("Front", static_cast<int>(V3d_TypeOfOrientation_Zup_Front));
    view_selector->addItem("Right", static_cast<int>(V3d_TypeOfOrientation_Zup_Right));
    view_selector->addItem("Back", static_cast<int>(V3d_TypeOfOrientation_Zup_Back));
    view_selector->addItem("Left", static_cast<int>(V3d_TypeOfOrientation_Zup_Left));
    view_selector->addItem("Bottom", static_cast<int>(V3d_TypeOfOrientation_Zup_Bottom));
    view_selector->addItem("Isometric", static_cast<int>(V3d_XposYposZpos));
    view_selector->setCurrentIndex(0);
    layout->addWidget(view_selector);

    m_object_list = new ObjectTreeWidget(panel);
    m_object_list->setHeaderHidden(true);
    m_object_list->setIndentation(16);
    m_object_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_object_list->setAlternatingRowColors(true);
    layout->addWidget(m_object_list);

    m_unit_position_group = new QGroupBox("Selected Unit Position", panel);
    auto *position_layout = new QFormLayout(m_unit_position_group);
    m_unit_position_group->hide();
    auto create_position_box = [this]()
    {
        auto *box = new QDoubleSpinBox(m_unit_position_group);
        box->setRange(-1.0e9, 1.0e9);
        box->setDecimals(6);
        box->setSingleStep(0.1);
        box->setEnabled(false);
        return box;
    };
    m_unit_position_x = create_position_box();
    m_unit_position_y = create_position_box();
    m_unit_position_z = create_position_box();
    position_layout->addRow("X", m_unit_position_x);
    position_layout->addRow("Y", m_unit_position_y);
    position_layout->addRow("Z", m_unit_position_z);
    layout->addWidget(m_unit_position_group);

    m_unit_direction_group = new QGroupBox("Unit Direction", panel);
    auto *direction_layout = new QVBoxLayout(m_unit_direction_group);
    m_unit_direction_group->hide();
    m_unit_direction_mode = new QComboBox(m_unit_direction_group);
    m_unit_direction_mode->addItem("Vector", static_cast<int>(Single_Direction_Mode::Vector));
    m_unit_direction_mode->addItem("Pitch / Yaw", static_cast<int>(Single_Direction_Mode::Pitch_Yaw));
    m_unit_direction_mode->addItem("Target Hitpoint", static_cast<int>(Single_Direction_Mode::Target_Hitpoint));
    direction_layout->addWidget(m_unit_direction_mode);
    m_unit_direction_stack = new QStackedWidget(m_unit_direction_group);
    auto create_direction_box = [this]()
    {
        auto *box = new QDoubleSpinBox(m_unit_direction_group);
        box->setRange(-1.0e9, 1.0e9);
        box->setDecimals(6);
        box->setSingleStep(0.1);
        box->setEnabled(false);
        return box;
    };
    auto *vector_page = new QWidget(m_unit_direction_stack);
    auto *vector_layout = new QFormLayout(vector_page);
    m_unit_direction_x = create_direction_box();
    m_unit_direction_y = create_direction_box();
    m_unit_direction_z = create_direction_box();
    vector_layout->addRow("X", m_unit_direction_x);
    vector_layout->addRow("Y", m_unit_direction_y);
    vector_layout->addRow("Z", m_unit_direction_z);
    auto *angle_page = new QWidget(m_unit_direction_stack);
    auto *angle_layout = new QFormLayout(angle_page);
    m_unit_pitch = new QDoubleSpinBox(m_unit_direction_group);
    m_unit_yaw = new QDoubleSpinBox(m_unit_direction_group);
    for (QDoubleSpinBox *box : {m_unit_pitch, m_unit_yaw})
    {
        box->setRange(-360.0, 360.0);
        box->setDecimals(4);
        box->setSingleStep(1.0);
        box->setEnabled(false);
    }
    angle_layout->addRow("Pitch (deg)", m_unit_pitch);
    angle_layout->addRow("Yaw (deg)", m_unit_yaw);
    auto *target_page = new QWidget(m_unit_direction_stack);
    auto *target_layout = new QFormLayout(target_page);
    auto create_target_box = [this]()
    {
        auto *box = new QDoubleSpinBox(m_unit_direction_group);
        box->setRange(-1.0e9, 1.0e9);
        box->setDecimals(6);
        box->setSingleStep(0.1);
        box->setEnabled(false);
        return box;
    };
    m_unit_target_x = create_target_box();
    m_unit_target_y = create_target_box();
    m_unit_target_z = create_target_box();
    target_layout->addRow("Target X", m_unit_target_x);
    target_layout->addRow("Target Y", m_unit_target_y);
    target_layout->addRow("Target Z", m_unit_target_z);
    m_unit_target_scope = new QComboBox(m_unit_direction_group);
    m_unit_target_scope->addItem("World", static_cast<int>(Single_Target_Scope::World));
    m_unit_target_scope->addItem("Array Local", static_cast<int>(Single_Target_Scope::Array_Local));
    m_unit_target_scope->addItem("Parent Local", static_cast<int>(Single_Target_Scope::Parent_Local));
    m_unit_target_scope->addItem("Reference Local", static_cast<int>(Single_Target_Scope::Reference_Local));
    m_unit_target_scope->setEnabled(false);
    target_layout->addRow("Target Scope", m_unit_target_scope);
    m_unit_direction_stack->addWidget(vector_page);
    m_unit_direction_stack->addWidget(angle_page);
    m_unit_direction_stack->addWidget(target_page);
    direction_layout->addWidget(m_unit_direction_stack);
    const auto refresh_inspector_units = [this]()
    {
        const QString length = UnitSystem::preferred_display_unit("m");
        const QString angle = UnitSystem::preferred_display_unit("deg");
        for (QDoubleSpinBox *box : {m_unit_position_x, m_unit_position_y,
                                    m_unit_position_z, m_unit_target_x,
                                    m_unit_target_y, m_unit_target_z})
        {
            if (box != nullptr) box->setSuffix(" " + length);
        }
        m_unit_pitch->setSuffix(" " + angle);
        m_unit_yaw->setSuffix(" " + angle);
    };
    refresh_inspector_units();
    layout->addWidget(m_unit_direction_group);

    connect(m_unit_direction_mode, &QComboBox::currentIndexChanged, this,
            [this](int index)
    {
        if (m_unit_direction_stack != nullptr)
        {
            m_unit_direction_stack->setCurrentIndex(index);
        }
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr)
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        const auto mode = static_cast<Single_Direction_Mode>(
            m_unit_direction_mode->itemData(index).toInt());
        if (!uuid.isNull() && m_3d_widget->set_unit_single_direction_mode_by_uuid(uuid, mode))
        {
            update_unit_position_controls();
            mark_project_dirty();
        }
    });

    m_object_list_dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, m_object_list_dock);

    connect(m_object_list, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *, QTreeWidgetItem *)
    {
        update_unit_position_controls();
    });
    const auto apply_position = [this](QDoubleSpinBox *x,
                                       QDoubleSpinBox *y,
                                       QDoubleSpinBox *z)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr ||
            x == nullptr || y == nullptr || z == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        bool ok_x = false;
        bool ok_y = false;
        bool ok_z = false;
        const QString display_unit = UnitSystem::preferred_display_unit("m");
        const auto to_storage = [&display_unit](double value, bool *ok)
        {
            return UnitSystem::convert(value, display_unit, "m", ok);
        };
        const QVector3D storage_position(
            static_cast<float>(to_storage(x->value(), &ok_x)),
            static_cast<float>(to_storage(y->value(), &ok_y)),
            static_cast<float>(to_storage(z->value(), &ok_z)));
        if (!uuid.isNull() && ok_x && ok_y && ok_z &&
            m_3d_widget->set_unit_position_by_uuid(uuid, storage_position))
        {
            update_unit_position_controls();
        }
    };
    connect(m_unit_position_x, &QDoubleSpinBox::editingFinished, this,
            [this, apply_position]()
    {
        apply_position(m_unit_position_x, m_unit_position_y, m_unit_position_z);
    });
    connect(m_unit_position_y, &QDoubleSpinBox::editingFinished, this,
            [this, apply_position]()
    {
        apply_position(m_unit_position_x, m_unit_position_y, m_unit_position_z);
    });
    connect(m_unit_position_z, &QDoubleSpinBox::editingFinished, this,
            [this, apply_position]()
    {
        apply_position(m_unit_position_x, m_unit_position_y, m_unit_position_z);
    });
    const auto apply_direction = [this](QDoubleSpinBox *x,
                                        QDoubleSpinBox *y,
                                        QDoubleSpinBox *z)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr ||
            x == nullptr || y == nullptr || z == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        if (!uuid.isNull() && m_3d_widget->set_unit_direction_by_uuid(
                uuid, QVector3D(static_cast<float>(x->value()),
                                static_cast<float>(y->value()),
                                static_cast<float>(z->value()))))
        {
            update_unit_position_controls();
        }
    };
    connect(m_unit_direction_x, &QDoubleSpinBox::editingFinished, this,
            [this, apply_direction]()
    {
        apply_direction(m_unit_direction_x, m_unit_direction_y, m_unit_direction_z);
    });
    connect(m_unit_direction_y, &QDoubleSpinBox::editingFinished, this,
            [this, apply_direction]()
    {
        apply_direction(m_unit_direction_x, m_unit_direction_y, m_unit_direction_z);
    });
    connect(m_unit_direction_z, &QDoubleSpinBox::editingFinished, this,
            [this, apply_direction]()
    {
        apply_direction(m_unit_direction_x, m_unit_direction_y, m_unit_direction_z);
    });
    const auto apply_pitch_yaw = [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr ||
            m_unit_pitch == nullptr || m_unit_yaw == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr)
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        if (!uuid.isNull() && m_3d_widget->set_unit_single_pitch_yaw_by_uuid(
                uuid, m_unit_pitch->value(), m_unit_yaw->value()))
        {
            update_unit_position_controls();
        }
    };
    connect(m_unit_pitch, &QDoubleSpinBox::editingFinished, this, apply_pitch_yaw);
    connect(m_unit_yaw, &QDoubleSpinBox::editingFinished, this, apply_pitch_yaw);
    const auto apply_target = [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr ||
            m_unit_target_x == nullptr || m_unit_target_y == nullptr ||
            m_unit_target_z == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr)
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        if (!uuid.isNull() && m_3d_widget->set_unit_single_target_by_uuid(
                uuid, QVector3D(static_cast<float>(m_unit_target_x->value()),
                                static_cast<float>(m_unit_target_y->value()),
                                static_cast<float>(m_unit_target_z->value()))))
        {
            update_unit_position_controls();
        }
    };
    connect(m_unit_target_x, &QDoubleSpinBox::editingFinished, this, apply_target);
    connect(m_unit_target_y, &QDoubleSpinBox::editingFinished, this, apply_target);
    connect(m_unit_target_z, &QDoubleSpinBox::editingFinished, this, apply_target);
    connect(m_unit_target_scope, &QComboBox::currentIndexChanged, this,
            [this](int index)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr || index < 0 ||
            m_unit_target_scope == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item == nullptr)
        {
            return;
        }
        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        m_3d_widget->set_unit_single_target_scope_by_uuid(
            uuid, static_cast<Single_Target_Scope>(
                m_unit_target_scope->itemData(index).toInt()));
        update_unit_position_controls();
    });

    connect(fit_all_button, &QPushButton::clicked, m_3d_widget,
            &OCCTWidget::fit_all_view);
    connect(fit_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget == nullptr || m_object_list == nullptr)
        {
            return;
        }

        QTreeWidgetItem *item = m_object_list->currentItem();
        if (item != nullptr)
        {
            const QString object_id = item->data(0, Qt::UserRole).toString();
            if (object_id == QStringLiteral("reference"))
            {
                m_3d_widget->select_reference_geometry();
            }
            else
            {
                const QUuid uuid(object_id);
                if (!uuid.isNull())
                {
                    m_3d_widget->select_unit_by_uuid(uuid);
                }
            }
        }
        m_3d_widget->fit_selected_view();
    });
    connect(view_selector, &QComboBox::currentIndexChanged, this,
            [this, view_selector](int index)
    {
        if (m_3d_widget == nullptr || view_selector == nullptr || index < 0)
        {
            return;
        }

        m_3d_widget->set_standard_view(
            static_cast<V3d_TypeOfOrientation>(
                view_selector->itemData(index).toInt()));
    });
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

    const auto apply_selected_units = [this](const std::function<void(const QUuid &)> &operation)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }

            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull())
            {
                operation(uuid);
            }
        }
        update_object_list_panel();
    };
    connect(show_selected_button, &QPushButton::clicked, this,
            [this, apply_selected_units]()
    {
        apply_selected_units([this](const QUuid &uuid)
        {
            m_3d_widget->set_unit_visible(uuid, true);
        });
    });
    connect(hide_selected_button, &QPushButton::clicked, this,
            [this, apply_selected_units]()
    {
        apply_selected_units([this](const QUuid &uuid)
        {
            m_3d_widget->set_unit_visible(uuid, false);
        });
    });
    connect(lock_selected_button, &QPushButton::clicked, this,
            [this, apply_selected_units]()
    {
        apply_selected_units([this](const QUuid &uuid)
        {
            m_3d_widget->set_unit_locked(uuid, true);
        });
    });
    connect(unlock_selected_button, &QPushButton::clicked, this,
            [this, apply_selected_units]()
    {
        apply_selected_units([this](const QUuid &uuid)
        {
            m_3d_widget->set_unit_locked(uuid, false);
        });
    });
    connect(delete_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }

        if (selected_units.isEmpty())
        {
            statusBar()->showMessage("Select one or more injectors first", 4000);
            return;
        }

        const auto answer = QMessageBox::question(
            this,
            "Delete Selected Injectors",
            QString("Delete %1 selected injector(s)?").arg(selected_units.size()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return;
        }

        for (const QUuid &uuid : selected_units)
        {
            m_3d_widget->remove_unit_by_uuid(uuid);
        }
    });
    connect(paste_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        if (!m_3d_widget->has_copied_unit())
        {
            statusBar()->showMessage("Copy an injector before pasting", 4000);
            return;
        }

        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }

        if (selected_units.isEmpty())
        {
            statusBar()->showMessage("Select one or more injectors first", 4000);
            return;
        }

        int pasted_count = 0;
        for (const QUuid &uuid : selected_units)
        {
            if (m_3d_widget->paste_unit_by_uuid(uuid))
            {
                ++pasted_count;
            }
        }
        statusBar()->showMessage(
            QString("Pasted injector parameters to %1 of %2 selected unit(s)")
                .arg(pasted_count)
                .arg(selected_units.size()),
            5000);
    });
    connect(translate_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget != nullptr)
        {
            m_3d_widget->set_interaction_mode(
                OCCTWidget::Interaction_Mode::Translation);
            statusBar()->showMessage(
                "Translation mode: select an injector and drag a world axis", 5000);
            return;
        }
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }

        if (selected_units.isEmpty())
        {
            statusBar()->showMessage("Select one or more injectors first", 4000);
            return;
        }

        if (selected_units.size() == 1 &&
            m_3d_widget->activate_translation_gizmo(selected_units.first()))
        {
            statusBar()->showMessage(
                "Translation gizmo active: drag an axis arrow; press Escape to cancel",
                5000);
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle("Translate Selected Injectors");
        auto *form = new QFormLayout(&dialog);
        auto create_offset_box = [&dialog]()
        {
            auto *box = new QDoubleSpinBox(&dialog);
            box->setRange(-1.0e9, 1.0e9);
            box->setDecimals(6);
            box->setSingleStep(0.1);
            return box;
        };
        auto *x_box = create_offset_box();
        auto *y_box = create_offset_box();
        auto *z_box = create_offset_box();
        form->addRow("dX", x_box);
        form->addRow("dY", y_box);
        form->addRow("dZ", z_box);
        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        const int translated_count = m_3d_widget->translate_units_by_uuid(
            selected_units,
            QVector3D(static_cast<float>(x_box->value()),
                      static_cast<float>(y_box->value()),
                      static_cast<float>(z_box->value())));
        statusBar()->showMessage(
            QString("Translated %1 of %2 selected unit(s); locked units were skipped")
                .arg(translated_count)
                .arg(selected_units.size()),
            5000);
    });
    connect(rotate_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget != nullptr)
        {
            m_3d_widget->set_interaction_mode(
                OCCTWidget::Interaction_Mode::Rotation);
            statusBar()->showMessage(
                "Rotation mode: select an injector and drag a world rotation ring", 5000);
            return;
        }
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }
        if (selected_units.isEmpty())
        {
            statusBar()->showMessage("Select one or more injectors first", 4000);
            return;
        }

        if (selected_units.size() == 1 &&
            m_3d_widget->activate_rotation_gizmo(selected_units.first()))
        {
            statusBar()->showMessage(
                "Rotation gizmo active: drag a rotation ring; press Escape to cancel",
                5000);
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle("Rotate Selected Injectors");
        auto *form = new QFormLayout(&dialog);
        auto create_box = [&dialog](double value)
        {
            auto *box = new QDoubleSpinBox(&dialog);
            box->setRange(-1.0e9, 1.0e9);
            box->setDecimals(6);
            box->setValue(value);
            return box;
        };
        auto *axis_x = create_box(0.0);
        auto *axis_y = create_box(0.0);
        auto *axis_z = create_box(1.0);
        auto *angle = create_box(0.0);
        auto *axis_source = new QComboBox(&dialog);
        axis_source->addItem("Custom", 0);
        QVector3D reference_origin;
        QVector3D reference_x;
        QVector3D reference_z;
        const bool has_reference_frame = m_3d_widget->reference_frame(
            &reference_origin, &reference_x, &reference_z);
        const QVector3D reference_y = QVector3D::crossProduct(reference_z,
                                                               reference_x).normalized();
        if (has_reference_frame)
        {
            axis_source->addItem("Reference X", 1);
            axis_source->addItem("Reference Y", 2);
            axis_source->addItem("Reference Z", 3);
        }
        const auto set_axis_values = [axis_x, axis_y, axis_z](const QVector3D &axis_value)
        {
            axis_x->setValue(axis_value.x());
            axis_y->setValue(axis_value.y());
            axis_z->setValue(axis_value.z());
        };
        connect(axis_source, &QComboBox::currentIndexChanged, &dialog,
                [axis_source, set_axis_values, reference_x, reference_y, reference_z](int index)
        {
            if (index == 1)
            {
                set_axis_values(reference_x);
            }
            else if (index == 2)
            {
                set_axis_values(reference_y);
            }
            else if (index == 3)
            {
                set_axis_values(reference_z);
            }
        });
        form->addRow("Axis Source", axis_source);
        auto *pivot_source = new QComboBox(&dialog);
        pivot_source->addItem("Current Unit", 0);
        if (has_reference_frame)
        {
            pivot_source->addItem("Reference Origin", 1);
        }
        QVector3D assembly_parent_origin;
        bool has_assembly_parent = false;
        for (const QUuid &selected_uuid : selected_units)
        {
            const auto selected_unit = m_3d_widget->unit_hash.value(selected_uuid);
            if (selected_unit != nullptr && !selected_unit->assembly_parent_uuid.isNull())
            {
                const auto parent_unit = m_3d_widget->unit_hash.value(
                    selected_unit->assembly_parent_uuid);
                if (parent_unit != nullptr)
                {
                    assembly_parent_origin = parent_unit->inj.injector_data.pos;
                    has_assembly_parent = true;
                    break;
                }
            }
        }
        if (has_assembly_parent)
        {
            pivot_source->addItem("Assembly Parent", 2);
        }
        form->addRow("Pivot", pivot_source);
        form->addRow("Axis X", axis_x);
        form->addRow("Axis Y", axis_y);
        form->addRow("Axis Z", axis_z);
        form->addRow("Angle (deg)", angle);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             &dialog);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        const int rotated_count = m_3d_widget->rotate_units_by_uuid(
            selected_units,
            QVector3D(static_cast<float>(axis_x->value()),
                      static_cast<float>(axis_y->value()),
                      static_cast<float>(axis_z->value())),
            static_cast<float>(angle->value()),
            pivot_source->currentData().toInt() == 1
                ? reference_origin
                : assembly_parent_origin,
            pivot_source->currentData().toInt() == 1 ||
                pivot_source->currentData().toInt() == 2);
        statusBar()->showMessage(
            QString("Rotated %1 of %2 selected unit(s); locked units were skipped")
                .arg(rotated_count)
            .arg(selected_units.size()),
            5000);
    });
    connect(selection_mode_button, &QPushButton::clicked, this, [this]()
    {
        if (m_3d_widget != nullptr)
        {
            m_3d_widget->set_interaction_mode(
                OCCTWidget::Interaction_Mode::Selection);
            statusBar()->showMessage("Selection mode", 3000);
        }
    });
    connect(assembly_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }
        if (!m_3d_widget->create_assembly(selected_units))
        {
            statusBar()->showMessage(
                "Select at least two ungrouped Units to create an Assembly", 5000);
            return;
        }
        statusBar()->showMessage(
            QString("Created Assembly with %1 member Unit(s)").arg(selected_units.size()),
            5000);
        update_object_list_panel();
    });
    connect(detach_assembly_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        int detached_count = 0;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->detach_from_assembly(uuid))
            {
                ++detached_count;
            }
        }
        statusBar()->showMessage(
            QString("Detached %1 selected Unit(s) from Assembly").arg(detached_count),
            5000);
        update_object_list_panel();
    });
    connect(dissolve_assembly_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }
        int dissolved_count = 0;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->dissolve_assembly(uuid))
            {
                ++dissolved_count;
            }
        }
        statusBar()->showMessage(
            QString("Dissolved %1 Assembly node(s)").arg(dissolved_count), 5000);
        update_object_list_panel();
    });
    connect(material_selected_button, &QPushButton::clicked, this, [this]()
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QList<QUuid> selected_units;
        for (QTreeWidgetItem *item : m_object_list->selectedItems())
        {
            if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
            {
                continue;
            }
            const QUuid uuid(item->data(0, Qt::UserRole).toString());
            if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
            {
                selected_units.append(uuid);
            }
        }
        if (selected_units.isEmpty())
        {
            statusBar()->showMessage("Select one or more injectors first", 4000);
            return;
        }

        const QStringList species_names = m_chemkin_species_names;
        if (species_names.isEmpty())
        {
            statusBar()->showMessage("No Chemkin species are available", 4000);
            return;
        }

        bool accepted = false;
        const QString material = QInputDialog::getItem(
            this,
            "Set Material",
            "Material:",
            species_names,
            0,
            false,
            &accepted);
        if (!accepted)
        {
            return;
        }

        const int changed_count = m_3d_widget->set_material_for_units_by_uuid(
            selected_units, material);
        statusBar()->showMessage(
            QString("Assigned material to %1 of %2 selected unit(s)")
                .arg(changed_count)
                .arg(selected_units.size()),
            5000);
    });

    connect(m_object_list, &QTreeWidget::itemClicked, this,
            [this](QTreeWidgetItem *item, int)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const QString object_id = item->data(0, Qt::UserRole).toString();
        const QString item_kind = item->data(0, Qt::UserRole + 1).toString();
        if (object_id == QStringLiteral("reference"))
        {
            m_3d_widget->select_reference_geometry();
            return;
        }
        if (item_kind != QStringLiteral("unit"))
        {
            return;
        }

        const QUuid uuid(object_id);
        if (uuid.isNull() || !m_3d_widget->select_unit_by_uuid(uuid))
        {
            update_object_list_panel();
        }
    });
    connect(m_object_list, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const QString item_kind = item->data(0, Qt::UserRole + 1).toString();
        if (item_kind != QStringLiteral("unit"))
        {
            if (item_kind == QStringLiteral("reference") &&
                m_3d_widget->select_reference_geometry() &&
                m_reference_geometry_dock != nullptr)
            {
                m_reference_geometry_dock->show();
                m_reference_geometry_dock->raise();
            }
            return;
        }

        const QUuid uuid(item->data(0, Qt::UserRole).toString());
        if (item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
        {
            if (m_3d_widget->select_reference_geometry() &&
                m_reference_geometry_dock != nullptr)
            {
                m_reference_geometry_dock->show();
                m_reference_geometry_dock->raise();
            }
        }
        else if (!uuid.isNull())
        {
            m_3d_widget->edit_unit_by_uuid(uuid);
        }
    });
    connect(m_object_list, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int)
    {
        if (item == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        const bool visible = item->checkState(0) == Qt::Checked;
        const QString object_id = item->data(0, Qt::UserRole).toString();
        const QString item_kind = item->data(0, Qt::UserRole + 1).toString();
        if (item_kind != QStringLiteral("unit") &&
            item_kind != QStringLiteral("reference"))
        {
            return;
        }
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
            [this](const QString &)
    {
        update_object_list_panel();
    });
    m_object_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_object_list, &QTreeWidget::customContextMenuRequested, this,
            [this](const QPoint &position)
    {
        if (m_object_list == nullptr || m_3d_widget == nullptr)
        {
            return;
        }

        QTreeWidgetItem *item = m_object_list->itemAt(position);
        if (item == nullptr)
        {
            return;
        }

        const QString object_id = item->data(0, Qt::UserRole).toString();
        if (object_id == QStringLiteral("reference"))
        {
            QMenu menu(m_object_list);
            QAction *fit_all_action = menu.addAction("Fit All");
            QAction *fit_selected_action = menu.addAction("Fit Selected");
            QAction *clear_face_action = menu.addAction("Clear Selected Face");
            QAction *align_face_action = menu.addAction("Align View to Selected Face");
            align_face_action->setEnabled(m_align_reference_face != nullptr &&
                                           m_align_reference_face->isEnabled());
            QAction *array_fill_face_action = menu.addAction(
                "Open Array/Fill Tools for Selected Face...");
            QAction *assembly_face_action = menu.addAction(
                "Create Assembly on Selected Face...");
            const auto selected_unit_uuids = [this]()
            {
                QList<QUuid> ids;
                if (m_object_list == nullptr || m_3d_widget == nullptr)
                {
                    return ids;
                }
                for (QTreeWidgetItem *selected_item : m_object_list->selectedItems())
                {
                    if (selected_item == nullptr ||
                        selected_item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
                    {
                        continue;
                    }
                    const QUuid uuid(selected_item->data(0, Qt::UserRole).toString());
                    if (!uuid.isNull() && m_3d_widget->unit_hash.contains(uuid))
                    {
                        ids.append(uuid);
                    }
                }
                return ids;
            };
            assembly_face_action->setEnabled(m_3d_widget->has_selected_face() &&
                                              selected_unit_uuids().size() >= 2);
            const auto open_current_unit_menu = [this]()
            {
                if (m_object_list == nullptr)
                {
                    return;
                }
                QTreeWidgetItem *source_item = m_object_list->currentItem();
                if (source_item == nullptr ||
                    source_item->data(0, Qt::UserRole + 1).toString() != QStringLiteral("unit"))
                {
                    statusBar()->showMessage(
                        "Select a mother injector or Assembly first", 4000);
                    return;
                }
                const QPoint source_position =
                    m_object_list->visualItemRect(source_item).center();
                if (!source_position.isNull())
                {
                    emit m_object_list->customContextMenuRequested(source_position);
                }
            };
            menu.addSeparator();
            QAction *lock_action = menu.addAction(
                m_3d_widget->reference_geometry_locked()
                    ? "Unlock Reference Geometry"
                    : "Lock Reference Geometry");
            QAction *clear_reference_action = menu.addAction(
                "Clear Reference Geometry");
            QAction *chosen_action = menu.exec(
                m_object_list->viewport()->mapToGlobal(position));
            if (chosen_action == fit_all_action)
            {
                m_3d_widget->fit_all_view();
            }
            else if (chosen_action == fit_selected_action)
            {
                m_3d_widget->select_reference_geometry();
                m_3d_widget->fit_selected_view();
            }
            else if (chosen_action == clear_face_action)
            {
                m_3d_widget->clear_selection();
            }
            else if (chosen_action == align_face_action)
            {
                m_3d_widget->align_view_to_selected_face();
            }
            else if (chosen_action == array_fill_face_action)
            {
                open_current_unit_menu();
            }
            else if (chosen_action == assembly_face_action)
            {
                const QList<QUuid> selected_unit_ids = selected_unit_uuids();
                if (selected_unit_ids.size() >= 2 &&
                    m_3d_widget->create_assembly(selected_unit_ids))
                {
                    m_3d_widget->attach_unit_to_selected_face(
                        selected_unit_ids.first());
                    update_object_list_panel();
                    mark_project_dirty();
                    statusBar()->showMessage(
                        "Created and attached Assembly to selected face", 4000);
                }
            }
            else if (chosen_action == lock_action)
            {
                m_3d_widget->set_reference_geometry_locked(
                    !m_3d_widget->reference_geometry_locked());
            }
            else if (chosen_action == clear_reference_action)
            {
                const QMessageBox::StandardButton answer = QMessageBox::question(
                    this,
                    "Clear Reference Geometry",
                    "Remove the currently loaded reference geometry?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
                if (answer == QMessageBox::Yes &&
                    m_3d_widget->clear_reference_geometry())
                {
                    mark_project_dirty();
                    save_reference_geometry_state();
                    update_reference_geometry_panel();
                    update_object_list_panel();
                    statusBar()->showMessage(
                        "Reference geometry cleared", 5000);
                }
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
        QAction *fit_selected_action = menu.addAction("Fit Selected");
        QAction *copy_action = menu.addAction("Copy");
        QAction *clone_tree_action = menu.addAction("Clone Unit Tree");
        QAction *paste_action = menu.addAction("Paste to replace");
        paste_action->setEnabled(m_3d_widget->has_copied_unit());
        QAction *rename_action = menu.addAction("Rename");
        QAction *lock_action = menu.addAction(
            m_3d_widget->unit_locked(uuid) ? "Unlock Movement"
                                            : "Lock Movement");
        QAction *follow_array_action = menu.addAction("Follow Array");
        QAction *restore_inheritance_action = menu.addAction(
            "Restore Array Inheritance");
        const std::shared_ptr<Unit> selected_unit = m_3d_widget->unit_hash.value(uuid);
        follow_array_action->setCheckable(true);
        follow_array_action->setChecked(selected_unit != nullptr &&
                                         selected_unit->is_array_child &&
                                         selected_unit->follows_array);
        follow_array_action->setEnabled(selected_unit != nullptr &&
                                        selected_unit->is_array_child);
        restore_inheritance_action->setEnabled(
            selected_unit != nullptr && selected_unit->is_array_child &&
            !selected_unit->follows_array);
        QAction *array_action = menu.addAction("Create Array...");
        QAction *fill_action = menu.addAction("Create Fill...");
        QAction *collapse_action = nullptr;
        if (item->childCount() > 0)
        {
            collapse_action = menu.addAction(
                item->isExpanded() ? "Collapse Children" : "Expand Children");
        }
        QList<QUuid> selected_unit_ids;
        for (QTreeWidgetItem *selected_item : m_object_list->selectedItems())
        {
            const QUuid selected_uuid(selected_item->data(0, Qt::UserRole).toString());
            if (!selected_uuid.isNull() && m_3d_widget->unit_hash.contains(selected_uuid))
            {
                selected_unit_ids.append(selected_uuid);
            }
        }
        if (!selected_unit_ids.contains(uuid))
        {
            selected_unit_ids.append(uuid);
        }
        menu.addSeparator();
        QAction *delete_action = menu.addAction("Delete");
        QAction *chosen_action = menu.exec(m_object_list->viewport()->mapToGlobal(position));
        if (chosen_action == edit_action)
        {
            m_3d_widget->edit_unit_by_uuid(uuid);
        }
        else if (chosen_action == fit_selected_action)
        {
            m_3d_widget->select_unit_by_uuid(uuid);
            m_3d_widget->fit_selected_view();
        }
        else if (chosen_action == copy_action)
        {
            m_3d_widget->copy_unit_by_uuid(uuid);
        }
        else if (chosen_action == clone_tree_action)
        {
            m_3d_widget->clone_unit_tree_by_uuid(uuid);
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
                if (!m_3d_widget->set_unit_name(uuid, new_name))
                {
                    statusBar()->showMessage(
                        "Injector name is empty, unchanged, or already in use", 5000);
                }
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
        else if (chosen_action == array_action)
        {
            const QStringList array_types = {"Linear", "Rotational", "Mirror", "Elliptical"};
            bool accepted = false;
            const QString array_type = QInputDialog::getItem(
                this, "Create Array", "Array type:", array_types, 0,
                false, &accepted);
            if (!accepted)
            {
                return;
            }

            const int count = QInputDialog::getInt(
                this, "Create Array", "Number of children:", 4, 1, 100000,
                1, &accepted);
            if (!accepted)
            {
                return;
            }

            UnitArraySpec spec;
            spec.count = count;
            if (m_3d_widget->reference_geometry_visible() ||
                m_3d_widget->has_selected_face())
            {
                const QString reference_prompt =
                    m_3d_widget->has_selected_face()
                        ? "Use the selected reference face frame?"
                        : "Use the visible reference geometry frame?";
                const auto use_reference = QMessageBox::question(
                    this, "Create Array", reference_prompt,
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (use_reference == QMessageBox::Yes)
                {
                    QVector3D reference_origin;
                    QVector3D reference_x;
                    QVector3D reference_z;
                    if (!m_3d_widget->reference_frame(&reference_origin,
                                                      &reference_x,
                                                      &reference_z))
                    {
                        statusBar()->showMessage(
                            "Reference geometry has no usable coordinate frame", 5000);
                        return;
                    }
                    spec.use_reference_geometry = true;
                    spec.origin = reference_origin;
                    spec.direction = reference_x;
                    spec.plane_normal = reference_z;
                    const auto conform_to_reference = QMessageBox::question(
                        this, "Create Array",
                        "Align injector directions to the reference face normal?",
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    spec.conform_to_reference_normal =
                        conform_to_reference == QMessageBox::Yes;
                }
            }
            if (array_type == "Linear")
            {
                spec.type = UnitArrayType::Linear;
                spec.spacing = static_cast<float>(QInputDialog::getDouble(
                    this, "Linear Array", "Spacing:", 5.0, -1.0e6, 1.0e6,
                    3, &accepted));
            }
            else if (array_type == "Rotational")
            {
                spec.type = UnitArrayType::Rotational;
                spec.direction = QVector3D(1.0f, 0.0f, 0.0f);
                spec.angle_degrees = QInputDialog::getDouble(
                    this, "Rotational Array", "Total angle (degrees):",
                    360.0, -360000.0, 360000.0, 3, &accepted);
                if (!accepted)
                {
                    return;
                }
                spec.spacing = static_cast<float>(QInputDialog::getDouble(
                    this, "Rotational Array", "Axial spacing per child:",
                    0.0, -1.0e6, 1.0e6, 3, &accepted));
                if (!accepted)
                {
                    return;
                }
            }
            else if (array_type == "Mirror")
            {
                spec.type = UnitArrayType::Mirror;
                spec.plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
                spec.count = 2;
            }
            else
            {
                spec.type = UnitArrayType::Elliptical;
                spec.major_radius = static_cast<float>(QInputDialog::getDouble(
                    this, "Elliptical Array", "Major radius:", 10.0,
                    0.0, 1.0e6, 3, &accepted));
                if (!accepted)
                {
                    return;
                }
                spec.minor_radius = static_cast<float>(QInputDialog::getDouble(
                    this, "Elliptical Array", "Minor radius:", 5.0,
                    0.0, 1.0e6, 3, &accepted));
                if (!accepted)
                {
                    return;
                }
                spec.angle_degrees = QInputDialog::getDouble(
                    this, "Elliptical Array", "Total angle (degrees):",
                    360.0, -360000.0, 360000.0, 3, &accepted);
                if (!accepted)
                {
                    return;
                }
            }

            if (!accepted)
            {
                return;
            }
            const int created = m_3d_widget->create_unit_array(uuid, spec);
            statusBar()->showMessage(
                QString("Created %1 array child units").arg(created), 5000);
        }
        else if (chosen_action == fill_action)
        {
            const QStringList fill_types = {"Square", "Hexagonal"};
            bool accepted = false;
            const QString fill_type = QInputDialog::getItem(
                this, "Create Fill", "Fill pattern:", fill_types, 0,
                false, &accepted);
            if (!accepted)
            {
                return;
            }
            UnitFillSpec spec;
            spec.pattern = fill_type == "Hexagonal"
                               ? UnitFillPattern::Hexagonal
                               : UnitFillPattern::Square;
            if (m_3d_widget->reference_geometry_visible() ||
                m_3d_widget->has_selected_face())
            {
                const QString reference_prompt =
                    m_3d_widget->has_selected_face()
                        ? "Use the selected reference face frame?"
                        : "Use the visible reference geometry frame?";
                const auto use_reference = QMessageBox::question(
                    this, "Create Fill", reference_prompt,
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (use_reference == QMessageBox::Yes)
                {
                    QVector3D reference_origin;
                    QVector3D reference_x;
                    QVector3D reference_z;
                    if (!m_3d_widget->reference_frame(&reference_origin,
                                                      &reference_x,
                                                      &reference_z))
                    {
                        statusBar()->showMessage(
                            "Reference geometry has no usable coordinate frame", 5000);
                        return;
                    }
                    spec.use_reference_geometry = true;
                    spec.origin = reference_origin;
                    spec.direction = reference_x;
                    spec.plane_normal = reference_z;
                    spec.conform_to_reference_normal =
                        QMessageBox::question(
                            this, "Create Fill",
                            "Align injector directions to the reference face normal?",
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
                        QMessageBox::Yes;
                }
            }
            spec.rows = QInputDialog::getInt(
                this, "Create Fill", "Rows:", 4, 1, 1000, 1, &accepted);
            if (!accepted)
            {
                return;
            }
            spec.columns = QInputDialog::getInt(
                this, "Create Fill", "Columns:", 4, 1, 1000, 1, &accepted);
            if (!accepted)
            {
                return;
            }
            if (selected_unit_ids.size() > 1)
            {
                const QString weights_text = QInputDialog::getText(
                    this, "Create Fill", "Source weights (comma-separated):",
                    QLineEdit::Normal, QString("1,%1").arg(selected_unit_ids.size() > 2 ? "1,1" : "1"),
                    &accepted);
                if (!accepted)
                {
                    return;
                }
                const QStringList weight_tokens = weights_text.split(',', Qt::SkipEmptyParts);
                if (weight_tokens.size() != selected_unit_ids.size())
                {
                    QMessageBox::warning(this, "Create Fill",
                                         "Enter one positive integer weight for each selected injector.");
                    return;
                }
                for (const QString &token : weight_tokens)
                {
                    bool weight_ok = false;
                    const int weight = token.trimmed().toInt(&weight_ok);
                    if (!weight_ok || weight <= 0)
                    {
                        QMessageBox::warning(this, "Create Fill",
                                             "Source weights must be positive integers.");
                        return;
                    }
                    spec.source_weights.append(weight);
                }
            }
            spec.spacing_x = static_cast<float>(QInputDialog::getDouble(
                this, "Create Fill", "X spacing:", 5.0, -1.0e6, 1.0e6,
                3, &accepted));
            if (!accepted)
            {
                return;
            }
            spec.spacing_y = static_cast<float>(QInputDialog::getDouble(
                this, "Create Fill", "Y spacing:", 5.0, -1.0e6, 1.0e6,
                3, &accepted));
            if (!accepted)
            {
                return;
            }
            spec.circular_boundary = QMessageBox::question(
                this, "Create Fill", "Clip layout to a circular boundary?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
                                     QMessageBox::Yes;
            if (spec.circular_boundary)
            {
                spec.boundary_radius = static_cast<float>(QInputDialog::getDouble(
                    this, "Create Fill", "Boundary radius:", 20.0,
                    0.0, 1.0e6, 3, &accepted));
                if (!accepted)
                {
                    return;
                }
            }
            if (!spec.use_reference_geometry)
            {
                spec.origin = m_3d_widget->unit_hash.value(uuid)
                                  ->inj.injector_data.pos;
            }
            const int created = m_3d_widget->create_unit_fill(
                selected_unit_ids, spec);
            statusBar()->showMessage(
                QString("Created %1 fill child units").arg(created), 5000);
        }
        else if (chosen_action == collapse_action)
        {
            item->setExpanded(!item->isExpanded());
        }
        else if (chosen_action == follow_array_action)
        {
            m_3d_widget->set_unit_follow_array(
                uuid, follow_array_action->isChecked());
            statusBar()->showMessage(
                follow_array_action->isChecked()
                    ? "Selected array child will follow its parent"
                    : "Selected array child is now independent",
                5000);
        }
        else if (chosen_action == restore_inheritance_action)
        {
            if (m_3d_widget->restore_unit_array_inheritance(uuid))
            {
                statusBar()->showMessage(
                    "Selected array child restored to parent inheritance", 5000);
            }
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

    const auto tree_item_key = [](QTreeWidgetItem *item)
    {
        if (item == nullptr)
        {
            return QString();
        }
        const QString explicit_key =
            item->data(0, Qt::UserRole + 4).toString();
        if (!explicit_key.isEmpty())
        {
            return explicit_key;
        }

        const QString kind = item->data(0, Qt::UserRole + 1).toString();
        const QString object_id = item->data(0, Qt::UserRole).toString();
        if (kind == QStringLiteral("unit") ||
            kind == QStringLiteral("reference"))
        {
            return object_id;
        }
        if (kind == QStringLiteral("array_layer"))
        {
            return QStringLiteral("array_layer:%1:%2")
                .arg(item->data(0, Qt::UserRole + 2).toString())
                .arg(item->data(0, Qt::UserRole + 3).toInt());
        }
        if (kind == QStringLiteral("fill"))
        {
            return QStringLiteral("fill:%1")
                .arg(item->data(0, Qt::UserRole + 2).toString());
        }
        return QString();
    };

    for (QTreeWidgetItem *item : m_object_list->all_items())
    {
        const QString key = tree_item_key(item);
        if (!key.isEmpty())
        {
            m_tree_expansion_state.insert(key, item->isExpanded());
        }
    }

    QSet<QString> selected_object_ids;
    for (QTreeWidgetItem *item : m_object_list->selectedItems())
    {
        const QString object_id = item->data(0, Qt::UserRole).toString();
        if (!object_id.isEmpty())
        {
            selected_object_ids.insert(object_id);
        }
    }
    const QString current_object_id = m_object_list->currentItem() == nullptr
                                          ? QString()
                                          : m_object_list->currentItem()
                                                ->data(0, Qt::UserRole).toString();
    const QSignalBlocker blocker(m_object_list);
    m_object_list->clear();

    if (!m_3d_widget->geometry.getShape().IsNull())
    {
        QString reference_name = QStringLiteral("Reference Geometry");
        if (m_3d_widget->reference_geometry_locked())
        {
            reference_name = QStringLiteral("[Locked] ") + reference_name;
        }
        auto *reference_item = new QTreeWidgetItem(m_object_list,
                                                   QStringList{reference_name});
        reference_item->setData(0, Qt::UserRole, QStringLiteral("reference"));
        reference_item->setData(0, Qt::UserRole + 1, QStringLiteral("reference"));
        reference_item->setData(0, Qt::UserRole + 4,
                                QStringLiteral("reference"));
        reference_item->setToolTip(0,
            QString("File: %1\nVisible: %2\nLocked: %3")
                .arg(m_3d_widget->geometry.file_path(),
                     m_3d_widget->reference_geometry_visible()
                         ? QStringLiteral("Yes")
                         : QStringLiteral("No"),
                     m_3d_widget->reference_geometry_locked()
                         ? QStringLiteral("Yes")
                         : QStringLiteral("No")));
        reference_item->setFlags(reference_item->flags() | Qt::ItemIsUserCheckable);
        reference_item->setCheckState(0, m_3d_widget->reference_geometry_visible()
                                          ? Qt::Checked
                                          : Qt::Unchecked);
    }

    const auto injection_type_name = [](Injection_Type type)
    {
        switch (type)
        {
        case single: return QStringLiteral("Single");
        case group: return QStringLiteral("Group");
        case surface: return QStringLiteral("Surface");
        case volume: return QStringLiteral("Volume");
        case cone: return QStringLiteral("Cone");
        case plain_oriface_atomizer: return QStringLiteral("Plain Orifice Atomizer");
        case pressure_swirl_atomizer: return QStringLiteral("Pressure Swirl Atomizer");
        case air_blast_atomizer: return QStringLiteral("Air Blast Atomizer");
        case flat_fan_atomizer: return QStringLiteral("Flat Fan Atomizer");
        case effervescent_atomizer: return QStringLiteral("Effervescent Atomizer");
        case file_: return QStringLiteral("File");
        case condensate: return QStringLiteral("Condensate");
        }
        return QStringLiteral("Unknown");
    };

    const auto particle_type_name = [](DPM_Type type)
    {
        switch (type)
        {
        case Massless: return QStringLiteral("Massless");
        case Inert: return QStringLiteral("Inert");
        case Droplet: return QStringLiteral("Droplet");
        case Combusting: return QStringLiteral("Combusting");
        case Multicomponent: return QStringLiteral("Multicomponent");
        }
        return QStringLiteral("Unknown");
    };

    const auto unit_display_name = [this](const std::shared_ptr<Unit> &unit)
    {
        if (unit == nullptr)
        {
            return QStringLiteral("<invalid>");
        }
        QString name = unit->inj.injector_data.name.trimmed();
        if (name.isEmpty())
        {
            name = unit->inj.uuid.toString(QUuid::WithoutBraces);
        }
        if (m_3d_widget->unit_locked(unit->inj.uuid))
        {
            name = QStringLiteral("[Locked] ") + name;
        }
        if (unit->type == Assebly)
        {
            name += QStringLiteral(" [Assembly]");
        }
        return name;
    };

    const auto configure_unit_item = [&](QTreeWidgetItem *item,
                                         const std::shared_ptr<Unit> &unit)
    {
        if (item == nullptr || unit == nullptr)
        {
            return;
        }
        const Unit &value = *unit;
        const QUuid uuid = value.inj.uuid;
        item->setData(0, Qt::UserRole, uuid.toString(QUuid::WithoutBraces));
        item->setData(0, Qt::UserRole + 1, QStringLiteral("unit"));
        item->setData(0, Qt::UserRole + 4,
                      uuid.toString(QUuid::WithoutBraces));
        item->setToolTip(0,
            QString("Injection: %1\nParticle: %2\nMaterial: %3\nAssembly parent: %4\nUUID: %5")
                .arg(injection_type_name(value.inj.injector_data.injection_type),
                     particle_type_name(value.inj.injector_data.type),
                     value.inj.injector_data.material.trimmed().isEmpty()
                         ? QStringLiteral("<none>")
                         : value.inj.injector_data.material,
                     value.assembly_parent_uuid.isNull()
                         ? QStringLiteral("<none>")
                         : value.assembly_parent_uuid.toString(QUuid::WithoutBraces),
                     uuid.toString(QUuid::WithoutBraces)));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, m_3d_widget->unit_visible(uuid)
                                ? Qt::Checked
                                : Qt::Unchecked);
    };

    std::function<void(const std::shared_ptr<Unit> &, QTreeWidgetItem *, bool)> append_unit;
    append_unit = [&](const std::shared_ptr<Unit> &unit,
                      QTreeWidgetItem *parent_item,
                      bool group_array_layers)
    {
        if (unit == nullptr)
        {
            return;
        }
        QTreeWidgetItem *item = parent_item == nullptr
            ? new QTreeWidgetItem(m_object_list,
                                  QStringList{unit_display_name(unit)})
            : new QTreeWidgetItem(parent_item,
                                  QStringList{unit_display_name(unit)});
        configure_unit_item(item, unit);

        if (!group_array_layers)
        {
            for (const std::shared_ptr<Unit> &child : unit->child_units)
            {
                append_unit(child, item, false);
            }
            return;
        }

        QMap<int, QList<std::shared_ptr<Unit>>> array_layers;
        QList<std::shared_ptr<Unit>> fill_children;
        for (const std::shared_ptr<Unit> &child : unit->child_units)
        {
            if (child == nullptr)
            {
                continue;
            }
            if (child->is_array_child && child->array_layer > 0)
            {
                array_layers[child->array_layer].append(child);
            }
            else if (child->is_array_child)
            {
                fill_children.append(child);
            }
            else if (child->assembly_parent_uuid == unit->inj.uuid)
            {
                append_unit(child, item, true);
            }
        }

        for (auto layer_it = array_layers.cbegin();
             layer_it != array_layers.cend(); ++layer_it)
        {
            auto *layer_item = new QTreeWidgetItem(
                item,
                QStringList{QStringLiteral("Array Layer %1 (%2 instances)")
                                .arg(layer_it.key())
                                .arg(layer_it.value().size())});
            layer_item->setData(0, Qt::UserRole, QString());
            layer_item->setData(0, Qt::UserRole + 1, QStringLiteral("array_layer"));
            layer_item->setData(0, Qt::UserRole + 2,
                                unit->inj.uuid.toString(QUuid::WithoutBraces));
            layer_item->setData(0, Qt::UserRole + 3, layer_it.key());
            layer_item->setData(
                0, Qt::UserRole + 4,
                QStringLiteral("array_layer:%1:%2")
                    .arg(unit->inj.uuid.toString(QUuid::WithoutBraces))
                    .arg(layer_it.key()));
            layer_item->setFlags(Qt::ItemIsEnabled);
            layer_item->setExpanded(true);
            for (const std::shared_ptr<Unit> &child : layer_it.value())
            {
                append_unit(child, layer_item, false);
            }
        }

        if (!fill_children.isEmpty())
        {
            auto *fill_item = new QTreeWidgetItem(
                item,
                QStringList{QStringLiteral("Fill Instances (%1)")
                                .arg(fill_children.size())});
            fill_item->setData(0, Qt::UserRole, QString());
            fill_item->setData(0, Qt::UserRole + 1, QStringLiteral("fill"));
            fill_item->setData(0, Qt::UserRole + 2,
                               unit->inj.uuid.toString(QUuid::WithoutBraces));
            fill_item->setData(
                0, Qt::UserRole + 4,
                QStringLiteral("fill:%1")
                    .arg(unit->inj.uuid.toString(QUuid::WithoutBraces)));
            fill_item->setFlags(Qt::ItemIsEnabled);
            fill_item->setExpanded(true);
            for (const std::shared_ptr<Unit> &child : fill_children)
            {
                append_unit(child, fill_item, false);
            }
        }
    };

    QList<std::shared_ptr<Unit>> roots;
    for (auto it = m_3d_widget->unit_hash.constBegin();
         it != m_3d_widget->unit_hash.constEnd(); ++it)
    {
        const std::shared_ptr<Unit> unit = it.value();
        if (unit == nullptr || unit->is_array_child)
        {
            continue;
        }
        if (!unit->assembly_parent_uuid.isNull() &&
            m_3d_widget->unit_hash.contains(unit->assembly_parent_uuid))
        {
            continue;
        }
        roots.append(unit);
    }
    std::sort(roots.begin(), roots.end(),
              [](const std::shared_ptr<Unit> &left,
                 const std::shared_ptr<Unit> &right)
              {
                  const QString left_name = left == nullptr
                      ? QString() : left->inj.injector_data.name;
                  const QString right_name = right == nullptr
                      ? QString() : right->inj.injector_data.name;
                  return QString::compare(left_name, right_name,
                                          Qt::CaseInsensitive) < 0;
              });
    for (const std::shared_ptr<Unit> &root : roots)
    {
        append_unit(root, nullptr, true);
    }

    for (QTreeWidgetItem *item : m_object_list->all_items())
    {
        const QString key = tree_item_key(item);
        const QString kind = item->data(0, Qt::UserRole + 1).toString();
        if (m_tree_expansion_state.contains(key))
        {
            item->setExpanded(m_tree_expansion_state.value(key));
        }
        else if (kind == QStringLiteral("array_layer") ||
                 kind == QStringLiteral("fill"))
        {
            item->setExpanded(true);
        }
    }

    const QString filter = m_object_filter == nullptr
        ? QString() : m_object_filter->text().trimmed();
    std::function<bool(QTreeWidgetItem *)> apply_filter;
    apply_filter = [&](QTreeWidgetItem *item)
    {
        if (item == nullptr)
        {
            return false;
        }
        const bool self_matches = filter.isEmpty() ||
            item->text(0).contains(filter, Qt::CaseInsensitive) ||
            item->data(0, Qt::UserRole).toString().contains(filter,
                                                          Qt::CaseInsensitive) ||
            item->toolTip(0).contains(filter, Qt::CaseInsensitive);
        bool child_matches = false;
        for (int index = 0; index < item->childCount(); ++index)
        {
            child_matches = apply_filter(item->child(index)) || child_matches;
        }
        item->setHidden(!self_matches && !child_matches);
        if (child_matches && !filter.isEmpty())
        {
            item->setExpanded(true);
        }
        return self_matches || child_matches;
    };
    for (int index = 0; index < m_object_list->topLevelItemCount(); ++index)
    {
        apply_filter(m_object_list->topLevelItem(index));
    }

    for (QTreeWidgetItem *item : m_object_list->all_items())
    {
        const QString object_id = item->data(0, Qt::UserRole).toString();
        if (!object_id.isEmpty() && selected_object_ids.contains(object_id))
        {
            item->setSelected(true);
        }
        if (!object_id.isEmpty() && object_id == current_object_id)
        {
            m_object_list->setCurrentItem(item, QItemSelectionModel::NoUpdate);
            m_object_list->scrollToItem(item);
        }
    }

    const QPalette palette = m_object_list->palette();
    for (QTreeWidgetItem *item : m_object_list->all_items())
    {
        item->setForeground(0, item->isSelected()
                                ? QBrush(QColor("#5AA9FF"))
                                : palette.brush(QPalette::Text));
    }
}

void MainWindow::update_object_list_selection(const QUuid &uuid,
                                               bool reference_geometry)
{
    if (m_object_list == nullptr)
    {
        return;
    }

    if (reference_geometry && m_reference_geometry_dock != nullptr)
    {
        m_reference_geometry_dock->show();
        m_reference_geometry_dock->raise();
        update_reference_geometry_panel();
    }

    const QSignalBlocker blocker(m_object_list);
    m_object_list->clearSelection();
    const QString object_id = reference_geometry
                                  ? QStringLiteral("reference")
                                  : uuid.toString(QUuid::WithoutBraces);
    for (int row = 0; row < m_object_list->count(); ++row)
    {
        QTreeWidgetItem *item = m_object_list->item(row);
        if (item->data(0, Qt::UserRole).toString() == object_id)
        {
            m_object_list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
            m_object_list->scrollToItem(item);
            break;
        }
    }
    update_unit_position_controls();
}

void MainWindow::update_unit_position_controls()
{
    if (m_unit_position_x == nullptr || m_unit_position_y == nullptr ||
        m_unit_position_z == nullptr || m_object_list == nullptr ||
        m_3d_widget == nullptr || m_unit_direction_x == nullptr ||
        m_unit_direction_y == nullptr || m_unit_direction_z == nullptr ||
        m_unit_pitch == nullptr || m_unit_yaw == nullptr ||
        m_unit_target_x == nullptr || m_unit_target_y == nullptr ||
        m_unit_target_z == nullptr || m_unit_target_scope == nullptr)
    {
        return;
    }

    QTreeWidgetItem *item = m_object_list->currentItem();
    if (item == nullptr || item->data(0, Qt::UserRole).toString() == QStringLiteral("reference"))
    {
        m_unit_position_group->hide();
        m_unit_direction_group->hide();
        m_unit_position_x->setEnabled(false);
        m_unit_position_y->setEnabled(false);
        m_unit_position_z->setEnabled(false);
        m_unit_direction_x->setEnabled(false);
        m_unit_direction_y->setEnabled(false);
        m_unit_direction_z->setEnabled(false);
        m_unit_pitch->setEnabled(false);
        m_unit_yaw->setEnabled(false);
        m_unit_target_x->setEnabled(false);
        m_unit_target_y->setEnabled(false);
        m_unit_target_z->setEnabled(false);
        m_unit_target_scope->setEnabled(false);
        m_unit_direction_mode->setEnabled(false);
        return;
    }

    const QUuid uuid(item->data(0, Qt::UserRole).toString());
    if (uuid.isNull() || !m_3d_widget->unit_hash.contains(uuid))
    {
        m_unit_position_group->hide();
        m_unit_direction_group->hide();
        m_unit_position_x->setEnabled(false);
        m_unit_position_y->setEnabled(false);
        m_unit_position_z->setEnabled(false);
        m_unit_direction_x->setEnabled(false);
        m_unit_direction_y->setEnabled(false);
        m_unit_direction_z->setEnabled(false);
        m_unit_pitch->setEnabled(false);
        m_unit_yaw->setEnabled(false);
        m_unit_target_x->setEnabled(false);
        m_unit_target_y->setEnabled(false);
        m_unit_target_z->setEnabled(false);
        m_unit_target_scope->setEnabled(false);
        m_unit_direction_mode->setEnabled(false);
        return;
    }

    const QVector3D position = m_3d_widget->unit_position_by_uuid(uuid);
    const QVector3D direction = m_3d_widget->unit_direction_by_uuid(uuid);
    double pitch = 0.0;
    double yaw = 0.0;
    const bool has_pitch_yaw = m_3d_widget->unit_single_pitch_yaw_by_uuid(
        uuid, &pitch, &yaw);
    const QVector3D target = m_3d_widget->unit_single_target_by_uuid(uuid);
    const Single_Target_Scope target_scope =
        m_3d_widget->unit_single_target_scope_by_uuid(uuid);
    const std::shared_ptr<Unit> unit = m_3d_widget->unit_hash.value(uuid);
    const bool has_target = unit != nullptr &&
        unit->inj.injector_data.injection_type == single &&
        unit->inj.injector_data.single_direction_mode == Single_Direction_Mode::Target_Hitpoint;
    const QSignalBlocker x_blocker(m_unit_position_x);
    const QSignalBlocker y_blocker(m_unit_position_y);
    const QSignalBlocker z_blocker(m_unit_position_z);
    const QSignalBlocker direction_x_blocker(m_unit_direction_x);
    const QSignalBlocker direction_y_blocker(m_unit_direction_y);
    const QSignalBlocker direction_z_blocker(m_unit_direction_z);
    const QSignalBlocker pitch_blocker(m_unit_pitch);
    const QSignalBlocker yaw_blocker(m_unit_yaw);
    const QSignalBlocker target_x_blocker(m_unit_target_x);
    const QSignalBlocker target_y_blocker(m_unit_target_y);
    const QSignalBlocker target_z_blocker(m_unit_target_z);
    const QSignalBlocker target_scope_blocker(m_unit_target_scope);
    const QString display_length = UnitSystem::preferred_display_unit("m");
    const QString display_angle = UnitSystem::preferred_display_unit("deg");
    for (QDoubleSpinBox *box : {m_unit_position_x, m_unit_position_y,
                                m_unit_position_z, m_unit_target_x,
                                m_unit_target_y, m_unit_target_z})
    {
        box->setSuffix(" " + display_length);
    }
    m_unit_pitch->setSuffix(" " + display_angle);
    m_unit_yaw->setSuffix(" " + display_angle);
    const auto from_storage = [&display_length](double value)
    {
        bool ok = false;
        const double converted = UnitSystem::convert(value, "m", display_length, &ok);
        return ok ? converted : value;
    };
    m_unit_position_x->setValue(from_storage(position.x()));
    m_unit_position_y->setValue(from_storage(position.y()));
    m_unit_position_z->setValue(from_storage(position.z()));
    m_unit_direction_x->setValue(direction.x());
    m_unit_direction_y->setValue(direction.y());
    m_unit_direction_z->setValue(direction.z());
    m_unit_pitch->setValue(pitch);
    m_unit_yaw->setValue(yaw);
    m_unit_target_x->setValue(from_storage(target.x()));
    m_unit_target_y->setValue(from_storage(target.y()));
    m_unit_target_z->setValue(from_storage(target.z()));
    const int target_scope_index = m_unit_target_scope->findData(
        static_cast<int>(target_scope));
    if (target_scope_index >= 0)
    {
        m_unit_target_scope->setCurrentIndex(target_scope_index);
    }
    const bool editable_unit = unit != nullptr && unit->type != Assebly &&
        !m_3d_widget->unit_locked(uuid);
    const bool show_inspector = unit != nullptr && unit->type != Assebly;
    const bool show_direction = show_inspector &&
        unit->inj.injector_data.injection_type != volume;
    const bool is_single = unit->inj.injector_data.injection_type == single;
    const int direction_mode_index = m_unit_direction_mode->findData(
        static_cast<int>(unit->inj.injector_data.single_direction_mode));
    {
        const QSignalBlocker mode_blocker(m_unit_direction_mode);
        m_unit_direction_mode->setCurrentIndex(
            direction_mode_index >= 0 ? direction_mode_index : 0);
    }
    m_unit_direction_stack->setCurrentIndex(
        direction_mode_index >= 0 ? direction_mode_index : 0);
    m_unit_position_group->setVisible(show_inspector);
    m_unit_direction_group->setVisible(show_direction);
    m_unit_direction_mode->setVisible(is_single);
    m_unit_direction_mode->setEnabled(is_single && editable_unit);
    m_unit_position_x->setEnabled(editable_unit);
    m_unit_position_y->setEnabled(editable_unit);
    m_unit_position_z->setEnabled(editable_unit);
    const bool editable_direction = editable_unit &&
        !(unit->inj.injector_data.injection_type == single &&
          unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Vector);
    m_unit_direction_x->setEnabled(editable_direction);
    m_unit_direction_y->setEnabled(editable_direction);
    m_unit_direction_z->setEnabled(editable_direction);
    m_unit_pitch->setEnabled(has_pitch_yaw && editable_unit);
    m_unit_yaw->setEnabled(has_pitch_yaw && editable_unit);
    const bool editable_target = has_target && editable_unit;
    m_unit_target_x->setEnabled(editable_target);
    m_unit_target_y->setEnabled(editable_target);
    m_unit_target_z->setEnabled(editable_target);
    m_unit_target_scope->setEnabled(editable_target);
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
        QTreeWidgetItem *item = m_object_list->item(row);
        if (item->data(0, Qt::UserRole).toString() ==
            uuid.toString(QUuid::WithoutBraces))
        {
            item->setText(0, display_name);
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

    m_3d_widget->begin_reference_transform_transaction();
    m_3d_widget->set_reference_transform(
        QVector3D(static_cast<float>(m_reference_position_x->value()),
                  static_cast<float>(m_reference_position_y->value()),
                  static_cast<float>(m_reference_position_z->value())),
        QVector3D(static_cast<float>(m_reference_rotation_x->value()),
                  static_cast<float>(m_reference_rotation_y->value()),
                  static_cast<float>(m_reference_rotation_z->value())));
    m_3d_widget->finish_reference_transform_transaction();
    mark_project_dirty();
    save_reference_geometry_state();
}

QList<Unit> MainWindow::build_test_injector_units() const
{
    QList<Unit> result;

    // Preview geometry is authored in convenient millimeter-sized numbers;
    // convert only this built-in showcase to the application's meter base
    // unit. Imported DPM/project data is never passed through this path.
    constexpr float kPreviewGeometryScale = 1.0e-3f;
    auto scale_preview_geometry = [=](Unit &unit)
    {
        Injector &injector = unit.inj.injector_data;
        const auto scale_vector = [](QVector3D &value)
        {
            value *= kPreviewGeometryScale;
        };
        scale_vector(injector.pos);
        scale_vector(injector.pos2);
        scale_vector(injector.ff_center);
        scale_vector(injector.ff_virtual_origin);
        scale_vector(injector.volume_bgeom_min);
        scale_vector(injector.volume_bgeom_max);
        if (unit.has_array_spec)
        {
            scale_vector(unit.array_spec.origin);
            unit.array_spec.spacing *= kPreviewGeometryScale;
            unit.array_spec.major_radius *= kPreviewGeometryScale;
            unit.array_spec.minor_radius *= kPreviewGeometryScale;
        }
        if (unit.has_fill_spec)
        {
            scale_vector(unit.fill_spec.origin);
            unit.fill_spec.spacing_x *= kPreviewGeometryScale;
            unit.fill_spec.spacing_y *= kPreviewGeometryScale;
            unit.fill_spec.boundary_radius *= kPreviewGeometryScale;
        }
        injector.diameter *= kPreviewGeometryScale;
        injector.diameter2 *= kPreviewGeometryScale;
        injector.inner_diameter *= kPreviewGeometryScale;
        injector.outer_diameter *= kPreviewGeometryScale;
        injector.radius *= kPreviewGeometryScale;
        injector.inner_radius *= kPreviewGeometryScale;
        injector.volume_bgeom_radius *= kPreviewGeometryScale;
        injector.plain_length *= kPreviewGeometryScale;
        injector.ff_oriface_width *= kPreviewGeometryScale;
        injector.stagger_radius *= kPreviewGeometryScale;
    };

    auto finalize_unit = [&](Unit &unit)
    {
        if (!m_chemkin_species_names.isEmpty())
        {
            unit.inj.injector_data.material = m_chemkin_species_names.first();
            unit.inj.injector_data.evaporating_species = m_chemkin_species_names.first();
        }
        scale_preview_geometry(unit);
        if (!unit.inj.create_injector())
        {
            qDebug() << "Failed to build test injector:" << unit.inj.injector_data.name;
        }
        result.append(unit);
    };

    // Keep the startup scene intentionally small: one array source and one
    // independent injector are enough to exercise selection and array edits.
    {
        Unit unit = make_test_unit("linear_array_source", single,
                                   QVector3D(18.0f, -12.0f, 0.0f));
        unit.type = array;
        unit.has_array_spec = true;
        unit.array_spec.type = UnitArrayType::Linear;
        unit.array_spec.count = 4;
        unit.array_spec.spacing = 4.0f;
        unit.array_spec.direction = QVector3D(0.0f, 1.0f, 0.0f);
        unit.array_spec.origin = unit.inj.injector_data.pos;
        unit.array_spec.plane_normal = QVector3D(1.0f, 0.0f, 0.0f);
        finalize_unit(unit);
    }

    {
        Unit unit = make_test_unit("single_demo", single,
                                   QVector3D(-18.0f, 0.0f, 0.0f));
        unit.inj.injector_data.single_direction_mode = Single_Direction_Mode::Pitch_Yaw;
        unit.inj.injector_data.single_pitch_degrees = 8.0;
        unit.inj.injector_data.single_yaw_degrees = -12.0;
        finalize_unit(unit);
    }

    return result;
}





// void MainWindow::on_actionNew_triggered()
// {

// }

