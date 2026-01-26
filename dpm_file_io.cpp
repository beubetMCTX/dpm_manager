#include "dpm_file_io.h"

dpm_file_io::dpm_file_io() {}


QString Read_File_Dialog()
{
    QString file_path=QFileDialog::getOpenFileName(nullptr, "选择文件", ".","所有文件 (*.*)");
    qDebug()<<file_path;
    return file_path;
}

bool read_dpm(QFile *file,QTextStream *in,const QString title,int *data)
{
    QChar leftblanket;
    QString temp,dot;
    QChar rightblanket;
    *in>>leftblanket>>temp>>dot>>*data>>rightblanket;
    //qDebug()<<leftblanket;
    //qDebug()<<temp;
    //qDebug()<<dot;
    //qDebug()<<*data;
    //qDebug()<<rightblanket;
    if(leftblanket=='('&&temp==title&&dot=='.'&&rightblanket==')')
    {
        //in.reset();
        //QMessageBox::information(nullptr, "完成", "成功读取"+file->fileName()+"中\n变量："+title);
        //qDebug()<<"\n";
        return true;
    }
    else
    {
        //in.reset();
        QMessageBox::critical(nullptr, "错误", "在读取"+file->fileName()+"中\n变量：\n"+title+"时出现错误");
        return false;
    }
}

Unit read_single_dpm_file(bool *ok)
{
    QFile *file=new QFile(Read_File_Dialog());
    Unit unit;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "错误", "无法打开文件: " + file->errorString());
        *ok=false;
        delete(*file)
        return unit;
    }
    else
    {
        QTextStream *in=new QTextStream(file);
        int *num=new int();
        read_dpm(file,in,"test_data",num);
        read_dpm(file,in,"test_data2",num);
        delete(num);
        return unit;
    }
}
