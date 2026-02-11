#include "injector.h"
#include <qdebug.h>





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
    pos(0, 0.17, 0),
    pos2(0, 1, 0),
    ff_center(0, 1, 0),
    ff_virtual_origin(1, 0, 0),
    ff_normal(1, 0, 0),
    vel(100, 100, 0),
    vel2(100, 0, 0),
    ang_vel(100, 0, 0),
    ang_vel2(100, 0, 0),
    atomizer_axis(1, 0, 0),
    diameter(0),
    diameter2(0),
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
    cone_angle(0),
    inner_radius(0),
    radius(0),
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
    qDebug()<<uuid;
    builder.MakeCompound(inj);
    builder.MakeCompound(shape);

    m_document.Nullify();

    if(!create_geometry_single()){qDebug()<<"sb";}
}

Injector_OCCT::~Injector_OCCT()
{
    if (!m_document.IsNull())
    {
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        app->Close(m_document);
    }
}



bool Injector_OCCT::initialize_OCAF()
{
    try {
        // 获取OCAF应用实例[1,7](@ref)
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        if (app.IsNull()) {
            return false;
        }

        // 创建新文档[1](@ref)
        app->NewDocument("BinXCAF", m_document);
        if (m_document.IsNull()) {
            return false;
        }

        // 获取根标签
        m_geometryLabel = m_document->Main().NewChild();

        return true;
    }
    catch (...) {
        return false;
    }
}

bool Injector_OCCT::create_injector()
{

    return false;
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

        gp_Pnt base_pnt(injector_data.pos.x(),injector_data.pos.y(),injector_data.pos.z());
        gp_Dir base_dir(injector_data.vel.x(),injector_data.vel.y(),injector_data.vel.z());
        gp_Ax2 base_ax2(base_pnt,base_dir);
        Standard_Real cyl_l=sqrt(injector_data.vel.length());
        Standard_Real cyl_d=3*sqrt(injector_data.total_flow_rate/injector_data.vel.length());
        TopoDS_Shape base = BRepPrimAPI_MakeCylinder(base_ax2,2*cyl_d,0.01);
        TopoDS_Compound arrow = create_arrow(base_ax2,cyl_d,cyl_l,2*cyl_d,0.25*cyl_l);

        builder.Add(inj,base);
        builder.Add(inj,arrow);

        shape=inj;

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












