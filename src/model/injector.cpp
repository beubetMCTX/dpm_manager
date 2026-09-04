#include "injector.h"
#include "runtime_debug.h"

#include <qdebug.h>
#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr float kTiny = 1.0e-6f;
constexpr Standard_Integer kPreviewArrowCount = 8;
constexpr Standard_Real kBaseThickness = 0.01;
// Keep parsing/data intact while allowing experimental previews to be enabled
// explicitly through the DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW build option.
#ifdef DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW
constexpr bool kEnableAdvancedAtomizerGeometryPreview = true;
#else
constexpr bool kEnableAdvancedAtomizerGeometryPreview = false;
#endif

double clamp_double(double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}

double deg_to_rad(double degree)
{
    return degree * kPi / 180.0;
}

QVector3D normalized_or(const QVector3D &value, const QVector3D &fallback);

QVector3D single_direction(const Injector &injector)
{
    switch (injector.single_direction_mode)
    {
    case Single_Direction_Mode::Pitch_Yaw:
    {
        const double pitch = deg_to_rad(injector.single_pitch_degrees);
        const double yaw = deg_to_rad(injector.single_yaw_degrees);
        return QVector3D(static_cast<float>(std::cos(pitch) * std::cos(yaw)),
                         static_cast<float>(std::cos(pitch) * std::sin(yaw)),
                         static_cast<float>(std::sin(pitch)));
    }
    case Single_Direction_Mode::Target_Hitpoint:
        return normalized_or(injector.single_target_hitpoint - injector.pos,
                             injector.vel);
    case Single_Direction_Mode::Vector:
    default:
        return normalized_or(injector.vel, QVector3D(1.0f, 0.0f, 0.0f));
    }
}

QVector3D single_velocity(const Injector &injector)
{
    const double speed = injector.vel.length();
    return single_direction(injector) * static_cast<float>(speed);
}

QVector3D normalized_or(const QVector3D &value, const QVector3D &fallback)
{
    if (value.length() > kTiny)
    {
        return value.normalized();
    }
    if (fallback.length() > kTiny)
    {
        return fallback.normalized();
    }
    return QVector3D(0.0f, 0.0f, 1.0f);
}

void build_basis(const QVector3D &axis_vec, QVector3D &tangent_vec, QVector3D &bitangent_vec)
{
    QVector3D ref_vec(1.0f, 0.0f, 0.0f);
    if (std::abs(QVector3D::dotProduct(axis_vec, ref_vec)) > 0.99f)
    {
        ref_vec = QVector3D(0.0f, 1.0f, 0.0f);
    }

    tangent_vec = QVector3D::crossProduct(axis_vec, ref_vec);
    if (tangent_vec.length() <= kTiny)
    {
        tangent_vec = QVector3D::crossProduct(axis_vec, QVector3D(0.0f, 0.0f, 1.0f));
    }
    tangent_vec.normalize();

    bitangent_vec = QVector3D::crossProduct(axis_vec, tangent_vec);
    if (bitangent_vec.length() <= kTiny)
    {
        bitangent_vec = QVector3D(0.0f, 1.0f, 0.0f);
    }
    bitangent_vec.normalize();
}

gp_Pnt to_pnt(const QVector3D &value)
{
    return gp_Pnt(value.x(), value.y(), value.z());
}

gp_Dir to_dir(const QVector3D &value, const QVector3D &fallback = QVector3D(0.0f, 0.0f, 1.0f))
{
    QVector3D dir_vec = normalized_or(value, fallback);
    return gp_Dir(dir_vec.x(), dir_vec.y(), dir_vec.z());
}

gp_Ax2 to_ax2(const QVector3D &origin, const QVector3D &direction, const QVector3D &fallback = QVector3D(0.0f, 0.0f, 1.0f))
{
    return gp_Ax2(to_pnt(origin), to_dir(direction, fallback));
}

QVector3D blend_vec(const QVector3D &lhs, const QVector3D &rhs, double t)
{
    return lhs * static_cast<float>(1.0 - t) + rhs * static_cast<float>(t);
}

double characteristic_speed(const Injector &injector)
{
    if (injector.vel.length() > kTiny)
    {
        return injector.vel.length();
    }
    if (injector.vel2.length() > kTiny)
    {
        return injector.vel2.length();
    }
    if (injector.vel_mag > kTiny)
    {
        return injector.vel_mag;
    }
    return 1.0;
}

double characteristic_flow(const Injector &injector)
{
    if (injector.total_flow_rate > 0.0)
    {
        return injector.total_flow_rate;
    }
    if (injector.flow_rate > 0.0)
    {
        return injector.flow_rate;
    }
    if (injector.flow_rate2 > 0.0)
    {
        return injector.flow_rate2;
    }
    return 0.0;
}

double fallback_size_hint(const Injector &injector)
{
    double size_hint = 0.001;
    size_hint = std::max(size_hint, injector.diameter);
    size_hint = std::max(size_hint, injector.diameter2);
    size_hint = std::max(size_hint, injector.inner_diameter);
    size_hint = std::max(size_hint, injector.outer_diameter);
    size_hint = std::max(size_hint, injector.radius);
    size_hint = std::max(size_hint, injector.inner_radius);
    size_hint = std::max(size_hint, injector.ff_oriface_width);
    size_hint = std::max(size_hint, injector.plain_length);
    size_hint = std::max(size_hint, injector.volume_bgeom_radius);
    return size_hint;
}

Standard_Real preview_arrow_length(const Injector &injector)
{
    double speed = std::max(characteristic_speed(injector), 1.0e-6);
    // Preview geometry is stored in metres. Keep the empirical velocity
    // scale visually comparable to the millimetre-sized injector bodies.
    constexpr double kVelocityPreviewScale = 1.0e-3;
    return static_cast<Standard_Real>(std::max(
        1.0e-3, kVelocityPreviewScale * std::sqrt(speed)));
}

