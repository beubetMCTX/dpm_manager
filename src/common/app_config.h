#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QColor>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include "unit_system.h"

struct MaterialConfigEntry
{
    QString name;
    double density = 0.0;
};

struct ReferenceGeometryConfig
{
    QString kind = "file";
    QString file_path;
    QVector3D position;
    QVector3D rotation;
    QVector3D construction_direction = QVector3D(0.0f, 0.0f, 1.0f);
    double construction_size = 0.01;
    double construction_thickness = 1.0e-5;
    double construction_radius = 5.0e-5;
    bool locked = false;
    bool visible = true;
    bool section_clipping = false;
};

QString app_config_directory_path();
QString app_color_config_directory_path();
QString app_material_config_directory_path();
QString app_settings_file_path();

bool ensure_app_config_directories(QString *error_message = nullptr);

bool load_last_chemkin_file_path(QString *file_path, QString *error_message = nullptr);
bool save_last_chemkin_file_path(const QString &file_path, QString *error_message = nullptr);
bool clear_last_chemkin_file_path(QString *error_message = nullptr);
bool load_recent_project_paths(QStringList *file_paths,
                               QString *error_message = nullptr);
bool save_recent_project_paths(const QStringList &file_paths,
                               QString *error_message = nullptr);
bool load_reference_geometry_config(ReferenceGeometryConfig *config,
                                    QString *error_message = nullptr);
bool save_reference_geometry_config(const ReferenceGeometryConfig &config,
                                    QString *error_message = nullptr);
bool load_main_window_state(QByteArray *geometry,
                            QByteArray *window_state,
                            QString *error_message = nullptr);
bool save_main_window_state(const QByteArray &geometry,
                            const QByteArray &window_state,
                            QString *error_message = nullptr);
bool load_unit_preferences(Unit_Preferences *preferences,
                           QString *error_message = nullptr);
bool save_unit_preferences(const Unit_Preferences &preferences,
                           QString *error_message = nullptr);

bool load_species_color_config(const QString &chemkin_file_path,
                               const QStringList &species_names,
                               QHash<QString, QColor> *species_colors,
                               QString *error_message = nullptr);
bool save_species_color_config(const QString &chemkin_file_path,
                               const QStringList &species_names,
                               const QHash<QString, QColor> &species_colors,
                               QString *error_message = nullptr);

bool load_material_table_config(QList<MaterialConfigEntry> *materials,
                                QString *error_message = nullptr);
bool save_material_table_config(const QList<MaterialConfigEntry> &materials,
                                QString *error_message = nullptr);
bool validate_material_entries(const QList<MaterialConfigEntry> &materials,
                               QString *error_message = nullptr);
QStringList material_names_from_entries(const QList<MaterialConfigEntry> &materials);

#endif // APP_CONFIG_H
