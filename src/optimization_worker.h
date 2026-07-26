#ifndef OPTIMIZATION_WORKER_H
#define OPTIMIZATION_WORKER_H

#include "win_clean_includes.h"
#include <QThread>
#include "memory_cleaner.h"

class OptimizationWorker : public QThread {
    Q_OBJECT

public:
    enum ActionType {
        ActionMaximum,
        ActionQuick,
        ActionPurgeStandby,
        ActionFlushModified,
        ActionTrimProcess
    };

    OptimizationWorker(ActionType type, const QStringList &exclusions, QObject *parent = nullptr);
    void setTargetProcess(DWORD pid, const QString &procName);

signals:
    void optimizationCompleted(const QString &actionTitle, const OptimizationResult &res);
    void singleProcessTrimCompleted(DWORD pid, const QString &procName, bool success, double savedMB, int beforeLoad, int afterLoad);

protected:
    void run() override;

private:
    ActionType m_type;
    QStringList m_exclusions;
    DWORD m_targetPid;
    QString m_targetProcName;
    MemoryCleaner m_cleaner;
};

#endif // OPTIMIZATION_WORKER_H
