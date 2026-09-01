#include "occtwidget.h"
#include "runtime_debug.h"
#include <AIS_ViewCube.hxx>
#include <QTimer>
#include <QtMath>

namespace
{
QVector3D to_qvector3d(const gp_Pnt &point)
{
    return QVector3D(static_cast<float>(point.X()),
                     static_cast<float>(point.Y()),
                     static_cast<float>(point.Z()));
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
    discard_auxiliary_dialogs();

    try
    {
        selected_shape.Nullify();
        selected_face.Nullify();

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

void OCCTWidget::create_cube(Standard_Real _dx, Standard_Real _dy, Standard_Real _dz)
{
    Unit unit;
    const std::shared_ptr<Unit> stored_unit = std::make_shared<Unit>(unit);
    unit_hash.insert(stored_unit->inj.uuid, stored_unit);

    stored_unit->ais_display->Set(stored_unit->inj.shape);

    stored_unit->u_owner->set_unit(stored_unit.get());

    qDebug() << stored_unit.get();

    stored_unit->ais_display->SetOwner(stored_unit->u_owner);

    stored_unit->inj.injector_data.name="inj2";

    stored_unit->ais_display->SetColor(Quantity_Color(0.2,0.3,0.9,Quantity_TOC_RGB));

    m_context->Activate(stored_unit->ais_display, TopAbs_SHAPE, Standard_True);

    m_context->Display(stored_unit->ais_display, Standard_True);


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
        for (auto it = unit_hash.begin(); it != unit_hash.end(); ++it)
        {
            if (it.value() != nullptr && !it.value()->ais_display.IsNull())
            {
                m_context->Remove(it.value()->ais_display, Standard_False);
            }
        }
        unit_hash.clear();
        m_unit_visibility.clear();
    }

    static const Quantity_Color palette[] = {
        Quantity_Color(0.90, 0.30, 0.25, Quantity_TOC_RGB),
        Quantity_Color(0.15, 0.55, 0.90, Quantity_TOC_RGB),
        Quantity_Color(0.25, 0.70, 0.35, Quantity_TOC_RGB),
        Quantity_Color(0.90, 0.65, 0.20, Quantity_TOC_RGB),
        Quantity_Color(0.60, 0.40, 0.90, Quantity_TOC_RGB),
        Quantity_Color(0.15, 0.70, 0.70, Quantity_TOC_RGB)
    };

    for (int i = 0; i < units.size(); ++i)
    {
        const Unit &unit = units[i];
        const std::shared_ptr<Unit> stored_unit = std::make_shared<Unit>(unit);
        unit_hash.insert(stored_unit->inj.uuid, stored_unit);
        m_unit_visibility.insert(stored_unit->inj.uuid, true);

        stored_unit->ais_display->Set(stored_unit->inj.shape);
        stored_unit->u_owner->set_unit(stored_unit.get());
        stored_unit->ais_display->SetOwner(stored_unit->u_owner);
        stored_unit->ais_display->SetColor(palette[i % (sizeof(palette) / sizeof(palette[0]))]);
        stored_unit->ais_display->SetTransparency(
            stored_unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);

        m_context->Activate(stored_unit->ais_display, TopAbs_SHAPE, Standard_True);
        m_context->Display(stored_unit->ais_display, Standard_False);
    }

    m_view->FitAll();
    m_view->Redraw();
    emit unit_display_list_changed();
}

bool OCCTWidget::select_unit_by_uuid(const QUuid &uuid)
{
    if (uuid.isNull() || m_context.IsNull() || m_view.IsNull())
    {
        return false;
    }

    const std::shared_ptr<Unit> unit = unit_hash.value(uuid);
    if (unit == nullptr || unit->ais_display.IsNull())
    {
        return false;
    }

    clear_face_reference();
    m_context->ClearSelected(Standard_False);
    selected_shape = unit->ais_display;
    m_context->AddOrRemoveSelected(selected_shape, Standard_True);
    m_view->Redraw();
    emit selection_changed(uuid, false);
    return true;
}

bool OCCTWidget::select_reference_geometry()
{
    if (base_geometry.IsNull() || m_context.IsNull() || m_view.IsNull() ||
        ref_geom.IsNull())
    {
        return false;
    }

    m_context->ClearSelected(Standard_False);
    selected_shape = base_geometry;
    m_context->AddOrRemoveSelected(base_geometry, Standard_True);
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

    m_unit_visibility.insert(uuid, visible);
    if (visible)
    {
        m_context->Display(unit->ais_display, Standard_False);
    }
    else
    {
        m_context->Erase(unit->ais_display, Standard_False);
    }

    if (!visible && selected_shape == unit->ais_display)
    {
        clear_context_selection_safely();
    }
    m_view->Redraw();
    return true;
}

bool OCCTWidget::set_reference_geometry_visible(bool visible)
{
    if (base_geometry.IsNull() || m_context.IsNull() || ref_geom.IsNull())
    {
        return false;
    }

    m_reference_geometry_visible = visible;
    if (visible)
    {
        m_context->Display(base_geometry, Standard_False);
        if (!face_trihedron.IsNull())
        {
            m_context->Display(face_trihedron, Standard_False);
        }
    }
    else
    {
        m_context->Erase(base_geometry, Standard_False);
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

void OCCTWidget::set_chemkin_species_names(const QStringList &species_names)
{
    m_chemkin_species_names = species_names;
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
    return cbrt(geometry.xyz_length.x()*geometry.xyz_length.y()*geometry.xyz_length.z())/10;
}




void OCCTWidget::add_readed_geometry()
{
    clear_face_reference();
    set_reference_geometry_locked(false);
    set_reference_transform(QVector3D(0.0f, 0.0f, 0.0f),
                            QVector3D(0.0f, 0.0f, 0.0f));
    builder.Remove(compound,ref_geom);
    ref_geom=geometry.getShape();
    builder.Add(compound,ref_geom);

    base_geometry->Set(compound);
    m_reference_geometry_visible = true;

    m_context->Redisplay(base_geometry, Standard_True);
    m_view->FitAll();

    trihedron_main->SetSize(get_trihedron_size());
    m_context->Redisplay(trihedron_main, Standard_True);

    builded=true;
    emit reference_geometry_available(true);

}

void OCCTWidget::set_reference_transform(const QVector3D &position,
                                         const QVector3D &rotation_degrees)
{
    m_reference_position = position;
    m_reference_rotation = rotation_degrees;
    apply_reference_transform();
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
    m_view->Redraw();
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
    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    if (event->button() == Qt::LeftButton)
    {
        m_x_max=pos.x();
        m_y_max=pos.y();
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);

    if (select_face_reference())
        {
            selected_shape = base_geometry;
            myIsDragging = !m_reference_geometry_locked;
            emit selection_changed(QUuid(), true);
            return;
        }

        clear_face_reference();
        myIsDragging = select_injector();
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
    if (event->button() == Qt::LeftButton)
    {
        QPoint pos = event->pos();
        pos.setX(pos.x()*m_dpi_scale);
        pos.setY(pos.y()*m_dpi_scale);
        // 将鼠标位置传递到交互环境
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
        if(myIsDragging)
        {
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

    selected_shape.Nullify();
    m_context->Activate(select_mode);
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
            dialog->raise();
            dialog->activateWindow();
            return;
        }
    }

    unit_edit_dialog* inj_edit_dialog = new unit_edit_dialog(unit,
                                                             m_chemkin_species_names,
                                                             m_material_names,
                                                             this);
    inj_edit_dialog->setProperty("unit_ptr", QVariant::fromValue(target_unit_ptr));
    m_open_edit_dialogs.append(inj_edit_dialog);
    connect(inj_edit_dialog, &QObject::destroyed, this, [this, inj_edit_dialog]()
    {
        m_open_edit_dialogs.removeAll(inj_edit_dialog);
    });
    connect(inj_edit_dialog, &unit_edit_dialog::dialog_closed, this, [this](Unit *closed_unit)
    {
        if (closed_unit == nullptr)
        {
            return;
        }

        if (!selected_shape.IsNull() && get_unit(selected_shape) == closed_unit)
        {
            selected_shape.Nullify();
        }

        clear_context_selection_safely();
    });
    connect(inj_edit_dialog, &unit_edit_dialog::injector_data_changed, this, [this](Unit *changed_unit)
    {
        emit unit_data_updated(changed_unit);
    });
    connect(inj_edit_dialog, &unit_edit_dialog::injector_geometry_changed, this, [this](Unit *changed_unit)
    {
        schedule_unit_visual_refresh(changed_unit);
    });
    connect(this, &OCCTWidget::unit_data_updated, inj_edit_dialog, &unit_edit_dialog::refresh_from_unit_data);
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
    QTimer::singleShot(0, this, [this, uuid]()
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
        qDebug() << "Failed to rebuild injector geometry for" << unit->inj.injector_data.name;
        return;
    }

    unit->ais_display->SetLocalTransformation(gp_Trsf());
    unit->ais_display->Set(unit->inj.shape);
    unit->ais_display->SetTransparency(
        unit->inj.injector_data.injection_type == volume ? 0.82f : 0.0f);
    m_context->Redisplay(unit->ais_display, Standard_False);
    m_view->Redraw();
    emit unit_data_updated(unit);
}

void OCCTWidget::mouseMoveEvent(QMouseEvent *event)
{
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
            emit unit_data_updated(unit);
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
    QPointF pos = event->position();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    m_view->StartZoomAtPoint(pos.x(),pos.y());
    m_view->ZoomAtPoint(0, 0, 0.15*event->angleDelta().x(), 0.15*event->angleDelta().y()); //执行缩放

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

    m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
    selected_shape.Nullify();

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


        const Handle(AIS_Shape) target_shape = selected_shape;
        connect(act_edit, &QAction::triggered, this,
                [this, target_shape]() { open_edit_widget(target_shape); });


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



