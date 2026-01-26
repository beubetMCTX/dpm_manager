#ifndef INJECTOR_H
#define INJECTOR_H
#pragma pack(1)
#pragma once

#include <QString>
#include <qvector.h>
#include <QVector3D>

// 可能需要的枚举
//粒子类型
enum DPM_Type
{
    Massless,
    Inert,
    Droplet,
    Combusting,
    Multicomponent
};
//喷射源类型
enum Injection_Type
{
    single,
    group,
    surface,
    volume,
    cone,
    plain_oriface_atomizer,
    flat_fan_atomizer,
    effervescent_atomizer,
    file,
    condensate
};
//圆锥喷注圆柱类型
enum Cone_Type
{
    point,
    hollow,
    ring,
    solid
};
//颗粒聚团模型
enum Parcel_Model
{
    standard,
    const_number,
    const_mass,
    const_diameter
};

enum Drag_Law
{
    spherical,
    nonspherical,
    Strokes_Cunningham,
    high_Mach_number,
    dynamic_drag
};

enum Volume_Specification
{
    zone,
    bouning_geometry
};

enum Volume_Bgeom_Shapes
{
    sphere,
    cylinder,
    cone_,
    hexahedron

};

enum Rot_Drag_Law
{
    none,
    Dennis_et_al
};

enum Rot_Lift_Law
{
    Oesterle_Bui_Dinh,
    Tsuji_et_al,
    Rubinow_Keller,
    none_
};

//类
class Injector
{
public:
    Injector();

public:

    QString name; //名字
    DPM_Type type;//粒子类型
    Injection_Type injection_type;//喷射源类型

    int numpts;//喷射点数
    QString dpm_fname;//dpm文件名，默认为空
    QVector<int> surfaces;//表面喷注选择面id

    bool stochastic;//随机脉动开关
    bool random_eddy;//离散随机轨迹模型
    int ntries;//随机涡尝试次数
    double time_scale_constant;//随机涡时间尺度常数

    bool cloud;//颗粒云追踪
    double cloud_min_dia;//颗粒云最小直径
    double cloud_max_dia;//颗粒云最大直径

    QString material;//材料

    bool scale_by_area;//面积缩放开关
    bool use_face_normal;//面法向使用开关
    bool random_surface;//随机表面开关

    QString devolatilizing_species;//热解组分
    QString evaporating_species;//蒸发组分
    QString oxidizing_species;//氧化物组分
    QString product_species;//生成物组分
    //若为空则默认#f

    bool rr_disturb;//rr直径分布
    bool rr_uniform_ln_d;//均匀对数正态分布开关（若为#t则上一项也为#t）

    //wet_combustion
    bool evaporating_liquid;//蒸发液体开关
    QString evaporating_material;//蒸发液体组分（留空则为#f）
    double liquid_fraction;//液体分数（不启用蒸发液体则为-1）
    //未猜出部分
    QString dpm_domain="none";//DPM域
    QString collision_partner="dem-unknow";//碰撞对象

    int parcel_number;//颗粒聚团数量
    double parcel_mass;//聚团质量
    double parcel_diameter;//聚团直径
    Parcel_Model parcel_model;//聚团模型（直接输出enum值，无需输出eunm变量名）

    Drag_Law drag_law;//曳力准则（输出需带双引号）
    double shape_factor;//形状因子（nonspherical）
    double cunningham_correction;//坎宁安修正系数
    QString drag_fcn="none";//曳力函数,搁置

    bool brownian_motion;//布朗运动开关

    bool seco_breakup_on;//seco颗粒破碎模型开关
    bool seco_breakup_tab;//seco颗粒破碎表格模式
    bool seco_breakup_wave;//seco波浪模式开关
    bool seco_break_up_khrt;//secoKHRT模式开关
    bool seco_breakup_ssd;//secoSSD模式开关
    bool seco_breakup_madahushi;//secoMadabushi模式开关
    bool seco_breakup_schmehl;//secoSchmehl模式开关
    //tab
    double seco_breakup_tab_y0;
    int number_tab_diameters;
    //wave
    double seco_breakup_wave_b1;
    double seco_breakup_wave_b0;
    //khrt
    double seco_breakup_khrt_cl;
    double seco_breakup_khrt_ctau;
    double seco_breakup_khrt_crt;
    //ssd
    double seco_breakup_ssd_we_cr;
    double seco_breakup_ssd_core_bu;
    double seco_breakup_ssd_np_target;
    double seco_breakup_ssd_x_si;
    //madabushi
    double seco_breakup_madabushi_c0;
    double seco_breakup_madabushi_column_drag_cd;
    double seco_breakup_madabushi_ligament_factor;
    double seco_breakup_madabushi_jet_diameter;
    //schmehl
    double seco_breakup_schmehl_np;

