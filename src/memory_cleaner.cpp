#include "memory_cleaner.h"

#include <QDebug>
#include <QThread>
#include <QCoreApplication>

// NTSTATUS code definitions
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

typedef NTSTATUS(NTAPI *pfnNtSetSystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);

static const ULONG SystemMemoryListInformation = 80;
static const int CommandEmptySystemWorkingSets = 2;
static const int CommandFlushModifiedList = 3;
static const int CommandPurgeStandbyList = 4;

MemoryCleaner::MemoryCleaner() {
    enablePrivileges();
}

MemoryCleaner::~MemoryCleaner() {
}

bool MemoryCleaner::enablePrivileges() {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    auto setPrivilege = [hToken](LPCWSTR privName) -> bool {
        TOKEN_PRIVILEGES tp;
        LUID luid;
        if (!LookupPrivilegeValueW(NULL, privName, &luid)) {
            return false;
        }
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        return AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL) &&
               GetLastError() == ERROR_SUCCESS;
    };

    bool p1 = setPrivilege(L"SeProfileSingleProcessPrivilege");
    bool p2 = setPrivilege(L"SeIncreaseQuotaPrivilege");

    CloseHandle(hToken);
    return (p1 || p2);
}

MemoryMetrics MemoryCleaner::getMemoryMetrics() {
    MemoryMetrics metrics = {};
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&status)) {
        metrics.totalPhysBytes = status.ullTotalPhys;
        metrics.availPhysBytes = status.ullAvailPhys;
        metrics.usedPhysBytes = metrics.totalPhysBytes - metrics.availPhysBytes;
        metrics.memoryLoadPercent = static_cast<int>(status.dwMemoryLoad);

        const double bytesInGB = 1024.0 * 1024.0 * 1024.0;
        metrics.totalPhysGB = metrics.totalPhysBytes / bytesInGB;
        metrics.availPhysGB = metrics.availPhysBytes / bytesInGB;
        metrics.usedPhysGB = metrics.usedPhysBytes / bytesInGB;
    }

    metrics.standbyCacheBytes = getStandbyMemoryBytes();
    metrics.standbyCacheGB = metrics.standbyCacheBytes / (1024.0 * 1024.0 * 1024.0);

    return metrics;
}

quint64 MemoryCleaner::getStandbyMemoryBytes() {
    PERFORMANCE_INFORMATION perfInfo = {};
    perfInfo.cb = sizeof(PERFORMANCE_INFORMATION);
    if (GetPerformanceInfo(&perfInfo, sizeof(PERFORMANCE_INFORMATION))) {
        quint64 pageSize = perfInfo.PageSize;
        return static_cast<quint64>(perfInfo.SystemCache) * pageSize;
    }
    return 0;
}

static NTSTATUS CallNtSetSystemInformation(int command) {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return (NTSTATUS)0xC0000001L;

    pfnNtSetSystemInformation NtSetSystemInformation =
        (pfnNtSetSystemInformation)GetProcAddress(hNtdll, "NtSetSystemInformation");

    if (!NtSetSystemInformation) return (NTSTATUS)0xC0000001L;

    int cmdValue = command;
    return NtSetSystemInformation(SystemMemoryListInformation, &cmdValue, sizeof(int));
}

bool MemoryCleaner::purgeStandbyList() {
    NTSTATUS status = CallNtSetSystemInformation(CommandPurgeStandbyList);
    return (status == STATUS_SUCCESS);
}

bool MemoryCleaner::flushModifiedList() {
    NTSTATUS status = CallNtSetSystemInformation(CommandFlushModifiedList);
    return (status == STATUS_SUCCESS);
}

bool MemoryCleaner::trimSystemWorkingSets() {
    NTSTATUS status = CallNtSetSystemInformation(CommandEmptySystemWorkingSets);
    return (status == STATUS_SUCCESS);
}