Standard_Real preview_arrow_radius(const Injector &injector)
{
    double flow = characteristic_flow(injector);
    double speed = characteristic_speed(injector);
    if (flow > 0.0 && speed > kTiny)
    {
        constexpr double kFlowPreviewScale = 1.0e-3;
        return static_cast<Standard_Real>(std::max(
            1.0e-5, kFlowPreviewScale * 3.0 * std::sqrt(flow / speed)));
    }
    return static_cast<Standard_Real>(std::max(1.0e-4, 0.25 * fallback_size_hint(injector)));
}

double cone_half_angle_rad(const Injector &injector)
{
    if (std::abs(injector.cone_angle) > 0.0)
    {
        return 0.5 * deg_to_rad(injector.cone_angle);
    }
    if (std::abs(injector.atomizer_disp_angle) > 0.0)
    {
        return 0.5 * deg_to_rad(injector.atomizer_disp_angle);
    }
    return 0.0;
}

double atomizer_half_angle_rad(const Injector &injector)
{
    if (std::abs(injector.half_angle) > 0.0)
    {
        return injector.half_angle;
    }
    if (std::abs(injector.atomizer_disp_angle) > 0.0)
    {
        return 0.5 * deg_to_rad(injector.atomizer_disp_angle);
    }
    return 0.0;
}

QVector3D preferred_axis(const Injector &injector)
{
    if (injector.atomizer_axis.length() > kTiny)
    {
        return injector.atomizer_axis.normalized();
    }
    if (injector.axis.length() > kTiny)
    {
        return injector.axis.normalized();
    }
    if (injector.vel.length() > kTiny)
    {
        return injector.vel.normalized();
    }
    if (injector.vel2.length() > kTiny)
    {
        return injector.vel2.normalized();
    }
    return QVector3D(0.0f, 0.0f, 1.0f);
}

bool is_advanced_atomizer_preview_type(Injection_Type type)
{
    switch (type)
    {
    case plain_oriface_atomizer:
    case pressure_swirl_atomizer:
    case air_blast_atomizer:
    case flat_fan_atomizer:
    case effervescent_atomizer:
        return true;
    default:
        return false;
    }
}
}





Injector::Injector():

    name("injector"),
    type(Droplet),
    injection_type(single),
    local_reference_frame("global"),
    numpts(10),
    dpm_fname("\" \""),
    surfaces({-1}),
    boundary({-1}),
    stochastic(false),
    random_eddy(false),
    ntries(1),
    time_scale_constant(0.15),
    cloud(false),
    cloud_min_dia(0),
    cloud_max_dia(100000),
    material(""),
    scale_by_area(false),
    use_face_normal(false),
    random_surface(false),
    devolatilizing_species(""),
    evaporating_species(""),
    oxidizing_species(""),
    product_species(""),
    rr_disturb(true),
    rr_uniform_ln_d(false),
    evaporating_liquid(false),
    evaporating_material(""),
    liquid_fraction(-1),
    dpm_domain("none"),
    collision_partner("dem-unknow"),
    parcel_number(500),
    parcel_mass(1e-9),
    parcel_diameter(1e-5),
    parcel_model(standard),
    drag_law(spherical),
    shape_factor(1),
    cunningham_correction(1),
    drag_fcn("none"),
    brownian_motion(false),
    seco_breakup_on(false),
    seco_breakup_tab(false),
    seco_breakup_wave(false),
    seco_break_up_khrt(false),
    seco_breakup_ssd(false),
    seco_breakup_madahushi(false),
    seco_breakup_schmehl(false),
    seco_breakup_tab_y0(0),
    number_tab_diameters(0),
    seco_breakup_wave_b1(0),
    seco_breakup_wave_b0(0),
    seco_breakup_khrt_cl(0),
    seco_breakup_khrt_ctau(0),
    seco_breakup_khrt_crt(0),
    seco_breakup_ssd_we_cr(0),
    seco_breakup_ssd_core_bu(0),
    seco_breakup_ssd_np_target(0),
    seco_breakup_ssd_x_si(0),
    seco_breakup_madabushi_c0(0),
    seco_breakup_madabushi_column_drag_cd(0),
    seco_breakup_madabushi_ligament_factor(0),
    seco_breakup_madabushi_jet_diameter(0),
    seco_breakup_schmehl_np(0),
    //swit("\"Default\""),
    //udf_inject_init("\"none\""),
    //udf_heat_mass("none"),
    volume_specification(zone),
    volume_streams_spec(total_parcel_count),
    volume_streams_total(0),
    volume_bgeom_shapes(sphere),
    volume_bgeom_min(0, 0, 0),
    volume_bgeom_max(0, 0, 0),
    volume_bgeom_radius(0),
    volume_bgeom_viconeangle(0),
    mass_input_on(false),
    volfrac_input_on(false),
    rotation_on(false),
    rot_drag_law(none),
    rot_lift_law(none_),
    cone_type(point),
    uniform_mass_dist_on(false),
    spatial_staggering_std_inj_on(false),
    spatial_staggering_atomizer_on(true),
    stagger_radius(0),
    rough_wall_on(false),
    cphace_domain("none"),
    pos(0, 0, 0),
    pos2(0, 0, 0),
    ff_center(0, 1, 0),
    ff_virtual_origin(1, 0, 0),
    ff_normal(1, 0, 0),
    vel(100, 0, 0),
    vel2(100, 0, 0),
    ang_vel(100, 0, 0),
    ang_vel2(100, 0, 0),
    atomizer_axis(1, 0, 0),
    diameter(0.01),
    diameter2(0.005),
    temperature(0),
    temperature2(0),
    flow_rate(0.0),
    flow_rate2(0),
    unsteady_start(0),
    unsteady_stop(0),
    start_at_flow_time_in_unsteady_inj_file(0),
    interval_to_repeat_in_unsteady_inj_file(0),
    unsteady_ca_start(0),
    unsteady_ca_stop(0),
    vapor_pressure(0),
    inner_diameter(0),
    outer_diameter(0),
    half_angle(0),
    plain_length(0),
    plain_corner_size(0),
    plain_const_a(0),
    pswirl_inj_press(0),
    airbl_rel_vel(0),
    effer_quality(0),
    effer_t_sat(0),
    ff_oriface_width(0),
    phi_start(0),
    phi_stop(0),
    sheet_const(0),
    lig_const(0),
    effer_const(0),
    effer_half_angle_max(0),
    ff_sheet_const(0),
    atomizer_disp_angle(0),
    axis(1, 0, 0),
    vel_mag(0),
    ang_vel_mag(0),
    cone_angle(15),
    inner_radius(0.005),
    radius(0.01),
    swirl_frac(0),
    total_flow_rate(0.3),
    total_mass(0),
    rr_min(0),
    rr_max(0),
    rr_mean(0),
    rr_numdia(0),
    posr(1, 0, 0),
    posu(1, 0, 0)
{

}

