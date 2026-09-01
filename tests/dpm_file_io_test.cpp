#include "dpm_file_io.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <limits>

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

bool write_file(const QString &path, const QString &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    stream << contents;
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

    bool ok = true;
    QString error_message;
    const QList<Unit> missing_file = read_dpm_file(
        temporary_directory.filePath("missing.dpm"), &ok, &error_message, false);
    if (!check(!ok && missing_file.isEmpty(),
               "Missing DPM file should fail safely") ||
        !check(error_message.contains("does not exist"),
               "Missing DPM file should provide a useful error"))
    {
        return 1;
    }

    const QString empty_path = temporary_directory.filePath("empty.dpm");
    if (!check(write_file(empty_path, QString()), "Unable to create empty DPM fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QList<Unit> empty_file = read_dpm_file(empty_path, &ok, &error_message, false);
    if (!check(!ok && empty_file.isEmpty(), "Empty DPM file should fail safely") ||
        !check(error_message.contains("empty"),
               "Empty DPM file should provide a useful error"))
    {
        return 1;
    }

    const QString malformed_path = temporary_directory.filePath("malformed.dpm");
    if (!check(write_file(malformed_path, "not a DPM block"),
               "Unable to create malformed DPM fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QList<Unit> malformed_file = read_dpm_file(
        malformed_path, &ok, &error_message, false);
    if (!check(!ok && malformed_file.isEmpty(),
               "Malformed DPM file should fail without partial units") ||
        !check(error_message.contains("top-level DPM block"),
               "Malformed DPM file should provide a useful error"))
    {
        return 1;
    }

    Unit first;
    first.inj.injector_data.name = "writer_first";
    first.inj.injector_data.pos = QVector3D(1.0f, 2.0f, 3.0f);
    first.inj.injector_data.vel = QVector3D(4.0f, 5.0f, 6.0f);
    first.inj.injector_data.axis = QVector3D(1.0f, 0.0f, 0.0f);
    first.inj.injector_data.atomizer_axis = QVector3D(0.0f, 0.0f, 1.0f);
    first.inj.injector_data.total_flow_rate = 0.25;
    first.inj.injector_data.cone_angle = 37.0;
    first.inj.injector_data.material = "water";

    Unit second = first;
    second.inj.injector_data.name = "writer_second";
    second.inj.injector_data.pos = QVector3D(7.0f, 8.0f, 9.0f);
    second.inj.injector_data.injection_type = cone;
    second.inj.injector_data.cone_type = hollow;

    const QString written_path = temporary_directory.filePath("written.dpm");
    if (!check(write_dpm_file(written_path, {first, second}, &error_message), error_message))
    {
        return 1;
    }

    const QList<Unit> round_trip = read_dpm_file(written_path, &ok, &error_message, false);
    if (!check(ok && round_trip.size() == 2,
               "written DPM should be readable with both injector blocks") ||
        !check(round_trip.at(0).inj.injector_data.name == "writer_first" &&
                   round_trip.at(1).inj.injector_data.name == "writer_second",
               "written DPM should preserve injector names") ||
        !check(round_trip.at(0).inj.injector_data.pos == QVector3D(1.0f, 2.0f, 3.0f) &&
                   round_trip.at(1).inj.injector_data.pos == QVector3D(7.0f, 8.0f, 9.0f),
               "written DPM should preserve injector positions") ||
        !check(round_trip.at(1).inj.injector_data.injection_type == cone &&
                   round_trip.at(1).inj.injector_data.cone_type == hollow,
               "written DPM should preserve enum fields"))
    {
        return 1;
    }

    Unit invalid = first;
    invalid.inj.injector_data.total_flow_rate = std::numeric_limits<double>::quiet_NaN();
    error_message.clear();
    if (!check(!write_dpm_file(temporary_directory.filePath("invalid.dpm"),
                               {invalid},
                               &error_message) &&
                   error_message.contains("not finite"),
               "non-finite DPM values should be rejected before writing") ||
        !check(!write_dpm_file(temporary_directory.filePath("empty-list.dpm"),
                               {},
                               &error_message),
               "empty DPM lists should be rejected"))
    {
        return 1;
    }

    return 0;
}
