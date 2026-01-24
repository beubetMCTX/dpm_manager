#ifndef INJECTOR_H
#define INJECTOR_H

#include <QString>
#include <qvector.h>

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

//类
class Injector
{
public:
    Injector();

public:

    QString name; //名字
    DPM_Type type;//粒子类型
    Injection_Type injection_type;//喷射源类型
    Cone_Type cone_type;//圆锥喷注圆柱类型
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







};



#endif // INJECTOR_H