// Injector::~Injector()
// {

// }


Injector_OCCT::Injector_OCCT()
{
    if (runtime_debug::verbose_debug_enabled())
    {
        qDebug() << "Injector_OCCT created with uuid" << uuid;
    }
    rebuild_runtime_state();
}

Injector_OCCT::Injector_OCCT(const Injector_OCCT &other)
{
    copy_persistent_state_from(other);
    rebuild_runtime_state();
}

Injector_OCCT &Injector_OCCT::operator=(const Injector_OCCT &other)
{
    if (this == &other)
    {
        return *this;
    }

    copy_persistent_state_from(other);
    rebuild_runtime_state();
    return *this;
}

Injector_OCCT::Injector_OCCT(Injector_OCCT &&other) noexcept
{
    move_persistent_state_from(std::move(other));
    rebuild_runtime_state();
}

Injector_OCCT &Injector_OCCT::operator=(Injector_OCCT &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    move_persistent_state_from(std::move(other));
    rebuild_runtime_state();
    return *this;
}

Injector_OCCT::~Injector_OCCT()
{
    // if (!m_document.IsNull())
    // {
    //     Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    //     app->Close(m_document);
    // }
}

void Injector_OCCT::reset_runtime_state()
{
    shape.Nullify();
    builder.MakeCompound(shape);

    m_document.Nullify();
    m_label = TDF_Label();
    uuid_label = TDF_Label();
    name_label = TDF_Label();
    geometry_label = TDF_Label();
    position_label = TDF_Label();
    position2_label = TDF_Label();
    velocity_label = TDF_Label();
    velocity2_label = TDF_Label();
    axis_label = TDF_Label();
    atomizer_axis_label = TDF_Label();
    material_label = TDF_Label();
    color_label = TDF_Label();
}

void Injector_OCCT::copy_persistent_state_from(const Injector_OCCT &other)
{
    uuid = other.uuid;
    injector_data = other.injector_data;
    edit_mode = other.edit_mode;
}

void Injector_OCCT::move_persistent_state_from(Injector_OCCT &&other)
{
    uuid = other.uuid;
    injector_data = std::move(other.injector_data);
    edit_mode = other.edit_mode;
}

bool Injector_OCCT::rebuild_runtime_state()
{
    reset_runtime_state();

    if (!create_injector())
    {
        return false;
    }

    return initialize_OCAF();
}



bool Injector_OCCT::initialize_OCAF()
{
    try {
        // 获取OCAF应用实例[1,7](@ref)
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        if (app.IsNull())
        {
            return false;
        }

        // 创建新文档[1](@ref)
        app->NewDocument("BinXCAF", m_document);
        if (m_document.IsNull())
        {
            return false;
        }

        // 获取根标签
        m_label = m_document->Main().NewChild();

        uuid_label          = m_label.FindChild(1   ,Standard_True);
        name_label          = m_label.FindChild(2   ,Standard_True);
        geometry_label      = m_label.FindChild(3   ,Standard_True);
        position_label      = m_label.FindChild(4   ,Standard_True);
        position2_label     = m_label.FindChild(5   ,Standard_True);
        velocity_label      = m_label.FindChild(6   ,Standard_True);
        velocity2_label     = m_label.FindChild(7   ,Standard_True);
        //angvel_label        = m_label.FindChild(8   ,Standard_True);
        //angvel2_label       = m_label.FindChild(9   ,Standard_True);
        axis_label          = m_label.FindChild(8   ,Standard_True);
        atomizer_axis_label = m_label.FindChild(9   ,Standard_True);
        material_label      = m_label.FindChild(10  ,Standard_True);
        color_label         = m_label.FindChild(11  ,Standard_True);

        Handle(TDataStd_Name)       uuid_attr       =   TDataStd_Name::     Set(uuid_label,uuid.toString().toStdString().c_str());
        Handle(TDataStd_Name)       name_attr       =   TDataStd_Name::     Set(name_label,injector_data.name.toStdString().c_str());
        Handle(TDataXtd_Shape)  geometry_attr       =   TDataXtd_Shape::    Set(geometry_label,shape);

        TDF_Label pos_x_label = position_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) pos_x_attr = TDataStd_Real::Set(pos_x_label, injector_data.pos.x());

        TDF_Label pos_y_label = position_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) pos_y_attr = TDataStd_Real::Set(pos_y_label, injector_data.pos.y());

        TDF_Label pos_z_label = position_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) pos__attr = TDataStd_Real::Set(pos_z_label, injector_data.pos.z());


        TDF_Label pos2_x_label = position2_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) pos2_x_attr = TDataStd_Real::Set(pos2_x_label, injector_data.pos2.x());

        TDF_Label pos2_y_label = position2_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) pos2_y_attr = TDataStd_Real::Set(pos2_y_label, injector_data.pos2.y());

        TDF_Label pos2_z_label = position2_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) pos2_z_attr = TDataStd_Real::Set(pos2_z_label, injector_data.pos2.z());


        TDF_Label vel_x_label = velocity_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) vel_x_attr = TDataStd_Real::Set(vel_x_label, injector_data.vel.x());

        TDF_Label vel_y_label = velocity_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) vel_y_attr = TDataStd_Real::Set(vel_y_label, injector_data.vel.y());

        TDF_Label vel_z_label = velocity_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) vel_z_attr = TDataStd_Real::Set(vel_z_label, injector_data.vel.z());


        TDF_Label vel2_x_label = velocity2_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) vel2_x_attr = TDataStd_Real::Set(vel2_x_label, injector_data.vel2.x());

        TDF_Label vel2_y_label = velocity2_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) vel2_y_attr = TDataStd_Real::Set(vel2_y_label, injector_data.vel2.y());

        TDF_Label vel2_z_label = velocity2_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) vel2_z_attr = TDataStd_Real::Set(vel2_z_label, injector_data.vel2.z());



        TDF_Label axis_x_label = axis_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) axis_x_attr = TDataStd_Real::Set(axis_x_label, injector_data.axis.x());

        TDF_Label axis_y_label = axis_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) axis_y_attr = TDataStd_Real::Set(axis_y_label, injector_data.axis.y());

        TDF_Label axis_z_label = axis_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) axis_z_attr = TDataStd_Real::Set(axis_z_label, injector_data.axis.z());


        TDF_Label atomizer_axis_x_label = atomizer_axis_label.FindChild(1, Standard_True);
        Handle(TDataStd_Real) atomizer_axis_x_attr = TDataStd_Real::Set(atomizer_axis_x_label, injector_data.atomizer_axis.x());

        TDF_Label atomizer_axis_y_label = atomizer_axis_label.FindChild(2, Standard_True);
        Handle(TDataStd_Real) atomizer_axis_y_attr = TDataStd_Real::Set(atomizer_axis_y_label, injector_data.atomizer_axis.y());

        TDF_Label atomizer_axis_z_label = atomizer_axis_label.FindChild(3, Standard_True);
        Handle(TDataStd_Real) atomizer_axis_z_attr = TDataStd_Real::Set(atomizer_axis_z_label, injector_data.atomizer_axis.z());


        Handle(TDataStd_Name)       material_attr       =   TDataStd_Name::     Set(material_label,injector_data.material.toStdString().c_str());

        Handle(XCAFDoc_Color)       color_attr          =   XCAFDoc_Color::     Set(color_label ,Quantity_Color(0.1,0.2,0.3,Quantity_TOC_RGB));


        TNaming_Builder label_builder(m_label);
        //Handle(AIS_Shape) AIS_TEST;
        label_builder.Generated(shape);



        return true;
    }
    catch (...) {
        return false;
    }
}

