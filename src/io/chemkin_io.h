#ifndef CHEMKIN_IO_H
#define CHEMKIN_IO_H

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>

class chemkin_io
{
public:
    chemkin_io();
};

QString Read_Chemkin_File_Dialog();
QStringList read_chemkin_species_names(bool *ok);
QStringList read_chemkin_species_names(const QString& file_path, bool *ok = nullptr);
QStringList read_chemkin_species_names(const QString& file_path,
                                       bool *ok,
                                       QString *error_message,
                                       bool show_message_box);

#endif // CHEMKIN_IO_H
