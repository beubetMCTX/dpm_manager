#include "dpm_file_io.h"

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

    return 0;
}
