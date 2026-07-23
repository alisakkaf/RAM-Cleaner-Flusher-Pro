#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "win_clean_includes.h"

#include <QString>
#include <QStringList>
#include <QSettings>

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    void saveSettings();

    // Auto-clean & Timer Settings
    bool isAutoCleanEnabled() const;
    void setAutoCleanEnabled(bool enabled);

    int getAutoCleanThreshold() const;
    void setAutoCleanThreshold(int thresholdPercent);

    bool isTimerCleanEnabled() const;
    void setTimerCleanEnabled(bool enabled);

    int getTimerCleanIntervalMinutes() const;
    void setTimerCleanIntervalMinutes(int minutes);

    // Exclusion List
    QStringList getExclusionList() const;
    void setExclusionList(const QStringList &list);
    void addExclusion(const QString &processName);
    void removeExclusion(const QString &processName);

    // General App Settings
    bool isStartWithWindows() const;
    void setStartWithWindows(bool enabled);

    bool isMinimizeToTray() const;
    void setMinimizeToTray(bool enabled);

    bool isStartMinimized() const;
    void setStartMinimized(bool enabled);

    bool isDarkTheme() const;
    void setDarkTheme(bool isDark);

    QString getBoostAppPath() const;
    void setBoostAppPath(const QString &path);

    int getBoostPriority() const;
    void setBoostPriority(int priorityIndex);

private:
    QSettings *m_settings;
    void updateWindowsStartupRegistry(bool enable);
};

#endif // SETTINGS_MANAGER_H
