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
    // 创建一个立方体作测试
    //    TopoDS_Shape t_topo_bottle = MakeBottle(70.0, 50.0, 30.0);
    //    Handle(AIS_Shape) t_ais_bottle = new AIS_Shape(t_topo_bottle);
    //    m_context->Display(t_ais_bottle, Standard_True);
    //    m_view->FitAll();
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
}

void OCCTWidget::create_cube(Standard_Real _dx, Standard_Real _dy, Standard_Real _dz)
{
    TopoDS_Shape t_topo_box = BRepPrimAPI_MakeBox(_dx, _dy, _dz).Shape();
    Handle(AIS_Shape) t_ais_box = new AIS_Shape(t_topo_box);
    m_context->Display(t_ais_box, Standard_True);
    m_view->FitAll();
}

// void OCCTWidget::create_cylinder(Standard_Real _R, Standard_Real _H)
// {
//     TopoDS_Shape t_topo_cylinder = BRepPrimAPI_ma    (_R , _H).Shape();
//     Handle(AIS_Shape) t_ais_cylinder = new AIS_Shape(t_topo_cylinder);
//     m_context->Display(t_ais_cylinder, Standard_True);
//     m_view->FitAll();
// }

void OCCTWidget::create_sphere(Standard_Real _R)
{
    TopoDS_Shape t_topo_sphere = BRepPrimAPI_MakeSphere(_R).Shape();
    Handle(AIS_Shape) t_ais_sphere = new AIS_Shape(t_topo_sphere);
    m_context->Display(t_ais_sphere, Standard_True);
    m_view->FitAll();
}

void OCCTWidget::create_cone(Standard_Real _R1, Standard_Real _R2, Standard_Real _H)
{
    TopoDS_Shape t_topo_cone = BRepPrimAPI_MakeCone(_R1,_R2,_H).Shape();
    Handle(AIS_Shape) t_ais_cone = new AIS_Shape(t_topo_cone);
    m_context->Display(t_ais_cone, Standard_True);
    m_view->FitAll();
}

void OCCTWidget::create_torus(Standard_Real _R1, Standard_Real _R2)
{
    TopoDS_Shape t_topo_torus = BRepPrimAPI_MakeTorus(_R1 ,_R2).Shape();
    Handle(AIS_Shape) t_ais_torus = new AIS_Shape(t_topo_torus);
    m_context->Display(t_ais_torus, Standard_True);
    m_view->FitAll();
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
        //配置查看器的光照
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();
        //设置视图的背景颜色为灰色
        m_view->SetBackgroundColor(Quantity_NOC_BLACK);
        m_view->MustBeResized();
        //显示视方体
        auto view_cube = new AIS_ViewCube();
        auto transform_pers = new Graphic3d_TransformPers(Graphic3d_TMF_TriedronPers,
                                                          Aspect_TOTP_LEFT_LOWER,
                                                          Graphic3d_Vec2i(0, 0));
        view_cube->SetTransformPersistence(transform_pers);
        m_context->Display(view_cube, Standard_True);

        //设置显示模式
        m_context->SetDisplayMode(AIS_Shaded, Standard_True);



        // 设置模型高亮的风格
        Handle(Prs3d_Drawer) t_hilight_style = m_context->HighlightStyle(); // 获取高亮风格
        t_hilight_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
        t_hilight_style->SetColor(Quantity_NOC_LIGHTBLUE);    // 设置高亮颜色
        t_hilight_style->SetDisplayMode(AIS_Shaded); // 整体高亮
        t_hilight_style->SetTransparency(0.9f); // 设置透明度

        // 设置选择模型的风格
        Handle(Prs3d_Drawer) t_select_style = m_context->SelectionStyle();  // 获取选择风格
        t_select_style->SetMethod(Aspect_TOHM_COLOR);  // 颜色显示方式
        t_select_style->SetColor(Quantity_NOC_LIGHTSEAGREEN);   // 设置选择后颜色
        t_select_style->SetDisplayMode(1); // 整体高亮
        t_select_style->SetTransparency(0.99f); // 设置透明度

        m_view->SetZoom(100);   // 放大

        // 激活二维网格
        m_viewer->SetRectangularGridValues(0,0,1,1,0);
        m_viewer->SetRectangularGridGraphicValues(10.01,10.01,0);
        m_viewer->ActivateGrid(Aspect_GT_Rectangular,Aspect_GDM_Lines);
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
        // 鼠标左键按下：初始化平移
        m_x_max=pos.x();
        m_y_max=pos.y();
    }
    else if(event->buttons()&Qt::RightButton)
    {

        // 鼠标右键按下：初始化旋转
        m_view->StartRotation(pos.x(),pos.y());
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

}

void OCCTWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.setX(pos.x()*m_dpi_scale);
    pos.setY(pos.y()*m_dpi_scale);

    if((event->buttons()&Qt::LeftButton))
    {
        m_view->Pan(pos.x()-m_x_max,m_y_max-pos.y());
        m_x_max=pos.x();
        m_y_max=pos.y();
    }
    else if(event->buttons()&Qt::RightButton)
    {
        m_view->Rotation(pos.x(),pos.y());
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