int MemoryCleaner::trimProcessWorkingSets(const QStringList &exclusions, int *outSkipped, int *outProtected) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }

    int trimmedCount = 0;
    int skippedCount = 0;
    int protectedCount = 0;
    DWORD currentPid = GetCurrentProcessId();

    int loopCounter = 0;
    do {
        if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4 || pe32.th32ProcessID == currentPid) {
            continue;
        }

        QString processName = QString::fromWCharArray(pe32.szExeFile);
        bool isProtected = false;

        for (const QString &ex : exclusions) {
            if (processName.compare(ex, Qt::CaseInsensitive) == 0 ||
                processName.compare(ex + ".exe", Qt::CaseInsensitive) == 0) {
                isProtected = true;
                break;
            }
        }

        if (isProtected) {
            protectedCount++;
            continue;
        }

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            if (EmptyWorkingSet(hProcess)) {
                trimmedCount++;
            } else {
                if (SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1)) {
                    trimmedCount++;
                } else {
                    skippedCount++;
                }
            }
            CloseHandle(hProcess);
        } else {
            skippedCount++;
        }

        if (++loopCounter % 15 == 0) {
            QCoreApplication::processEvents();
        }
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    if (outSkipped) *outSkipped = skippedCount;
    if (outProtected) *outProtected = protectedCount;

    return trimmedCount;
}

OptimizationResult MemoryCleaner::runMaximumOptimization(const QStringList &exclusions) {
    OptimizationResult res = {};
    res.before = getMemoryMetrics();

    flushModifiedList();
    purgeStandbyList();
    int skipped = 0, prot = 0;
    res.trimmedProcesses = trimProcessWorkingSets(exclusions, &skipped, &prot);
    res.skippedProcesses = skipped;
    res.protectedProcesses = prot;
    trimSystemWorkingSets();

    // Smart Adaptive Sampling Loop:
    // Sample memory metrics over 4 seconds (8 samples at 500ms intervals),
    // keeping the UI 100% responsive while tracking the true lowest settled RAM load!
    MemoryMetrics bestAfter = getMemoryMetrics();

    for (int i = 0; i < 8; ++i) {
        QThread::msleep(500);
        QCoreApplication::processEvents();

        MemoryMetrics sample = getMemoryMetrics();
        if (sample.usedPhysBytes < bestAfter.usedPhysBytes) {
            bestAfter = sample;
        }
    }

    res.after = bestAfter;

    if (res.before.usedPhysBytes > res.after.usedPhysBytes) {
        res.reclaimedBytes = res.before.usedPhysBytes - res.after.usedPhysBytes;
    } else {
        res.reclaimedBytes = 0;
    }

    res.reclaimedMB = res.reclaimedBytes / (1024.0 * 1024.0);
    res.reclaimedGB = res.reclaimedBytes / (1024.0 * 1024.0 * 1024.0);
    res.loadDiffPercent = res.before.memoryLoadPercent - res.after.memoryLoadPercent;

    return res;
}

OptimizationResult MemoryCleaner::runQuickOptimization(const QStringList &exclusions) {
    OptimizationResult res = {};
    res.before = getMemoryMetrics();

    int skipped = 0, prot = 0;
    res.trimmedProcesses = trimProcessWorkingSets(exclusions, &skipped, &prot);
    res.skippedProcesses = skipped;
    res.protectedProcesses = prot;

    // Smart Adaptive Sampling Loop (5 samples over 2.5 seconds)
    MemoryMetrics bestAfter = getMemoryMetrics();

    for (int i = 0; i < 5; ++i) {
        QThread::msleep(500);
        QCoreApplication::processEvents();

        MemoryMetrics sample = getMemoryMetrics();
        if (sample.usedPhysBytes < bestAfter.usedPhysBytes) {
            bestAfter = sample;
        }
    }

    res.after = bestAfter;

    if (res.before.usedPhysBytes > res.after.usedPhysBytes) {
        res.reclaimedBytes = res.before.usedPhysBytes - res.after.usedPhysBytes;
    } else {
        res.reclaimedBytes = 0;
    }

    res.reclaimedMB = res.reclaimedBytes / (1024.0 * 1024.0);
    res.reclaimedGB = res.reclaimedBytes / (1024.0 * 1024.0 * 1024.0);
    res.loadDiffPercent = res.before.memoryLoadPercent - res.after.memoryLoadPercent;

    return res;
}

QString MemoryCleaner::formatSize(quint64 bytes) {
    double mb = bytes / (1024.0 * 1024.0);
    double gb = bytes / (1024.0 * 1024.0 * 1024.0);
    return QString("%1 GB (%2 MB)").arg(gb, 0, 'f', 2).arg(mb, 0, 'f', 2);
}
