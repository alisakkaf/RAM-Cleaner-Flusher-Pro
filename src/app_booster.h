#ifndef APP_BOOSTER_H
#define APP_BOOSTER_H

#include "win_clean_includes.h"

#include <QString>
#include <QStringList>
#include "memory_cleaner.h"

class AppBooster {
public:
    enum PriorityLevel {
        NormalPriority = 0,
        AboveNormalPriority,
        HighPriority,
        RealtimePriority
    };

    AppBooster();
    ~AppBooster();

    bool boostAndLaunch(const QString &appExecutablePath, 
                        PriorityLevel priority = HighPriority, 
                        const QStringList &exclusions = QStringList(),
                        OptimizationResult *outOptResult = nullptr);

    static QString priorityToString(PriorityLevel level);
};

#endif // APP_BOOSTER_H
