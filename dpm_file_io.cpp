#include "dpm_file_io.h"

dpm_file_io::dpm_file_io() {}



bool read_dpm(QFile *file,QTextStream *in,const QString title,int &data)
{
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>leftblanket>>temp>>dot>>data>>rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')')  return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nint变量：\n"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,double &data)
{
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>leftblanket>>temp>>dot>>data>>rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')') return true;
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\ndouble变量：\n"+title+"时出现错误");
        return false;
    }
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,QString &data)
{
    QChar leftblanket;
    QString temp,dot;
    *in>>leftblanket>>temp>>dot>>data;
    if(leftblanket=='('&&temp==title&&dot=='.'&&data.back()==')')
    {
        data.chop(1);
        return true;
    }
    else
    {
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\nstring变量：\n"+title+"时出现错误");
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
        if(!read_dpm(file,in,"test_data1",num)) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data2",fnum)) {*ok=false;return unit;};
        if(!read_dpm(file,in,"test_data3",str)) {*ok=false;return unit;};
        return unit;
    }
}
