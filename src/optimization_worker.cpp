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
    if (m_type == ActionMaximum) {
        OptimizationResult res = m_cleaner.runMaximumOptimization(m_exclusions);
        emit optimizationCompleted("Maximum Memory Optimization (Full System Flush)", res);
    } else if (m_type == ActionQuick) {
        OptimizationResult res = m_cleaner.runQuickOptimization(m_exclusions);
        emit optimizationCompleted("Quick Trim (Process Working Sets Only)", res);
    } else if (m_type == ActionPurgeStandby) {
        MemoryMetrics before = m_cleaner.getMemoryMetrics();
        m_cleaner.purgeStandbyList();

        MemoryMetrics bestAfter = m_cleaner.getMemoryMetrics();
        for (int i = 0; i < 5; ++i) {
            QThread::msleep(400);
            MemoryMetrics sample = m_cleaner.getMemoryMetrics();
            if (sample.usedPhysBytes < bestAfter.usedPhysBytes) {
                bestAfter = sample;
            }
        }

        double purgedGB = (before.standbyCacheGB > bestAfter.standbyCacheGB) ? (before.standbyCacheGB - bestAfter.standbyCacheGB) : 0.0;
        OptimizationResult res = {};
        res.before = before;
        res.after = bestAfter;
        res.reclaimedGB = purgedGB;
        res.reclaimedMB = purgedGB * 1024.0;
        res.loadDiffPercent = before.memoryLoadPercent - bestAfter.memoryLoadPercent;

        emit optimizationCompleted("Purge Standby Memory Cache Lists", res);
    } else if (m_type == ActionFlushModified) {
        MemoryMetrics before = m_cleaner.getMemoryMetrics();
        m_cleaner.flushModifiedList();

        MemoryMetrics bestAfter = m_cleaner.getMemoryMetrics();
        for (int i = 0; i < 5; ++i) {
            QThread::msleep(400);
            MemoryMetrics sample = m_cleaner.getMemoryMetrics();
            if (sample.usedPhysBytes < bestAfter.usedPhysBytes) {
                bestAfter = sample;
            }
        }

        double reclaimedGB = (before.usedPhysGB > bestAfter.usedPhysGB) ? (before.usedPhysGB - bestAfter.usedPhysGB) : 0.0;
        OptimizationResult res = {};
        res.before = before;
        res.after = bestAfter;
        res.reclaimedGB = reclaimedGB;
        res.reclaimedMB = reclaimedGB * 1024.0;
        res.loadDiffPercent = before.memoryLoadPercent - bestAfter.memoryLoadPercent;

        emit optimizationCompleted("Flush Modified Memory Page Lists to Disk", res);
    } else if (m_type == ActionTrimProcess) {
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
    }
}
