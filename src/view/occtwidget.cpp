#include "occtwidget.h"
#include "runtime_debug.h"
#include <AIS_ViewCube.hxx>
#include <QTimer>
#include <QtMath>
#include <QMessageBox>

#include <algorithm>
#include <cmath>

namespace
{
QVector3D to_qvector3d(const gp_Pnt &point)
{
    return QVector3D(static_cast<float>(point.X()),
                     static_cast<float>(point.Y()),
                     static_cast<float>(point.Z()));
}

QVector3D injector_frame_origin(const Injector &injector)
{
    return injector.injection_type == flat_fan_atomizer
               ? injector.ff_center
               : injector.pos;
}

QVector3D injector_frame_direction(const Injector &injector)
{
    QVector3D direction;
    switch (injector.injection_type)
    {
    case single:
        if (injector.single_direction_mode == Single_Direction_Mode::Pitch_Yaw)
        {
            const double pitch = qDegreesToRadians(injector.single_pitch_degrees);
            const double yaw = qDegreesToRadians(injector.single_yaw_degrees);
            direction = QVector3D(
                static_cast<float>(std::cos(pitch) * std::cos(yaw)),
                static_cast<float>(std::cos(pitch) * std::sin(yaw)),
                static_cast<float>(std::sin(pitch)));
        }
        else if (injector.single_direction_mode == Single_Direction_Mode::Target_Hitpoint)
        {
            direction = injector.single_target_hitpoint - injector.pos;
        }
        else
        {
            direction = injector.vel;
        }
        break;
    case cone:
        direction = injector.axis;
        break;
    case flat_fan_atomizer:
        direction = injector.ff_normal;
        break;
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case effervescent_atomizer:
        direction = injector.atomizer_axis;
        break;
    default:
        direction = injector.vel;
        break;
    }

    if (!std::isfinite(direction.x()) ||
        !std::isfinite(direction.y()) ||
        !std::isfinite(direction.z()) ||
        direction.lengthSquared() <= 1.0e-12f)
    {
        direction = QVector3D(1.0f, 0.0f, 0.0f);
    }
    return direction.normalized();
}

Handle(AIS_Trihedron) make_local_trihedron(const gp_Ax2 &axis,
                                           Standard_Real size)
{
    Handle(Geom_Axis2Placement) placement = new Geom_Axis2Placement(axis);
    Handle(AIS_Trihedron) trihedron = new AIS_Trihedron(placement);
    trihedron->SetSize(std::max(size, 1.0));
    trihedron->SetDatumDisplayMode(Prs3d_DM_Shaded);
    trihedron->SetDatumPartColor(Prs3d_DP_XAxis, Quantity_NOC_RED);
    trihedron->SetDatumPartColor(Prs3d_DP_XArrow, Quantity_NOC_RED);
    trihedron->SetDatumPartColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);
    trihedron->SetDatumPartColor(Prs3d_DP_YArrow, Quantity_NOC_GREEN);
    trihedron->SetDatumPartColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);
    trihedron->SetDatumPartColor(Prs3d_DP_ZArrow, Quantity_NOC_BLUE);
    trihedron->SetTextColor(Prs3d_DP_XAxis, Quantity_NOC_RED);
    trihedron->SetTextColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);
    trihedron->SetTextColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);
    return trihedron;
}

bool face_local_axis(const TopoDS_Face &face, gp_Ax2 &axis)
{
    if (face.IsNull())
    {
        return false;
    }

    GProp_GProps properties;
    BRepGProp::SurfaceProperties(face, properties);
    if (properties.Mass() <= Precision::Confusion())
    {
        return false;
    }

    BRepAdaptor_Surface surface(face, Standard_True);
    Standard_Real u_min = 0.0;
    Standard_Real u_max = 0.0;
    Standard_Real v_min = 0.0;
    Standard_Real v_max = 0.0;
    BRepTools::UVBounds(face, u_min, u_max, v_min, v_max);

    try
    {
        gp_Pnt point;
        gp_Vec du;
        gp_Vec dv;
        surface.D1(0.5 * (u_min + u_max),
                   0.5 * (v_min + v_max),
                   point,
                   du,
                   dv);
        gp_Vec normal_vector = du.Crossed(dv);
        if (normal_vector.SquareMagnitude() <= Precision::Confusion())
        {
            return false;
        }

        gp_Dir normal(normal_vector);
        if (face.Orientation() == TopAbs_REVERSED)
        {
            normal.Reverse();
        }

        if (du.SquareMagnitude() > Precision::Confusion())
        {
            axis = gp_Ax2(point, normal, gp_Dir(du));
        }
        else if (dv.SquareMagnitude() > Precision::Confusion())
        {
            axis = gp_Ax2(point, normal, gp_Dir(dv));
        }
        else
        {
            axis = gp_Ax2(point, normal);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}
}




OCCTWidget::OCCTWidget(QWidget *parent) : QWidget(parent), m_dpi_scale(this->devicePixelRatioF())
{
    //配置QWidget
    setBackgroundRole( QPalette::NoRole );  //无背景
    setMouseTracking( true );   //开启鼠标位置追踪
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_NativeWindow);
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::DefaultContextMenu);

    if (m_context.IsNull()) // 若未定义交互环境
    {
        m_initialize_context(); // 初始化交互环境
    }
}

OCCTWidget::~OCCTWidget()
{
    runtime_debug::trace("OCCTWidget destructor begin");
    m_is_destroying = true;
    discard_auxiliary_dialogs();

    try
    {
        selected_shape.Nullify();
        selected_face.Nullify();
        clear_unit_local_coordinate_frames();
        clear_reference_face_coordinate_frames();

        if (!m_context.IsNull())
        {
            m_context->ClearSelected(Standard_False);
            m_context->RemoveAll(Standard_False);
            m_context.Nullify();
        }

        if (!m_view.IsNull())
        {
            m_view->Remove();
            m_view.Nullify();
        }
    }
    catch (...)
    {
    }

    unit_hash.clear();
    m_unit_visibility.clear();
    m_unit_locks.clear();
    reference_geometry.Nullify();
    base_geometry.Nullify();
    trihedron_main.Nullify();
    axis_placement_main.Nullify();
    face_trihedron.Nullify();
    face_axis_placement.Nullify();
    emit face_reference_changed(false);
    m_viewer.Nullify();
    m_graphic_driver.Nullify();
    runtime_debug::trace("OCCTWidget destructor end");
}

void OCCTWidget::close_auxiliary_dialogs()
{
    runtime_debug::trace(
        QString("OCCTWidget::close_auxiliary_dialogs begin, count=%1").arg(m_open_edit_dialogs.size()));
    const QList<QPointer<unit_edit_dialog>> dialogs = m_open_edit_dialogs;
    m_open_edit_dialogs.clear();

    for (const QPointer<unit_edit_dialog> &dialog : dialogs)
    {
        if (dialog == nullptr)
        {
            continue;
        }

        runtime_debug::trace(
            QString("Closing unit_edit_dialog %1")
                .arg(reinterpret_cast<quintptr>(dialog.data()), 0, 16));
        dialog->close();
    }
    runtime_debug::trace("OCCTWidget::close_auxiliary_dialogs end");
}

void OCCTWidget::discard_auxiliary_dialogs()
{
    runtime_debug::trace(
        QString("OCCTWidget::discard_auxiliary_dialogs begin, count=%1").arg(m_open_edit_dialogs.size()));
    const QList<QPointer<unit_edit_dialog>> dialogs = m_open_edit_dialogs;
    m_open_edit_dialogs.clear();

    for (const QPointer<unit_edit_dialog> &dialog : dialogs)
    {
        if (dialog == nullptr)
        {
            continue;
        }

        dialog->close();
        delete dialog.data();
    }
    runtime_debug::trace("OCCTWidget::discard_auxiliary_dialogs end");
}

void OCCTWidget::display_units(const QList<Unit> &units, bool clear_existing)
{
    if (m_context.IsNull())
    {
        return;
    }

    if (clear_existing)
    {
        discard_auxiliary_dialogs();
        clear_unit_local_coordinate_frames();
        clear_move_history();
        clear_edit_history();
        clear_delete_history();
        m_copied_unit.reset();
        for (auto it = unit_hash.begin(); it != unit_hash.end(); ++it)
        {
            if (it.value() != nullptr && !it.value()->ais_display.IsNull())
            {
                m_context->Remove(it.value()->ais_display, Standard_False);
            }
        }
        unit_hash.clear();
        m_unit_visibility.clear();
        m_unit_locks.clear();
    }

    for (int i = 0; i < units.size(); ++i)
    {
        const Unit &unit = units[i];
        const std::shared_ptr<Unit> stored_unit = std::make_shared<Unit>(unit);
        unit_hash.insert(stored_unit->inj.uuid, stored_unit);
        m_unit_visibility.insert(stored_unit->inj.uuid, true);
        m_unit_locks.insert(stored_unit->inj.uuid, false);

        stored_unit->ais_display->Set(stored_unit->inj.shape);
        stored_unit->u_owner->set_unit(stored_unit.get());
        stored_unit->ais_display->SetOwner(stored_unit->u_owner);
        stored_unit->ais_display->SetColor(
            color_for_material(stored_unit->inj.injector_data.material));
        stored_unit->ais_display->SetTransparency(
            stored_unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);

        m_context->Activate(stored_unit->ais_display, TopAbs_SHAPE, Standard_True);
        m_context->Display(stored_unit->ais_display, Standard_False);
    }

    for (auto it = unit_hash.begin(); it != unit_hash.end(); ++it)
    {
        const std::shared_ptr<Unit> parent = it.value();
        if (parent == nullptr || parent->assembly_child_uuids.isEmpty())
        {
            continue;
        }
        parent->child_units.clear();
        for (const QUuid &child_uuid : parent->assembly_child_uuids)
        {
            const std::shared_ptr<Unit> child = unit_hash.value(child_uuid);
            if (child != nullptr)
            {
                child->assembly_parent_uuid = parent->inj.uuid;
                parent->child_units.append(child);
            }
        }
    }

    const QList<QUuid> array_sources = [&]()
    {
        QList<QUuid> ids;
        for (auto it = unit_hash.constBegin(); it != unit_hash.constEnd(); ++it)
        {
            if (it.value() != nullptr && it.value()->has_array_spec &&
                !it.value()->is_array_child)
            {
                ids.append(it.key());
            }
        }
        return ids;
    }();
    for (const QUuid &source_uuid : array_sources)
    {
        rebuild_unit_array(source_uuid);
    }

    const QList<QUuid> fill_sources = [&]()
    {
        QList<QUuid> ids;
        for (auto it = unit_hash.constBegin(); it != unit_hash.constEnd(); ++it)
        {
            if (it.value() != nullptr && it.value()->has_fill_spec &&
                !it.value()->is_array_child)
            {
                ids.append(it.key());
            }
        }
        return ids;
    }();
    for (const QUuid &source_uuid : fill_sources)
    {
        rebuild_unit_fill(source_uuid);
    }

    rebuild_unit_local_coordinate_frames();

    m_view->FitAll();
    m_view->Redraw();
    // display_units() is also called from MainWindow's constructor, before
    // this native view has received its final size. Fit once after layout so
    // the initial scene is not compressed into a corner.
    QTimer::singleShot(0, this, [this]()
    {
        if (m_view.IsNull() || width() <= 0 || height() <= 0)
        {
            return;
        }
        m_view->MustBeResized();
        m_view->FitAll();
        m_view->Redraw();
    });
    emit unit_display_list_changed();
}

bool OCCTWidget::select_unit_by_uuid(const QUuid &uuid)
{
    if (uuid.isNull() || m_context.IsNull() || m_view.IsNull())
    {
        return false;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->ais_display.IsNull() || !unit_visible(uuid))
    {
        return false;
    }

    clear_face_reference();
    m_context->ClearSelected(Standard_False);
    selected_shape = unit->ais_display;
    m_context->SetSelected(selected_shape, Standard_True);
    m_view->Redraw();
    emit selection_changed(uuid, false);
    return true;
}

bool OCCTWidget::select_reference_geometry()
{
    if (base_geometry.IsNull() || m_context.IsNull() || m_view.IsNull() ||
        ref_geom.IsNull() || !m_reference_geometry_visible)
    {
        return false;
    }

    m_context->ClearSelected(Standard_False);
    selected_shape = base_geometry;
    m_context->SetSelected(base_geometry, Standard_True);
    m_view->Redraw();
    emit selection_changed(QUuid(), true);
    return true;
}

bool OCCTWidget::set_unit_visible(const QUuid &uuid, bool visible)
{
    if (uuid.isNull() || m_context.IsNull())
    {
        return false;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->ais_display.IsNull())
    {
        return false;
    }

    QSet<QUuid> visited;
    std::function<void(const QUuid &)> apply_visibility;
    apply_visibility = [&](const QUuid &current_uuid)
    {
        if (visited.contains(current_uuid))
        {
            return;
        }
        visited.insert(current_uuid);
        const std::shared_ptr<Unit> current = unit_hash.value(current_uuid);
        if (current == nullptr || current->ais_display.IsNull())
        {
            return;
        }
        m_unit_visibility.insert(current_uuid, visible);
        if (visible)
        {
            m_context->Display(current->ais_display, Standard_False);
            if (!m_unit_local_trihedrons.value(current_uuid).IsNull())
            {
                m_context->Display(m_unit_local_trihedrons.value(current_uuid), Standard_False);
            }
        }
        else
        {
            m_context->Erase(current->ais_display, Standard_False);
            if (!m_unit_local_trihedrons.value(current_uuid).IsNull())
            {
                m_context->Erase(m_unit_local_trihedrons.value(current_uuid), Standard_False);
            }
        }
        if (!visible && selected_shape == current->ais_display)
        {
            clear_context_selection_safely();
        }
        for (const QUuid &child_uuid : current->assembly_child_uuids)
        {
            apply_visibility(child_uuid);
        }
    };
    apply_visibility(uuid);
    m_view->Redraw();
    return true;
}

