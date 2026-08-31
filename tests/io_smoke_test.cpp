#include "chemkin_io.h"
#include "dpm_file_io.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    if (argc < 3)
    {
        qCritical() << "Usage: io_smoke_test <dpm-file> <chemkin-file>";
        return 2;
    }

    bool dpm_ok = false;
    QString dpm_error;
    const QList<Unit> units = read_dpm_file(QString::fromLocal8Bit(argv[1]),
                                            &dpm_ok,
                                            &dpm_error);
    if (!dpm_ok || units.isEmpty())
    {
        qCritical() << "DPM smoke test failed:" << dpm_error;
        return 1;
    }

    bool chemkin_ok = false;
    QString chemkin_error;
    const QStringList species = read_chemkin_species_names(
        QString::fromLocal8Bit(argv[2]),
        &chemkin_ok,
        &chemkin_error,
        false);
    if (!chemkin_ok || species.isEmpty())
    {
        qCritical() << "Chemkin smoke test failed:" << chemkin_error;
        return 1;
    }

    bool invalid_ok = true;
    QString invalid_error;
    const QList<Unit> invalid_units = read_dpm_file(
        QString::fromLocal8Bit(argv[1]) + QStringLiteral(".missing"),
        &invalid_ok,
        &invalid_error,
        false);
    if (invalid_ok || !invalid_units.isEmpty() || invalid_error.isEmpty())
    {
        qCritical() << "Invalid DPM path was not rejected safely:" << invalid_error;
        return 1;
    }

    qInfo() << "DPM injectors:" << units.size();
    qInfo() << "Chemkin species:" << species.size();
    return 0;
}
