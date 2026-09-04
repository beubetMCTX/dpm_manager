#ifndef OCCTWIDGET_H
#define OCCTWIDGET_H

#include <QWidget>
#include <QColor>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QHash>
#include <QList>
#include <QMenu>
#include <QPointer>
#include <QStringList>
#include <QSet>
#include <QVector>
#include <QVector3D>

#include <QApplication>
#include <memory>
#include <optional>

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
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>

#include <Prs3d_Arrow.hxx>

#include <BRepAlgoAPI_Cut.hxx>

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>

#include <TNaming_Tool.hxx>
#include <TDF_Tool.hxx>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <AIS_Trihedron.hxx>
#include <Geom_Axis2Placement.hxx>

#include <ProjLib.hxx>

#include <ElSLib.hxx>

#include <AIS_TextLabel.hxx>//ShapeLabelExtension.hxx>

#include "base_geom_read.h"
#include "unit.h"
#include "unit_array.h"
#include "unit_edit_dialog.h"


class OCCTWidget : public QWidget
{
    Q_OBJECT

public:
    OCCTWidget(QWidget *parent);
    ~OCCTWidget() override;
    //! 获取三维环境交互对象
    Handle(AIS_InteractiveContext) m_get_context(){return m_context;}

    //! 获取三维显示界面
    Handle(V3d_View)  m_get_view(){return m_view;}

    Base_Geom_Read geometry;

    void add_readed_geometry();
    bool clear_reference_geometry();
    void set_reference_transform(const QVector3D &position, const QVector3D &rotation_degrees);
    QVector3D reference_position() const { return m_reference_position; }
    QVector3D reference_rotation() const { return m_reference_rotation; }
    bool reference_frame(QVector3D *origin, QVector3D *x_axis,
                         QVector3D *z_axis) const;
    void set_reference_geometry_locked(bool locked);
    bool reference_geometry_locked() const { return m_reference_geometry_locked; }
    void align_view_to_selected_face();
    void begin_reference_transform_transaction();
    void finish_reference_transform_transaction();
    bool undo_reference_transform();
    bool redo_reference_transform();
    bool can_undo_reference_transform() const;
    bool can_redo_reference_transform() const;

    void display_units(const QList<Unit> &units, bool clear_existing = true);
    bool select_unit_by_uuid(const QUuid &uuid);
    bool select_reference_geometry();
    bool set_unit_visible(const QUuid &uuid, bool visible);
    void set_all_units_visible(bool visible);
    bool set_reference_geometry_visible(bool visible);
    bool unit_visible(const QUuid &uuid) const;
    bool reference_geometry_visible() const { return m_reference_geometry_visible; }
    bool set_unit_locked(const QUuid &uuid, bool locked);
    bool unit_locked(const QUuid &uuid) const;
    int translate_units_by_uuid(const QList<QUuid> &uuids,
                                const QVector3D &delta);
    QVector3D unit_position_by_uuid(const QUuid &uuid) const;
    bool set_unit_position_by_uuid(const QUuid &uuid, const QVector3D &position);
    QVector3D unit_direction_by_uuid(const QUuid &uuid) const;
    bool set_unit_direction_by_uuid(const QUuid &uuid, const QVector3D &direction);
    bool unit_single_pitch_yaw_by_uuid(const QUuid &uuid,
                                       double *pitch_degrees,
                                       double *yaw_degrees) const;
    bool set_unit_single_pitch_yaw_by_uuid(const QUuid &uuid,
                                           double pitch_degrees,
                                           double yaw_degrees);
    QVector3D unit_single_target_by_uuid(const QUuid &uuid) const;
    bool set_unit_single_target_by_uuid(const QUuid &uuid, const QVector3D &target);
    Single_Target_Scope unit_single_target_scope_by_uuid(const QUuid &uuid) const;
    bool set_unit_single_target_scope_by_uuid(const QUuid &uuid,
                                              Single_Target_Scope scope);
    int rotate_units_by_uuid(const QList<QUuid> &uuids,
                             const QVector3D &axis,
                             float angle_degrees,
                             const QVector3D &pivot = QVector3D(),
                             bool use_shared_pivot = false);
    int set_material_for_units_by_uuid(const QList<QUuid> &uuids,
                                       const QString &material);
    bool set_unit_name(const QUuid &uuid, const QString &name);
    bool edit_unit_by_uuid(const QUuid &uuid);
    bool remove_unit_by_uuid(const QUuid &uuid);
    bool copy_unit_by_uuid(const QUuid &uuid);
    bool paste_unit_by_uuid(const QUuid &uuid);
    int create_unit_array(const QUuid &source_uuid,
                          const UnitArraySpec &spec);
    int create_unit_fill(const QList<QUuid> &source_uuids,
                         const UnitFillSpec &spec);
    bool create_assembly(const QList<QUuid> &uuids);
    bool detach_from_assembly(const QUuid &uuid);
    bool dissolve_assembly(const QUuid &uuid);
    int rebuild_unit_array(const QUuid &source_uuid);
    int rebuild_unit_fill(const QUuid &source_uuid);
    bool set_unit_follow_array(const QUuid &uuid, bool follow);
    bool restore_unit_array_inheritance(const QUuid &uuid);
    bool has_copied_unit() const { return m_copied_unit.has_value(); }
    void fit_all_view();
    void fit_selected_view();
    void set_standard_view(V3d_TypeOfOrientation orientation);
    void clear_selection();
    bool undo_last_move();
    bool redo_move();
    bool can_undo_move() const;
    bool can_redo_move() const;
    void begin_unit_edit_transaction(Unit *unit);
    void finish_unit_edit_transaction(Unit *unit, bool changed);
    bool cancel_unit_edit_transaction(Unit *unit);
    bool undo_last_edit();
    bool redo_edit();
    bool can_undo_edit() const;
    bool can_redo_edit() const;
    bool undo_last_delete();
    bool redo_delete();
    bool can_undo_delete() const;
    bool can_redo_delete() const;
    void set_chemkin_species_names(const QStringList &species_names);
    void set_species_colors(const QHash<QString, QColor> &species_colors);
    void set_material_names(const QStringList &material_names);
    // Shared case capabilities used by every unit editor.
    void set_unit_editor_case_context(const Unit_Edit_Case_Context &context);
    Unit_Edit_Case_Context unit_editor_case_context() const
    {
        return m_unit_editor_case_context;
    }
    void close_auxiliary_dialogs();
    void discard_auxiliary_dialogs();
    void refresh_open_unit_editors();

