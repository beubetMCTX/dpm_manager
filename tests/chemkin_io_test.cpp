#include "chemkin_io.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

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

    const QString valid_path = temporary_directory.filePath("valid.inp");
    if (!check(write_file(valid_path,
                          "ELEMENTS\n"
                          "C H O\n"
                          "END\n"
                          "SPECIES  H2 O2\n"
                          "  H2O  ! inline comment\n"
                          "  OH\n"
                          "END\n"
                          "REACTIONS\n"
                          "END\n"),
                "Unable to create valid Chemkin fixture"))
    {
        return 1;
    }

    bool ok = false;
    QString error_message;
    const QStringList species = read_chemkin_species_names(
        valid_path, &ok, &error_message, false);
    if (!check(ok, "Valid Chemkin file should parse") ||
        !check(error_message.isEmpty(), "Valid Chemkin parse should not report an error") ||
        !check(species == QStringList({"H2", "O2", "H2O", "OH"}),
               "Chemkin species order or comment handling is incorrect"))
    {
        return 1;
    }

    const QString duplicate_path = temporary_directory.filePath("duplicate.inp");
    if (!check(write_file(duplicate_path,
                          "SPECIES\n"
                          "H2 h2 O2\n"
                          "o2 OH\n"
                          "END\n"),
                "Unable to create duplicate-species Chemkin fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QStringList duplicate_species = read_chemkin_species_names(
        duplicate_path, &ok, &error_message, false);
    if (!check(ok && duplicate_species == QStringList({"H2", "O2", "OH"}),
               "Chemkin species should be deduplicated case-insensitively") ||
        !check(error_message.isEmpty(),
               "Duplicate-species Chemkin parse should not report an error"))
    {
        return 1;
    }

    const QString missing_end_path = temporary_directory.filePath("missing_end.inp");
    if (!check(write_file(missing_end_path, "SPECIES\nN2\n"),
                "Unable to create missing-END Chemkin fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QStringList missing_end_species = read_chemkin_species_names(
        missing_end_path, &ok, &error_message, false);
    if (!check(!ok && missing_end_species.isEmpty(),
               "Missing SPECIES END should fail without returning species") ||
        !check(error_message.contains("missing END"),
               "Missing SPECIES END should provide a useful error"))
    {
        return 1;
    }

    const QString empty_species_path = temporary_directory.filePath("empty_species.inp");
    if (!check(write_file(empty_species_path,
                          "SPECIES\n"
                          "END\n"),
                "Unable to create empty-SPECIES Chemkin fixture"))
    {
        return 1;
    }

    error_message.clear();
    const QStringList empty_species = read_chemkin_species_names(
        empty_species_path, &ok, &error_message, false);
    if (!check(!ok && empty_species.isEmpty(),
               "An empty SPECIES section should fail safely") ||
        !check(error_message.contains("contains no species"),
               "Empty SPECIES section should provide a useful error"))
    {
        return 1;
    }

    error_message.clear();
    const QStringList missing_file_species = read_chemkin_species_names(
        temporary_directory.filePath("does-not-exist.inp"), &ok, &error_message, false);
    if (!check(!ok && missing_file_species.isEmpty(),
               "Missing Chemkin file should fail safely") ||
        !check(error_message.contains("does not exist"),
               "Missing Chemkin file should provide a useful error"))
    {
        return 1;
    }

    return 0;
}
