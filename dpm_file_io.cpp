#include "dpm_file_io.h"

#define  Kill_Read  *ok=false;delete(in);delete(file);return unit;

dpm_file_io::dpm_file_io() {}


bool read_dpm_head(QFile *file,QTextStream *in,QString &name)
{
    QChar space,space2;
    QChar leftblanket;
    *in>>space>>leftblanket>>name>>space2;
    qDebug()<<name;
    if(leftblanket=='(')
    {
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\n开头:时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,int &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')')  return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nint变量：\n"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,double &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')') return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\ndouble变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,QString &data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<"!";
    if(title=="dpm-fname")
    {
        *in>>data;
        data="\" \"";
        qDebug()<<data;
        return true;
    }
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data=="#f") data.clear();
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nstring变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,QVector3D &vect,Coord coord)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    float data;
    *in>>space>>leftblanket>>temp>>dot>>data>>rightblanket;
    qDebug()<<space<<leftblanket<<temp<<dot<<data<<rightblanket;
    if(leftblanket=='('&&temp.contains(title,Qt::CaseSensitivity::CaseInsensitive)&&dot=='.'&&rightblanket==')')
    {
        switch(coord)
        {

        case x:vect.setX(data);break;
        case y:vect.setY(data);break;
        case z:vect.setZ(data);break;

        }

        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nvector变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,DPM_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("massless",Qt::CaseInsensitive)) enum_data=Massless;
        else if(data.contains("inert",Qt::CaseInsensitive)) enum_data=Inert;
        else if(data.contains("droplet",Qt::CaseInsensitive)) enum_data=Droplet;
        else if(data.contains("combusting",Qt::CaseInsensitive)) enum_data=Combusting;
        else if(data.contains("multicomponent",Qt::CaseInsensitive)) enum_data=Multicomponent;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量DPM_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Injection_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
             if(data.contains("single",Qt::CaseInsensitive)) enum_data=single;
        else if(data.contains("group",Qt::CaseInsensitive)) enum_data=group;
        else if(data.contains("surface",Qt::CaseInsensitive)) enum_data=surface;
        else if(data.contains("volume",Qt::CaseInsensitive)) enum_data=volume;
        else if(data.contains("cone",Qt::CaseInsensitive)) enum_data=cone;
        else if(data.contains("plain-orifice-atomizer",Qt::CaseInsensitive)) enum_data=plain_oriface_atomizer;
        else if(data.contains("pressure-swirl-atomizer",Qt::CaseInsensitive)) enum_data=pressure_swirl_atomizer;
        else if(data.contains("air-blast_atomizer",Qt::CaseInsensitive)) enum_data=air_blast_atomizer;
        else if(data.contains("flat-fan-atomizer",Qt::CaseInsensitive)) enum_data=flat_fan_atomizer;
        else if(data.contains("effervescent-atomizer",Qt::CaseInsensitive)) enum_data=effervescent_atomizer;
        else if(data.contains("file",Qt::CaseInsensitive)) enum_data=file_;
        else if(data.contains("condensate",Qt::CaseInsensitive)) enum_data=condensate;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Injection_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Cone_Type &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("point",Qt::CaseInsensitive)) enum_data=point;
        else if(data.contains("hollow",Qt::CaseInsensitive)) enum_data=hollow;
        else if(data.contains("ring",Qt::CaseInsensitive)) enum_data=ring;
        else if(data.contains("solid",Qt::CaseInsensitive)) enum_data=solid;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Cone_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Parcel_Model &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("0")) enum_data=standard;
        else if(data.contains("1")) enum_data=const_number;
        else if(data.contains("2")) enum_data=const_mass;
        else if(data.contains("3")) enum_data=const_diameter;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Parcel_Model时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Drag_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("spherical",Qt::CaseInsensitive)) enum_data=spherical;
        else if(data.contains("nonspherical",Qt::CaseInsensitive)) enum_data=nonspherical;
        else if(data.contains("Strokes",Qt::CaseInsensitive)) enum_data=Strokes_Cunningham;
        else if(data.contains("mach",Qt::CaseInsensitive)) enum_data=high_Mach_number;
        else if(data.contains("dynamic",Qt::CaseInsensitive)) enum_data=dynamic_drag;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量DPM_Type时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nstring变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Streams_Spec &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("total",Qt::CaseInsensitive)) enum_data=total_parcel_count;
        else if(data.contains("cell",Qt::CaseInsensitive)) enum_data=parcel_per_cell;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Stream_Spec时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Specification &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("zone",Qt::CaseInsensitive)) enum_data=zone;
        else if(data.contains("bouning",Qt::CaseInsensitive)) enum_data=bouning_geometry;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Specification时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Volume_Bgeom_Shapes &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("sphere",Qt::CaseInsensitive)) enum_data=sphere;
        else if(data.contains("cylinder",Qt::CaseInsensitive)) enum_data=cylinder;
        else if(data.contains("cone",Qt::CaseInsensitive)) enum_data=cone_;
        else if(data.contains("hex",Qt::CaseInsensitive)) enum_data=hexahedron;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Volume_Bgeom_Shapes时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Rot_Drag_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("dennis",Qt::CaseInsensitive)) enum_data=Dennis_et_al;
        else if(data.contains("none",Qt::CaseInsensitive)) enum_data=none;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Rot_Drag_Law时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,Rot_Lift_Law &enum_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("oest",Qt::CaseInsensitive)) enum_data=Oesterle_Bui_Dinh;
        else if(data.contains("tsuji",Qt::CaseInsensitive)) enum_data=Tsuji_et_al;
        else if(data.contains("rubinow",Qt::CaseInsensitive)) enum_data=Rubinow_Keller;
        else if(data.contains("none",Qt::CaseInsensitive)) enum_data=none_;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量Rot_Lift_Law时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nenum变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,bool &bool_data)
{
    QChar space;
    QChar leftblanket;
    QString temp,dot,data;
    *in>>space>>leftblanket>>temp>>dot>>data;
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        if(data.contains("#f",Qt::CaseInsensitive)) bool_data=false;
        else if(data.contains("#t",Qt::CaseInsensitive)) bool_data=true;
        else
        {
            QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nbool变量"+title+"时检测到非法输入");
            return false;
        }
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nbool变量:"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,QVector<int> vect)
{
    QChar space;
    QChar leftblanket;
    QString temp,data;
    vect.clear();
    if(title=="surfaces")
    {
        *in>>space>>leftblanket>>temp>>data;
        if(data.contains("."))
        {
            vect.push_back(-1);
            *in>>data;
            qDebug()<<data;
        }
        else
        {
            while(1)
            {

                if(data.contains(")"))
                {
                    data.chop(1);
                    vect.push_back(data.toInt());
                    //qDebug()<<data.toInt();
                    break;
                }
                else
                {
                    vect.push_back(data.toInt());
                    *in>>data;
                }
                //qDebug()<<data.toInt();
            }
        }
        return true;
    }
    else if(title=="boundary")
    {
        *in>>space>>leftblanket>>temp>>data;
        while(1)
        {
            if(data.contains(")"))
            {
                data.chop(1);
                vect.push_back(data.toInt());
                //qDebug()<<data;
                break;
            }
            else
            {
                vect.push_back(data.toInt());
                //qDebug()<<data;
                *in>>data;
            }
        }
        return true;
    }
    else if(title=="volume-zones")
    {
        *in>>temp;
        if(temp.contains(")")) return true;
        else
        {
            while(1)
            {
                *in>>data;
                if(data.contains(")"))
                {
                    data.chop(1);
                    vect.push_back(data.toInt());
                    //qDebug()<<data;
                    break;
                }
                else
                {
                    vect.push_back(data.toInt());
                    //qDebug()<<data;
                    *in>>data;
                }
            }
            return true;
        }
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nvector<int>变量:"+title+"时出现错误");
        return false;
    }
}

QString Read_File_Dialog()
{
    QString file_path=QFileDialog::getOpenFileName(nullptr, "选择文件", ".","所有文件 (*.*)");
    qDebug()<<file_path;
    return file_path;
}


void Ignore_input(QTextStream *in,int ignore_number)
{
    QString T;
    int i=0;
    while(1)
    {
        *in>>T;
        if(T.contains(")"))
        {
            i+=T.count(')');
            if(i>=ignore_number) break;
        }
    }

}

bool read_end(QTextStream *in,QString name)
{
    QString temp;
    *in>>temp;
    if(!in->atEnd())
    {
        qDebug()<<temp<<name<<"not end";
        return false;
    }
    else
    {
        qDebug()<<temp<<name<<"end";
        return true;
    }
    // else
    // {
    //     QMessageBox::critical(nullptr, "错误", "在读取"+unit.inj->name+"文件尾时出现错误");
    //     return false;
    // }
}

QList<Unit> read_single_dpm_file(bool *ok)
{
    QFile *file=new QFile(Read_File_Dialog());
    QList<Unit> unit;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "错误", "无法打开文件: " + file->errorString());
        *ok=false;
        delete(file);
        return unit;
    }
    else
    {
        QTextStream *in=new QTextStream(file);
        Unit iterator;
        while(1)
        {
            if(!read_dpm_head(file,in,iterator.inj.name)){Kill_Read};
            if(!read_dpm(file,in,"type",iterator.inj.type)){Kill_Read};
            if(!read_dpm(file,in,"injection-type",iterator.inj.injection_type)){Kill_Read};
            if(!read_dpm(file,in,"local-reference-frame",iterator.inj.local_reference_frame)){Kill_Read};
            if(!read_dpm(file,in,"numpts",iterator.inj.numpts)){Kill_Read};
            if(!read_dpm(file,in,"dpm-fname",iterator.inj.dpm_fname)){Kill_Read};
            if(!read_dpm(file,in,"surfaces",iterator.inj.surfaces)){Kill_Read};
            if(!read_dpm(file,in,"boundary",iterator.inj.boundary)){Kill_Read};
            //基础配置
            if (!read_dpm(file, in, "stochastic-on", iterator.inj.stochastic)) { Kill_Read };
            if (!read_dpm(file, in, "random-eddy-on", iterator.inj.random_eddy)) { Kill_Read };
            if (!read_dpm(file, in, "ntries", iterator.inj.ntries)) { Kill_Read };
            if (!read_dpm(file, in, "time-scale-constant", iterator.inj.time_scale_constant)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-on", iterator.inj.cloud)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-min-dia", iterator.inj.cloud_min_dia)) { Kill_Read };
            if (!read_dpm(file, in, "cloud-max-dia", iterator.inj.cloud_max_dia)) { Kill_Read };
            if (!read_dpm(file, in, "material", iterator.inj.material)) { Kill_Read };
            if (!read_dpm(file, in, "scale-by-area", iterator.inj.scale_by_area)) { Kill_Read };
            if (!read_dpm(file, in, "use-face-normal", iterator.inj.use_face_normal)) { Kill_Read };
            if (!read_dpm(file, in, "random-surface?", iterator.inj.random_surface)) { Kill_Read };
            //表格雾化
            if (!read_dpm(file, in, "tabulated-diam-dist?", iterator.inj.tabulated_diam_dist)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-table-name", iterator.inj.tabulated_diam_table_name)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-ref-diam-col", iterator.inj.tabulated_diam_ref_diam_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-num-frac-col", iterator.inj.tabulated_diam_num_frac_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-mas-frac-col", iterator.inj.tabulated_diam_mas_frac_col)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-num-frac-accum?", iterator.inj.tabulated_diam_num_frac_accum)) { Kill_Read };
            if (!read_dpm(file, in, "tabulated-diam-mas-frac-accum?", iterator.inj.tabulated_diam_mas_frac_accum)) { Kill_Read };
            // 组分与分布
            if (!read_dpm(file, in, "devolatilizing-species", iterator.inj.devolatilizing_species)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-species", iterator.inj.evaporating_species)) { Kill_Read };
            if (!read_dpm(file, in, "oxidizing-species", iterator.inj.oxidizing_species)) { Kill_Read };
            if (!read_dpm(file, in, "product-species", iterator.inj.product_species)) { Kill_Read };
            if (!read_dpm(file, in, "rr-distrib", iterator.inj.rr_disturb)) { Kill_Read };
            if (!read_dpm(file, in, "rr-uniform-ln-d", iterator.inj.rr_uniform_ln_d)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-liquid-on", iterator.inj.evaporating_liquid)) { Kill_Read };
            if (!read_dpm(file, in, "evaporating-material", iterator.inj.evaporating_material)) { Kill_Read };
            if (!read_dpm(file, in, "liquid-fraction", iterator.inj.liquid_fraction)) { Kill_Read };
            // DPM域与碰撞
            if (!read_dpm(file, in, "dpm-domain", iterator.inj.dpm_domain)) { Kill_Read };
            if (!read_dpm(file, in, "collision-partner", iterator.inj.collision_partner)) { Kill_Read };
            //multiple-surface
            Ignore_input(in,1);
            // 颗粒聚团模型
            if (!read_dpm(file, in, "parcel-number", iterator.inj.parcel_number)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-mass", iterator.inj.parcel_mass)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-diameter", iterator.inj.parcel_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "parcel-model", iterator.inj.parcel_model)) { Kill_Read };

            // 曳力与运动
            if (!read_dpm(file, in, "drag-law", iterator.inj.drag_law)) { Kill_Read };
            if (!read_dpm(file, in, "shape-factor", iterator.inj.shape_factor)) { Kill_Read };
            if (!read_dpm(file, in, "cunningham-correction", iterator.inj.cunningham_correction)) { Kill_Read };
            if (!read_dpm(file, in, "drag-fcn", iterator.inj.drag_fcn)) { Kill_Read };

            //htc
            Ignore_input(in,3);

            if (!read_dpm(file, in, "brownian-motion", iterator.inj.brownian_motion)) { Kill_Read };

            // 颗粒破碎模型
            if (!read_dpm(file, in, "seco-breakup-on?", iterator.inj.seco_breakup_on)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-tab?", iterator.inj.seco_breakup_tab)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave?", iterator.inj.seco_breakup_wave)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt?", iterator.inj.seco_break_up_khrt)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd?", iterator.inj.seco_breakup_ssd)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi?", iterator.inj.seco_breakup_madahushi)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-schmehl?", iterator.inj.seco_breakup_schmehl)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-tab-y0", iterator.inj.seco_breakup_tab_y0)) { Kill_Read };
            if (!read_dpm(file, in, "number-tab-diameters", iterator.inj.number_tab_diameters)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave-b1", iterator.inj.seco_breakup_wave_b1)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-wave-b0", iterator.inj.seco_breakup_wave_b0)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-cl", iterator.inj.seco_breakup_khrt_cl)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-ctau", iterator.inj.seco_breakup_khrt_ctau)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-khrt-crt", iterator.inj.seco_breakup_khrt_crt)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-we-cr", iterator.inj.seco_breakup_ssd_we_cr)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-core-bu", iterator.inj.seco_breakup_ssd_core_bu)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-np-target", iterator.inj.seco_breakup_ssd_np_target)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-ssd-x-si", iterator.inj.seco_breakup_ssd_x_si)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-c0", iterator.inj.seco_breakup_madabushi_c0)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-column-drag-cd", iterator.inj.seco_breakup_madabushi_column_drag_cd)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-ligament-factor", iterator.inj.seco_breakup_madabushi_ligament_factor)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-madabhushi-jet-diameter", iterator.inj.seco_breakup_madabushi_jet_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "seco-breakup-schmehl-np", iterator.inj.seco_breakup_schmehl_np)) { Kill_Read };


            // 物理定律与UDF
            Ignore_input(in,12);//laws
            Ignore_input(in,2);//udf
            // if (!read_dpm(file, in, "laws", iterator.inj->laws)) { Kill_Read };
            // if (!read_dpm(file, in, "switch", iterator.inj->swit)) { Kill_Read };
            // if (!read_dpm(file, in, "udf-inject-init", iterator.inj->udf_inject_init)) { Kill_Read };
            // if (!read_dpm(file, in, "udf-heat-mass", iterator.inj->udf_heat_mass)) { Kill_Read };
            //component
            Ignore_input(in,1);
            //体积喷注设置
            if (!read_dpm(file, in, "volume-specification", iterator.inj.volume_specification)) { Kill_Read };
            if (!read_dpm(file, in, "volume-zones", iterator.inj.volume_zones)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-spec", iterator.inj.volume_streams_spec)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-total", iterator.inj.volume_streams_total)) { Kill_Read };
            if (!read_dpm(file, in, "volume-streams-per-cell", iterator.inj.volume_streams_per_cell)) { Kill_Read };
            if (!read_dpm(file, in, "volume-packing-limit-per-cell", iterator.inj.volume_packing_limit_per_cell)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-shapes", iterator.inj.volume_bgeom_shapes)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-xmin", iterator.inj.volume_bgeom_min,x)) { Kill_Read }; // 假设QVector3D分量单独读取
            if (!read_dpm(file, in, "volume-bgeom-ymin", iterator.inj.volume_bgeom_min,y)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-zmin", iterator.inj.volume_bgeom_min,z)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-xmax", iterator.inj.volume_bgeom_max,x)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-ymax", iterator.inj.volume_bgeom_max,y)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-zmax", iterator.inj.volume_bgeom_max,z)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-radius", iterator.inj.volume_bgeom_radius)) { Kill_Read };
            if (!read_dpm(file, in, "volume-bgeom-viconeangle", iterator.inj.volume_bgeom_viconeangle)) { Kill_Read };
            if (!read_dpm(file, in, "mass-input-on", iterator.inj.mass_input_on)) { Kill_Read };
            if (!read_dpm(file, in, "volfrac-input-on", iterator.inj.volfrac_input_on)) { Kill_Read };

            // 旋转与圆锥配置
            if (!read_dpm(file, in, "rotation-on?", iterator.inj.rotation_on)) { Kill_Read };
            if (!read_dpm(file, in, "rot-drag-law", iterator.inj.rot_drag_law)) { Kill_Read };
            if (!read_dpm(file, in, "rot-lift-law", iterator.inj.rot_lift_law)) { Kill_Read };
            if (!read_dpm(file, in, "cone-type", iterator.inj.cone_type)) { Kill_Read };
            if (!read_dpm(file, in, "uniform-mass-dist-on?", iterator.inj.uniform_mass_dist_on)) { Kill_Read };
            if (!read_dpm(file, in, "spatial-staggering/std-inj/on?", iterator.inj.spatial_staggering_std_inj_on)) { Kill_Read };
            if (!read_dpm(file, in, "spatial-staggering/atomizer/on?", iterator.inj.spatial_staggering_atomizer_on)) { Kill_Read };
            if (!read_dpm(file, in, "stagger-radius", iterator.inj.stagger_radius)) { Kill_Read };
            if (!read_dpm(file, in, "rough-wall-on?", iterator.inj.rough_wall_on)) { Kill_Read };
            if (!read_dpm(file, in, "cphase-domain", iterator.inj.cphace_domain)) { Kill_Read };

            // 位置与速度（QVector3D分量）
            if (!read_dpm(file, in, "pos", iterator.inj.pos,x)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.pos2,x)) { Kill_Read };
            if (!read_dpm(file, in, "pos", iterator.inj.pos,y)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.pos2,y)) { Kill_Read };
            if (!read_dpm(file, in, "pos", iterator.inj.pos,z)) { Kill_Read };
            if (!read_dpm(file, in, "pos2", iterator.inj.pos2,z)) { Kill_Read };

            // 扁平风扇坐标
            if (!read_dpm(file, in, "ff-center", iterator.inj.ff_center,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-center", iterator.inj.ff_center,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-center", iterator.inj.ff_center,z)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.ff_virtual_origin,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.ff_virtual_origin,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-virtual-origin", iterator.inj.ff_virtual_origin,z)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.ff_normal,x)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.ff_normal,y)) { Kill_Read };
            if (!read_dpm(file, in, "ff-normal", iterator.inj.ff_normal,z)) { Kill_Read };

            // 速度与角速度
            if (!read_dpm(file, in, "vel", iterator.inj.vel,x)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.vel2,x)) { Kill_Read };
            if (!read_dpm(file, in, "vel", iterator.inj.vel,y)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.vel2,y)) { Kill_Read };
            if (!read_dpm(file, in, "vel", iterator.inj.vel,z)) { Kill_Read };
            if (!read_dpm(file, in, "vel2", iterator.inj.vel2,z)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.ang_vel,x)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.ang_vel2,x)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.ang_vel,y)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.ang_vel2,y)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel", iterator.inj.ang_vel,z)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel2", iterator.inj.ang_vel2,z)) { Kill_Read };

            // 雾化器与几何参数
            if (!read_dpm(file, in, "atomizer-x-axis", iterator.inj.atomizer_axis,x)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-y-axis", iterator.inj.atomizer_axis,y)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-z-axis", iterator.inj.atomizer_axis,z)) { Kill_Read };
            if (!read_dpm(file, in, "diameter", iterator.inj.diameter)) { Kill_Read };
            if (!read_dpm(file, in, "diameter2", iterator.inj.diameter2)) { Kill_Read };
            if (!read_dpm(file, in, "temperature", iterator.inj.temperature)) { Kill_Read };
            if (!read_dpm(file, in, "temperature2", iterator.inj.temperature2)) { Kill_Read };
            if (!read_dpm(file, in, "flow-rate", iterator.inj.flow_rate)) { Kill_Read };
            if (!read_dpm(file, in, "flow-rate2", iterator.inj.flow_rate2)) { Kill_Read };

            // 非稳态参数
            if (!read_dpm(file, in, "unsteady-start", iterator.inj.unsteady_start)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-stop", iterator.inj.unsteady_stop)) { Kill_Read };
            if (!read_dpm(file, in, "start-at-flow-time-in-unsteady-inj-file", iterator.inj.start_at_flow_time_in_unsteady_inj_file)) { Kill_Read };
            if (!read_dpm(file, in, "interval-to-repeat-in-unsteady-inj-file", iterator.inj.interval_to_repeat_in_unsteady_inj_file)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-ca-start", iterator.inj.unsteady_ca_start)) { Kill_Read };
            if (!read_dpm(file, in, "unsteady-ca-stop", iterator.inj.unsteady_ca_stop)) { Kill_Read };

            // 物性参数
            if (!read_dpm(file, in, "vapor-pressure", iterator.inj.vapor_pressure)) { Kill_Read };
            if (!read_dpm(file, in, "inner-diameter", iterator.inj.inner_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "outer-diameter", iterator.inj.outer_diameter)) { Kill_Read };
            if (!read_dpm(file, in, "half-angle", iterator.inj.half_angle)) { Kill_Read };
            if (!read_dpm(file, in, "plain-length", iterator.inj.plain_length)) { Kill_Read };
            if (!read_dpm(file, in, "plain-corner-size", iterator.inj.plain_corner_size)) { Kill_Read };
            if (!read_dpm(file, in, "plain-const-a", iterator.inj.plain_const_a)) { Kill_Read };
            if (!read_dpm(file, in, "pswirl-inj-press", iterator.inj.pswirl_inj_press)) { Kill_Read };
            if (!read_dpm(file, in, "airbl-rel-vel", iterator.inj.airbl_rel_vel)) { Kill_Read };
            if (!read_dpm(file, in, "effer-quality", iterator.inj.effer_quality)) { Kill_Read };
            if (!read_dpm(file, in, "effer-t-sat", iterator.inj.effer_t_sat)) { Kill_Read };
            if (!read_dpm(file, in, "ff-orifice-width", iterator.inj.ff_oriface_width)) { Kill_Read };
            if (!read_dpm(file, in, "phi-start", iterator.inj.phi_start)) { Kill_Read };
            if (!read_dpm(file, in, "phi-stop", iterator.inj.phi_stop)) { Kill_Read };
            if (!read_dpm(file, in, "sheet-const", iterator.inj.sheet_const)) { Kill_Read };
            if (!read_dpm(file, in, "lig-const", iterator.inj.lig_const)) { Kill_Read };
            if (!read_dpm(file, in, "effer-const", iterator.inj.effer_const)) { Kill_Read };
            if (!read_dpm(file, in, "effer-half-angle-max", iterator.inj.effer_half_angle_max)) { Kill_Read };
            if (!read_dpm(file, in, "ff-sheet-const", iterator.inj.ff_sheet_const)) { Kill_Read };
            if (!read_dpm(file, in, "atomizer-disp-angle", iterator.inj.atomizer_disp_angle)) { Kill_Read };

            // 轴与速度参数
            if (!read_dpm(file, in, "axis", iterator.inj.axis,x)) { Kill_Read };
            if (!read_dpm(file, in, "axis", iterator.inj.axis,y)) { Kill_Read };
            if (!read_dpm(file, in, "axis", iterator.inj.axis,z)) { Kill_Read };
            if (!read_dpm(file, in, "vel-mag", iterator.inj.vel_mag)) { Kill_Read };
            if (!read_dpm(file, in, "ang-vel-mag", iterator.inj.ang_vel_mag)) { Kill_Read };
            if (!read_dpm(file, in, "cone-angle", iterator.inj.cone_angle)) { Kill_Read };
            if (!read_dpm(file, in, "inner-radius", iterator.inj.inner_radius)) { Kill_Read };
            if (!read_dpm(file, in, "radius", iterator.inj.radius)) { Kill_Read };
            if (!read_dpm(file, in, "swirl-frac", iterator.inj.swirl_frac)) { Kill_Read };

            // 流量与质量
            if (!read_dpm(file, in, "total-flow-rate", iterator.inj.total_flow_rate)) { Kill_Read };
            if (!read_dpm(file, in, "total-mass", iterator.inj.total_mass)) { Kill_Read };
            if (!read_dpm(file, in, "volume-fraction", iterator.inj.volume_fraction)) { Kill_Read };

            // RR分布参数
            if (!read_dpm(file, in, "rr-min", iterator.inj.rr_min)) { Kill_Read };
            if (!read_dpm(file, in, "rr-max", iterator.inj.rr_max)) { Kill_Read };
            if (!read_dpm(file, in, "rr-mean", iterator.inj.rr_mean)) { Kill_Read };
            if (!read_dpm(file, in, "rr-spread", iterator.inj.rr_spread)) { Kill_Read };
            if (!read_dpm(file, in, "rr-numdia", iterator.inj.rr_numdia)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.posr,x)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.posr,y)) { Kill_Read };
            if (!read_dpm(file, in, "posr", iterator.inj.posr,z)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.posu,x)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.posu,y)) { Kill_Read };
            if (!read_dpm(file, in, "posu", iterator.inj.posu,z)) { Kill_Read };

            qDebug()<<"-2";

            unit.push_back(iterator);

            qDebug()<<"-1";

            if(read_end(in,iterator.inj.name)) break;

        }
        qDebug()<<"0";
        //if(!read_dpm(file,in,"",)) {*ok=false;return unit;};
        *ok=true;
        qDebug()<<"1";
        delete(in);
        qDebug()<<"2";
        delete(file);
        qDebug()<<"3";
        return unit;
    }
}
