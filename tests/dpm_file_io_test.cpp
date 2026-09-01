#include "dpm_file_io.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QRegularExpression>

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

    const QString unbalanced_path = temporary_directory.filePath("unbalanced.dpm");
    if (!check(write_file(unbalanced_path, "((injector ((type . droplet)") ,
               "Unable to create unbalanced DPM fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QList<Unit> unbalanced_file = read_dpm_file(
        unbalanced_path, &ok, &error_message, false);
    if (!check(!ok && unbalanced_file.isEmpty(),
               "Unbalanced DPM file should fail safely") ||
        !check(error_message.contains("unbalanced parentheses"),
               "Unbalanced DPM file should provide a useful error"))
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
    first.inj.injector_data.dpm_fname = "\"spray_profile.inj\"";

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
        !check(round_trip.at(0).inj.injector_data.dpm_fname ==
                   "\"spray_profile.inj\"",
               "written DPM should preserve the DPM file name") ||
        !check(round_trip.at(1).inj.injector_data.injection_type == cone &&
                   round_trip.at(1).inj.injector_data.cone_type == hollow,
               "written DPM should preserve enum fields"))
    {
        return 1;
    }

    QFile written_input(written_path);
    if (!check(written_input.open(QIODevice::ReadOnly | QIODevice::Text),
               "Unable to reopen DPM output for enum validation") )
    {
        return 1;
    }
    const QString written_contents = QString::fromUtf8(written_input.readAll());
    QString invalid_enum_contents = written_contents;
    invalid_enum_contents.replace(
        "(injection-type . single)",
        "(injection-type . not-a-cone)");
    written_input.close();
    const QString invalid_enum_path = temporary_directory.filePath("invalid-enum.dpm");
    if (!check(write_file(invalid_enum_path, invalid_enum_contents),
               "Unable to create invalid enum fixture") )
    {
        return 1;
    }
    error_message.clear();
    const QList<Unit> invalid_enum = read_dpm_file(
        invalid_enum_path, &ok, &error_message, false);
    if (!check(!ok && invalid_enum.isEmpty(),
               "DPM parser should reject enum values that only contain a valid token") ||
        !check(error_message.contains("injection-type"),
               "Invalid enum failure should identify the field"))
    {
        return 1;
    }

    QString invalid_numeric_contents = written_contents;
    invalid_numeric_contents.replace(
        QRegularExpression("\\(temperature\\s*\\.\\s*[^()]*\\)"),
        "(temperature . nan)");
    const QString invalid_numeric_path = temporary_directory.filePath("invalid-numeric.dpm");
    if (!check(write_file(invalid_numeric_path, invalid_numeric_contents),
               "Unable to create invalid numeric fixture") )
    {
        return 1;
    }
    error_message.clear();
    const QList<Unit> invalid_numeric = read_dpm_file(
        invalid_numeric_path, &ok, &error_message, false);
    if (!check(!ok && invalid_numeric.isEmpty(),
               "DPM parser should reject non-finite numeric values") ||
        !check(error_message.contains("temperature"),
               "Invalid numeric failure should identify the field"))
    {
        return 1;
    }

    QString quoted_enum_contents = written_contents;
    quoted_enum_contents.replace("(drag-law . spherical)",
                                 "(drag-law . \"spherical\")");
    quoted_enum_contents.replace("(rot-drag-law . none)",
                                 "(rot-drag-law . \"none\")");
    const QString quoted_enum_path = temporary_directory.filePath("quoted-enums.dpm");
    if (!check(write_file(quoted_enum_path, quoted_enum_contents),
               "Unable to create quoted enum fixture") )
    {
        return 1;
    }
    error_message.clear();
    const QList<Unit> quoted_enums = read_dpm_file(
        quoted_enum_path, &ok, &error_message, false);
    if (!check(ok && quoted_enums.size() == 2,
               "DPM parser should accept quoted Fluent enum values") ||
        !check(quoted_enums.at(0).inj.injector_data.drag_law == spherical &&
                   quoted_enums.at(0).inj.injector_data.rot_drag_law == none,
               "Quoted Fluent enum values should preserve their enum values"))
    {
        return 1;
    }

    QFile written_file(written_path);
    if (!check(written_file.open(QIODevice::ReadOnly),
               "Unable to read DPM output fixture") )
    {
        return 1;
    }
    const QByteArray original_output = written_file.readAll();
    written_file.close();

    Unit invalid = first;
    invalid.inj.injector_data.total_flow_rate = std::numeric_limits<double>::quiet_NaN();
    error_message.clear();
    if (!check(!write_dpm_file(written_path,
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

    if (!check(written_file.open(QIODevice::ReadOnly),
               "Unable to reopen DPM output fixture") ||
        !check(written_file.readAll() == original_output,
               "failed DPM export must not overwrite an existing file"))
    {
        return 1;
    }

    Unit spacer;
    spacer.type = line_spacer;
    if (!check(!write_dpm_file(temporary_directory.filePath("spacer.dpm"),
                               {spacer},
                               &error_message) &&
                   error_message.contains("injector units"),
               "non-injector units should be rejected by DPM export"))
    {
        return 1;
    }

    return 0;
}