void OCCTWidget::set_all_units_visible(bool visible)
{
    if (m_context.IsNull())
    {
        return;
    }

    const QList<QUuid> unit_ids = unit_hash.keys();
    bool cleared_selected_unit = false;
    for (const QUuid &uuid : unit_ids)
    {
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit == nullptr || unit->ais_display.IsNull())
        {
            continue;
        }

        m_unit_visibility.insert(uuid, visible);
        if (visible)
        {
            m_context->Display(unit->ais_display, Standard_False);
            if (!m_unit_local_trihedrons.value(uuid).IsNull())
            {
                m_context->Display(m_unit_local_trihedrons.value(uuid), Standard_False);
            }
        }
        else
        {
            m_context->Erase(unit->ais_display, Standard_False);
            if (!m_unit_local_trihedrons.value(uuid).IsNull())
            {
                m_context->Erase(m_unit_local_trihedrons.value(uuid), Standard_False);
            }
        }

        if (!visible && selected_shape == unit->ais_display)
        {
            myIsDragging = false;
            selected_shape.Nullify();
            cleared_selected_unit = true;
        }
    }

    if (cleared_selected_unit)
    {
        clear_context_selection_safely();
    }
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}

bool OCCTWidget::set_reference_geometry_visible(bool visible)
{
    if (base_geometry.IsNull() || m_context.IsNull() || ref_geom.IsNull())
    {
        return false;
    }

    // A hidden reference object cannot have an actionable face reference.
    // Clear the independent face trihedron as well, otherwise the panel and
    // alignment actions would continue to describe a face that is no longer
    // visible or selectable.
    if (!visible && (!selected_face.IsNull() || !face_trihedron.IsNull()))
    {
        clear_face_reference();
    }

    m_reference_geometry_visible = visible;
    if (visible)
    {
        m_context->Display(base_geometry, Standard_False);
        for (const Handle(AIS_Trihedron) &trihedron : m_reference_face_trihedrons)
        {
            if (!trihedron.IsNull())
            {
                m_context->Display(trihedron, Standard_False);
            }
        }
        if (!face_trihedron.IsNull())
        {
            m_context->Display(face_trihedron, Standard_False);
        }
    }
    else
    {
        m_context->Erase(base_geometry, Standard_False);
        for (const Handle(AIS_Trihedron) &trihedron : m_reference_face_trihedrons)
        {
            if (!trihedron.IsNull())
            {
                m_context->Erase(trihedron, Standard_False);
            }
        }
        if (!face_trihedron.IsNull())
        {
            m_context->Erase(face_trihedron, Standard_False);
        }
    }

    if (!visible && selected_shape == base_geometry)
    {
        clear_context_selection_safely();
    }
    m_view->Redraw();
    return true;
}

bool OCCTWidget::unit_visible(const QUuid &uuid) const
{
    return m_unit_visibility.value(uuid, false);
}

bool OCCTWidget::set_unit_locked(const QUuid &uuid, bool locked)
{
    if (uuid.isNull() || !unit_hash.contains(uuid))
    {
        return false;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr)
    {
        return false;
    }

    QSet<QUuid> visited;
    std::function<void(const QUuid &)> apply_lock;
    apply_lock = [&](const QUuid &current_uuid)
    {
        if (visited.contains(current_uuid))
        {
            return;
        }
        visited.insert(current_uuid);
        const std::shared_ptr<Unit> current = unit_hash.value(current_uuid);
        if (current == nullptr)
        {
            return;
        }
        const bool changed = m_unit_locks.value(current_uuid, false) != locked;
        m_unit_locks.insert(current_uuid, locked);
        if (locked && selected_shape == current->ais_display)
        {
            myIsDragging = false;
        }
        if (changed)
        {
            emit unit_lock_changed(current_uuid, locked);
        }
        for (const QUuid &child_uuid : current->assembly_child_uuids)
        {
            apply_lock(child_uuid);
        }
    };
    apply_lock(uuid);
    return true;
}

bool OCCTWidget::unit_locked(const QUuid &uuid) const
{
    return m_unit_locks.value(uuid, false);
}

int OCCTWidget::translate_units_by_uuid(const QList<QUuid> &uuids,
                                        const QVector3D &delta)
{
    if (m_context.IsNull() || delta.isNull())
    {
        return 0;
    }

    QList<QUuid> operation_uuids;
    QSet<QUuid> operation_set;
    std::function<void(const QUuid &)> append_unit_and_members;
    append_unit_and_members = [&](const QUuid &uuid)
    {
        if (operation_set.contains(uuid))
        {
            return;
        }
        operation_set.insert(uuid);
        operation_uuids.append(uuid);
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit != nullptr)
        {
            for (const QUuid &child_uuid : unit->assembly_child_uuids)
            {
                append_unit_and_members(child_uuid);
            }
        }
    };
    for (const QUuid &uuid : uuids)
    {
        append_unit_and_members(uuid);
    }

    m_active_edit_batch_id = QUuid::createUuid();
    m_active_move_batch_id = QUuid::createUuid();
    int translated_count = 0;
    for (const QUuid &uuid : operation_uuids)
    {
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit == nullptr || unit_locked(uuid) || unit->ais_display.IsNull())
        {
            continue;
        }

        const UnitMoveSnapshot before = make_move_snapshot(*unit);
        Injector &injector = unit->inj.injector_data;
        injector.pos += delta;
        injector.pos2 += delta;
        injector.ff_center += delta;
        injector.ff_virtual_origin += delta;
        injector.volume_bgeom_min += delta;
        injector.volume_bgeom_max += delta;

        if (!unit->inj.create_injector())
        {
            injector.pos = before.pos;
            injector.pos2 = before.pos2;
            injector.ff_center = before.ff_center;
            injector.ff_virtual_origin = before.ff_virtual_origin;
            injector.volume_bgeom_min = before.volume_bgeom_min;
            injector.volume_bgeom_max = before.volume_bgeom_max;
            continue;
        }

        unit->ais_display->SetLocalTransformation(gp_Trsf());
        unit->ais_display->Set(unit->inj.shape);
        m_context->Redisplay(unit->ais_display, Standard_False);
        const UnitMoveSnapshot after = make_move_snapshot(*unit);
        record_move(uuid, before, after);
        emit unit_data_updated(unit.get());
        ++translated_count;
    }

    m_active_edit_batch_id = QUuid();
    m_active_move_batch_id = QUuid();
    if (translated_count > 0 && !m_view.IsNull())
    {
        m_view->Redraw();
    }
    return translated_count;
}

QVector3D OCCTWidget::unit_position_by_uuid(const QUuid &uuid) const
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    return unit != nullptr ? unit->inj.injector_data.pos : QVector3D();
}

QVector3D OCCTWidget::unit_direction_by_uuid(const QUuid &uuid) const
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    return unit != nullptr ? injector_frame_direction(unit->inj.injector_data)
                           : QVector3D();
}

bool OCCTWidget::set_unit_direction_by_uuid(const QUuid &uuid,
                                            const QVector3D &direction)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || unit_locked(uuid) ||
        !std::isfinite(direction.x()) || !std::isfinite(direction.y()) ||
        !std::isfinite(direction.z()) || direction.lengthSquared() <= 1.0e-12f)
    {
        return false;
    }

    Injector &injector = unit->inj.injector_data;
    if (injector.injection_type == single &&
        injector.single_direction_mode != Single_Direction_Mode::Vector)
    {
        return false;
    }
    const Injector before = injector;
    begin_unit_edit_transaction(unit.get());
    switch (injector.injection_type)
    {
    case cone:
        injector.axis = direction;
        break;
    case flat_fan_atomizer:
        injector.ff_normal = direction;
        break;
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case effervescent_atomizer:
        injector.atomizer_axis = direction;
        break;
    default:
        injector.vel = direction;
        break;
    }

    if (!unit->inj.create_injector())
    {
        injector = before;
        m_edit_transactions.remove(uuid);
        return false;
    }
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        injector.injection_type == volume ? 0.82f : 0.0f);
    unit->ais_display->SetColor(color_for_material(injector.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(uuid);
    emit unit_data_updated(unit.get());
    finish_unit_edit_transaction(unit.get(), true);
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

bool OCCTWidget::unit_single_pitch_yaw_by_uuid(const QUuid &uuid,
                                               double *pitch_degrees,
                                               double *yaw_degrees) const
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || pitch_degrees == nullptr || yaw_degrees == nullptr ||
        unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Pitch_Yaw)
    {
        return false;
    }
    *pitch_degrees = unit->inj.injector_data.single_pitch_degrees;
    *yaw_degrees = unit->inj.injector_data.single_yaw_degrees;
    return true;
}

bool OCCTWidget::set_unit_single_pitch_yaw_by_uuid(const QUuid &uuid,
                                                   double pitch_degrees,
                                                   double yaw_degrees)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || unit_locked(uuid) ||
        !std::isfinite(pitch_degrees) || !std::isfinite(yaw_degrees) ||
        unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Pitch_Yaw)
    {
        return false;
    }
    Injector &injector = unit->inj.injector_data;
    begin_unit_edit_transaction(unit.get());
    const double old_pitch = injector.single_pitch_degrees;
    const double old_yaw = injector.single_yaw_degrees;
    injector.single_pitch_degrees = pitch_degrees;
    injector.single_yaw_degrees = yaw_degrees;
    if (!unit->inj.create_injector())
    {
        injector.single_pitch_degrees = old_pitch;
        injector.single_yaw_degrees = old_yaw;
        m_edit_transactions.remove(uuid);
        return false;
    }
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        injector.injection_type == volume ? 0.82f : 0.0f);
    unit->ais_display->SetColor(color_for_material(injector.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(uuid);
    emit unit_data_updated(unit.get());
    finish_unit_edit_transaction(unit.get(), true);
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

QVector3D OCCTWidget::unit_single_target_by_uuid(const QUuid &uuid) const
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Target_Hitpoint)
    {
        return QVector3D();
    }
    return unit->inj.injector_data.single_target_hitpoint;
}

bool OCCTWidget::set_unit_single_target_by_uuid(const QUuid &uuid,
                                                const QVector3D &target)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || unit_locked(uuid) ||
        !std::isfinite(target.x()) || !std::isfinite(target.y()) ||
        !std::isfinite(target.z()) ||
        unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Target_Hitpoint ||
        (target - unit->inj.injector_data.pos).lengthSquared() <= 1.0e-12f)
    {
        return false;
    }
    Injector &injector = unit->inj.injector_data;
    begin_unit_edit_transaction(unit.get());
    const QVector3D old_target = injector.single_target_hitpoint;
    injector.single_target_hitpoint = target;
    if (!unit->inj.create_injector())
    {
        injector.single_target_hitpoint = old_target;
        m_edit_transactions.remove(uuid);
        return false;
    }
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        injector.injection_type == volume ? 0.82f : 0.0f);
    unit->ais_display->SetColor(color_for_material(injector.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(uuid);
    emit unit_data_updated(unit.get());
    finish_unit_edit_transaction(unit.get(), true);
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

Single_Target_Scope OCCTWidget::unit_single_target_scope_by_uuid(const QUuid &uuid) const
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Target_Hitpoint)
    {
        return Single_Target_Scope::Array_Local;
    }
    return unit->inj.injector_data.single_target_scope;
}

bool OCCTWidget::set_unit_single_target_scope_by_uuid(const QUuid &uuid,
                                                      Single_Target_Scope scope)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || unit_locked(uuid) ||
        unit->inj.injector_data.injection_type != single ||
        unit->inj.injector_data.single_direction_mode != Single_Direction_Mode::Target_Hitpoint ||
        scope < Single_Target_Scope::World || scope > Single_Target_Scope::Reference_Local)
    {
        return false;
    }
    if (unit->inj.injector_data.single_target_scope == scope)
    {
        return true;
    }
    begin_unit_edit_transaction(unit.get());
    unit->inj.injector_data.single_target_scope = scope;
    if (!unit->inj.create_injector())
    {
        m_edit_transactions.remove(uuid);
        return false;
    }
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);
    unit->ais_display->SetColor(color_for_material(unit->inj.injector_data.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(uuid);
    emit unit_data_updated(unit.get());
    finish_unit_edit_transaction(unit.get(), true);
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

bool OCCTWidget::set_unit_position_by_uuid(const QUuid &uuid,
                                           const QVector3D &position)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || !std::isfinite(position.x()) ||
        !std::isfinite(position.y()) || !std::isfinite(position.z()))
    {
        return false;
    }

    const QVector3D delta = position - unit->inj.injector_data.pos;
    return !delta.isNull() && translate_units_by_uuid({uuid}, delta) > 0;
}