    QHash<QUuid, std::shared_ptr<Unit>> unit_hash;

signals:
    void unit_data_updated(Unit *unit);
    void unit_position_updated(Unit *unit);
    void unit_geometry_refresh_failed(const QUuid &uuid,
                                      const QString &message);
    void unit_added(Unit *unit);
    void reference_transform_changed(const QVector3D &position,
                                     const QVector3D &rotation_degrees);
    void reference_geometry_available(bool available);
    void face_reference_changed(bool available);
    void face_reference_info_changed(const QVector3D &origin,
                                     const QVector3D &normal);
    void reference_geometry_lock_changed(bool locked);
    void unit_lock_changed(const QUuid &uuid, bool locked);
    void unit_display_list_changed();
    void unit_removed(const QUuid &uuid);
    void selection_changed(const QUuid &uuid, bool reference_geometry);
    void move_history_changed(bool can_undo, bool can_redo);
    void edit_history_changed(bool can_undo, bool can_redo);
    void delete_history_changed(bool can_undo, bool can_redo);
    void reference_transform_history_changed(bool can_undo, bool can_redo);

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
    bool select_injector();
    void ensure_reference_face_selection_mode();
    bool select_face_reference();
    void clear_face_reference();
    void clear_context_selection_safely();
    void show_face_reference(const TopoDS_Face &face);
    void clear_unit_local_coordinate_frames();
    void rebuild_unit_local_coordinate_frames();
    void update_unit_local_coordinate_frame(const QUuid &uuid);
    void clear_reference_face_coordinate_frames();
    void rebuild_reference_face_coordinate_frames();
    void update_reference_face_coordinate_frames_transform();
    void apply_reference_transform();

    Unit* get_unit(Handle(AIS_Shape) shape);
    void schedule_unit_visual_refresh(Unit *unit);
    void refresh_unit_visual(Unit *unit);
    void clear_unit_array_children(Unit &source);
    Quantity_Color color_for_material(const QString &material) const;
    void refresh_unit_colors();

