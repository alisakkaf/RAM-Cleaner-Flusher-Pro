#include "win_clean_includes.h"
#include "version.h"
#include "installer_manager.h"

#include "mainwindow.h"
#include "settings_manager.h"
#include "memory_cleaner.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QMetaType>

bool IsRunningAsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
            &NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pAdminGroup))
    {
        CheckTokenMembership(NULL, pAdminGroup, &fIsRunAsAdmin);
        FreeSid(pAdminGroup);
    }
    return fIsRunAsAdmin == TRUE;
}

void RestartAsAdmin(const QStringList &args) {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.cbSize = sizeof(SHELLEXECUTEINFOW);
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        std::wstring wArgs = args.join(" ").toStdWString();
        sei.lpParameters = wArgs.c_str();
        sei.nShow = SW_NORMAL;

        ShellExecuteExW(&sei);
    }
}

#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// ─── Enable Windows Native Immersive Dark Titlebar ───
void enableWindowsDarkTitleBar(WId windowId, bool enable) {
    HWND hwnd = reinterpret_cast<HWND>(windowId);
    BOOL useDarkMode = enable ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
}

int main(int argc, char *argv[]) {
    // Check Administrative Privileges (Fallback check if manifest is bypassed)
    if (!IsRunningAsAdmin()) {
        QStringList args;
        for (int i = 1; i < argc; ++i) {
            args.append(QString::fromLocal8Bit(argv[i]));
        }
        RestartAsAdmin(args);
        return 0;
    }

    #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    #endif

    // Disable High DPI Scaling globally
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    // Force Windows to treat the application as DPI Unaware
    qputenv("QT_QPA_PLATFORM", "windows:dpiawareness=0");

    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setOrganizationName(APP_COMPANY);
    app.setApplicationVersion(APP_VERSION_STR);

    // Register custom struct type for QueuedConnection signal/slot threading
    qRegisterMetaType<OptimizationResult>("OptimizationResult");

    QCommandLineParser parser;
    parser.setApplicationDescription(QString("%1 (%2)").arg(APP_NAME).arg(APP_VERSION_DISPLAY));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption minimizedOption("minimized", "Start application minimized to system tray.");
    QCommandLineOption portableOption("portable", "Run application in portable mode without self-installing.");
    parser.addOption(minimizedOption);
    parser.addOption(portableOption);

    parser.process(app);

    // Automatic Win32 Self-Installation check
    if (!parser.isSet(portableOption) && !InstallerManager::isRunningFromProgramFiles()) {
        InstallerManager::instance()->performSelfInstallation();
    }

    SettingsManager settings;
    bool startMinimized = parser.isSet(minimizedOption) || settings.isStartMinimized();

    MainWindow w;

    if (!startMinimized) {
        enableWindowsDarkTitleBar(w.winId(), settings.isDarkTheme());
        w.show();
        enableWindowsDarkTitleBar(w.winId(), settings.isDarkTheme());
    }

    return app.exec();
}