int OCCTWidget::rotate_units_by_uuid(const QList<QUuid> &uuids,
                                     const QVector3D &axis,
                                     float angle_degrees,
                                     const QVector3D &shared_pivot,
                                     bool use_shared_pivot)
{
    if (m_context.IsNull() || axis.lengthSquared() <= 1.0e-12f ||
        !std::isfinite(angle_degrees))
    {
        return 0;
    }

    const QVector3D unit_axis = axis.normalized();
    const float radians = qDegreesToRadians(angle_degrees);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const auto rotate_vector = [&](const QVector3D &value)
    {
        return value * cosine + QVector3D::crossProduct(unit_axis, value) * sine +
               unit_axis * QVector3D::dotProduct(unit_axis, value) * (1.0f - cosine);
    };

    QList<QUuid> operation_uuids;
    QSet<QUuid> operation_set;
    std::function<void(const QUuid &)> append_unit_and_members;
    append_unit_and_members = [&](const QUuid &uuid)
    {
        if (operation_set.contains(uuid))
        {
            return;
        }
        operation_set.insert(uuid);
        operation_uuids.append(uuid);
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit != nullptr)
        {
            for (const QUuid &child_uuid : unit->assembly_child_uuids)
            {
                append_unit_and_members(child_uuid);
            }
        }
    };
    for (const QUuid &uuid : uuids)
    {
        append_unit_and_members(uuid);
    }

    m_active_edit_batch_id = QUuid::createUuid();
    int rotated_count = 0;
    for (const QUuid &uuid : operation_uuids)
    {
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit == nullptr || unit_locked(uuid) || unit->ais_display.IsNull())
        {
            continue;
        }

        Injector &injector = unit->inj.injector_data;
        const Injector before = injector;
        UnitEditTransaction transaction;
        transaction.uuid = uuid;
        transaction.before_type = unit->type;
        transaction.before_data = before;
        const QVector3D pivot = use_shared_pivot ? shared_pivot : injector.pos;
        const auto rotate_point = [&](const QVector3D &point)
        {
            return pivot + rotate_vector(point - pivot);
        };

        injector.pos2 = rotate_point(injector.pos2);
        injector.ff_center = rotate_point(injector.ff_center);
        injector.ff_virtual_origin = rotate_point(injector.ff_virtual_origin);
        injector.volume_bgeom_min = rotate_point(injector.volume_bgeom_min);
        injector.volume_bgeom_max = rotate_point(injector.volume_bgeom_max);
        if (injector.single_target_scope != Single_Target_Scope::World)
        {
            injector.single_target_hitpoint =
                rotate_point(injector.single_target_hitpoint);
        }
        injector.vel = rotate_vector(injector.vel);
        injector.vel2 = rotate_vector(injector.vel2);
        injector.ang_vel = rotate_vector(injector.ang_vel);
        injector.ang_vel2 = rotate_vector(injector.ang_vel2);
        injector.ff_normal = rotate_vector(injector.ff_normal);
        injector.atomizer_axis = rotate_vector(injector.atomizer_axis);

        if (!unit->inj.create_injector())
        {
            injector = before;
            continue;
        }

        unit->ais_display->SetLocalTransformation(gp_Trsf());
        unit->ais_display->Set(unit->inj.shape);
        unit->ais_display->SetColor(color_for_material(injector.material));
        m_context->Redisplay(unit->ais_display, Standard_False);
        update_unit_local_coordinate_frame(uuid);
        record_edit(transaction, *unit);
        emit unit_data_updated(unit.get());
        ++rotated_count;
    }

    m_active_edit_batch_id = QUuid();
    if (rotated_count > 0 && !m_view.IsNull())
    {
        m_view->Redraw();
    }
    return rotated_count;
}

int OCCTWidget::set_material_for_units_by_uuid(const QList<QUuid> &uuids,
                                               const QString &material)
{
    const QString normalized_material = material.trimmed();
    if (normalized_material.isEmpty())
    {
        return 0;
    }
    if (!m_material_names.contains(normalized_material, Qt::CaseInsensitive))
    {
        return 0;
    }

    int changed_count = 0;
    for (const QUuid &uuid : uuids)
    {
        const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
        if (unit == nullptr ||
            unit->inj.injector_data.material.compare(
                normalized_material, Qt::CaseSensitive) == 0)
        {
            continue;
        }

        UnitEditTransaction transaction;
        transaction.uuid = uuid;
        transaction.before_type = unit->type;
        transaction.before_data = unit->inj.injector_data;
        unit->inj.injector_data.material = normalized_material;
        record_edit(transaction, *unit);
        emit unit_data_updated(unit.get());
        ++changed_count;
    }
    return changed_count;
}

bool OCCTWidget::set_unit_name(const QUuid &uuid, const QString &name)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr)
    {
        return false;
    }

    const QString normalized_name = name.trimmed();
    if (normalized_name.isEmpty() ||
        unit->inj.injector_data.name == normalized_name)
    {
        return false;
    }

    for (auto it = unit_hash.constBegin(); it != unit_hash.constEnd(); ++it)
    {
        if (it.key() == uuid || it.value() == nullptr)
        {
            continue;
        }
        if (it.value()->inj.injector_data.name.compare(
                normalized_name, Qt::CaseInsensitive) == 0)
        {
            return false;
        }
    }

    UnitEditTransaction transaction;
    transaction.uuid = uuid;
    transaction.before_type = unit->type;
    transaction.before_data = unit->inj.injector_data;
    unit->inj.injector_data.name = normalized_name;
    record_edit(transaction, *unit);
    emit unit_data_updated(unit.get());
    return true;
}

bool OCCTWidget::edit_unit_by_uuid(const QUuid &uuid)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || unit->ais_display.IsNull())
    {
        return false;
    }

    open_edit_widget(unit->ais_display);
    return true;
}

bool OCCTWidget::copy_unit_by_uuid(const QUuid &uuid)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly)
    {
        return false;
    }

    CopiedUnit copied;
    copied.type = unit->type;
    copied.injector_data = unit->inj.injector_data;
    m_copied_unit = std::move(copied);
    return true;
}

bool OCCTWidget::paste_unit_by_uuid(const QUuid &uuid)
{
    if (!m_copied_unit.has_value())
    {
        return false;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->type == Assebly || m_context.IsNull() ||
        unit->ais_display.IsNull())
    {
        return false;
    }

    UnitEditTransaction transaction;
    transaction.uuid = uuid;
    transaction.before_type = unit->type;
    transaction.before_data = unit->inj.injector_data;

    unit->type = m_copied_unit->type;
    unit->inj.injector_data = m_copied_unit->injector_data;
    if (!unit->inj.create_injector())
    {
        unit->type = transaction.before_type;
        unit->inj.injector_data = transaction.before_data;
        return false;
    }

    unit->ais_display->SetLocalTransformation(gp_Trsf());
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(uuid);
    clear_move_history();
    m_view->Redraw();
    record_edit(transaction, *unit);
    emit unit_data_updated(unit.get());
    return true;
}

bool OCCTWidget::create_assembly(const QList<QUuid> &uuids)
{
    if (uuids.size() < 2)
    {
        return false;
    }

    const std::shared_ptr<Unit> parent = unit_hash.value(uuids.first());
    if (parent == nullptr || parent->is_array_child)
    {
        return false;
    }

    const QUuid parent_uuid = parent->inj.uuid;
    const auto contains_descendant = [&](const std::shared_ptr<Unit> &root,
                                         const QUuid &target,
                                         const auto &self) -> bool
    {
        if (root == nullptr)
        {
            return false;
        }
        for (const QUuid &child_uuid : root->assembly_child_uuids)
        {
            if (child_uuid == target ||
                self(unit_hash.value(child_uuid), target, self))
            {
                return true;
            }
        }
        return false;
    };

    QList<std::shared_ptr<Unit>> valid_children;
    QSet<QUuid> child_ids;
    for (int index = 1; index < uuids.size(); ++index)
    {
        const std::shared_ptr<Unit> child = unit_hash.value(uuids.at(index));
        if (child == nullptr || child == parent || child->is_array_child ||
            child->inj.uuid == parent_uuid || child_ids.contains(child->inj.uuid) ||
            contains_descendant(child, parent_uuid, contains_descendant))
        {
            continue;
        }
        child_ids.insert(child->inj.uuid);
        valid_children.append(child);
    }
    if (valid_children.isEmpty())
    {
        return false;
    }

    // Reusing an array source as an Assembly must not leave its old runtime
    // instances orphaned in the scene or unit hash.
    clear_unit_array_children(*parent);
    parent->has_array_spec = false;
    parent->has_fill_spec = false;
    parent->fill_source_uuids.clear();

    for (const QUuid &old_child_uuid : parent->assembly_child_uuids)
    {
        const std::shared_ptr<Unit> old_child = unit_hash.value(old_child_uuid);
        if (old_child != nullptr && old_child->assembly_parent_uuid == parent_uuid)
        {
            old_child->assembly_parent_uuid = QUuid();
        }
    }
    parent->type = Assebly;
    parent->assembly_child_uuids.clear();
    parent->child_units.clear();
    for (const std::shared_ptr<Unit> &child : valid_children)
    {
        if (!child->assembly_parent_uuid.isNull())
        {
            const std::shared_ptr<Unit> old_parent =
                unit_hash.value(child->assembly_parent_uuid);
            if (old_parent != nullptr)
            {
                old_parent->assembly_child_uuids.removeAll(child->inj.uuid);
                old_parent->child_units.removeAll(child);
                if (old_parent->assembly_child_uuids.isEmpty())
                {
                    old_parent->type = injector;
                }
            }
        }
        child->assembly_parent_uuid = parent_uuid;
        parent->assembly_child_uuids.append(child->inj.uuid);
        parent->child_units.append(child);
    }
    if (parent->assembly_child_uuids.isEmpty())
    {
        parent->type = injector;
        return false;
    }
    emit unit_data_updated(parent.get());
    emit unit_display_list_changed();
    return true;
}

bool OCCTWidget::detach_from_assembly(const QUuid &uuid)
{
    const std::shared_ptr<Unit> child = unit_hash.value(uuid);
    if (child == nullptr || child->assembly_parent_uuid.isNull())
    {
        return false;
    }

    const std::shared_ptr<Unit> parent = unit_hash.value(child->assembly_parent_uuid);
    if (parent != nullptr)
    {
        parent->assembly_child_uuids.removeAll(uuid);
        parent->child_units.removeAll(child);
        if (parent->assembly_child_uuids.isEmpty())
        {
            parent->type = injector;
        }
        emit unit_data_updated(parent.get());
    }
    child->assembly_parent_uuid = QUuid();
    emit unit_display_list_changed();
    return true;
}

bool OCCTWidget::dissolve_assembly(const QUuid &uuid)
{
    const std::shared_ptr<Unit> assembly = unit_hash.value(uuid);
    if (assembly == nullptr || assembly->type != Assebly)
    {
        return false;
    }

    clear_unit_array_children(*assembly);

    const QList<QUuid> children = assembly->assembly_child_uuids;
    for (const QUuid &child_uuid : children)
    {
        const std::shared_ptr<Unit> child = unit_hash.value(child_uuid);
        if (child != nullptr && child->assembly_parent_uuid == uuid)
        {
            child->assembly_parent_uuid = QUuid();
        }
    }
    assembly->assembly_child_uuids.clear();
    assembly->child_units.clear();
    assembly->has_array_spec = false;
    assembly->has_fill_spec = false;
    assembly->fill_source_uuids.clear();
    assembly->type = injector;
    emit unit_data_updated(assembly.get());
    emit unit_display_list_changed();
    return true;
}

int OCCTWidget::create_unit_array(const QUuid &source_uuid,
                                  const UnitArraySpec &spec)
{
    const std::shared_ptr<Unit> source = unit_hash.value(source_uuid);
    if (source == nullptr || source->ais_display.IsNull() || m_context.IsNull())
    {
        return 0;
    }

    clear_unit_array_children(*source);
    source->has_array_spec = true;
    source->array_spec = spec;
    const bool source_is_assembly = source->type == Assebly ||
                                    !source->assembly_child_uuids.isEmpty();
    if (!source_is_assembly)
    {
        source->type = array;
    }
    QList<Unit> children;
    if (source_is_assembly)
    {
        QList<QUuid> seed_uuids;
        QSet<QUuid> visited;
        std::function<void(const QUuid &)> collect_seeds;
        collect_seeds = [&](const QUuid &uuid)
        {
            if (visited.contains(uuid))
            {
                return;
            }
            visited.insert(uuid);
            const std::shared_ptr<Unit> seed = unit_hash.value(uuid);
            if (seed == nullptr || seed->is_array_child)
            {
                return;
            }
            seed_uuids.append(uuid);
            for (const QUuid &child_uuid : seed->assembly_child_uuids)
            {
                collect_seeds(child_uuid);
            }
        };
        collect_seeds(source_uuid);
        for (const QUuid &seed_uuid : seed_uuids)
        {
            const std::shared_ptr<Unit> seed = unit_hash.value(seed_uuid);
            if (seed != nullptr)
            {
                children.append(expand_unit_array(*seed, spec));
            }
        }
    }
    else
    {
        children = expand_unit_array(*source, spec);
    }
    int displayed_count = 0;
    for (const Unit &child : children)
    {
        const std::shared_ptr<Unit> stored_child = std::make_shared<Unit>(child);
        stored_child->type = array;
        stored_child->array_parent_uuid = source_uuid;
        stored_child->is_array_child = true;
        stored_child->follows_array = true;
        // Assembly sources are flattened in this phase; do not copy their
        // persistent parent/child links into a runtime array instance.
        stored_child->assembly_parent_uuid = QUuid();
        stored_child->assembly_child_uuids.clear();
        stored_child->child_units.clear();
        stored_child->ais_display->Set(stored_child->inj.shape);
        stored_child->u_owner->set_unit(stored_child.get());
        stored_child->ais_display->SetOwner(stored_child->u_owner);
        stored_child->ais_display->SetColor(
            color_for_material(stored_child->inj.injector_data.material));
        stored_child->ais_display->SetTransparency(
            stored_child->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);

        unit_hash.insert(stored_child->inj.uuid, stored_child);
        m_unit_visibility.insert(stored_child->inj.uuid, true);
        m_unit_locks.insert(stored_child->inj.uuid, false);
        m_context->Activate(stored_child->ais_display, TopAbs_SHAPE, Standard_True);
        m_context->Display(stored_child->ais_display, Standard_False);
        source->child_units.append(stored_child);
        ++displayed_count;
    }

    if (displayed_count > 0)
    {
        rebuild_unit_local_coordinate_frames();
        m_view->FitAll();
        m_view->Redraw();
        emit unit_display_list_changed();
    }
    emit unit_data_updated(source.get());
    return displayed_count;
}