// // 创建一个新的文档
// Handle(TDocStd_Document) aDoc = new TDocStd_Document;

// // 获取文档的根目录
// Handle(TDataStd_Root) aRoot = aDoc->GetRoot();

// // 创建一个新的Shape对象
// TopoDS_Shape aShape = BRepPrimAPI_MakeBox(10, 20, 30).Shape();

// // 将Shape对象添加到数据框架中
// Handle(TDataXtd_Shape) aShapeAttribute = new TDataXtd_Shape;
// aShapeAttribute->Set(aShape);
// aRoot->NewChild()->Label().Set(aShapeAttribute);

// // 保存文档
// XmlObjMgt::Save(aDoc, "example.xml");


bool Injector_OCCT::create_injector()
{
    const TopoDS_Compound previous_shape = shape;
    bool success = false;

    if (!kEnableAdvancedAtomizerGeometryPreview &&
        is_advanced_atomizer_preview_type(injector_data.injection_type))
    {
        builder.MakeCompound(shape);
        return true;
    }

    switch(injector_data.injection_type)
    {
    case single:
        success = create_geometry_single();
        break;
    case group:
        success = create_geometry_group(shape);
        break;
    case surface:
        success = create_geometry_group(shape);
        break;
    case volume:
        success = create_geometry_volume(shape);
        break;
    case cone:
        success = create_geometry_cone(shape);
        break;
    case plain_oriface_atomizer:
        success = create_geometry_p_o_a(shape);
        break;
    case pressure_swirl_atomizer:
        success = create_geometry_p_s_a(shape);
        break;
    case air_blast_atomizer:
        success = create_geometry_a_b_a(shape);
        break;
    case flat_fan_atomizer:
        success = create_geometry_f_f_a(shape);
        break;
    case effervescent_atomizer:
        success = create_geometry_e_a(shape);
        break;
    case file_:
        success = create_geometry_group(shape);
        break;
    case condensate:
        success = create_geometry_condensate(shape);
        break;
    default:
        success = false;
        break;
    }

    if (!success)
    {
        shape = previous_shape;
    }

    return success;
}

TopoDS_Compound Injector_OCCT::create_arrow(gp_Ax2 ax2, Standard_Real cyli_diameter, Standard_Real cyli_length, Standard_Real cone_diameter, Standard_Real cone_length)
{
    TopoDS_Compound arrow;
    builder.MakeCompound(arrow);
    gp_Pnt cone_pnt(ax2.Location().X()+cyli_length*ax2.Direction().X(),
                    ax2.Location().Y()+cyli_length*ax2.Direction().Y(),
                    ax2.Location().Z()+cyli_length*ax2.Direction().Z());
    gp_Ax2 cone_locate(cone_pnt,ax2.Direction());

    TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(ax2,cyli_diameter,cyli_length);
    TopoDS_Shape cone =BRepPrimAPI_MakeCone(cone_locate,cone_diameter,0,cone_length);

    builder.Add(arrow,cylinder);
    builder.Add(arrow,cone);

    return arrow;
}

// bool Injector_OCCT::create_geometry(TopoDS_Shape &shape)
// {
//     switch(injector_data.injection_type)
//     {
//     case single:break;
//     case group:
//     case surface:
//     case volume:
//     case cone:
//     case plain_oriface_atomizer:
//     case pressure_swirl_atomizer:
//     case air_blast_atomizer:
//     case flat_fan_atomizer:
//     case effervescent_atomizer:
//     case file_:
//     case condensate:

//     default: return false;
//     }
//     return true;
// }

