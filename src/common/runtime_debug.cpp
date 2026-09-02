#include "runtime_debug.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

#include <algorithm>
#include <cstdarg>

#if defined(_DEBUG)
#include <crtdbg.h>
#include <rtcapi.h>
#endif

namespace
{
constexpr int kMaxLogFilesToKeep = 20;

QFile *g_runtime_log_file = nullptr;
QMutex *g_runtime_log_mutex = nullptr;
QString g_runtime_log_file_path;
QtMessageHandler g_previous_qt_message_handler = nullptr;
bool g_runtime_debug_installed = false;

QString log_directory_path()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("logs");
}

void append_runtime_log_line(const QString &message)
{
    if (g_runtime_log_file == nullptr || g_runtime_log_mutex == nullptr || !g_runtime_log_file->isOpen())
    {
        return;
    }

    QMutexLocker locker(g_runtime_log_mutex);
    QTextStream stream(g_runtime_log_file);
    stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
           << " | " << message << Qt::endl;
    stream.flush();
}

void prune_old_log_files(const QString &directory_path)
{
    QDir log_dir(directory_path);
    const QFileInfoList entries = log_dir.entryInfoList(
        QStringList() << "runtime_debug_*.log",
        QDir::Files,
        QDir::Time | QDir::Reversed);

    if (entries.size() <= kMaxLogFilesToKeep)
    {
        return;
    }

    for (int i = 0; i < entries.size() - kMaxLogFilesToKeep; ++i)
    {
        log_dir.remove(entries.at(i).fileName());
    }
}

void write_session_banner()
{
    append_runtime_log_line("========== session start ==========");
    append_runtime_log_line(QString("PID: %1").arg(QCoreApplication::applicationPid()));
    append_runtime_log_line(QString("Application dir: %1").arg(QCoreApplication::applicationDirPath()));
    append_runtime_log_line(QString("Log file: %1").arg(g_runtime_log_file_path));
}

void qt_file_message_handler(QtMsgType type,
                             const QMessageLogContext &context,
                             const QString &message)
{
    QString level;
    switch (type)
    {
    case QtDebugMsg: level = "DEBUG"; break;
    case QtInfoMsg: level = "INFO"; break;
    case QtWarningMsg: level = "WARN"; break;
    case QtCriticalMsg: level = "CRIT"; break;
    case QtFatalMsg: level = "FATAL"; break;
    }

    const QString source = context.file != nullptr
        ? QString("%1:%2").arg(QString::fromUtf8(context.file)).arg(context.line)
        : QString("unknown");
    append_runtime_log_line(QString("[%1] %2 | %3").arg(level, source, message));

    if (g_previous_qt_message_handler != nullptr)
    {
        g_previous_qt_message_handler(type, context, message);
    }

    if (type == QtFatalMsg)
    {
        abort();
    }
}

#if defined(_DEBUG)
int __cdecl crt_report_hook(int report_type, wchar_t *message, int *)
{
    append_runtime_log_line(
        QString("[CRT %1] %2")
            .arg(report_type)
            .arg(message != nullptr ? QString::fromWCharArray(message) : QString("(null)")));
    return 0;
}

int __cdecl rtc_error_hook(int error_number,
                           const wchar_t *file,
                           int line,
                           const wchar_t *module,
                           const wchar_t *format,
                           ...)
{
    wchar_t buffer[2048] = {};
    if (format != nullptr)
    {
        va_list args;
        va_start(args, format);
        _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, format, args);
        va_end(args);
    }

    const QString description = (error_number >= 0 && error_number < _RTC_NumErrors())
        ? QString::fromLatin1(_RTC_GetErrDesc(static_cast<_RTC_ErrorNumber>(error_number)))
        : QString("unknown-rtc-error");
    append_runtime_log_line(
        QString("[RTC %1] desc=%2 | file=%3 | line=%4 | module=%5 | message=%6")
            .arg(error_number)
            .arg(description,
                 file != nullptr ? QString::fromWCharArray(file) : QString("(null)"))
            .arg(line)
            .arg(module != nullptr ? QString::fromWCharArray(module) : QString("(null)"))
            .arg(buffer[0] != L'\0'
                     ? QString::fromWCharArray(buffer)
                     : (format != nullptr ? QString::fromWCharArray(format) : QString("(null)"))));
    return 0;
}
#endif
}

void runtime_debug::install()
{
    if (g_runtime_debug_installed)
    {
        return;
    }

    const QString directory_path = log_directory_path();
    QDir().mkpath(directory_path);
    prune_old_log_files(directory_path);

    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    const qint64 pid = QCoreApplication::applicationPid();
    g_runtime_log_file_path = QDir(directory_path).filePath(
        QString("runtime_debug_%1_pid%2.log").arg(timestamp).arg(pid));

    g_runtime_log_file = new QFile(g_runtime_log_file_path);
    if (g_runtime_log_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        g_runtime_log_mutex = new QMutex();
        write_session_banner();
    }

    g_previous_qt_message_handler = qInstallMessageHandler(qt_file_message_handler);

#if defined(_DEBUG)
    _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, crt_report_hook);
#if defined(_RTC)
    _RTC_SetErrorFuncW(rtc_error_hook);
#endif
#endif

    g_runtime_debug_installed = true;
}

void runtime_debug::shutdown()
{
    if (!g_runtime_debug_installed)
    {
        return;
    }

    append_runtime_log_line("========== session close ==========");

#if defined(_DEBUG)
    _CrtSetReportHookW2(_CRT_RPTHOOK_REMOVE, crt_report_hook);
#if defined(_RTC)
    _RTC_SetErrorFuncW(nullptr);
#endif
#endif

    qInstallMessageHandler(g_previous_qt_message_handler);
    g_previous_qt_message_handler = nullptr;

    if (g_runtime_log_file != nullptr)
    {
        if (g_runtime_log_file->isOpen())
        {
            g_runtime_log_file->flush();
            g_runtime_log_file->close();
        }
        delete g_runtime_log_file;
        g_runtime_log_file = nullptr;
    }

    delete g_runtime_log_mutex;
    g_runtime_log_mutex = nullptr;
    g_runtime_debug_installed = false;
}

void runtime_debug::checkpoint(const QString &message)
{
    append_runtime_log_line(QString("[CHECKPOINT] %1").arg(message));
}

void runtime_debug::trace(const QString &message)
{
    if (!verbose_debug_enabled())
    {
        return;
    }

    append_runtime_log_line(QString("[TRACE] %1").arg(message));
}

QString runtime_debug::current_log_file_path()
{
    return g_runtime_log_file_path;
}

QString runtime_debug::log_directory_path()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("logs");
}

bool runtime_debug::verbose_debug_enabled()
{
    static const bool enabled = qEnvironmentVariableIntValue("DPM_VERBOSE_DEBUG") > 0;
    return enabled;
}
