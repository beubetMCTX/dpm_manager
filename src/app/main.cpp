#include "mainwindow.h"
#include "runtime_debug.h"

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <memory>

int main(int argc, char *argv[])
{
    int exit_code = 0;

    {
        QApplication a(argc, argv);
        QCoreApplication::setOrganizationName("dpm_manager");
        QCoreApplication::setApplicationName("dpm_manager");
        a.setWindowIcon(QIcon(":/ui/icons/dpm_manager.ico"));
        runtime_debug::install();

        QTranslator translator;
        QSettings language_settings;
        QStringList uiLanguages;
        const QString configured_locale =
            language_settings.value("language/locale").toString().trimmed();
        if (!configured_locale.isEmpty())
        {
            uiLanguages.append(configured_locale);
        }
        for (const QString &locale : QLocale::system().uiLanguages())
        {
            if (!uiLanguages.contains(locale))
            {
                uiLanguages.append(locale);
            }
        }
        for (const QString &locale : uiLanguages) {
            const QString baseName = "dpm_manager_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName) ||
                translator.load(QCoreApplication::applicationDirPath() +
                                "/i18n/" + baseName)) {
                a.installTranslator(&translator);
                break;
            }
        }
        qInfo() << "Application startup complete";

        runtime_debug::checkpoint("before MainWindow make_unique");
        auto main_window = std::make_unique<MainWindow>();
        runtime_debug::checkpoint("after MainWindow make_unique");
        main_window->show();
        runtime_debug::checkpoint("after MainWindow show");
        main_window->resize(1280,720);
        runtime_debug::checkpoint("after MainWindow resize");
        exit_code = a.exec();
        qInfo() << "Application event loop exited with code" << exit_code;

        runtime_debug::checkpoint("before MainWindow reset");
        main_window.reset();
        runtime_debug::checkpoint("after MainWindow reset");
    }

    runtime_debug::checkpoint("after QApplication scope");
    runtime_debug::checkpoint("========== session end ==========");
    runtime_debug::shutdown();
    return exit_code;
}
