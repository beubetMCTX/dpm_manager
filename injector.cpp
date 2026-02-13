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

    builder.MakeCompound(shape);

    m_document.Nullify();

    create_geometry_single();

    initialize_OCAF();

}

Injector_OCCT::~Injector_OCCT()
{
    // if (!m_document.IsNull())
    // {
    //     Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    //     app->Close(m_document);
    // }
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
        builder.MakeCompound(shape);

        gp_Pnt base_pnt(injector_data.pos.x(),injector_data.pos.y(),injector_data.pos.z());
        gp_Dir base_dir(injector_data.vel.x(),injector_data.vel.y(),injector_data.vel.z());
        gp_Ax2 base_ax2(base_pnt,base_dir);
        Standard_Real cyl_l=sqrt(injector_data.vel.length());
        Standard_Real cyl_d=3*sqrt(injector_data.total_flow_rate/injector_data.vel.length());
        TopoDS_Shape base = BRepPrimAPI_MakeCylinder(base_ax2,2*cyl_d,0.01);
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












