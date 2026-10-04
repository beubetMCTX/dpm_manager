#include "base_geom_read.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <IGESControl_Writer.hxx>
#include <STEPControl_Writer.hxx>
#include <UnitsMethods.hxx>
#include <UnitsMethods_LengthUnit.hxx>

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

QVector3D shape_lengths(const TopoDS_Shape &shape)
{
    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds);
    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    return QVector3D(static_cast<float>(xmax - xmin),
                     static_cast<float>(ymax - ymin),
                     static_cast<float>(zmax - zmin));
}

bool write_step_fixture(const QString &path)
{
    STEPControl_Writer writer;
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    const bool transferred =
        writer.Transfer(box, STEPControl_ManifoldSolidBrep) == IFSelect_RetDone;
    const bool written = transferred &&
                         writer.Write(path.toUtf8().constData()) == IFSelect_RetDone;
    return written;
}

bool make_step_metre_fixture(const QString &source_path, const QString &target_path)
{
    QFile source(source_path);
    if (!source.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QByteArray contents = source.readAll();
    const QByteArray millimetre_unit("SI_UNIT(.MILLI.,.METRE.)");
    const QByteArray metre_unit("SI_UNIT($,.METRE.)");
    if (contents.count(millimetre_unit) != 1)
    {
        return false;
    }
    contents.replace(millimetre_unit, metre_unit);

    QFile target(target_path);
    if (!target.open(QIODevice::WriteOnly))
    {
        return false;
    }
    return target.write(contents) == contents.size();
}

bool write_iges_fixture(const QString &path, const char *unit)
{
    IGESControl_Writer writer(unit, 1);
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(2.0, 3.0, 4.0).Shape();
    return writer.AddShape(box) && writer.Write(path.toUtf8().constData());
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

    const TopoDS_Shape loaded_shape = reader.getShape();
    const QString loaded_path = reader.file_path();
    if (!check(!reader.readFile(unsupported_path),
               "An invalid replacement geometry should fail") ||
        !check(!reader.getShape().IsNull() && reader.getShape().IsSame(loaded_shape),
               "A failed geometry replacement must preserve the loaded shape") ||
        !check(reader.file_path() == loaded_path,
               "A failed geometry replacement must preserve the loaded path") ||
        !check(reader.last_error_message().contains("不支持"),
               "A failed geometry replacement should report its error"))
    {
        return 1;
    }

    Bnd_Box bounds;
    BRepBndLib::Add(reader.getShape(), bounds);
    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    if (!check(std::abs((xmax - xmin) - 0.002) < 5.0e-7 &&
                   std::abs((ymax - ymin) - 0.003) < 5.0e-7 &&
                   std::abs((zmax - zmin) - 0.004) < 5.0e-7,
               "Imported geometry should convert millimetres to internal metres"))
    {
        return 1;
    }

    QString step_mm_path = temporary_directory.filePath("box_mm.step");
    QString step_m_path = temporary_directory.filePath("box_m.step");
    if (!check(write_step_fixture(step_mm_path),
               "Unable to create millimetre STEP fixture") ||
        !check(make_step_metre_fixture(step_mm_path, step_m_path),
               "Unable to create metre STEP fixture"))
    {
        return 1;
    }

    Base_Geom_Read step_mm_reader;
    Base_Geom_Read step_m_reader;
    if (!check(step_mm_reader.readFile(const_cast<QString &>(step_mm_path)),
               "Millimetre STEP fixture should be readable") ||
        !check(step_m_reader.readFile(const_cast<QString &>(step_m_path)),
               "Metre STEP fixture should be readable"))
    {
        return 1;
    }

    const QVector3D step_mm_lengths = shape_lengths(step_mm_reader.getShape());
    const QVector3D step_m_lengths = shape_lengths(step_m_reader.getShape());
    if (!check(std::abs(step_mm_lengths.x() - 0.002f) < 5.0e-7f &&
                   std::abs(step_mm_lengths.y() - 0.003f) < 5.0e-7f &&
                   std::abs(step_mm_lengths.z() - 0.004f) < 5.0e-7f &&
                   std::abs(step_m_lengths.x() - 2.0f) < 5.0e-4f &&
                   std::abs(step_m_lengths.y() - 3.0f) < 5.0e-4f &&
                   std::abs(step_m_lengths.z() - 4.0f) < 5.0e-4f,
               "STEP millimetre and metre units should be converted to metres"))
    {
        return 1;
    }

    QString iges_mm_path = temporary_directory.filePath("box_mm.iges");
    QString iges_m_path = temporary_directory.filePath("box_m.iges");
    if (!check(write_iges_fixture(iges_mm_path, "MM"),
               "Unable to create millimetre IGES fixture") ||
        !check(write_iges_fixture(iges_m_path, "M"),
               "Unable to create metre IGES fixture"))
    {
        return 1;
    }

    Base_Geom_Read iges_mm_reader;
    Base_Geom_Read iges_m_reader;
    const double previous_cascade_unit = UnitsMethods::GetCasCadeLengthUnit();
    UnitsMethods::SetCasCadeLengthUnit(
        1000.0, UnitsMethods_LengthUnit_Millimeter);
    if (!check(iges_mm_reader.readFile(iges_mm_path),
               "Millimetre IGES fixture should be readable") ||
        !check(std::abs(UnitsMethods::GetCasCadeLengthUnit() - 1000.0) < 1.0e-12,
               "IGES import should restore the previous Cascade unit") ||
        !check(iges_m_reader.readFile(iges_m_path),
               "Metre IGES fixture should be readable"))
    {
        UnitsMethods::SetCasCadeLengthUnit(
            previous_cascade_unit, UnitsMethods_LengthUnit_Millimeter);
        return 1;
    }
    UnitsMethods::SetCasCadeLengthUnit(
        previous_cascade_unit, UnitsMethods_LengthUnit_Millimeter);

    const QVector3D iges_mm_lengths = shape_lengths(iges_mm_reader.getShape());
    const QVector3D iges_m_lengths = shape_lengths(iges_m_reader.getShape());
    if (!check(std::abs(iges_mm_lengths.x() - 0.002f) < 5.0e-7f &&
                   std::abs(iges_mm_lengths.y() - 0.003f) < 5.0e-7f &&
                   std::abs(iges_mm_lengths.z() - 0.004f) < 5.0e-7f &&
                   std::abs(iges_m_lengths.x() - 2.0f) < 5.0e-4f &&
                   std::abs(iges_m_lengths.y() - 3.0f) < 5.0e-4f &&
                   std::abs(iges_m_lengths.z() - 4.0f) < 5.0e-4f,
               "IGES millimetre and metre units should be converted to metres"))
    {
        return 1;
    }

    return 0;
}
