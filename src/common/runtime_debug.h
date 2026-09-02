#ifndef RUNTIME_DEBUG_H
#define RUNTIME_DEBUG_H

#include <QString>

namespace runtime_debug
{
void install();
void shutdown();
void checkpoint(const QString &message);
void trace(const QString &message);
QString current_log_file_path();
QString log_directory_path();
bool verbose_debug_enabled();
}

#endif // RUNTIME_DEBUG_H