bool Injector_OCCT::create_geometry_single()
{
    try
    {
        if (injector_data.vel.length() <= 0.0f)
        {
            return false;
        }

        builder.MakeCompound(shape);

        gp_Pnt base_pnt(injector_data.pos.x(),injector_data.pos.y(),injector_data.pos.z());
        const QVector3D effective_velocity = single_velocity(injector_data);
        gp_Dir base_dir(effective_velocity.x(), effective_velocity.y(), effective_velocity.z());
        gp_Ax2 base_ax2(base_pnt,base_dir);
        Standard_Real cyl_l = preview_arrow_length(injector_data);
        Standard_Real cyl_d = preview_arrow_radius(injector_data);
        TopoDS_Shape base = BRepPrimAPI_MakeCylinder(base_ax2,2*cyl_d,kBaseThickness);
        TopoDS_Compound arrow = create_arrow(base_ax2,cyl_d,cyl_l,2*cyl_d,0.25*cyl_l);

        builder.Add(shape,base);
        builder.Add(shape,arrow);

        //builder.Remove(inj,base);
        //builder.Remove(inj,arrow);

        //shape=inj;
    }
    catch (...)
    {
        return false;
    }
        return true;
}

bool Injector_OCCT::create_geometry_ring()
{
    try
    {
        builder.MakeCompound(shape);

        if (injector_data.axis.length() <= 0 ||
            injector_data.radius <= injector_data.inner_radius ||
            injector_data.inner_radius < 0 ||
            injector_data.vel.length() <= 0)
        {
            return false;
        }

        QVector3D axis_vec = injector_data.axis.normalized();
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        gp_Ax2 base_ax2 = to_ax2(injector_data.pos, axis_vec);

        TopoDS_Shape outer_base = BRepPrimAPI_MakeCylinder(base_ax2, injector_data.radius, kBaseThickness);
        TopoDS_Shape ring_base = outer_base;
        if (injector_data.inner_radius > 0)
        {
            TopoDS_Shape inner_base = BRepPrimAPI_MakeCylinder(base_ax2, injector_data.inner_radius, kBaseThickness);
            ring_base = BRepAlgoAPI_Cut(outer_base, inner_base).Shape();
        }

        builder.Add(shape, ring_base);

        Standard_Real cyl_l = preview_arrow_length(injector_data);
        Standard_Real cyl_d = preview_arrow_radius(injector_data);
        Standard_Real half_angle = static_cast<Standard_Real>(0.5 * deg_to_rad(injector_data.cone_angle));
        Standard_Real mid_r = 0.5 * (injector_data.inner_radius + injector_data.radius);

        for (Standard_Integer i = 0; i < kPreviewArrowCount; ++i)
        {
            double theta = 2.0 * kPi * i / kPreviewArrowCount;
            QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                   bitangent_vec * static_cast<float>(std::sin(theta));
            QVector3D start_vec = injector_data.pos + radial_vec * static_cast<float>(mid_r);
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(half_angle)) +
                                radial_vec * static_cast<float>(std::sin(half_angle));
            dir_vec.normalize();

            gp_Pnt arrow_pnt(start_vec.x(), start_vec.y(), start_vec.z());
            gp_Dir arrow_dir(dir_vec.x(), dir_vec.y(), dir_vec.z());
            gp_Ax2 arrow_ax2(arrow_pnt, arrow_dir);

            TopoDS_Compound arrow = create_arrow(arrow_ax2, cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l);
            builder.Add(shape, arrow);
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool Injector_OCCT::create_geometry_group(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        const Standard_Integer point_count = std::max(1, injector_data.numpts);
        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const QVector3D fallback_dir = normalized_or(injector_data.pos2 - injector_data.pos,
                                                     preferred_axis(injector_data));

        QVector3D prev_pos;
        bool has_prev = false;

        for (Standard_Integer i = 0; i < point_count; ++i)
        {
            const double t = (point_count == 1) ? 0.0 : static_cast<double>(i) / static_cast<double>(point_count - 1);
            const QVector3D start_vec = blend_vec(injector_data.pos, injector_data.pos2, t);
            QVector3D dir_vec = blend_vec(injector_data.vel, injector_data.vel2, t);
            dir_vec = normalized_or(dir_vec, fallback_dir);

            builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(start_vec), std::max(1.5 * cyl_d, 0.5 * kBaseThickness)).Shape());

            gp_Ax2 arrow_ax2(to_pnt(start_vec), to_dir(dir_vec, fallback_dir));
            builder.Add(shape, create_arrow(arrow_ax2, cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));

            if (has_prev)
            {
                QVector3D seg_vec = start_vec - prev_pos;
                if (seg_vec.length() > kTiny)
                {
                    gp_Ax2 link_ax2(to_pnt(prev_pos), to_dir(seg_vec, fallback_dir));
                    builder.Add(shape, BRepPrimAPI_MakeCylinder(link_ax2, std::max(0.5 * cyl_d, 1.0e-4), seg_vec.length()).Shape());
                }
            }

            prev_pos = start_vec;
            has_prev = true;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_cone(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = normalized_or(injector_data.axis, preferred_axis(injector_data));
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const Standard_Real half_angle = static_cast<Standard_Real>(cone_half_angle_rad(injector_data));
        const Standard_Real outer_radius = static_cast<Standard_Real>(std::max(injector_data.radius, std::max(4.0 * cyl_d, 0.001)));
        const Standard_Real inner_radius = static_cast<Standard_Real>(clamp_double(injector_data.inner_radius, 0.0, 0.95 * outer_radius));
        const gp_Ax2 base_ax2 = to_ax2(injector_data.pos, axis_vec);

        auto add_cone_arrow = [&](const QVector3D &start_vec, const QVector3D &dir_vec)
        {
            builder.Add(shape, create_arrow(to_ax2(start_vec, dir_vec, axis_vec), cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));
        };

        auto add_surface_ring = [&](double start_radius, double direction_angle, Standard_Integer arrow_count)
        {
            for (Standard_Integer i = 0; i < arrow_count; ++i)
            {
                const double theta = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(arrow_count);
                const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                             bitangent_vec * static_cast<float>(std::sin(theta));
                QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(direction_angle)) +
                                    radial_vec * static_cast<float>(std::sin(direction_angle));
                dir_vec.normalize();
                add_cone_arrow(injector_data.pos + radial_vec * static_cast<float>(start_radius), dir_vec);
            }
        };

        auto add_solid_disk_distribution = [&](Standard_Real disk_radius, Standard_Real max_angle)
        {
            add_cone_arrow(injector_data.pos, axis_vec);

            const QVector<Standard_Integer> ring_counts = {6, 10};
            const QVector<double> ring_ratios = {0.45, 0.82};

            for (int ring_index = 0; ring_index < ring_counts.size(); ++ring_index)
            {
                const double radius_ratio = ring_ratios[ring_index];
                const Standard_Real ring_radius = static_cast<Standard_Real>(radius_ratio * disk_radius);
                const Standard_Real local_angle = static_cast<Standard_Real>(radius_ratio * max_angle);
                const Standard_Integer arrow_count = ring_counts[ring_index];

                for (Standard_Integer i = 0; i < arrow_count; ++i)
                {
                    const double theta = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(arrow_count);
                    const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                                 bitangent_vec * static_cast<float>(std::sin(theta));
                    const QVector3D start_vec = injector_data.pos + radial_vec * static_cast<float>(ring_radius);
                    QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(local_angle)) +
                                        radial_vec * static_cast<float>(std::sin(local_angle));
                    dir_vec.normalize();
                    add_cone_arrow(start_vec, dir_vec);
                }
            }
        };

        switch (injector_data.cone_type)
        {
        case point:
            builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(injector_data.pos), std::max(2.0 * cyl_d, 0.001)).Shape());
            add_cone_arrow(injector_data.pos, axis_vec);
            if (std::abs(half_angle) > 1.0e-6)
            {
                add_surface_ring(0.0, half_angle, 4);
            }
            break;
        case hollow:
        {
            const Standard_Real shell_thickness = static_cast<Standard_Real>(
                std::max(0.75 * cyl_d, std::min(0.06 * outer_radius, 1.2 * cyl_d)));
            const Standard_Real hollow_inner_radius = static_cast<Standard_Real>(
                std::max(0.0, outer_radius - shell_thickness));
            TopoDS_Shape outer_base = BRepPrimAPI_MakeCylinder(base_ax2, outer_radius, kBaseThickness).Shape();
            TopoDS_Shape hollow_base = outer_base;
            if (hollow_inner_radius > 0.0)
            {
                TopoDS_Shape inner_base = BRepPrimAPI_MakeCylinder(base_ax2, hollow_inner_radius, kBaseThickness).Shape();
                hollow_base = BRepAlgoAPI_Cut(outer_base, inner_base).Shape();
            }
            builder.Add(shape, hollow_base);
            add_surface_ring(0.5 * (outer_radius + hollow_inner_radius), half_angle, kPreviewArrowCount);
            break;
        }
        case ring:
        {
            if (outer_radius <= inner_radius)
            {
                return false;
            }
            TopoDS_Shape outer_base = BRepPrimAPI_MakeCylinder(base_ax2, outer_radius, kBaseThickness).Shape();
            TopoDS_Shape inner_base = BRepPrimAPI_MakeCylinder(base_ax2, inner_radius, kBaseThickness).Shape();
            builder.Add(shape, BRepAlgoAPI_Cut(outer_base, inner_base).Shape());
            add_surface_ring(0.5 * (outer_radius + inner_radius), half_angle, kPreviewArrowCount);
            break;
        }
        case solid:
            builder.Add(shape, BRepPrimAPI_MakeCylinder(base_ax2, outer_radius, kBaseThickness).Shape());
            add_solid_disk_distribution(outer_radius, half_angle);
            break;
        default:
            return false;
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_volume(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = preferred_axis(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);

        QVector3D min_vec = injector_data.volume_bgeom_min;
        QVector3D max_vec = injector_data.volume_bgeom_max;
        QVector3D span_vec = max_vec - min_vec;
        QVector3D center_vec = 0.5f * (min_vec + max_vec);
        double span_length = span_vec.length();
        double radius = std::max(injector_data.volume_bgeom_radius, 2.0 * static_cast<double>(cyl_d));

        if (injector_data.volume_specification == bouning_geometry)
        {
            switch (injector_data.volume_bgeom_shapes)
            {
            case sphere:
                builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(center_vec), radius).Shape());
                break;
            case cylinder:
            {
                QVector3D cyl_axis = normalized_or(span_vec, axis_vec);
                double cyl_length = (span_length > kTiny) ? span_length : 2.0 * radius;
                QVector3D cyl_start = center_vec - cyl_axis * static_cast<float>(0.5 * cyl_length);
                builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(cyl_start, cyl_axis, axis_vec), radius, cyl_length).Shape());
                break;
            }
            case cone_:
            {
                QVector3D cone_axis = normalized_or(span_vec, axis_vec);
                double cone_length = (span_length > kTiny) ? span_length : 2.0 * radius;
                double top_radius = std::max(0.0, radius - cone_length * std::tan(std::abs(injector_data.volume_bgeom_viconeangle)));
                QVector3D cone_start = center_vec - cone_axis * static_cast<float>(0.5 * cone_length);
                builder.Add(shape, BRepPrimAPI_MakeCone(to_ax2(cone_start, cone_axis, axis_vec), radius, top_radius, cone_length).Shape());
                break;
            }
            case hexahedron:
            default:
            {
                QVector3D box_min = min_vec;
                QVector3D box_max = max_vec;
                if (span_vec.length() <= kTiny)
                {
                    const float half_size = static_cast<float>(std::max(radius, 0.01));
                    box_min = center_vec - QVector3D(half_size, half_size, half_size);
                    box_max = center_vec + QVector3D(half_size, half_size, half_size);
                }
                box_min.setX(std::min(box_min.x(), box_max.x()));
                box_min.setY(std::min(box_min.y(), box_max.y()));
                box_min.setZ(std::min(box_min.z(), box_max.z()));
                box_max.setX(std::max(min_vec.x(), max_vec.x()));
                box_max.setY(std::max(min_vec.y(), max_vec.y()));
                box_max.setZ(std::max(min_vec.z(), max_vec.z()));
                if (span_vec.length() <= kTiny)
                {
                    const float half_size = static_cast<float>(std::max(radius, 0.01));
                    box_max = center_vec + QVector3D(half_size, half_size, half_size);
                }
                builder.Add(shape, BRepPrimAPI_MakeBox(to_pnt(box_min), to_pnt(box_max)).Shape());
                break;
            }
            }
        }
        else
        {
            QVector3D marker_center = (injector_data.pos.length() > kTiny) ? injector_data.pos : center_vec;
            double marker_radius = std::max(radius, std::max(4.0 * static_cast<double>(cyl_d), 0.01));
            builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(marker_center), marker_radius).Shape());
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_p_o_a(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = normalized_or(injector_data.atomizer_axis, preferred_axis(injector_data));
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double body_length = std::max(injector_data.plain_length, std::max(4.0 * injector_data.diameter, 0.01));
        const double core_radius = std::max(0.5 * injector_data.diameter, std::max(static_cast<double>(cyl_d), 1.0e-4));
        const double outer_radius = std::max(0.5 * injector_data.outer_diameter, std::max(1.6 * core_radius, 2.0 * static_cast<double>(cyl_d)));
        const double half_angle = atomizer_half_angle_rad(injector_data);

        QVector3D body_start = injector_data.pos - axis_vec * static_cast<float>(body_length);

        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), outer_radius, body_length).Shape());
        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(injector_data.pos - axis_vec * static_cast<float>(0.2 * body_length), axis_vec, axis_vec),
                                                    core_radius,
                                                    std::max(0.2 * body_length, 0.01)).Shape());
        builder.Add(shape, create_arrow(to_ax2(injector_data.pos, axis_vec, axis_vec), cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));

        for (Standard_Integer i = 0; i < 6; ++i)
        {
            if (half_angle <= 1.0e-6)
            {
                break;
            }

            const double theta = 2.0 * kPi * static_cast<double>(i) / 6.0;
            const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                         bitangent_vec * static_cast<float>(std::sin(theta));
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(half_angle)) +
                                radial_vec * static_cast<float>(std::sin(half_angle));
            dir_vec.normalize();
            builder.Add(shape, create_arrow(to_ax2(injector_data.pos, dir_vec, axis_vec), 0.75 * cyl_d, 0.9 * cyl_l, 1.5 * cyl_d, 0.2 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_p_s_a(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = normalized_or(injector_data.atomizer_axis, preferred_axis(injector_data));
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double body_length = std::max(injector_data.plain_length, std::max(injector_data.outer_diameter * 3.0, 0.01));
        const double outer_radius = std::max(0.5 * injector_data.outer_diameter, std::max(2.5 * static_cast<double>(cyl_d), 0.001));
        const double inner_radius = clamp_double(std::max(0.5 * injector_data.inner_diameter, 0.8 * static_cast<double>(cyl_d)),
                                                 1.0e-4,
                                                 0.85 * outer_radius);
        const double half_angle = std::max(atomizer_half_angle_rad(injector_data), 0.05);

        QVector3D body_start = injector_data.pos - axis_vec * static_cast<float>(body_length);
        TopoDS_Shape outer_body = BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), outer_radius, body_length).Shape();
        TopoDS_Shape inner_body = BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), inner_radius, body_length).Shape();
        builder.Add(shape, BRepAlgoAPI_Cut(outer_body, inner_body).Shape());

        const double ring_radius = inner_radius + (outer_radius - inner_radius) * clamp_double(0.45 + 0.02 * injector_data.sheet_const, 0.2, 0.8);
        const double outer_angle = half_angle * (1.0 + 0.15 * clamp_double(injector_data.lig_const, 0.0, 1.0));

        for (Standard_Integer i = 0; i < kPreviewArrowCount; ++i)
        {
            const double theta = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(kPreviewArrowCount);
            const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                         bitangent_vec * static_cast<float>(std::sin(theta));
            const QVector3D start_vec = injector_data.pos + radial_vec * static_cast<float>(ring_radius);
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(outer_angle)) +
                                radial_vec * static_cast<float>(std::sin(outer_angle));
            dir_vec.normalize();
            builder.Add(shape, create_arrow(to_ax2(start_vec, dir_vec, axis_vec), cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_a_b_a(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = normalized_or(injector_data.atomizer_axis, preferred_axis(injector_data));
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double body_length = std::max(injector_data.plain_length, std::max(injector_data.outer_diameter * 3.0, 0.01));
        const double outer_radius = std::max(0.5 * injector_data.outer_diameter, std::max(3.0 * static_cast<double>(cyl_d), 0.0015));
        const double core_radius = clamp_double(std::max(0.5 * injector_data.inner_diameter, 1.5 * static_cast<double>(cyl_d)),
                                                1.0e-4,
                                                0.8 * outer_radius);
        const double air_half_angle = std::max(atomizer_half_angle_rad(injector_data),
                                               0.1 + 0.0015 * clamp_double(injector_data.airbl_rel_vel, 0.0, 200.0));

        QVector3D body_start = injector_data.pos - axis_vec * static_cast<float>(body_length);
        TopoDS_Shape air_outer = BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), outer_radius, body_length).Shape();
        TopoDS_Shape air_inner = BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), core_radius, body_length).Shape();
        builder.Add(shape, BRepAlgoAPI_Cut(air_outer, air_inner).Shape());
        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), 0.65 * core_radius, 0.75 * body_length).Shape());

        builder.Add(shape, create_arrow(to_ax2(injector_data.pos, axis_vec, axis_vec), cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));

        const double ring_radius = 0.5 * (outer_radius + core_radius);
        for (Standard_Integer i = 0; i < kPreviewArrowCount; ++i)
        {
            const double theta = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(kPreviewArrowCount);
            const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                         bitangent_vec * static_cast<float>(std::sin(theta));
            const QVector3D start_vec = injector_data.pos + radial_vec * static_cast<float>(ring_radius);
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(air_half_angle)) +
                                radial_vec * static_cast<float>(std::sin(air_half_angle));
            dir_vec.normalize();
            builder.Add(shape, create_arrow(to_ax2(start_vec, dir_vec, axis_vec), 0.85 * cyl_d, cyl_l, 1.7 * cyl_d, 0.2 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_f_f_a(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D center_vec = injector_data.pos;
        if ((injector_data.ff_center - injector_data.pos).length() > kTiny)
        {
            center_vec = injector_data.ff_center;
        }

        QVector3D axis_vec = normalized_or(injector_data.atomizer_axis,
                                           normalized_or(injector_data.ff_virtual_origin - center_vec, preferred_axis(injector_data)));
        QVector3D fan_normal = normalized_or(injector_data.ff_normal, QVector3D(0.0f, 0.0f, 1.0f));
        QVector3D fan_tangent = QVector3D::crossProduct(fan_normal, axis_vec);
        if (fan_tangent.length() <= kTiny)
        {
            QVector3D unused_bitangent;
            build_basis(axis_vec, fan_tangent, unused_bitangent);
        }
        fan_tangent.normalize();

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double slit_width = std::max(injector_data.ff_oriface_width, std::max(injector_data.diameter, 4.0 * static_cast<double>(cyl_d)));
        const double base_length = std::max(0.5 * slit_width, 0.01);
        const double angle_span = (injector_data.phi_stop > injector_data.phi_start)
                                      ? injector_data.phi_stop - injector_data.phi_start
                                      : 2.0 * std::max(atomizer_half_angle_rad(injector_data),
                                                       0.15 + 0.05 * clamp_double(injector_data.ff_sheet_const, 0.0, 6.0));
        const double half_angle = 0.5 * clamp_double(angle_span, 0.0, deg_to_rad(170.0));

        QVector3D base_start = center_vec - axis_vec * static_cast<float>(base_length);
        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(base_start, axis_vec, axis_vec), 0.5 * slit_width, base_length).Shape());

        const Standard_Integer fan_arrow_count = 7;
        for (Standard_Integer i = 0; i < fan_arrow_count; ++i)
        {
            const double alpha = (fan_arrow_count == 1)
                                     ? 0.0
                                     : -half_angle + (2.0 * half_angle * static_cast<double>(i) / static_cast<double>(fan_arrow_count - 1));
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(alpha)) +
                                fan_tangent * static_cast<float>(std::sin(alpha));
            dir_vec.normalize();

            double offset_ratio = (fan_arrow_count == 1) ? 0.0 : (static_cast<double>(i) / static_cast<double>(fan_arrow_count - 1) - 0.5);
            QVector3D start_vec = center_vec + fan_tangent * static_cast<float>(offset_ratio * slit_width);
            builder.Add(shape, create_arrow(to_ax2(start_vec, dir_vec, axis_vec), 0.8 * cyl_d, cyl_l, 1.6 * cyl_d, 0.2 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_e_a(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = normalized_or(injector_data.atomizer_axis, preferred_axis(injector_data));
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double outer_radius = std::max(0.5 * injector_data.outer_diameter, std::max(injector_data.diameter, 3.0 * static_cast<double>(cyl_d)));
        const double chamber_radius = 1.25 * outer_radius;
        const double body_length = std::max(injector_data.plain_length, std::max(4.0 * outer_radius, 0.01));
        const double half_angle = clamp_double(std::max(injector_data.effer_half_angle_max, atomizer_half_angle_rad(injector_data)) +
                                                   0.15 * clamp_double(injector_data.effer_const, 0.0, 1.0) +
                                                   0.10 * clamp_double(injector_data.effer_quality, 0.0, 1.0),
                                               0.05,
                                               deg_to_rad(45.0));

        QVector3D body_start = injector_data.pos - axis_vec * static_cast<float>(body_length);
        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(body_start, axis_vec, axis_vec), outer_radius, body_length).Shape());
        builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(body_start), chamber_radius).Shape());
        builder.Add(shape, create_arrow(to_ax2(injector_data.pos, axis_vec, axis_vec), cyl_d, cyl_l, 2 * cyl_d, 0.25 * cyl_l));

        for (Standard_Integer i = 0; i < kPreviewArrowCount; ++i)
        {
            const double theta = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(kPreviewArrowCount);
            const QVector3D radial_vec = tangent_vec * static_cast<float>(std::cos(theta)) +
                                         bitangent_vec * static_cast<float>(std::sin(theta));
            const QVector3D start_vec = injector_data.pos + radial_vec * static_cast<float>(0.45 * outer_radius);
            QVector3D dir_vec = axis_vec * static_cast<float>(std::cos(half_angle)) +
                                radial_vec * static_cast<float>(std::sin(half_angle));
            dir_vec.normalize();
            builder.Add(shape, create_arrow(to_ax2(start_vec, dir_vec, axis_vec), 0.8 * cyl_d, cyl_l, 1.6 * cyl_d, 0.2 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Injector_OCCT::create_geometry_condensate(TopoDS_Shape &targetShape)
{
    Q_UNUSED(targetShape);

    try
    {
        builder.MakeCompound(shape);

        QVector3D axis_vec = preferred_axis(injector_data);
        QVector3D tangent_vec;
        QVector3D bitangent_vec;
        build_basis(axis_vec, tangent_vec, bitangent_vec);

        const Standard_Real cyl_l = 0.6 * preview_arrow_length(injector_data);
        const Standard_Real cyl_d = preview_arrow_radius(injector_data);
        const double pad_radius = std::max(injector_data.radius, std::max(4.0 * static_cast<double>(cyl_d), 0.01));

        builder.Add(shape, BRepPrimAPI_MakeCylinder(to_ax2(injector_data.pos, axis_vec, axis_vec), pad_radius, kBaseThickness).Shape());

        const QVector<QVector3D> droplet_offsets = {
            QVector3D(0.0f, 0.0f, 0.0f),
            tangent_vec * static_cast<float>(0.35 * pad_radius),
            bitangent_vec * static_cast<float>(0.35 * pad_radius)
        };

        for (const QVector3D &offset : droplet_offsets)
        {
            builder.Add(shape, BRepPrimAPI_MakeSphere(to_pnt(injector_data.pos + offset), std::max(1.2 * cyl_d, 0.001)).Shape());
        }

        if (injector_data.vel.length() > kTiny || injector_data.vel_mag > 0.0)
        {
            builder.Add(shape, create_arrow(to_ax2(injector_data.pos, axis_vec, axis_vec), 0.8 * cyl_d, cyl_l, 1.6 * cyl_d, 0.2 * cyl_l));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}












