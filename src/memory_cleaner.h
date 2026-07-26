#ifndef MEMORY_CLEANER_H
#define MEMORY_CLEANER_H

#include "win_clean_includes.h"

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QMetaType>

struct MemoryMetrics {
    quint64 totalPhysBytes;
    quint64 availPhysBytes;
    quint64 usedPhysBytes;
    quint64 standbyCacheBytes;
    double totalPhysGB;
    double availPhysGB;
    double usedPhysGB;
    double standbyCacheGB;
    int memoryLoadPercent;
};

struct OptimizationResult {
    MemoryMetrics before;
    MemoryMetrics after;
    quint64 reclaimedBytes;
    double reclaimedMB;
    double reclaimedGB;
    int loadDiffPercent;
    int trimmedProcesses;
    int skippedProcesses;
    int protectedProcesses;
};

Q_DECLARE_METATYPE(OptimizationResult)

class MemoryCleaner {
public:
    MemoryCleaner();
    ~MemoryCleaner();

    // Core status and privilege methods
    bool enablePrivileges();
    MemoryMetrics getMemoryMetrics();
    quint64 getStandbyMemoryBytes();

    // Memory Flushing Actions
    bool purgeStandbyList();
    bool flushModifiedList();
    bool trimSystemWorkingSets();
    int trimProcessWorkingSets(const QStringList &exclusions = QStringList(), int *outSkipped = nullptr, int *outProtected = nullptr);

    // Convenience Master Actions
    OptimizationResult runMaximumOptimization(const QStringList &exclusions = QStringList());
    OptimizationResult runQuickOptimization(const QStringList &exclusions = QStringList());

    static QString formatSize(quint64 bytes);
};

#endif // MEMORY_CLEANER_H