int OCCTWidget::create_unit_fill(const QList<QUuid> &source_uuids,
                                 const UnitFillSpec &spec)
{
    if (source_uuids.isEmpty() || m_context.IsNull())
    {
        return 0;
    }

    QList<Unit> sources;
    for (const QUuid &uuid : source_uuids)
    {
        const std::shared_ptr<Unit> source = unit_hash.value(uuid);
        if (source != nullptr && !source->ais_display.IsNull())
        {
            sources.append(*source);
        }
    }
    if (sources.isEmpty())
    {
        return 0;
    }

    const std::shared_ptr<Unit> parent = unit_hash.value(source_uuids.first());
    if (parent != nullptr)
    {
        clear_unit_array_children(*parent);
        parent->has_fill_spec = true;
        parent->fill_spec = spec;
        parent->fill_source_uuids = source_uuids;
    }

    const QList<Unit> children = expand_unit_fill(sources, spec);
    int displayed_count = 0;
    for (const Unit &child : children)
    {
        const std::shared_ptr<Unit> stored_child = std::make_shared<Unit>(child);
        stored_child->type = array;
        stored_child->is_array_child = true;
        stored_child->follows_array = true;
        stored_child->array_parent_uuid = source_uuids.first();
        stored_child->assembly_parent_uuid = QUuid();
        stored_child->assembly_child_uuids.clear();
        stored_child->child_units.clear();
        stored_child->ais_display->Set(stored_child->inj.shape);
        stored_child->u_owner->set_unit(stored_child.get());
        stored_child->ais_display->SetOwner(stored_child->u_owner);

        unit_hash.insert(stored_child->inj.uuid, stored_child);
        m_unit_visibility.insert(stored_child->inj.uuid, true);
        m_unit_locks.insert(stored_child->inj.uuid, false);
        m_context->Activate(stored_child->ais_display, TopAbs_SHAPE, Standard_True);
        m_context->Display(stored_child->ais_display, Standard_False);
        if (stored_child->inj.injector_data.injection_type == volume)
        {
            stored_child->ais_display->SetTransparency(0.82f);
        }
        if (!sources.isEmpty())
        {
            stored_child->ais_display->SetColor(
                color_for_material(stored_child->inj.injector_data.material));
        }
        if (parent != nullptr)
        {
            parent->child_units.append(stored_child);
        }
        ++displayed_count;
    }

    if (displayed_count > 0)
    {
        rebuild_unit_local_coordinate_frames();
        m_view->FitAll();
        m_view->Redraw();
        emit unit_display_list_changed();
    }
    return displayed_count;
}

int OCCTWidget::rebuild_unit_fill(const QUuid &source_uuid)
{
    const std::shared_ptr<Unit> parent = unit_hash.value(source_uuid);
    if (parent == nullptr || !parent->has_fill_spec)
    {
        return 0;
    }
    return create_unit_fill(parent->fill_source_uuids, parent->fill_spec);
}

void OCCTWidget::clear_unit_array_children(Unit &source)
{
    const QVector<std::shared_ptr<Unit>> children = source.child_units;
    source.child_units.clear();
    for (const std::shared_ptr<Unit> &child : children)
    {
        if (child == nullptr)
        {
            continue;
        }
        if (!child->is_array_child || !child->follows_array)
        {
            source.child_units.append(child);
            continue;
        }
        const QUuid uuid = child->inj.uuid;
        if (!m_context.IsNull() && !child->ais_display.IsNull())
        {
            m_context->Remove(child->ais_display, Standard_False);
        }
        if (!m_context.IsNull() && !m_unit_local_trihedrons.value(uuid).IsNull())
        {
            m_context->Remove(m_unit_local_trihedrons.value(uuid), Standard_False);
        }
        m_unit_local_trihedrons.remove(uuid);
        m_unit_visibility.remove(uuid);
        m_unit_locks.remove(uuid);
        unit_hash.remove(uuid);
    }
}

int OCCTWidget::rebuild_unit_array(const QUuid &source_uuid)
{
    const std::shared_ptr<Unit> source = unit_hash.value(source_uuid);
    if (source == nullptr || !source->has_array_spec)
    {
        return 0;
    }
    const UnitArraySpec spec = source->array_spec;
    return create_unit_array(source_uuid, spec);
}

bool OCCTWidget::set_unit_follow_array(const QUuid &uuid, bool follow)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || !unit->is_array_child)
    {
        return false;
    }
    unit->follows_array = follow;
    return true;
}

bool OCCTWidget::restore_unit_array_inheritance(const QUuid &uuid)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || !unit->is_array_child ||
        unit->array_parent_uuid.isNull())
    {
        return false;
    }

    const QUuid parent_uuid = unit->array_parent_uuid;
    unit->follows_array = true;
    if (!remove_unit_by_uuid(uuid))
    {
        return false;
    }
    return rebuild_unit_array(parent_uuid) > 0 ||
           rebuild_unit_fill(parent_uuid) > 0;
}

bool OCCTWidget::remove_unit_by_uuid(const QUuid &uuid)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr)
    {
        return false;
    }

    // Derived array/fill children belong to their parent at runtime. Remove
    // following children with the parent and detach a child before deletion.
    if (!unit->child_units.isEmpty())
    {
        clear_unit_array_children(*unit);
    }
    for (const QUuid &child_uuid : unit->assembly_child_uuids)
    {
        const std::shared_ptr<Unit> child = unit_hash.value(child_uuid);
        if (child != nullptr)
        {
            child->assembly_parent_uuid = QUuid();
        }
    }
    if (!unit->assembly_parent_uuid.isNull())
    {
        const std::shared_ptr<Unit> parent = unit_hash.value(unit->assembly_parent_uuid);
        if (parent != nullptr)
        {
            parent->assembly_child_uuids.removeAll(uuid);
            parent->child_units.removeAll(unit);
            if (parent->assembly_child_uuids.isEmpty())
            {
                parent->type = injector;
            }
        }
    }
    if (unit->is_array_child && !unit->array_parent_uuid.isNull())
    {
        const std::shared_ptr<Unit> parent = unit_hash.value(unit->array_parent_uuid);
        if (parent != nullptr)
        {
            parent->child_units.removeAll(unit);
        }
    }

    if (!m_replaying_delete_history)
    {
        UnitDeleteHistoryEntry entry;
        entry.uuid = uuid;
        entry.type = unit->type;
        entry.injector_data = unit->inj.injector_data;
        entry.visible = unit_visible(uuid);
        entry.locked = unit_locked(uuid);
        if (!unit->ais_display.IsNull())
        {
            unit->ais_display->Color(entry.color);
            entry.has_color = true;
        }
        record_delete(entry);
    }

    const quintptr target_unit_ptr = reinterpret_cast<quintptr>(unit.get());
    const QList<QPointer<unit_edit_dialog>> dialogs = m_open_edit_dialogs;
    for (const QPointer<unit_edit_dialog> &dialog : dialogs)
    {
        if (dialog != nullptr &&
            dialog->property("unit_ptr").value<quintptr>() == target_unit_ptr)
        {
            m_open_edit_dialogs.removeAll(dialog);
            dialog->close();
            delete dialog.data();
        }
    }

    if (!m_context.IsNull() && !unit->ais_display.IsNull())
    {
        m_context->Remove(unit->ais_display, Standard_False);
    }

    if (!m_context.IsNull() && !m_unit_local_trihedrons.value(uuid).IsNull())
    {
        m_context->Remove(m_unit_local_trihedrons.value(uuid), Standard_False);
    }
    m_unit_local_trihedrons.remove(uuid);

    if (selected_shape == unit->ais_display)
    {
        clear_context_selection_safely();
    }

    m_pending_visual_refreshes.remove(uuid);
    m_unit_visibility.remove(uuid);
    m_unit_locks.remove(uuid);
    m_edit_transactions.remove(uuid);
    unit_hash.remove(uuid);
    for (int index = m_move_history.size() - 1; index >= 0; --index)
    {
        if (m_move_history[index].uuid == uuid)
        {
            m_move_history.removeAt(index);
            if (index < m_move_history_index)
            {
                --m_move_history_index;
            }
        }
    }
    m_move_history_index = qBound(0, m_move_history_index,
                                  m_move_history.size());
    emit move_history_changed(can_undo_move(), can_redo_move());

    for (int index = m_edit_history.size() - 1; index >= 0; --index)
    {
        if (m_edit_history[index].uuid == uuid)
        {
            m_edit_history.removeAt(index);
            if (index < m_edit_history_index)
            {
                --m_edit_history_index;
            }
        }
    }
    m_edit_history_index = qBound(0, m_edit_history_index,
                                  m_edit_history.size());
    emit edit_history_changed(can_undo_edit(), can_redo_edit());
    emit unit_removed(uuid);
    emit unit_display_list_changed();

    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    return true;
}

void OCCTWidget::record_delete(const UnitDeleteHistoryEntry &entry)
{
    if (entry.uuid.isNull())
    {
        return;
    }

    if (m_delete_history_index < m_delete_history.size())
    {
        m_delete_history.resize(m_delete_history_index);
    }
    m_delete_history.append(entry);
    m_delete_history_index = m_delete_history.size();
    emit delete_history_changed(can_undo_delete(), can_redo_delete());
}

void OCCTWidget::clear_delete_history()
{
    if (m_delete_history.isEmpty() && m_delete_history_index == 0)
    {
        return;
    }

    m_delete_history.clear();
    m_delete_history_index = 0;
    emit delete_history_changed(false, false);
}

bool OCCTWidget::restore_deleted_unit(const UnitDeleteHistoryEntry &entry)
{
    if (entry.uuid.isNull() || unit_hash.contains(entry.uuid) || m_context.IsNull())
    {
        return false;
    }

    const std::shared_ptr<Unit> restored_unit = std::make_shared<Unit>();
    restored_unit->type = entry.type;
    restored_unit->inj.uuid = entry.uuid;
    restored_unit->inj.injector_data = entry.injector_data;
    if (!restored_unit->inj.create_injector())
    {
        return false;
    }

    restored_unit->ais_display->Set(restored_unit->inj.shape);
    restored_unit->u_owner->set_unit(restored_unit.get());
    restored_unit->ais_display->SetOwner(restored_unit->u_owner);
    restored_unit->ais_display->SetColor(
        color_for_material(restored_unit->inj.injector_data.material));
    restored_unit->ais_display->SetTransparency(
        restored_unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);

    unit_hash.insert(entry.uuid, restored_unit);
    m_unit_visibility.insert(entry.uuid, entry.visible);
    m_unit_locks.insert(entry.uuid, entry.locked);
    m_context->Activate(restored_unit->ais_display, TopAbs_SHAPE, Standard_True);
    if (entry.visible)
    {
        m_context->Display(restored_unit->ais_display, Standard_False);
    }
    update_unit_local_coordinate_frame(entry.uuid);
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    emit unit_added(restored_unit.get());
    emit unit_display_list_changed();
    return true;
}

bool OCCTWidget::can_undo_delete() const
{
    return m_delete_history_index > 0;
}

bool OCCTWidget::can_redo_delete() const
{
    return m_delete_history_index < m_delete_history.size();
}

bool OCCTWidget::undo_last_delete()
{
    if (!can_undo_delete())
    {
        return false;
    }

    const UnitDeleteHistoryEntry &entry = m_delete_history[m_delete_history_index - 1];
    if (!restore_deleted_unit(entry))
    {
        return false;
    }

    --m_delete_history_index;
    emit delete_history_changed(can_undo_delete(), can_redo_delete());
    return true;
}

bool OCCTWidget::redo_delete()
{
    if (!can_redo_delete())
    {
        return false;
    }

    const UnitDeleteHistoryEntry &entry = m_delete_history[m_delete_history_index];
    m_replaying_delete_history = true;
    const bool removed = remove_unit_by_uuid(entry.uuid);
    m_replaying_delete_history = false;
    if (!removed)
    {
        return false;
    }

    ++m_delete_history_index;
    emit delete_history_changed(can_undo_delete(), can_redo_delete());
    return true;
}

void OCCTWidget::fit_all_view()
{
    if (!m_view.IsNull())
    {
        m_view->FitAll();
        m_view->Redraw();
    }
}

void OCCTWidget::fit_selected_view()
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        return;
    }

    m_context->FitSelected(m_view);
    m_view->Redraw();
}

void OCCTWidget::set_standard_view(V3d_TypeOfOrientation orientation)
{
    if (m_view.IsNull())
    {
        return;
    }

    m_view->SetProj(orientation, Standard_False);
    m_view->Redraw();
}

void OCCTWidget::clear_selection()
{
    finish_reference_transform_transaction();

    if (m_drag_move_snapshot_valid && !m_drag_unit_uuid.isNull())
    {
        const std::shared_ptr<Unit> unit = unit_hash.value(m_drag_unit_uuid);
        if (unit != nullptr)
        {
            const UnitMoveSnapshot after = make_move_snapshot(*unit);
            if (after.pos != m_drag_move_before.pos ||
                after.pos2 != m_drag_move_before.pos2 ||
                after.ff_center != m_drag_move_before.ff_center ||
                after.ff_virtual_origin != m_drag_move_before.ff_virtual_origin ||
                after.volume_bgeom_min != m_drag_move_before.volume_bgeom_min ||
                after.volume_bgeom_max != m_drag_move_before.volume_bgeom_max)
            {
                record_move(m_drag_unit_uuid, m_drag_move_before, after);
            }
        }
    }
    m_drag_unit_uuid = QUuid();
    m_drag_move_snapshot_valid = false;
    clear_face_reference();
    myIsDragging = false;
    clear_context_selection_safely();
}

OCCTWidget::UnitMoveSnapshot OCCTWidget::make_move_snapshot(const Unit &unit) const
{
    UnitMoveSnapshot snapshot;
    snapshot.pos = unit.inj.injector_data.pos;
    snapshot.pos2 = unit.inj.injector_data.pos2;
    snapshot.ff_center = unit.inj.injector_data.ff_center;
    snapshot.ff_virtual_origin = unit.inj.injector_data.ff_virtual_origin;
    snapshot.volume_bgeom_min = unit.inj.injector_data.volume_bgeom_min;
    snapshot.volume_bgeom_max = unit.inj.injector_data.volume_bgeom_max;
    return snapshot;
}

