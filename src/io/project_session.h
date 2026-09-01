#ifndef PROJECT_SESSION_H
#define PROJECT_SESSION_H

#include <QList>
#include <QHash>
#include <QColor>
#include <QString>
#include <QStringList>

#include "app_config.h"
#include "unit.h"

namespace project_session
{
struct Data
{
    QList<Unit> units;
    QString chemkin_file_path;
    QHash<QString, QColor> species_colors;
    QList<MaterialConfigEntry> materials;
    ReferenceGeometryConfig reference_geometry;
    Unit_Preferences unit_preferences = UnitSystem::default_preferences();
    bool has_unit_preferences = false;
};

bool validate(const Data &data, QString *error_message = nullptr);

bool validate_references(const Data &data,
                         const QStringList &chemkin_species_names,
                         QString *error_message = nullptr);

bool save(const QString &file_path,
          const Data &data,
          QString *error_message = nullptr);

bool load(const QString &file_path,
          Data *data,
          QString *error_message = nullptr);
}

#endif // PROJECT_SESSION_H
