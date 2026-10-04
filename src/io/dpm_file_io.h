#ifndef DPM_FILE_IO_H
#define DPM_FILE_IO_H

#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QFileIconProvider>
#include <QString>
#include <QMessageBox>
#include <QMainWindow>
#include <QList>
#include <QChar>

#include "unit.h"

#include <AIS_InteractiveContext.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <Aspect_Handle.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_GraphicDriver.hxx>


enum Coord
{
    x,y,z
};



class dpm_file_io
{
public:
    dpm_file_io();
};


[[deprecated("Use read_dpm_file(file_path, ...) instead.")]]
QString Read_File_Dialog();
[[deprecated("Use read_dpm_file(file_path, ...) instead.")]]
QList<Unit> read_single_dpm_file(bool *ok);
[[deprecated("Use read_dpm_file(file_path, ...) instead.")]]
QList<Unit> read_single_dpm_file_regex(bool *ok);
// Fluent injection files use SI values. Injector storage follows that
// contract: lengths in metres, velocity in m/s, mass flow in kg/s, and
// temperature in kelvin. Angle fields retain their Fluent field semantics.
QList<Unit> read_dpm_file(const QString &file_path,
                          bool *ok = nullptr,
                          QString *error_message = nullptr,
                          bool show_error_message_box = true,
                          QStringList *warning_messages = nullptr);

bool write_dpm_file(const QString &file_path,
                    const QList<Unit> &units,
                    QString *error_message = nullptr);

bool validate_dpm_units(const QList<Unit> &units,
                        QString *error_message = nullptr);



#endif // DPM_FILE_IO_H