bool OCCTWidget::apply_move_snapshot(const UnitMoveHistoryEntry &entry,
                                     const UnitMoveSnapshot &snapshot)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(entry.uuid);
    if (unit == nullptr || m_context.IsNull() || unit->ais_display.IsNull())
    {
        return false;
    }

    Injector &injector = unit->inj.injector_data;
    injector.pos = snapshot.pos;
    injector.pos2 = snapshot.pos2;
    injector.ff_center = snapshot.ff_center;
    injector.ff_virtual_origin = snapshot.ff_virtual_origin;
    injector.volume_bgeom_min = snapshot.volume_bgeom_min;
    injector.volume_bgeom_max = snapshot.volume_bgeom_max;

    if (!unit->inj.create_injector())
    {
        return false;
    }

    unit->ais_display->SetLocalTransformation(gp_Trsf());
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetColor(color_for_material(unit->inj.injector_data.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(entry.uuid);
    m_view->Redraw();
    emit unit_data_updated(unit.get());
    return true;
}

void OCCTWidget::record_move(const QUuid &uuid,
                             const UnitMoveSnapshot &before,
                             const UnitMoveSnapshot &after)
{
    if (uuid.isNull())
    {
        return;
    }

    if (m_move_history_index < m_move_history.size())
    {
        m_move_history.resize(m_move_history_index);
    }

    UnitMoveHistoryEntry entry;
    entry.uuid = uuid;
    entry.batch_id = m_active_move_batch_id;
    entry.before = before;
    entry.after = after;
    m_move_history.append(entry);
    m_move_history_index = m_move_history.size();
    emit move_history_changed(can_undo_move(), can_redo_move());
}

void OCCTWidget::clear_move_history()
{
    if (m_move_history.isEmpty() && m_move_history_index == 0)
    {
        return;
    }

    m_move_history.clear();
    m_move_history_index = 0;
    emit move_history_changed(false, false);
}

void OCCTWidget::begin_unit_edit_transaction(Unit *unit)
{
    if (unit == nullptr || unit->inj.uuid.isNull())
    {
        return;
    }

    UnitEditTransaction transaction;
    transaction.uuid = unit->inj.uuid;
    transaction.before_type = unit->type;
    transaction.before_data = unit->inj.injector_data;
    m_edit_transactions.insert(transaction.uuid, std::move(transaction));
}

void OCCTWidget::finish_unit_edit_transaction(Unit *unit, bool changed)
{
    if (unit == nullptr || unit->inj.uuid.isNull())
    {
        return;
    }

    const auto transaction_it = m_edit_transactions.find(unit->inj.uuid);
    if (transaction_it == m_edit_transactions.end())
    {
        return;
    }

    const UnitEditTransaction transaction = transaction_it.value();
    m_edit_transactions.erase(transaction_it);
    if (changed && unit_hash.contains(unit->inj.uuid))
    {
        record_edit(transaction, *unit);
    }
}

bool OCCTWidget::cancel_unit_edit_transaction(Unit *unit)
{
    if (unit == nullptr || unit->inj.uuid.isNull())
    {
        return false;
    }

    const auto transaction_it = m_edit_transactions.find(unit->inj.uuid);
    if (transaction_it == m_edit_transactions.end())
    {
        return false;
    }

    const UnitEditTransaction transaction = transaction_it.value();
    UnitEditHistoryEntry before_entry;
    before_entry.uuid = transaction.uuid;
    before_entry.before_type = transaction.before_type;
    before_entry.after_type = transaction.before_type;
    before_entry.before_data = transaction.before_data;
    before_entry.after_data = transaction.before_data;

    if (!apply_edit_snapshot(before_entry,
                             transaction.before_type,
                             transaction.before_data))
    {
        return false;
    }

    m_edit_transactions.erase(transaction_it);
    return true;
}

bool OCCTWidget::apply_edit_snapshot(const UnitEditHistoryEntry &entry,
                                     Unit_Type type,
                                     const Injector &data)
{
    const std::shared_ptr<Unit> unit = unit_hash.value(entry.uuid);
    if (unit == nullptr || m_context.IsNull() || unit->ais_display.IsNull())
    {
        return false;
    }

    const Unit_Type previous_type = unit->type;
    const Injector previous_data = unit->inj.injector_data;
    const TopoDS_Compound previous_shape = unit->inj.shape;
    unit->type = type;
    unit->inj.injector_data = data;
    if (!unit->inj.create_injector())
    {
        unit->type = previous_type;
        unit->inj.injector_data = previous_data;
        unit->inj.shape = previous_shape;
        return false;
    }

    unit->ais_display->SetLocalTransformation(gp_Trsf());
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);
    unit->ais_display->SetColor(color_for_material(unit->inj.injector_data.material));
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(entry.uuid);
    m_view->Redraw();
    emit unit_data_updated(unit.get());
    return true;
}

void OCCTWidget::record_edit(const UnitEditTransaction &transaction,
                             const Unit &unit)
{
    if (transaction.uuid.isNull())
    {
        return;
    }

    if (m_edit_history_index < m_edit_history.size())
    {
        m_edit_history.resize(m_edit_history_index);
    }

    UnitEditHistoryEntry entry;
    entry.uuid = transaction.uuid;
    entry.batch_id = m_active_edit_batch_id;
    entry.before_type = transaction.before_type;
    entry.after_type = unit.type;
    entry.before_data = transaction.before_data;
    entry.after_data = unit.inj.injector_data;
    m_edit_history.append(std::move(entry));
    m_edit_history_index = m_edit_history.size();
    emit edit_history_changed(can_undo_edit(), can_redo_edit());
}

void OCCTWidget::clear_edit_history()
{
    if (m_edit_history.isEmpty() && m_edit_history_index == 0 &&
        m_edit_transactions.isEmpty())
    {
        return;
    }

    m_edit_transactions.clear();
    m_edit_history.clear();
    m_edit_history_index = 0;
    emit edit_history_changed(false, false);
}

bool OCCTWidget::can_undo_edit() const
{
    return m_edit_history_index > 0;
}

bool OCCTWidget::can_redo_edit() const
{
    return m_edit_history_index < m_edit_history.size();
}

bool OCCTWidget::undo_last_edit()
{
    if (!can_undo_edit())
    {
        return false;
    }

    const QUuid batch_id = m_edit_history[m_edit_history_index - 1].batch_id;
    int batch_start = m_edit_history_index - 1;
    while (batch_start > 0 && !batch_id.isNull() &&
           m_edit_history[batch_start - 1].batch_id == batch_id)
    {
        --batch_start;
    }
    for (int index = m_edit_history_index - 1; index >= batch_start; --index)
    {
        const UnitEditHistoryEntry &entry = m_edit_history[index];
        if (!apply_edit_snapshot(entry, entry.before_type, entry.before_data))
        {
            return false;
        }
    }

    m_edit_history_index = batch_start;
    emit edit_history_changed(can_undo_edit(), can_redo_edit());
    return true;
}

bool OCCTWidget::redo_edit()
{
    if (!can_redo_edit())
    {
        return false;
    }

    const QUuid batch_id = m_edit_history[m_edit_history_index].batch_id;
    int batch_end = m_edit_history_index + 1;
    while (batch_end < m_edit_history.size() && !batch_id.isNull() &&
           m_edit_history[batch_end].batch_id == batch_id)
    {
        ++batch_end;
    }
    for (int index = m_edit_history_index; index < batch_end; ++index)
    {
        const UnitEditHistoryEntry &entry = m_edit_history[index];
        if (!apply_edit_snapshot(entry, entry.after_type, entry.after_data))
        {
            return false;
        }
    }

    m_edit_history_index = batch_end;
    emit edit_history_changed(can_undo_edit(), can_redo_edit());
    return true;
}

bool OCCTWidget::can_undo_move() const
{
    return m_move_history_index > 0;
}

bool OCCTWidget::can_redo_move() const
{
    return m_move_history_index < m_move_history.size();
}

bool OCCTWidget::undo_last_move()
{
    if (!can_undo_move())
    {
        return false;
    }

    const QUuid batch_id = m_move_history[m_move_history_index - 1].batch_id;
    int batch_start = m_move_history_index - 1;
    while (batch_start > 0 && !batch_id.isNull() &&
           m_move_history[batch_start - 1].batch_id == batch_id)
    {
        --batch_start;
    }
    for (int index = m_move_history_index - 1; index >= batch_start; --index)
    {
        const UnitMoveHistoryEntry &entry = m_move_history[index];
        if (!apply_move_snapshot(entry, entry.before))
        {
            return false;
        }
    }

    m_move_history_index = batch_start;
    emit move_history_changed(can_undo_move(), can_redo_move());
    return true;
}

bool OCCTWidget::redo_move()
{
    if (!can_redo_move())
    {
        return false;
    }

    const QUuid batch_id = m_move_history[m_move_history_index].batch_id;
    int batch_end = m_move_history_index + 1;
    while (batch_end < m_move_history.size() && !batch_id.isNull() &&
           m_move_history[batch_end].batch_id == batch_id)
    {
        ++batch_end;
    }
    for (int index = m_move_history_index; index < batch_end; ++index)
    {
        const UnitMoveHistoryEntry &entry = m_move_history[index];
        if (!apply_move_snapshot(entry, entry.after))
        {
            return false;
        }
    }

    m_move_history_index = batch_end;
    emit move_history_changed(can_undo_move(), can_redo_move());
    return true;
}

void OCCTWidget::set_chemkin_species_names(const QStringList &species_names)
{
    m_chemkin_species_names = species_names;

    for (const QPointer<unit_edit_dialog> &dialog : m_open_edit_dialogs)
    {
        if (dialog != nullptr)
        {
            dialog->set_chemkin_species_names(m_chemkin_species_names);
        }
    }
}

void OCCTWidget::set_species_colors(const QHash<QString, QColor> &species_colors)
{
    m_species_colors = species_colors;
    refresh_unit_colors();
}

void OCCTWidget::set_material_names(const QStringList &material_names)
{
    m_material_names = material_names;

    for (const QPointer<unit_edit_dialog> &dialog : m_open_edit_dialogs)
    {
        if (dialog != nullptr)
        {
            dialog->set_material_names(m_material_names);
        }
    }
}

Standard_Real OCCTWidget::get_trihedron_size()
{
    const Standard_Real size = cbrt(geometry.xyz_length.x() *
                                    geometry.xyz_length.y() *
                                    geometry.xyz_length.z()) / 10.0;
    return std::max(size, 1.0);
}

Quantity_Color OCCTWidget::color_for_material(const QString &material) const
{
    const QString normalized = material.trimmed();
    for (auto it = m_species_colors.constBegin();
         it != m_species_colors.constEnd(); ++it)
    {
        if (it.key().compare(normalized, Qt::CaseInsensitive) == 0 &&
            it.value().isValid())
        {
            const QColor color = it.value();
            return Quantity_Color(color.redF(), color.greenF(), color.blueF(),
                                  Quantity_TOC_RGB);
        }
    }
    return Quantity_Color(0.72, 0.72, 0.72, Quantity_TOC_RGB);
}

void OCCTWidget::refresh_unit_colors()
{
    for (auto it = unit_hash.begin(); it != unit_hash.end(); ++it)
    {
        if (it.value() != nullptr && !it.value()->ais_display.IsNull())
        {
            it.value()->ais_display->SetColor(
                color_for_material(it.value()->inj.injector_data.material));
        }
    }
    if (!m_context.IsNull() && !m_view.IsNull())
    {
        m_context->UpdateCurrentViewer();
        m_view->Redraw();
    }
}

void OCCTWidget::clear_unit_local_coordinate_frames()
{
    if (!m_context.IsNull())
    {
        for (auto it = m_unit_local_trihedrons.constBegin();
             it != m_unit_local_trihedrons.constEnd();
             ++it)
        {
            if (!it.value().IsNull())
            {
                m_context->Remove(it.value(), Standard_False);
            }
        }
    }
    m_unit_local_trihedrons.clear();
}

void OCCTWidget::update_unit_local_coordinate_frame(const QUuid &uuid)
{
    if (uuid.isNull() || m_context.IsNull())
    {
        return;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr)
    {
        return;
    }

    Handle(AIS_Trihedron) &trihedron = m_unit_local_trihedrons[uuid];
    if (trihedron.IsNull())
    {
        const QVector3D origin = injector_frame_origin(unit->inj.injector_data);
        const QVector3D direction = injector_frame_direction(unit->inj.injector_data);
        const gp_Ax2 axis(
            gp_Pnt(origin.x(), origin.y(), origin.z()),
            gp_Dir(direction.x(), direction.y(), direction.z()));
        trihedron = make_local_trihedron(axis, get_trihedron_size() * 0.45);
        m_context->Deactivate(trihedron, TopAbs_SHAPE);
        if (unit_visible(uuid))
        {
            m_context->Display(trihedron, Standard_False);
        }
        return;
    }

    const QVector3D origin = injector_frame_origin(unit->inj.injector_data);
    const QVector3D direction = injector_frame_direction(unit->inj.injector_data);
    const gp_Ax2 axis(
        gp_Pnt(origin.x(), origin.y(), origin.z()),
        gp_Dir(direction.x(), direction.y(), direction.z()));
    trihedron->SetComponent(new Geom_Axis2Placement(axis));
    trihedron->SetSize(std::max(get_trihedron_size() * 0.45, 1.0));
    m_context->Redisplay(trihedron, Standard_False);
}

void OCCTWidget::rebuild_unit_local_coordinate_frames()
{
    clear_unit_local_coordinate_frames();
    for (auto it = unit_hash.constBegin(); it != unit_hash.constEnd(); ++it)
    {
        update_unit_local_coordinate_frame(it.key());
    }
}

void OCCTWidget::clear_reference_face_coordinate_frames()
{
    if (!m_context.IsNull())
    {
        for (const Handle(AIS_Trihedron) &trihedron : m_reference_face_trihedrons)
        {
            if (!trihedron.IsNull())
            {
                m_context->Remove(trihedron, Standard_False);
            }
        }
    }
    m_reference_face_trihedrons.clear();
}

void OCCTWidget::update_reference_face_coordinate_frames_transform()
{
    for (const Handle(AIS_Trihedron) &trihedron : m_reference_face_trihedrons)
    {
        if (trihedron.IsNull())
        {
            continue;
        }
        trihedron->SetLocalTransformation(m_reference_transform);
        if (!m_context.IsNull())
        {
            m_context->Redisplay(trihedron, Standard_False);
        }
    }
}

