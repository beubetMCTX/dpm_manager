#include "dpm_file_io.h"

dpm_file_io::dpm_file_io() {}



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
    qDebug()<<space<<leftblanket<<temp<<dot<<data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
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

QString Read_File_Dialog()
{
    QString file_path=QFileDialog::getOpenFileName(nullptr, "选择文件", ".","所有文件 (*.*)");
    qDebug()<<file_path;
    return file_path;
}
Unit read_single_dpm_file(bool *ok)
{
    QFile *file=new QFile(Read_File_Dialog());
    Unit unit;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "错误", "无法打开文件: " + file->errorString());
        *ok=false;
        delete(file);
        return unit;
    }
    else
    {
        QTextStream *in=new QTextStream(file);
        int num;
        double fnum;
        QString str;
        QVector3D vect;
        Injection_Type enum_type;
        if(!read_dpm(file,in,"test_data1",num)) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data2",fnum)) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data3",str )) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data4",vect ,x )) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data4",vect ,y )) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data4",vect ,z )) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data5",enum_type )) {*ok=false;return unit;};
        qDebug()<<enum_type;
        return unit;
    }
}
