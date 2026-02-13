#include "occtwidget.h"
#include <AIS_ViewCube.hxx>



OCCTWidget::OCCTWidget(QWidget *parent) : QWidget(parent), m_dpi_scale(this->devicePixelRatioF())
{
    //配置QWidget
    setBackgroundRole( QPalette::NoRole );  //无背景
    setMouseTracking( true );   //开启鼠标位置追踪

    if (m_context.IsNull()) // 若未定义交互环境
    {
        m_initialize_context(); // 初始化交互环境
    }
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);


}

void OCCTWidget::create_cube(Standard_Real _dx, Standard_Real _dy, Standard_Real _dz)
{
    Unit unit;
    unit_hash.insert(unit.inj.uuid,unit);

    unit_hash[unit.inj.uuid].ais_display->Set(unit_hash[unit.inj.uuid].inj.shape);

    unit_hash[unit.inj.uuid].inj.injector_data.name="inj2";

    unit_hash[unit.inj.uuid].ais_display->SetColor(Quantity_Color(0.2,0.3,0.9,Quantity_TOC_RGB));

    m_context->Activate(unit_hash[unit.inj.uuid].ais_display, TopAbs_SHAPE, Standard_True);

    m_context->Display(unit_hash[unit.inj.uuid].ais_display, Standard_True);


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

    if((event->buttons()&Qt::LeftButton))
    {
        myIsDragging = true;
        m_x_max=pos.x();
        m_y_max=pos.y();
        m_context->MoveTo(pos.x(),pos.y(),m_view,Standard_True);
        select();
    }
    else if(event->buttons()&Qt::RightButton)
    {
        // 鼠标左键按下：初始化平移
        m_x_max=pos.x();
        m_y_max=pos.y();
    }
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
    if(m_context->HasDetected())
    {
        Handle(AIS_InteractiveObject) obj;
        obj=m_context->DetectedInteractive();
        moving_shape=Handle(AIS_Shape)::DownCast(obj);
        m_view->Update();

        if(moving_shape->HasColor())
        {
            Handle(Prs3d_Drawer) t_select_style = m_context->SelectionStyle();  // 获取选择风格
            Quantity_Color color;
            moving_shape->Color(color);
            t_select_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
            t_select_style->SetColor(color);   // 设置选择后颜色
            t_select_style->SetDisplayMode(AIS_Shaded); // 整体高亮
            t_select_style->SetTransparency(0.8f); // 设置透明度 // 设置选择后颜色
        }

        m_context->SelectDetected();
        m_view->Update();
        //qDebug()<<moving_shape->Shape().Orientation();
        return true;
    }
    return false;
}

void OCCTWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    if((event->buttons()&Qt::LeftButton))
    {
        Standard_Real occt_x1,occt_y1,occt_z1;
        Standard_Real occt_x2,occt_y2,occt_z2;

        m_view->Convert(pos.x(),pos.y(),occt_x1,occt_y1,occt_z1);
        m_view->Convert(m_x_max,m_y_max,occt_x2,occt_y2,occt_z2);

        gp_Pln ref_pln =get_moving_base_plane(moving_shape);

        gp_Pnt2d converted_pnt_pln  = ProjLib::Project(ref_pln,gp_Pnt(occt_x1,occt_y1,occt_z1));
        gp_Pnt2d converted_pnt_pln2 = ProjLib::Project(ref_pln,gp_Pnt(occt_x2,occt_y2,occt_z2));

        gp_Pnt ResultPoint = ElSLib::Value(converted_pnt_pln.X()-converted_pnt_pln2.X(),
                                           converted_pnt_pln.Y()-converted_pnt_pln2.Y(),
                                           ref_pln);
        gp_Trsf trsf;

        trsf.SetTranslation(gp_Vec(ResultPoint.X(),ResultPoint.Y(),ResultPoint.Z()));
        moving_shape->SetLocalTransformation(trsf * moving_shape->LocalTransformation());


        m_context->Update(moving_shape, Standard_True);

        // 更新起始位置
        m_x_max = pos.x();
        m_y_max = pos.y();

        // 重绘视图
        m_view->Update();



        //qDebug()<<ResultPoint.X()<<ResultPoint.Y()<<ResultPoint.Z();

        //qDebug()<<occt_x1-occt_x2<<occt_y1-occt_y2<<occt_z1-occt_z1;
    }
    else if(event->buttons()&Qt::RightButton)
    {
        m_view->Pan(pos.x()-m_x_max,m_y_max-pos.y());
        m_x_max=pos.x();
        m_y_max=pos.y();
    }
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



