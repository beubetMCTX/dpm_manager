#ifndef PROJECT_SESSION_H
#define PROJECT_SESSION_H

#include <QList>
#include <QString>

#include "app_config.h"
#include "unit.h"

namespace project_session
{
struct Data
{
    QList<Unit> units;
    QString chemkin_file_path;
    QList<MaterialConfigEntry> materials;
    ReferenceGeometryConfig reference_geometry;
};

bool save(const QString &file_path,
          const Data &data,
          QString *error_message = nullptr);

bool load(const QString &file_path,
          Data *data,
          QString *error_message = nullptr);
}

#endif // PROJECT_SESSION_H