    void open_edit_widget(Handle(AIS_Shape) shape);
    struct UnitMoveSnapshot
    {
        QVector3D pos;
        QVector3D pos2;
        QVector3D ff_center;
        QVector3D ff_virtual_origin;
        QVector3D volume_bgeom_min;
        QVector3D volume_bgeom_max;
    };
    struct UnitMoveHistoryEntry
    {
        QUuid uuid;
        QUuid batch_id;
        UnitMoveSnapshot before;
        UnitMoveSnapshot after;
    };
    struct UnitEditTransaction
    {
        QUuid uuid;
        Unit_Type before_type = injector;
        Injector before_data;
    };
    struct UnitEditHistoryEntry
    {
        QUuid uuid;
        QUuid batch_id;
        Unit_Type before_type = injector;
        Unit_Type after_type = injector;
        Injector before_data;
        Injector after_data;
    };
    struct CopiedUnit
    {
        Unit_Type type = injector;
        Injector injector_data;
    };
    struct ReferenceTransformHistoryEntry
    {
        QVector3D before_position;
        QVector3D before_rotation;
        QVector3D after_position;
        QVector3D after_rotation;
    };
    struct UnitDeleteHistoryEntry
    {
        QUuid uuid;
        Unit_Type type = injector;
        Injector injector_data;
        bool visible = true;
        bool locked = false;
        bool has_color = false;
        Quantity_Color color;
    };
    UnitMoveSnapshot make_move_snapshot(const Unit &unit) const;
    bool apply_move_snapshot(const UnitMoveHistoryEntry &entry,
                             const UnitMoveSnapshot &snapshot);
    void record_move(const QUuid &uuid,
                     const UnitMoveSnapshot &before,
                     const UnitMoveSnapshot &after);
    void clear_move_history();
    bool apply_edit_snapshot(const UnitEditHistoryEntry &entry,
                             Unit_Type type,
                             const Injector &data);
    void record_edit(const UnitEditTransaction &transaction,
                     const Unit &unit);
    void clear_edit_history();
    void record_reference_transform(const QVector3D &before_position,
                                    const QVector3D &before_rotation,
                                    const QVector3D &after_position,
                                    const QVector3D &after_rotation);
    void clear_reference_transform_history();
    void record_delete(const UnitDeleteHistoryEntry &entry);
    void clear_delete_history();
    bool restore_deleted_unit(const UnitDeleteHistoryEntry &entry);
    bool apply_reference_transform_snapshot(const QVector3D &position,
                                            const QVector3D &rotation);

protected:

    void paintEvent(QPaintEvent *) override;

    void resizeEvent(QResizeEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

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
    Handle(Geom_Axis2Placement) face_axis_placement;
    Handle(AIS_Trihedron) face_trihedron;
    gp_Ax2 selected_face_axis;
    QHash<QUuid, Handle(AIS_Trihedron)> m_unit_local_trihedrons;
    QVector<Handle(AIS_Trihedron)> m_reference_face_trihedrons;
    QVector3D m_reference_position;
    QVector3D m_reference_rotation;
    gp_Trsf m_reference_transform;
    bool m_reference_geometry_locked = false;
    QVector<ReferenceTransformHistoryEntry> m_reference_transform_history;
    int m_reference_transform_history_index = 0;
    bool m_reference_transform_transaction_active = false;
    QVector3D m_reference_transform_before_position;
    QVector3D m_reference_transform_before_rotation;

    QStringList m_chemkin_species_names;
    QStringList m_material_names;
    QHash<QString, QColor> m_species_colors;
    Unit_Edit_Case_Context m_unit_editor_case_context;
    QList<QPointer<unit_edit_dialog>> m_open_edit_dialogs;
    QSet<QUuid> m_pending_visual_refreshes;
    QHash<QUuid, bool> m_unit_visibility;
    QHash<QUuid, bool> m_unit_locks;
    bool m_reference_geometry_visible = true;
    bool m_is_destroying = false;
    std::optional<CopiedUnit> m_copied_unit;
    QVector<UnitMoveHistoryEntry> m_move_history;
    int m_move_history_index = 0;
    QUuid m_active_move_batch_id;
    QHash<QUuid, UnitEditTransaction> m_edit_transactions;
    QVector<UnitEditHistoryEntry> m_edit_history;
    int m_edit_history_index = 0;
    QUuid m_active_edit_batch_id;
    QVector<UnitDeleteHistoryEntry> m_delete_history;
    int m_delete_history_index = 0;
    bool m_replaying_delete_history = false;
    QUuid m_drag_unit_uuid;
    UnitMoveSnapshot m_drag_move_before;
    bool m_drag_move_snapshot_valid = false;


};

#endif // OCCTWIDGET_H
