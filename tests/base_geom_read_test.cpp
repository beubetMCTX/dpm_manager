#include "base_geom_read.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>

namespace
{
bool check(bool condition, const QString &message)
{
    if (!condition)
    {
        qCritical() << message;
        return false;
    }
    return true;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir temporary_directory;
    if (!check(temporary_directory.isValid(), "Unable to create temporary test directory"))
    {
        return 1;
    }

    Base_Geom_Read reader;
    QString empty_path;
    if (!check(!reader.readFile(empty_path) &&
                   reader.last_error_message().contains("为空"),
               "Empty geometry path should fail with an error"))
    {
        return 1;
    }

    QString missing_path = temporary_directory.filePath("missing.step");
    if (!check(!reader.readFile(missing_path) &&
                   reader.last_error_message().contains("不存在"),
               "Missing geometry file should fail with an error"))
    {
        return 1;
    }

    QString empty_file_path = temporary_directory.filePath("empty.step");
    QFile empty_file(empty_file_path);
    if (!check(empty_file.open(QIODevice::WriteOnly),
               "Unable to create empty geometry fixture"))
    {
        return 1;
    }
    empty_file.close();

    if (!check(!reader.readFile(empty_file_path) &&
                   reader.last_error_message().contains("为空"),
               "Empty geometry file should fail with an error"))
    {
        return 1;
    }

    QString unsupported_path = temporary_directory.filePath("model.xyz");
    QFile unsupported_file(unsupported_path);
    if (!check(unsupported_file.open(QIODevice::WriteOnly),
               "Unable to create unsupported geometry fixture"))
    {
        return 1;
    }
    unsupported_file.write("not geometry");
    unsupported_file.close();

    if (!check(!reader.readFile(unsupported_path) &&
                   reader.last_error_message().contains("不支持"),
               "Unsupported geometry extension should fail with an error"))
    {
        return 1;
    }

    QString valid_path = temporary_directory.filePath("box.brep");
    const TopoDS_Shape source_shape = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    if (!check(BRepTools::Write(source_shape, valid_path.toUtf8().constData()),
               "Unable to create valid BREP fixture"))
    {
        return 1;
    }

    if (!check(reader.readFile(valid_path), "Valid BREP file should be readable") ||
        !check(!reader.getShape().IsNull(), "Valid BREP read should produce a shape") ||
        !check(reader.file_path() == QFileInfo(valid_path).absoluteFilePath(),
               "Geometry reader should retain the absolute source path"))
    {
        return 1;
    }

    return 0;
}
