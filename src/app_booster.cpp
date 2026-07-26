#include "app_booster.h"

#include <QFileInfo>
#include <QProcess>

AppBooster::AppBooster() {
}

AppBooster::~AppBooster() {
}

bool AppBooster::boostAndLaunch(const QString &appExecutablePath, 
                                PriorityLevel priority, 
                                const QStringList &exclusions,
                                OptimizationResult *outOptResult) 
{
    if (appExecutablePath.isEmpty() || !QFileInfo::exists(appExecutablePath)) {
        return false;
    }

    // 1. Flush RAM to clear Maximum Standby & Working Set pages
    MemoryCleaner cleaner;
    OptimizationResult optRes = cleaner.runMaximumOptimization(exclusions);
    if (outOptResult) {
        *outOptResult = optRes;
    }

    // 2. Prepare process startup information
    std::wstring wPath = appExecutablePath.toStdWString();
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };

    DWORD dwCreationFlags = CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS;

    switch (priority) {
        case AboveNormalPriority:
            dwCreationFlags = ABOVE_NORMAL_PRIORITY_CLASS;
            break;
        case HighPriority:
            dwCreationFlags = HIGH_PRIORITY_CLASS;
            break;
        case RealtimePriority:
            dwCreationFlags = REALTIME_PRIORITY_CLASS;
            break;
        case NormalPriority:
        default:
            dwCreationFlags = NORMAL_PRIORITY_CLASS;
            break;
    }

    QFileInfo fi(appExecutablePath);
    std::wstring wDir = fi.absolutePath().toStdWString();

    BOOL success = CreateProcessW(
        wPath.c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        dwCreationFlags,
        NULL,
        wDir.c_str(),
        &si,
        &pi
    );

    if (success) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    return false;
}

QString AppBooster::priorityToString(PriorityLevel level) {
    switch (level) {
        case NormalPriority: return "Normal";
        case AboveNormalPriority: return "Above Normal";
        case HighPriority: return "High Priority (Recommended)";
        case RealtimePriority: return "Realtime (Extreme)";
        default: return "Normal";
    }
}