void OCCTWidget::rebuild_reference_face_coordinate_frames()
{
    clear_reference_face_coordinate_frames();
    if (ref_geom.IsNull() || m_context.IsNull() || !m_reference_geometry_visible)
    {
        return;
    }

    for (TopExp_Explorer explorer(ref_geom, TopAbs_FACE);
         explorer.More();
         explorer.Next())
    {
        gp_Ax2 axis;
        if (!face_local_axis(TopoDS::Face(explorer.Current()), axis))
        {
            continue;
        }

        Handle(AIS_Trihedron) trihedron = make_local_trihedron(
            axis, get_trihedron_size() * 0.22);
        trihedron->SetLocalTransformation(m_reference_transform);
        m_context->Display(trihedron, Standard_False);
        m_context->Deactivate(trihedron, TopAbs_SHAPE);
        m_reference_face_trihedrons.append(trihedron);
    }
}




void OCCTWidget::add_readed_geometry()
{
    clear_reference_transform_history();
    clear_face_reference();
    set_reference_geometry_locked(false);
    set_reference_transform(QVector3D(0.0f, 0.0f, 0.0f),
                            QVector3D(0.0f, 0.0f, 0.0f));
    builder.Remove(compound,ref_geom);
    ref_geom=geometry.getShape();
    builder.Add(compound,ref_geom);

    clear_reference_face_coordinate_frames();

    base_geometry->Set(compound);
    m_reference_geometry_visible = true;

    m_context->Redisplay(base_geometry, Standard_True);
    m_view->FitAll();

    trihedron_main->SetSize(get_trihedron_size());
    m_context->Redisplay(trihedron_main, Standard_True);

    rebuild_reference_face_coordinate_frames();

    builded=true;
    emit reference_geometry_available(true);

}

void OCCTWidget::refresh_open_unit_editors()
{
    for (const QPointer<unit_edit_dialog> &dialog : m_open_edit_dialogs)
    {
        if (dialog != nullptr)
        {
            const QVariant property_value = dialog->property("unit_ptr");
            Unit *unit = reinterpret_cast<Unit *>(property_value.value<quintptr>());
            if (unit != nullptr)
            {
                dialog->refresh_from_unit_data(unit);
            }
        }
    }
}

void OCCTWidget::set_unit_editor_case_context(const Unit_Edit_Case_Context &context)
{
    m_unit_editor_case_context = context;

    // A dialog may close while applying a new context, so iterate over a copy.
    const QList<QPointer<unit_edit_dialog>> dialogs = m_open_edit_dialogs;
    for (const QPointer<unit_edit_dialog> &dialog : dialogs)
    {
        if (dialog != nullptr)
        {
            dialog->set_case_context(m_unit_editor_case_context);
        }
    }
}

bool OCCTWidget::clear_reference_geometry()
{
    clear_reference_transform_history();
    clear_face_reference();
    set_reference_geometry_locked(false);

    if (!m_context.IsNull() && !base_geometry.IsNull())
    {
        m_context->Remove(base_geometry, Standard_False);
    }

    if (!compound.IsNull() && !ref_geom.IsNull())
    {
        builder.Remove(compound, ref_geom);
    }

    base_geometry.Nullify();
    reference_geometry.Nullify();
    ref_geom.Nullify();
    clear_reference_face_coordinate_frames();
    m_reference_position = QVector3D();
    m_reference_rotation = QVector3D();
    m_reference_transform = gp_Trsf();
    m_reference_geometry_visible = true;

    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }

    emit reference_geometry_available(false);
    emit reference_transform_changed(m_reference_position, m_reference_rotation);
    return true;
}

void OCCTWidget::set_reference_transform(const QVector3D &position,
                                         const QVector3D &rotation_degrees)
{
    m_reference_position = position;
    m_reference_rotation = rotation_degrees;
    apply_reference_transform();
}

bool OCCTWidget::reference_frame(QVector3D *origin, QVector3D *x_axis,
                                 QVector3D *z_axis) const
{
    if (origin == nullptr || x_axis == nullptr || z_axis == nullptr ||
        reference_geometry.IsNull() || !m_reference_geometry_visible)
    {
        return false;
    }

    if (!selected_face.IsNull())
    {
        gp_XYZ face_origin(selected_face_axis.Location().X(),
                           selected_face_axis.Location().Y(),
                           selected_face_axis.Location().Z());
        gp_XYZ face_x(selected_face_axis.XDirection().X(),
                      selected_face_axis.XDirection().Y(),
                      selected_face_axis.XDirection().Z());
        gp_XYZ face_z(selected_face_axis.Direction().X(),
                      selected_face_axis.Direction().Y(),
                      selected_face_axis.Direction().Z());
        m_reference_transform.Transforms(face_origin);
        m_reference_transform.Transforms(face_x);
        m_reference_transform.Transforms(face_z);
        *origin = QVector3D(static_cast<float>(face_origin.X()),
                            static_cast<float>(face_origin.Y()),
                            static_cast<float>(face_origin.Z()));
        *x_axis = QVector3D(static_cast<float>(face_x.X()),
                             static_cast<float>(face_x.Y()),
                             static_cast<float>(face_x.Z())).normalized();
        *z_axis = QVector3D(static_cast<float>(face_z.X()),
                             static_cast<float>(face_z.Y()),
                             static_cast<float>(face_z.Z())).normalized();
        return x_axis->lengthSquared() > 1.0e-12f &&
               z_axis->lengthSquared() > 1.0e-12f;
    }

    gp_XYZ x_vector(1.0, 0.0, 0.0);
    gp_XYZ z_vector(0.0, 0.0, 1.0);
    m_reference_transform.Transforms(x_vector);
    m_reference_transform.Transforms(z_vector);
    *origin = m_reference_position;
    *x_axis = QVector3D(static_cast<float>(x_vector.X()),
                        static_cast<float>(x_vector.Y()),
                        static_cast<float>(x_vector.Z())).normalized();
    *z_axis = QVector3D(static_cast<float>(z_vector.X()),
                        static_cast<float>(z_vector.Y()),
                        static_cast<float>(z_vector.Z())).normalized();
    return x_axis->lengthSquared() > 1.0e-12f &&
           z_axis->lengthSquared() > 1.0e-12f;
}

void OCCTWidget::begin_reference_transform_transaction()
{
    if (m_reference_transform_transaction_active)
    {
        return;
    }

    m_reference_transform_before_position = m_reference_position;
    m_reference_transform_before_rotation = m_reference_rotation;
    m_reference_transform_transaction_active = true;
}

void OCCTWidget::finish_reference_transform_transaction()
{
    if (!m_reference_transform_transaction_active)
    {
        return;
    }

    m_reference_transform_transaction_active = false;
    if (m_reference_transform_before_position == m_reference_position &&
        m_reference_transform_before_rotation == m_reference_rotation)
    {
        return;
    }

    record_reference_transform(m_reference_transform_before_position,
                               m_reference_transform_before_rotation,
                               m_reference_position,
                               m_reference_rotation);
}

bool OCCTWidget::can_undo_reference_transform() const
{
    return m_reference_transform_history_index > 0;
}

bool OCCTWidget::can_redo_reference_transform() const
{
    return m_reference_transform_history_index <
           m_reference_transform_history.size();
}

bool OCCTWidget::undo_reference_transform()
{
    if (!can_undo_reference_transform())
    {
        return false;
    }

    const ReferenceTransformHistoryEntry &entry =
        m_reference_transform_history[m_reference_transform_history_index - 1];
    if (!apply_reference_transform_snapshot(entry.before_position,
                                            entry.before_rotation))
    {
        return false;
    }

    --m_reference_transform_history_index;
    emit reference_transform_history_changed(can_undo_reference_transform(),
                                             can_redo_reference_transform());
    return true;
}

bool OCCTWidget::redo_reference_transform()
{
    if (!can_redo_reference_transform())
    {
        return false;
    }

    const ReferenceTransformHistoryEntry &entry =
        m_reference_transform_history[m_reference_transform_history_index];
    if (!apply_reference_transform_snapshot(entry.after_position,
                                            entry.after_rotation))
    {
        return false;
    }

    ++m_reference_transform_history_index;
    emit reference_transform_history_changed(can_undo_reference_transform(),
                                             can_redo_reference_transform());
    return true;
}

void OCCTWidget::record_reference_transform(
    const QVector3D &before_position,
    const QVector3D &before_rotation,
    const QVector3D &after_position,
    const QVector3D &after_rotation)
{
    if (m_reference_transform_history_index <
        m_reference_transform_history.size())
    {
        m_reference_transform_history.resize(m_reference_transform_history_index);
    }

    m_reference_transform_history.push_back({before_position, before_rotation,
                                             after_position, after_rotation});
    m_reference_transform_history_index = m_reference_transform_history.size();
    emit reference_transform_history_changed(can_undo_reference_transform(),
                                             can_redo_reference_transform());
}

void OCCTWidget::clear_reference_transform_history()
{
    m_reference_transform_transaction_active = false;
    m_reference_transform_history.clear();
    m_reference_transform_history_index = 0;
    emit reference_transform_history_changed(false, false);
}

bool OCCTWidget::apply_reference_transform_snapshot(const QVector3D &position,
                                                    const QVector3D &rotation)
{
    set_reference_transform(position, rotation);
    return true;
}

void OCCTWidget::set_reference_geometry_locked(bool locked)
{
    if (m_reference_geometry_locked == locked)
    {
        return;
    }

    m_reference_geometry_locked = locked;
    emit reference_geometry_lock_changed(locked);
}

void OCCTWidget::apply_reference_transform()
{
    gp_Trsf rotation_x;
    gp_Trsf rotation_y;
    gp_Trsf rotation_z;
    rotation_x.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0)),
                           qDegreesToRadians(static_cast<double>(m_reference_rotation.x())));
    rotation_y.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 1.0, 0.0)),
                           qDegreesToRadians(static_cast<double>(m_reference_rotation.y())));
    rotation_z.SetRotation(gp_Ax1(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)),
                           qDegreesToRadians(static_cast<double>(m_reference_rotation.z())));

    m_reference_transform = rotation_z;
    m_reference_transform.Multiply(rotation_y);
    m_reference_transform.Multiply(rotation_x);
    m_reference_transform.SetTranslationPart(
        gp_Vec(m_reference_position.x(), m_reference_position.y(), m_reference_position.z()));

    if (!base_geometry.IsNull())
    {
        base_geometry->SetLocalTransformation(m_reference_transform);
        if (!m_context.IsNull())
        {
            m_context->Redisplay(base_geometry, Standard_False);
        }
    }

    if (!face_trihedron.IsNull())
    {
        face_trihedron->SetLocalTransformation(m_reference_transform);
        if (!m_context.IsNull())
        {
            m_context->Redisplay(face_trihedron, Standard_False);
        }
    }

    update_reference_face_coordinate_frames_transform();

    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
    emit reference_transform_changed(m_reference_position, m_reference_rotation);
}

void OCCTWidget::align_view_to_selected_face()
{
    if (!m_view.IsNull() && !face_trihedron.IsNull())
    {
        gp_XYZ direction(selected_face_axis.Direction().X(),
                         selected_face_axis.Direction().Y(),
                         selected_face_axis.Direction().Z());
        m_reference_transform.Transforms(direction);
        m_view->SetProj(direction.X(), direction.Y(), direction.Z());
        m_view->FitAll();
        m_view->Redraw();
    }
}

void OCCTWidget::clear_face_reference()
{
    const bool had_face_reference = !selected_face.IsNull() ||
                                    !face_trihedron.IsNull();
    selected_face.Nullify();
    emit selection_changed(QUuid(), false);

    if (!face_trihedron.IsNull() && !m_context.IsNull())
    {
        m_context->Remove(face_trihedron, Standard_False);
    }

    face_trihedron.Nullify();
    face_axis_placement.Nullify();

    if (had_face_reference)
    {
        emit face_reference_changed(false);
        emit face_reference_info_changed(QVector3D(), QVector3D());
    }
}

void OCCTWidget::clear_context_selection_safely()
{
    if (!m_context.IsNull())
    {
        // Avoid an immediate native-view redraw while Qt is dispatching a
        // mouse or context-menu event.
        m_context->ClearSelected(Standard_False);
    }
    selected_shape.Nullify();

    if (m_is_destroying)
    {
        return;
    }

    emit selection_changed(QUuid(), false);

    QTimer::singleShot(0, this, [this]()
    {
        if (!m_context.IsNull() && !m_view.IsNull())
        {
            m_view->Redraw();
        }
    });
}