    QString laws[10]={0};
    QString swit="\"Default\"";

    QString udf_inject_init="\"none\"";
    QString udf_heat_mass="none";

    //components暂时搁置

    //volume
    Volume_Specification volume_specification;
    //volume_zones暂时搁置
    QString volume_streams_spec="total-parcel-count";
    int volume_streams_total;
    //int volume_streams_total;//体积分数法搁置
    //double volume_packing_limit_per_cell;
    Volume_Bgeom_Shapes volume_bgeom_shapes;
    QVector3D volume_bgeom_min;
    QVector3D volume_bgeom_max;
    double volume_bgeom_radius;
    double volume_bgeom_viconeangle;//存储按照弧度制
    bool mass_input_on=false;
    bool volfrac_input_on=false;//这两个做不出来，要考虑case和mesh的因素
    //rotation

    bool rotation_on;//开启旋转？
    Rot_Drag_Law rot_drag_law;//输出带双引号如："none"
    Rot_Lift_Law rot_lift_law;//输出带双引号如："none"

    Cone_Type cone_type;//圆锥喷注圆柱类型
    bool uniform_mass_dist_on;
    bool spatial_staggering_std_inj_on;//stagger开关
    bool spatial_staggering_atomizer_on=true;
    double stagger_radius;

    bool rough_wall_on;

    QString cphace_domain="none";

    QVector3D
        pos,
        pos2;

    QVector3D
        ff_center,
        ff_virtual_origin,
        ff_normal;
    QVector3D
        vel,
        vel2;
    QVector3D
        ang_vel,
        ang_vel2;
    QVector3D
        atomizer_axis;
    double//雾化直径
        diameter,
        diameter2;
    double
        temperature,
        temperature2;
    double
        flow_rate,
        flow_rate2;
    double
        unsteady_start,
        unsteady_stop;
    double
        start_at_flow_time_in_unsteady_inj_file,
        interval_to_repeat_in_unsteady_inj_file;
    double
        unsteady_ca_start,
        unsteady_ca_stop;
    double vapor_pressure;
    double//a_b_a内外孔径
        inner_diameter,
        outer_diameter;
    double half_angle;//p_s_a喷注器中的半角,pi制
    //p_ori_a选项
    double plain_length;//平口长度
    double plain_corner_size;
    double plain_const_a;

    double pswirl_inj_press; //p_s_a上流压力

    double airbl_rel_vel;//a_b_a气流相对速度

    double effer_quality;//effer扩散器品质
    double effer_t_sat;//effer饱和温度

    double ff_oriface_width;//ff扁平孔宽度

    double
        phi_start,
        phi_stop;

    double sheet_const;//p_s_a液膜常数
    double lig_const;//p_s_a韧带常数

    double effer_const;//effer扩散常数
    double effer_half_angle_max;//effer最大半角

    double ff_sheet_const;//扁平液膜常数

    double atomizer_disp_angle;//p_s_a雾化器扩散角(角度制)

    QVector3D axis;//cone,p_o_a,p_s_a,a_b_a,ff,effer等扩散器轴朝向
    double vel_mag;//无速度矢量的喷注器喷注速度
    double ang_vel_mag;//无速度矢量的喷注器喷注角速度
    double cone_angle;//锥角
    double inner_radius;//ring_cone内半径
    double radius;//cone喷注外半径
    double swirl_frac;//旋流分数

    double total_flow_rate;//总流量
    double total_mass;//总质量

    //rr分布设置
    double
        rr_min,
        rr_max,
        rr_mean;
    int rr_numdia;
    QVector3D posr;//rr参考坐标

    QVector3D posu;//未知坐标





};



#endif // INJECTOR_H
