#include "occtwidget.h"
#include <AIS_ViewCube.hxx>

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
    try
    {
        selected_shape.Nullify();

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
    reference_geometry.Nullify();
    base_geometry.Nullify();
    trihedron_main.Nullify();
    axis_placement_main.Nullify();
    m_viewer.Nullify();
    m_graphic_driver.Nullify();
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
        for (auto it = unit_hash.begin(); it != unit_hash.end(); ++it)
        {
            if (it.value() != nullptr && !it.value()->ais_display.IsNull())
            {
                m_context->Remove(it.value()->ais_display, Standard_False);
            }
        }
        unit_hash.clear();
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
}

void OCCTWidget::set_chemkin_species_names(const QStringList &species_names)
{
    m_chemkin_species_names = species_names;
}

Standard_Real OCCTWidget::get_trihedron_size()
{
    return cbrt(geometry.xyz_length.x()*geometry.xyz_length.y()*geometry.xyz_length.z())/10;
}




void OCCTWidget::add_readed_geometry()
{
    builder.Remove(compound,ref_geom);
    ref_geom=geometry.getShape();
    builder.Add(compound,ref_geom);

    base_geometry->Set(compound);

    m_context->Redisplay(base_geometry, Standard_True);
    m_view->FitAll();

    trihedron_main->SetSize(get_trihedron_size());
    m_context->Redisplay(trihedron_main, Standard_True);

    builded=true;

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
        myIsDragging = select() && !selected_shape.IsNull() && get_unit(selected_shape) != nullptr;
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
            m_context->ClearSelected(true);
            m_view->Update();
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
    m_context->Activate(select_mode);
    if(m_context->HasDetected()&&m_context->DetectedInteractive()->Type()!=1)
    {
        //qDebug()<<m_context->DetectedInteractive()->Type();
        Handle(AIS_InteractiveObject) obj;
        obj=m_context->DetectedInteractive();
        selected_shape=Handle(AIS_Shape)::DownCast(obj);
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
        if(m_context->HasDetected()) m_context->SelectDetected();
        m_view->Update();

        //get_unit(selected_shape)->test();

        return true;
    }
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

    unit_edit_dialog* inj_edit_dialog = new unit_edit_dialog(unit, m_chemkin_species_names, this);
    inj_edit_dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(inj_edit_dialog, &unit_edit_dialog::injector_data_changed, this, [this](Unit *changed_unit)
    {
        refresh_unit_visual(changed_unit);
    });
    connect(this, &OCCTWidget::unit_data_updated, inj_edit_dialog, &unit_edit_dialog::refresh_from_unit_data);
    inj_edit_dialog->show();
}

void OCCTWidget::refresh_unit_visual(Unit *unit)
{
    if (unit == nullptr || m_context.IsNull())
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
    if(1)
    {
        QPoint pos = event->pos();
        pos.setX(pos.x()*m_dpi_scale);
        pos.setY(pos.y()*m_dpi_scale);

        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
        if(!select()) return;

        if(get_unit(selected_shape)==nullptr)
        {
            qDebug()<<"1!";
        }
        else if(get_unit(selected_shape)->type==injector)
        {
            QMenu menu;
            QString label_edit=("Edit");
            QString label_copy=("Copy");
            QString label_paste=("Paste to replace");
            QString label_delete=("Delete");

            QAction *headerAction = new QAction(get_unit(selected_shape)->inj.injector_data.name, &menu);
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


            connect(act_edit, &QAction::triggered, this, [this](){ open_edit_widget(selected_shape);});


            QAction *selected_action =menu.exec(QCursor::pos());	// 右键菜单被模态显示出来了
            if(selected_action==nullptr)
            {
                on_menu_closed();
            }


        }
    }

}

void OCCTWidget::on_menu_closed()
{
    qDebug() << "菜单已关闭";
    m_context->ClearSelected(true);
    m_view->Update();
}