void OCCTWidget::show_face_reference(const TopoDS_Face &face)
{
    if (face.IsNull() || m_context.IsNull() || m_view.IsNull())
    {
        return;
    }

    GProp_GProps properties;
    BRepGProp::SurfaceProperties(face, properties);
    if (properties.Mass() <= Precision::Confusion())
    {
        return;
    }

    gp_Pnt origin = properties.CentreOfMass();
    BRepAdaptor_Surface surface(face, Standard_True);
    Standard_Real u_min = 0.0;
    Standard_Real u_max = 0.0;
    Standard_Real v_min = 0.0;
    Standard_Real v_max = 0.0;
    BRepTools::UVBounds(face, u_min, u_max, v_min, v_max);

    gp_Dir normal;
    gp_Dir x_direction;
    bool has_normal = false;
    bool has_x_direction = false;
    const Standard_Real u = 0.5 * (u_min + u_max);
    const Standard_Real v = 0.5 * (v_min + v_max);
    try
    {
        gp_Pnt sample_point;
        gp_Vec du;
        gp_Vec dv;
        surface.D1(u, v, sample_point, du, dv);
        gp_Vec candidate_normal = du.Crossed(dv);
        if (candidate_normal.SquareMagnitude() > Precision::Confusion())
        {
            normal = gp_Dir(candidate_normal);
            has_normal = true;
            if (face.Orientation() == TopAbs_REVERSED)
            {
                normal.Reverse();
            }

            if (du.SquareMagnitude() > Precision::Confusion())
            {
                x_direction = gp_Dir(du);
                has_x_direction = true;
            }
            else if (dv.SquareMagnitude() > Precision::Confusion())
            {
                x_direction = gp_Dir(dv);
                has_x_direction = true;
            }
        }
    }
    catch (...)
    {
        return;
    }

    if (!has_normal)
    {
        return;
    }

    gp_Ax2 face_axis = has_x_direction
                           ? gp_Ax2(origin, normal, x_direction)
                           : gp_Ax2(origin, normal);

    clear_face_reference();
    selected_face = face;
    selected_face_axis = face_axis;
    face_axis_placement = new Geom_Axis2Placement(face_axis);
    face_trihedron = new AIS_Trihedron(face_axis_placement);
    face_trihedron->SetSize(std::max(get_trihedron_size() * 0.75, 1.0));
    face_trihedron->SetDatumDisplayMode(Prs3d_DM_Shaded);
    face_trihedron->SetDatumPartColor(Prs3d_DP_XAxis, Quantity_NOC_RED);
    face_trihedron->SetDatumPartColor(Prs3d_DP_XArrow, Quantity_NOC_RED);
    face_trihedron->SetDatumPartColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);
    face_trihedron->SetDatumPartColor(Prs3d_DP_YArrow, Quantity_NOC_GREEN);
    face_trihedron->SetDatumPartColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);
    face_trihedron->SetDatumPartColor(Prs3d_DP_ZArrow, Quantity_NOC_BLUE);
    face_trihedron->SetTextColor(Prs3d_DP_XAxis, Quantity_NOC_RED);
    face_trihedron->SetTextColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);
    face_trihedron->SetTextColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);

    face_trihedron->SetLocalTransformation(m_reference_transform);
    m_context->Display(face_trihedron, Standard_False);
    m_context->Deactivate(face_trihedron, TopAbs_SHAPE);
    emit face_reference_changed(true);
    emit face_reference_info_changed(to_qvector3d(origin),
                                     QVector3D(static_cast<float>(normal.X()),
                                               static_cast<float>(normal.Y()),
                                               static_cast<float>(normal.Z())));
    emit selection_changed(QUuid(), true);
}

bool OCCTWidget::select_face_reference()
{
    if (m_context.IsNull() || base_geometry.IsNull() ||
        !m_context->HasDetected())
    {
        return false;
    }

    const Handle(AIS_InteractiveObject) detected_object =
        m_context->DetectedInteractive();
    if (detected_object.IsNull() || detected_object != base_geometry)
    {
        return false;
    }

    const TopoDS_Shape detected_shape = m_context->DetectedShape();
    if (detected_shape.IsNull() || detected_shape.ShapeType() != TopAbs_FACE)
    {
        return false;
    }

    m_context->ClearSelected(Standard_False);
    m_context->SelectDetected();
    show_face_reference(TopoDS::Face(detected_shape));
    m_view->Update();
    return !face_trihedron.IsNull();
}

void OCCTWidget::ensure_reference_face_selection_mode()
{
    if (m_context.IsNull() || base_geometry.IsNull())
    {
        return;
    }

    // Injector selection changes the active OCCT selection mode globally.
    // Keep the reference object in a dedicated face-only mode. In particular,
    // locking it must prevent movement without disabling face picking.
    m_context->SetSelectionModeActive(
        base_geometry, -1, Standard_False,
        AIS_SelectionModesConcurrency_Single);
    m_context->SetSelectionModeActive(
        base_geometry, TopAbs_FACE, Standard_True,
        AIS_SelectionModesConcurrency_Single, Standard_True);
}


void OCCTWidget::m_initialize_context()
{
    //若交互式上下文为空，则创建对象
    if (m_context.IsNull())
    {
        //此对象提供与X server的连接，在Windows和Mac OS中不起作用
        Handle(Aspect_DisplayConnection) m_display_donnection = new Aspect_DisplayConnection();
        //创建OpenGl图形驱动
        if (m_graphic_driver.IsNull())
        {
            m_graphic_driver = new OpenGl_GraphicDriver(m_display_donnection);
        }
        //获取QWidget的窗口系统标识符
        WId window_handle = (WId) winId();
#ifdef _WIN32
        // 创建Windows NT 窗口
        Handle(WNT_Window) wind = new WNT_Window((Aspect_Handle) window_handle);
#else
        // 创建XLib window 窗口
        Handle(Xw_Window) wind = new Xw_Window(m_display_donnection, (Window) window_handle);
#endif
        //创建3D查看器
        m_viewer = new V3d_Viewer(m_graphic_driver);
        //创建视图
        m_view = m_viewer->CreateView();
        m_view->SetWindow(wind);
        //打开窗口
        if (!wind->IsMapped())
        {
            wind->Map();
        }
        m_context = new AIS_InteractiveContext(m_viewer);  //创建交互式上下文

        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();

        m_view->SetBackgroundColor(Quantity_NOC_BLACK);
        m_view->MustBeResized();

        gp_Ax2 coordinate_system_main(gp::Origin(), gp::DZ(), gp::DX());
        axis_placement_main = new Geom_Axis2Placement(coordinate_system_main);
        trihedron_main = new AIS_Trihedron(axis_placement_main);
        Handle(Prs3d_Drawer) drawer_main = trihedron_main->Attributes();

        trihedron_main->SetDatumPartColor(Prs3d_DP_XAxis, Quantity_NOC_RED);      // X轴轴线红色
        trihedron_main->SetDatumPartColor(Prs3d_DP_XArrow, Quantity_NOC_RED);     // X轴箭头红色

        trihedron_main->SetDatumPartColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);      // Y轴轴线绿色
        trihedron_main->SetDatumPartColor(Prs3d_DP_YArrow, Quantity_NOC_GREEN);   // Y轴箭头绿色

        trihedron_main->SetDatumPartColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);      // Z轴轴线蓝色
        trihedron_main->SetDatumPartColor(Prs3d_DP_ZArrow, Quantity_NOC_BLUE);     // Z轴箭头蓝色

        trihedron_main->SetTextColor(Prs3d_DP_XAxis, Quantity_NOC_RED);           // X标签红色
        trihedron_main->SetTextColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN);         // Y标签绿色
        trihedron_main->SetTextColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE);

        trihedron_main->SetSize(2.0);
        trihedron_main->SetDatumDisplayMode(Prs3d_DM_Shaded);



        m_context->Display(trihedron_main, Standard_True);


        builder.MakeCompound(compound);

        base_geometry = new AIS_Shape(compound);

        // auto transform_pers = new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers,
        //                                                   Aspect_TOTP_LEFT_LOWER,
        //                                                   Graphic3d_Vec2i(85, 85));
        // view_cube->SetTransformPersistence(transform_pers);


        base_geometry->SetTransparency(0.8);
        base_geometry->SetColor(Quantity_Color(0.6,0.6,0.6,Quantity_TOC_RGB));

        m_context->SetDisplayMode(AIS_Shaded, Standard_True);
        m_context->Display(base_geometry, Standard_True);

        m_context->Deactivate(base_geometry, TopAbs_SHAPE);
        m_context->Activate(base_geometry, TopAbs_FACE, Standard_True);




        // 设置模型高亮的风格
        Handle(Prs3d_Drawer) t_hilight_style = m_context->HighlightStyle(); // 获取高亮风格
        t_hilight_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
        t_hilight_style->SetColor(Quantity_NOC_LIGHTBLUE);    // 设置高亮颜色
        t_hilight_style->SetDisplayMode(AIS_Shaded); // 整体高亮
        t_hilight_style->SetTransparency(0.5f); // 设置透明度

        // 设置选择模型的风格
        Handle(Prs3d_Drawer) t_select_style = m_context->SelectionStyle();  // 获取选择风格
        t_select_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
        //t_select_style->SetColor(selc);   // 设置选择后颜色
        t_select_style->SetDisplayMode(AIS_Shaded); // 整体高亮
        t_select_style->SetTransparency(0.6f); // 设置透明度


        m_view->SetZoom(100);   // 放大

        // //激活二维网格
        // m_viewer->SetRectangularGridValues(0,0,1,1,0);
        // m_viewer->SetRectangularGridGraphicValues(10.01,0,10.01);
        // m_viewer->ActivateGrid(Aspect_GT_Rectangular,Aspect_GDM_Lines);

        m_view->SetProj(V3d_Zpos);
    }
}

void OCCTWidget::paintEvent(QPaintEvent *)
{
    if (!m_view.IsNull())
    {
        m_view->Redraw();
    }
}

void OCCTWidget::resizeEvent(QResizeEvent *)
{
    if( !m_view.IsNull() )
    {
        m_view->MustBeResized();
    }
}

void OCCTWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        event->ignore();
        return;
    }

    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    if (event->button() == Qt::LeftButton)
    {
        m_x_max=pos.x();
        m_y_max=pos.y();
        ensure_reference_face_selection_mode();
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);

        if (select_face_reference())
        {
            selected_shape = base_geometry;
            myIsDragging = !m_reference_geometry_locked;
            if (myIsDragging)
            {
                begin_reference_transform_transaction();
            }
            emit selection_changed(QUuid(), true);
            return;
        }

        clear_face_reference();
        myIsDragging = select_injector();
        if (myIsDragging && !selected_shape.IsNull())
        {
            if (Unit *unit = get_unit(selected_shape))
            {
                myIsDragging = !unit_locked(unit->inj.uuid);
                if (myIsDragging)
                {
                    m_drag_unit_uuid = unit->inj.uuid;
                    m_drag_move_before = make_move_snapshot(*unit);
                    m_drag_move_snapshot_valid = true;
                }
            }
        }
        if (!myIsDragging)
        {
            selected_shape.Nullify();
        }
    }
    // else if(event->buttons()&Qt::RightButton)
    // {
    //     // 鼠标左键按下：初始化平移
    //     m_x_max=pos.x();
    //     m_y_max=pos.y();
    // }
    else if(event->buttons()&Qt::MiddleButton)
    {
        {
        // 鼠标滚轮键：初始化旋转
        m_view->StartRotation(pos.x(),pos.y());

        }
    }
    // 在mousePressEvent中
}

void OCCTWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        QPoint pos = event->pos();
        pos.setX(pos.x()*m_dpi_scale);
        pos.setY(pos.y()*m_dpi_scale);
        // 将鼠标位置传递到交互环境
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
        if (selected_shape == base_geometry)
        {
            finish_reference_transform_transaction();
            myIsDragging = false;
            clear_context_selection_safely();
            selected_shape.Nullify();
            return;
        }
        if(myIsDragging)
        {
            if (m_drag_move_snapshot_valid && !m_drag_unit_uuid.isNull())
            {
                const std::shared_ptr<Unit> unit = unit_hash.value(m_drag_unit_uuid);
                if (unit != nullptr)
                {
                    const UnitMoveSnapshot after = make_move_snapshot(*unit);
                    if (after.pos != m_drag_move_before.pos ||
                        after.pos2 != m_drag_move_before.pos2 ||
                        after.ff_center != m_drag_move_before.ff_center ||
                        after.ff_virtual_origin != m_drag_move_before.ff_virtual_origin ||
                        after.volume_bgeom_min != m_drag_move_before.volume_bgeom_min ||
                        after.volume_bgeom_max != m_drag_move_before.volume_bgeom_max)
                    {
                        record_move(m_drag_unit_uuid, m_drag_move_before, after);
                        emit unit_data_updated(unit.get());
                    }
                }
            }
            m_drag_unit_uuid = QUuid();
            m_drag_move_snapshot_valid = false;
            myIsDragging=false;
            clear_context_selection_safely();
        }
        selected_shape.Nullify();
    }
}


gp_Pln OCCTWidget::get_moving_base_plane(opencascade::handle<AIS_Shape> moving_shape)
{
    gp_Pnt oripnt(0,0,0);
    TopLoc_Location location;
    gp_Dir oridir=m_view->Camera()->Direction();
    return gp_Pln(oripnt,oridir);

}

bool OCCTWidget::select(TopAbs_ShapeEnum select_mode)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        return false;
    }

    // Selection modes are object-scoped. Do not use the context-wide
    // Activate() overload here, because it can leave an injector mode active
    // while the next event is trying to pick a reference-geometry face.
    for (auto it = unit_hash.constBegin(); it != unit_hash.constEnd(); ++it)
    {
        const std::shared_ptr<Unit> unit = it.value();
        if (unit == nullptr || unit->ais_display.IsNull())
        {
            continue;
        }

        m_context->SetSelectionModeActive(
            unit->ais_display, -1, Standard_False,
            AIS_SelectionModesConcurrency_Single);
        m_context->SetSelectionModeActive(
            unit->ais_display, select_mode, Standard_True,
            AIS_SelectionModesConcurrency_Single, Standard_True);
    }

    selected_shape.Nullify();
    if (!m_context->HasDetected())
    {
        return false;
    }

    const Handle(AIS_InteractiveObject) detected_object =
        m_context->DetectedInteractive();
    if (detected_object.IsNull() || detected_object->Type() == 1)
    {
        return false;
    }

    //qDebug()<<detected_object->Type();
    selected_shape=Handle(AIS_Shape)::DownCast(detected_object);
    if (selected_shape.IsNull())
    {
        return false;
    }

    m_view->Update();

    if(selected_shape->HasColor())
    {
        Handle(Prs3d_Drawer) t_select_style = m_context->SelectionStyle();  // 获取选择风格
        Quantity_Color color;
        selected_shape->Color(color);
        t_select_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
        t_select_style->SetColor(color);   // 设置选择后颜色
        t_select_style->SetDisplayMode(AIS_Shaded); // 整体高亮
        t_select_style->SetTransparency(0.8f); // 设置透明度 // 设置选择后颜色
    }
    m_context->SelectDetected();
    m_view->Update();

    return true;
}

bool OCCTWidget::select_injector()
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        return false;
    }

    if (select(TopAbs_COMPOUND) && get_unit(selected_shape) != nullptr)
    {
        emit selection_changed(get_unit(selected_shape)->inj.uuid, false);
        return true;
    }

    if (select(TopAbs_SHAPE) && get_unit(selected_shape) != nullptr)
    {
        emit selection_changed(get_unit(selected_shape)->inj.uuid, false);
        return true;
    }

    selected_shape.Nullify();
    return false;
}

