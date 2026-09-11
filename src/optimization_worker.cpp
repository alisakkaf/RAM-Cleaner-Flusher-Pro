#include "optimization_worker.h"

OptimizationWorker::OptimizationWorker(ActionType type, const QStringList &exclusions, QObject *parent)
    : QThread(parent)
    , m_type(type)
    , m_exclusions(exclusions)
    , m_targetPid(0)
{
}

void OptimizationWorker::setTargetProcess(DWORD pid, const QString &procName) {
    m_targetPid = pid;
    m_targetProcName = procName;
}

void OptimizationWorker::run() {
    if (m_type == ActionTrimProcess) {
        MemoryMetrics before = m_cleaner.getMemoryMetrics();
        HANDLE hProc = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, m_targetPid);
        bool ok = false;
        double savedMB = 0.0;
        if (hProc) {
            PROCESS_MEMORY_COUNTERS pmcBefore = { sizeof(pmcBefore) };
            GetProcessMemoryInfo(hProc, &pmcBefore, sizeof(pmcBefore));

            ok = (EmptyWorkingSet(hProc) || SetProcessWorkingSetSize(hProc, (SIZE_T)-1, (SIZE_T)-1));

            QThread::msleep(300);
            PROCESS_MEMORY_COUNTERS pmcAfter = { sizeof(pmcAfter) };
            GetProcessMemoryInfo(hProc, &pmcAfter, sizeof(pmcAfter));
            CloseHandle(hProc);

            if (pmcBefore.WorkingSetSize > pmcAfter.WorkingSetSize) {
                savedMB = (pmcBefore.WorkingSetSize - pmcAfter.WorkingSetSize) / (1024.0 * 1024.0);
            }
        }
        MemoryMetrics after = m_cleaner.getMemoryMetrics();
        emit singleProcessTrimCompleted(m_targetPid, m_targetProcName, ok, savedMB, before.memoryLoadPercent, after.memoryLoadPercent);
        return;
    }

    MemoryMetrics before = m_cleaner.getMemoryMetrics();
    int trimmed = 0, skipped = 0, prot = 0;
    QString actionTitle;

    if (m_type == ActionMaximum) {
        actionTitle = "Maximum Memory Optimization (Full System Flush)";
        m_cleaner.flushModifiedList();
        m_cleaner.purgeStandbyList();
        trimmed = m_cleaner.trimProcessWorkingSets(m_exclusions, &skipped, &prot);
        m_cleaner.trimSystemWorkingSets();
    } else if (m_type == ActionQuick) {
        actionTitle = "Quick Trim (Process Working Sets Only)";
        trimmed = m_cleaner.trimProcessWorkingSets(m_exclusions, &skipped, &prot);
    } else if (m_type == ActionPurgeStandby) {
        actionTitle = "Purge Standby Memory Cache Lists";
        m_cleaner.purgeStandbyList();
    } else if (m_type == ActionFlushModified) {
        actionTitle = "Flush Modified Memory Page Lists to Disk";
        m_cleaner.flushModifiedList();
    }

    emit settlingStarted(actionTitle);

    MemoryMetrics bestAfter = m_cleaner.getMemoryMetrics();

    // Exactly 10 seconds settling & sampling period (10 iterations x 1000ms)
    for (int sec = 1; sec <= 10; ++sec) {
        QThread::msleep(1000);
        MemoryMetrics sample = m_cleaner.getMemoryMetrics();

        if (sample.usedPhysBytes < bestAfter.usedPhysBytes) {
            bestAfter = sample;
        }

        emit settlingTick(sec, 10);
    }

    OptimizationResult res = {};
    res.before = before;
    res.after = bestAfter;
    if (before.usedPhysBytes > bestAfter.usedPhysBytes) {
        res.reclaimedBytes = before.usedPhysBytes - bestAfter.usedPhysBytes;
    } else {
        res.reclaimedBytes = 0;
    }
    res.reclaimedMB = res.reclaimedBytes / (1024.0 * 1024.0);
    res.reclaimedGB = res.reclaimedBytes / (1024.0 * 1024.0 * 1024.0);
    res.loadDiffPercent = before.memoryLoadPercent - bestAfter.memoryLoadPercent;
    res.trimmedProcesses = trimmed;
    res.skippedProcesses = skipped;
    res.protectedProcesses = prot;

    emit optimizationCompleted(actionTitle, res);
}
