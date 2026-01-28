#ifndef DPM_FILE_IO_H
#define DPM_FILE_IO_H

#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QFileIconProvider>
#include <QString>
#include <QDebug>
#include <QMessageBox>
#include <QMainWindow>
#include <QList>
#include <QChar>

#include "unit.h"


enum Coord
{
    x,y,z
};



class dpm_file_io
{
public:
    dpm_file_io();
};


QString Read_File_Dialog();
QList<Unit> read_single_dpm_file(bool *ok);



#endif // DPM_FILE_IO_H