Unit *OCCTWidget::get_unit(Handle(AIS_Shape) shape)
{
    if(shape.IsNull() || !shape->HasOwner())
    {
        return nullptr;
    }

    Handle(Unit_Owner) owner = Handle(Unit_Owner)::DownCast(shape->GetOwner());
    if(owner.IsNull() || !owner->IsValid())
    {
        return nullptr;
    }

    return owner->get_unit();
}

void OCCTWidget::open_edit_widget(opencascade::handle<AIS_Shape> shape)
{
    Unit *unit = get_unit(shape);
    if (unit == nullptr)
    {
        return;
    }

    const quintptr target_unit_ptr = reinterpret_cast<quintptr>(unit);
    for (const QPointer<unit_edit_dialog> &dialog : m_open_edit_dialogs)
    {
        if (dialog == nullptr)
        {
            continue;
        }

        if (dialog->property("unit_ptr").value<quintptr>() == target_unit_ptr)
        {
            if (!dialog->isVisible())
            {
                begin_unit_edit_transaction(unit);
                dialog->reset_edit_state();
                dialog->refresh_from_unit_data(unit);
                dialog->show();
            }
            dialog->raise();
            dialog->activateWindow();
            return;
        }
    }

    begin_unit_edit_transaction(unit);

    unit_edit_dialog* inj_edit_dialog = new unit_edit_dialog(unit,
                                                             m_chemkin_species_names,
                                                             m_material_names,
                                                             this);
    inj_edit_dialog->set_case_context(m_unit_editor_case_context);
    inj_edit_dialog->setProperty("unit_ptr", QVariant::fromValue(target_unit_ptr));
    m_open_edit_dialogs.append(inj_edit_dialog);
    connect(inj_edit_dialog, &QObject::destroyed, this, [this, inj_edit_dialog]()
    {
        m_open_edit_dialogs.removeAll(inj_edit_dialog);
    });
    connect(inj_edit_dialog, &unit_edit_dialog::dialog_closed, this,
            [this, inj_edit_dialog](Unit *closed_unit)
    {
        if (closed_unit == nullptr)
        {
            return;
        }

        finish_unit_edit_transaction(closed_unit,
                                     inj_edit_dialog->has_unsaved_changes());

        if (!selected_shape.IsNull() && get_unit(selected_shape) == closed_unit)
        {
            selected_shape.Nullify();
        }

        clear_context_selection_safely();
    });
    connect(inj_edit_dialog, &unit_edit_dialog::dialog_cancelled, this,
            [this](Unit *cancelled_unit)
    {
        cancel_unit_edit_transaction(cancelled_unit);
    });
    connect(inj_edit_dialog, &unit_edit_dialog::injector_data_changed, this, [this](Unit *changed_unit)
    {
        emit unit_data_updated(changed_unit);
    });
    connect(inj_edit_dialog, &unit_edit_dialog::injector_geometry_changed, this, [this](Unit *changed_unit)
    {
        schedule_unit_visual_refresh(changed_unit);
    });
    connect(this, &OCCTWidget::unit_data_updated,
            inj_edit_dialog, &unit_edit_dialog::refresh_from_unit_data);
    connect(this, &OCCTWidget::unit_position_updated,
            inj_edit_dialog, &unit_edit_dialog::refresh_geometry_from_unit_data);
    inj_edit_dialog->show();
}

void OCCTWidget::schedule_unit_visual_refresh(Unit *unit)
{
    if (unit == nullptr || !unit_hash.contains(unit->inj.uuid))
    {
        return;
    }

    const QUuid uuid = unit->inj.uuid;
    if (m_pending_visual_refreshes.contains(uuid))
    {
        return;
    }

    m_pending_visual_refreshes.insert(uuid);
    // Give a burst of field commits one short coalescing window. Data remains
    // synchronized immediately; only the expensive OCCT rebuild is deferred.
    QTimer::singleShot(30, this, [this, uuid]()
    {
        m_pending_visual_refreshes.remove(uuid);
        const std::shared_ptr<Unit> current_unit = unit_hash.value(uuid);
        if (current_unit != nullptr)
        {
            refresh_unit_visual(current_unit.get());
        }
    });
}

void OCCTWidget::refresh_unit_visual(Unit *unit)
{
    if (unit == nullptr || m_context.IsNull() || unit->ais_display.IsNull())
    {
        return;
    }

    const std::shared_ptr<Unit> current_unit = unit_hash.value(unit->inj.uuid);
    if (current_unit == nullptr || current_unit.get() != unit)
    {
        return;
    }

    if (!unit->inj.create_injector())
    {
        const QString message = QString("Failed to rebuild geometry for injector '%1'.")
                                    .arg(unit->inj.injector_data.name);
        qWarning() << message;
        emit unit_geometry_refresh_failed(unit->inj.uuid, message);
        return;
    }

    unit->ais_display->SetLocalTransformation(gp_Trsf());
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);
    m_context->Redisplay(unit->ais_display, Standard_False);
    update_unit_local_coordinate_frame(unit->inj.uuid);
    if (unit->has_array_spec && !unit->is_array_child)
    {
        rebuild_unit_array(unit->inj.uuid);
    }
    m_view->Redraw();
}

void OCCTWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        event->ignore();
        return;
    }

    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    if((event->buttons()&Qt::LeftButton) && myIsDragging && !selected_shape.IsNull())
    {
        Standard_Real occt_x1,occt_y1,occt_z1;
        Standard_Real occt_x2,occt_y2,occt_z2;

        m_view->Convert(pos.x(),pos.y(),occt_x1,occt_y1,occt_z1);
        m_view->Convert(m_x_max,m_y_max,occt_x2,occt_y2,occt_z2);

        gp_Pln ref_pln =get_moving_base_plane(selected_shape);

        gp_Pnt2d converted_pnt_pln  = ProjLib::Project(ref_pln,gp_Pnt(occt_x1,occt_y1,occt_z1));
        gp_Pnt2d converted_pnt_pln2 = ProjLib::Project(ref_pln,gp_Pnt(occt_x2,occt_y2,occt_z2));

        gp_Pnt ResultPoint = ElSLib::Value(converted_pnt_pln.X()-converted_pnt_pln2.X(),
                                           converted_pnt_pln.Y()-converted_pnt_pln2.Y(),
                                           ref_pln);
        gp_Trsf trsf;
        const QVector3D delta_vec = to_qvector3d(ResultPoint);

        if (selected_shape == base_geometry)
        {
            if (!m_reference_geometry_locked)
            {
                m_reference_position += delta_vec;
                apply_reference_transform();
            }

            m_x_max = pos.x();
            m_y_max = pos.y();
            return;
        }

        trsf.SetTranslation(gp_Vec(ResultPoint.X(),ResultPoint.Y(),ResultPoint.Z()));
        selected_shape->SetLocalTransformation(trsf * selected_shape->LocalTransformation());

        if (Unit *unit = get_unit(selected_shape))
        {
            Injector &injector = unit->inj.injector_data;
            injector.pos += delta_vec;
            injector.pos2 += delta_vec;
            injector.ff_center += delta_vec;
            injector.ff_virtual_origin += delta_vec;
            injector.volume_bgeom_min += delta_vec;
            injector.volume_bgeom_max += delta_vec;
            update_unit_local_coordinate_frame(unit->inj.uuid);
            emit unit_position_updated(unit);
        }


        m_context->Update(selected_shape, Standard_True);

        // 更新起始位置
        m_x_max = pos.x();
        m_y_max = pos.y();

        // 重绘视图
        m_view->Update();



        //qDebug()<<ResultPoint.X()<<ResultPoint.Y()<<ResultPoint.Z();

        //qDebug()<<occt_x1-occt_x2<<occt_y1-occt_y2<<occt_z1-occt_z1;
    }
    // else if(event->buttons()&Qt::RightButton)
    // {
    //     m_view->Pan(pos.x()-m_x_max,m_y_max-pos.y());
    //     m_x_max=pos.x();
    //     m_y_max=pos.y();
    // }
    else if(event->buttons()&Qt::MiddleButton)
    {
        // 鼠标滚轮键：执行旋转
        m_view->Rotation(pos.x(),pos.y());
    }
    else
    {
        // 将鼠标位置传递到交互环境
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
    }
}

void OCCTWidget::wheelEvent(QWheelEvent *event)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        event->ignore();
        return;
    }

    QPointF pos = event->position();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    m_view->StartZoomAtPoint(pos.x(),pos.y());
    m_view->ZoomAtPoint(0, 0, 0.15*event->angleDelta().x(), 0.15*event->angleDelta().y()); //执行缩放

}

void OCCTWidget::keyPressEvent(QKeyEvent *event)
{
    if (event == nullptr)
    {
        return;
    }

    if (event->key() == Qt::Key_Escape)
    {
        clear_selection();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void OCCTWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_context.IsNull() || m_view.IsNull())
    {
        event->ignore();
        return;
    }

    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    // A context menu is a new interaction boundary. Finalize any pending
    // drag transaction before hit-testing so stale selection state cannot
    // leak into the next mouse event.
    clear_selection();
    ensure_reference_face_selection_mode();
    m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);

    if (!m_context->HasDetected())
    {
        event->ignore();
        return;
    }

    const Handle(AIS_InteractiveObject) detected_object =
        m_context->DetectedInteractive();
    if (detected_object.IsNull())
    {
        event->ignore();
        return;
    }

    // Reference geometry has no Unit_Owner, so it must not go through the
    // injector-only selection path below.
    if (!base_geometry.IsNull() &&
        detected_object == base_geometry)
    {
        const TopoDS_Shape detected_shape = m_context->DetectedShape();
        if (!detected_shape.IsNull() &&
            detected_shape.ShapeType() == TopAbs_FACE)
        {
            // Establish the face reference without changing OCCT's selection
            // state while the context menu is being dispatched.
            show_face_reference(TopoDS::Face(detected_shape));
        }

        // Face selection and its local trihedron are handled by the left
        // mouse path. The right mouse path only refreshes the independent
        // face trihedron from the detected face.
        const bool face_selected = !face_trihedron.IsNull();

        QMenu menu;
        QAction *header_action = new QAction("Reference Geometry", &menu);
        header_action->setEnabled(false);
        menu.addAction(header_action);
        menu.addSeparator();

        QAction *align_action = menu.addAction("Align View to Selected Face");
        align_action->setEnabled(face_selected || !face_trihedron.IsNull());
        connect(align_action, &QAction::triggered, this,
                &OCCTWidget::align_view_to_selected_face);

        QAction *clear_face_action = menu.addAction("Clear Selected Face");
        clear_face_action->setEnabled(face_selected || !face_trihedron.IsNull());
        connect(clear_face_action, &QAction::triggered, this,
                &OCCTWidget::clear_face_reference);

        QAction *reset_transform_action = menu.addAction("Reset Transform");
        reset_transform_action->setEnabled(!m_reference_geometry_locked);
        connect(reset_transform_action, &QAction::triggered, this, [this]()
        {
            set_reference_transform(QVector3D(0.0f, 0.0f, 0.0f),
                                    QVector3D(0.0f, 0.0f, 0.0f));
        });

        QAction *lock_action = menu.addAction(
            m_reference_geometry_locked ? "Unlock Reference Geometry"
                                         : "Lock Reference Geometry");
        connect(lock_action, &QAction::triggered, this, [this]()
        {
            set_reference_geometry_locked(!m_reference_geometry_locked);
        });

        menu.exec(event->globalPos());
        clear_context_selection_safely();
        event->accept();
        return;
    }

    selected_shape = Handle(AIS_Shape)::DownCast(detected_object);
    if (selected_shape.IsNull())
    {
        event->ignore();
        return;
    }

    Unit *selected_unit = get_unit(selected_shape);
    if (selected_unit == nullptr)
    {
        clear_context_selection_safely();
        event->ignore();
        return;
    }
    else if(selected_unit->type==injector)
    {
        QMenu menu;
        QString label_edit=("Edit");
        QString label_copy=("Copy");
        QString label_paste=("Paste to replace");
        QString label_delete=("Delete");

        QAction *headerAction = new QAction(selected_unit->inj.injector_data.name, &menu);
        //QAction *headerAction = new QAction("test");
        headerAction->setEnabled(false);  // 禁用点击

        // 设置表头样式
        QFont headerFont = headerAction->font();
        headerFont.setBold(true);
        headerFont.setPointSize(headerFont.pointSize() + 1);
        headerAction->setFont(headerFont);

        menu.addAction(headerAction);
        menu.addSeparator();  // 分隔线

        QAction* act_edit  = menu.addAction(label_edit  );
        QAction* act_copy  = menu.addAction(label_copy  );
        QAction* act_paste = menu.addAction(label_paste );
        QAction* act_delte = menu.addAction(label_delete);
        act_paste->setEnabled(m_copied_unit.has_value());


        const Handle(AIS_Shape) target_shape = selected_shape;
        connect(act_edit, &QAction::triggered, this,
                [this, target_shape]() { open_edit_widget(target_shape); });
        const QUuid target_uuid = selected_unit->inj.uuid;
        connect(act_copy, &QAction::triggered, this,
                [this, target_uuid]() { copy_unit_by_uuid(target_uuid); });
        connect(act_paste, &QAction::triggered, this,
                [this, target_uuid]() { paste_unit_by_uuid(target_uuid); });
        connect(act_delte, &QAction::triggered, this,
                [this, target_uuid]()
        {
            const std::shared_ptr<Unit> unit = unit_hash.value(target_uuid);
            if (unit == nullptr)
            {
                return;
            }

            const auto answer = QMessageBox::question(
                this, "Delete Injector",
                QString("Delete injector \"%1\"?")
                    .arg(unit->inj.injector_data.name),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes)
            {
                remove_unit_by_uuid(target_uuid);
            }
        });


        menu.exec(event->globalPos());	// 右键菜单被模态显示出来了
        clear_context_selection_safely();
        event->accept();
        return;


    }

    clear_context_selection_safely();
    event->ignore();
}

void OCCTWidget::on_menu_closed()
{
    qDebug() << "菜单已关闭";
    clear_context_selection_safely();
}



