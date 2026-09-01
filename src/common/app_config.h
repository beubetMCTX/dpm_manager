#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QColor>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

struct MaterialConfigEntry
{
    QString name;
    double density = 0.0;
};

QString app_config_directory_path();
QString app_color_config_directory_path();
QString app_material_config_directory_path();
QString app_settings_file_path();

bool ensure_app_config_directories(QString *error_message = nullptr);

bool load_last_chemkin_file_path(QString *file_path, QString *error_message = nullptr);
bool save_last_chemkin_file_path(const QString &file_path, QString *error_message = nullptr);
bool clear_last_chemkin_file_path(QString *error_message = nullptr);
bool load_main_window_state(QByteArray *geometry,
                            QByteArray *window_state,
                            QString *error_message = nullptr);
bool save_main_window_state(const QByteArray &geometry,
                            const QByteArray &window_state,
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
QStringList material_names_from_entries(const QList<MaterialConfigEntry> &materials);

#endif // APP_CONFIG_H
