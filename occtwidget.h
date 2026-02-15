#ifndef OCCTWIDGET_H
#define OCCTWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QHash>
#include <QMenu>

#include <QApplication>

#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_CView.hxx>

#include <QWidget>
#ifdef _WIN32
#include <WNT_Window.hxx>
#else
#undef None
#include <Xw_Window.hxx>
#endif

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>

#include <Prs3d_Arrow.hxx>

#include <BRepAlgoAPI_Cut.hxx>

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>

#include <TNaming_Tool.hxx>
#include <TDF_Tool.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <AIS_Trihedron.hxx>
#include <Geom_Axis2Placement.hxx>

#include <ProjLib.hxx>

#include <ElSLib.hxx>

#include <AIS_TextLabel.hxx>//ShapeLabelExtension.hxx>

#include "base_geom_read.h"
#include "unit.h"
#include "unit_edit_dialog.h"


class OCCTWidget : public QWidget
{
    Q_OBJECT

public:
    OCCTWidget(QWidget *parent);
    //! 获取三维环境交互对象
    Handle(AIS_InteractiveContext) m_get_context(){return m_context;}

    //! 获取三维显示界面
    Handle(V3d_View)  m_get_view(){return m_view;}

    void create_cube(Standard_Real _dx = 1.0, Standard_Real _dy = 1.0, Standard_Real _dz = 1.0);

    void create_cylinder(Standard_Real _R = 0.5,  Standard_Real _H = 2.0);

    void create_sphere(Standard_Real _R = 1.0);

    void create_cone(Standard_Real _R1 = 1.0, Standard_Real _R2 = 0.0, Standard_Real _H = 2.0);

    void create_torus(Standard_Real _R1 =2.0, Standard_Real _R2 = 0.5);

    Base_Geom_Read geometry;

    void add_readed_geometry();

    QHash<QUuid,Unit> unit_hash;
private:

    //!初始化交互环境
    void m_initialize_context();
    //!交互式上下文能够管理一个或多个查看器(viewer)中的图形行为和交互式对象的选择
    Handle(AIS_InteractiveContext) m_context;
    //!定义查看器(viewer)类型对象上的服务
    Handle(V3d_Viewer) m_viewer;
    //!创建一个视图
    Handle(V3d_View) m_view;
    //!创建3d接口定义图形驱动程序
    Handle(Graphic3d_GraphicDriver) m_graphic_driver;

    Standard_Real get_trihedron_size();

    gp_Pln get_moving_base_plane(Handle(AIS_Shape) moving_shape);

    bool select(TopAbs_ShapeEnum select_mode=TopAbs_COMPOUND);

    Unit* get_unit(Handle(AIS_Shape) shape);

    void open_edit_widget(Handle(AIS_Shape) shape);

protected:

    void paintEvent(QPaintEvent *) override;

    void resizeEvent(QResizeEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void on_menu_closed();

    //void mouseDoubleClickEvent(QMouseEvent* event) override;

    //! 返回窗口的绘制引擎
    QPaintEngine *paintEngine() const override
    {
        return 0;
    }
protected:
    //!三维场景转换模式
    enum CurrentAction3d
    {
        CurAction3d_Nothing,
        CurAction3d_DynamicPanning, //平移
        CurAction3d_DynamicZooming, //缩放
        CurAction3d_DynamicRotation //旋转
    };
private:
    BRep_Builder builder;
    TopoDS_Compound compound;

    Handle(AIS_Shape) base_geometry;

    Handle(AIS_Shape) reference_geometry;
    TopoDS_Shape ref_geom;
    bool builded=false;

    Standard_Integer m_x_max;    //!记录鼠标平移坐标X
    Standard_Integer m_y_max;    //!记录鼠标平移坐标Y
    CurrentAction3d m_current_mode; //!三维场景转换模式

    bool mouse_middle_mod;
    Standard_Real m_dpi_scale;

    gp_Ax2 coordinate_system_main;
    Handle(Geom_Axis2Placement) axis_placement_main;
    Handle(AIS_Trihedron) trihedron_main;
    Handle(Prs3d_Drawer) drawer_main;

    Standard_Boolean myIsDragging = false;

    Handle(AIS_Shape) selected_shape;

    TopoDS_Face selected_face;


};

#endif // OCCTWIDGET_H
