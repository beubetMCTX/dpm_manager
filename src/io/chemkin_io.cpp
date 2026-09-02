#include "chemkin_io.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

chemkin_io::chemkin_io() {}

namespace
{
void append_species_tokens(const QString& text,
                           QStringList& species_names,
                           QSet<QString>& seen_species)
{
    const QStringList tokens = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString& token : tokens)
    {
        const QString normalized_token = token.toLower();
        if (!seen_species.contains(normalized_token))
        {
            seen_species.insert(normalized_token);
            species_names.push_back(token);
        }
    }
}
}

QString Read_Chemkin_File_Dialog()
{
    const QString file_path = QFileDialog::getOpenFileName(
        nullptr,
        "选择 Chemkin 文件",
        ".",
        "Chemkin Files (*.inp *.ck *.dat *.txt);;All Files (*.*)");
    qDebug() << file_path;
    return file_path;
}

QStringList read_chemkin_species_names(bool *ok)
{
    return read_chemkin_species_names(Read_Chemkin_File_Dialog(), ok);
}

QStringList read_chemkin_species_names(const QString& file_path, bool *ok)
{
    return read_chemkin_species_names(file_path, ok, nullptr, true);
}

QStringList read_chemkin_species_names(const QString& file_path,
                                       bool *ok,
                                       QString *error_message,
                                       bool show_message_box)
{
    if (ok != nullptr)
    {
        *ok = false;
    }
    if (error_message != nullptr)
    {
        error_message->clear();
    }

    if (file_path.trimmed().isEmpty())
    {
        return {};
    }

    const QFileInfo file_info(file_path);
    if (!file_info.exists() || !file_info.isFile())
    {
        const QString message = QString("Chemkin file does not exist or is not a regular file: %1")
                                    .arg(file_path);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_message_box)
        {
            QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
        }
        return {};
    }

    if (file_info.size() <= 0)
    {
        const QString message = QString("Chemkin file is empty: %1").arg(file_path);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_message_box)
        {
            QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
        }
        return {};
    }

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString message = QString("Unable to open Chemkin file: %1").arg(file_path);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_message_box)
        {
            QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
        }
        return {};
    }

    QTextStream stream(&file);
    QStringList species_names;
    QSet<QString> seen_species;
    bool in_species_section = false;
    bool found_species_section = false;

    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        const int comment_index = line.indexOf('!');
        if (comment_index >= 0)
        {
            line.truncate(comment_index);
        }

        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }

        if (!in_species_section)
        {
            if (trimmed.compare("SPECIES", Qt::CaseInsensitive) == 0)
            {
                in_species_section = true;
                found_species_section = true;
                continue;
            }

            if (trimmed.startsWith("SPECIES ", Qt::CaseInsensitive))
            {
                in_species_section = true;
                found_species_section = true;
                append_species_tokens(trimmed.mid(QStringLiteral("SPECIES").size()).trimmed(),
                                      species_names,
                                      seen_species);
                continue;
            }

            continue;
        }

        if (trimmed.compare("END", Qt::CaseInsensitive) == 0 ||
            trimmed.startsWith("END ", Qt::CaseInsensitive))
        {
            if (species_names.isEmpty())
            {
                const QString message = QString("SPECIES section contains no species: %1")
                                            .arg(file_path);
                if (error_message != nullptr)
                {
                    *error_message = message;
                }
                if (show_message_box)
                {
                    QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
                }
                return {};
            }

            if (ok != nullptr)
            {
                *ok = true;
            }
            return species_names;
        }

        append_species_tokens(trimmed, species_names, seen_species);
    }

    if (!found_species_section)
    {
        const QString message = QString("No SPECIES section found in Chemkin file: %1").arg(file_path);
        if (error_message != nullptr)
        {
            *error_message = message;
        }
        if (show_message_box)
        {
            QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
        }
        return {};
    }

    const QString message = QString("SPECIES section in Chemkin file is missing END: %1").arg(file_path);
    if (error_message != nullptr)
    {
        *error_message = message;
    }
    if (show_message_box)
    {
        QMessageBox::critical(nullptr, "Chemkin Parse Error", message);
    }
    return {};
}
