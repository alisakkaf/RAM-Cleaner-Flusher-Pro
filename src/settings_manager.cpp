#include "settings_manager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

SettingsManager::SettingsManager() {
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    m_settings = new QSettings(configPath, QSettings::IniFormat);
}

SettingsManager::~SettingsManager() {
    saveSettings();
    delete m_settings;
}

void SettingsManager::saveSettings() {
    if (m_settings) {
        m_settings->sync();
    }
}

bool SettingsManager::isAutoCleanEnabled() const {
    return m_settings->value("AutoClean/Enabled", true).toBool();
}

void SettingsManager::setAutoCleanEnabled(bool enabled) {
    m_settings->setValue("AutoClean/Enabled", enabled);
    saveSettings();
}

int SettingsManager::getAutoCleanThreshold() const {
    return m_settings->value("AutoClean/ThresholdPercent", 80).toInt();
}

void SettingsManager::setAutoCleanThreshold(int thresholdPercent) {
    m_settings->setValue("AutoClean/ThresholdPercent", thresholdPercent);
    saveSettings();
}

bool SettingsManager::isTimerCleanEnabled() const {
    return m_settings->value("TimerClean/Enabled", false).toBool();
}

void SettingsManager::setTimerCleanEnabled(bool enabled) {
    m_settings->setValue("TimerClean/Enabled", enabled);
    saveSettings();
}

int SettingsManager::getTimerCleanIntervalMinutes() const {
    return m_settings->value("TimerClean/IntervalMinutes", 30).toInt();
}

void SettingsManager::setTimerCleanIntervalMinutes(int minutes) {
    m_settings->setValue("TimerClean/IntervalMinutes", minutes);
    saveSettings();
}

QStringList SettingsManager::getExclusionList() const {
    QStringList defaultList;
    defaultList << "chrome.exe" << "msedge.exe" << "firefox.exe" << "devenv.exe" << "qtcreator.exe" << "code.exe";
    return m_settings->value("Exclusion/List", defaultList).toStringList();
}

void SettingsManager::setExclusionList(const QStringList &list) {
    m_settings->setValue("Exclusion/List", list);
    saveSettings();
}

void SettingsManager::addExclusion(const QString &processName) {
    if (processName.isEmpty()) return;
    QStringList list = getExclusionList();
    if (!list.contains(processName, Qt::CaseInsensitive)) {
        list.append(processName);
        setExclusionList(list);
    }
}

void SettingsManager::removeExclusion(const QString &processName) {
    QStringList list = getExclusionList();
    list.removeAll(processName);
    setExclusionList(list);
}

bool SettingsManager::isStartWithWindows() const {
    return m_settings->value("General/StartWithWindows", true).toBool();
}

void SettingsManager::setStartWithWindows(bool enabled) {
    m_settings->setValue("General/StartWithWindows", enabled);
    updateWindowsStartupRegistry(enabled);
    saveSettings();
}

bool SettingsManager::isMinimizeToTray() const {
    return m_settings->value("General/MinimizeToTray", true).toBool();
}

void SettingsManager::setMinimizeToTray(bool enabled) {
    m_settings->setValue("General/MinimizeToTray", enabled);
    saveSettings();
}

bool SettingsManager::isStartMinimized() const {
    return m_settings->value("General/StartMinimized", false).toBool();
}

void SettingsManager::setStartMinimized(bool enabled) {
    m_settings->setValue("General/StartMinimized", enabled);
    saveSettings();
}

bool SettingsManager::isDarkTheme() const {
    return m_settings->value("Appearance/DarkTheme", true).toBool();
}

void SettingsManager::setDarkTheme(bool isDark) {
    m_settings->setValue("Appearance/DarkTheme", isDark);
    saveSettings();
}

QString SettingsManager::getLanguage() const {
    return m_settings->value("General/Language", "en").toString();
}

void SettingsManager::setLanguage(const QString &lang) {
    m_settings->setValue("General/Language", lang);
    saveSettings();
}

QString SettingsManager::getBoostAppPath() const {
    return m_settings->value("Booster/AppPath", "").toString();
}

void SettingsManager::setBoostAppPath(const QString &path) {
    m_settings->setValue("Booster/AppPath", path);
    saveSettings();
}

int SettingsManager::getBoostPriority() const {
    return m_settings->value("Booster/PriorityIndex", 2).toInt(); // Default High
}

void SettingsManager::setBoostPriority(int priorityIndex) {
    m_settings->setValue("Booster/PriorityIndex", priorityIndex);
    saveSettings();
}

bool SettingsManager::isCustomFontEnabled() const {
    return m_settings->value("Font/CustomEnabled", false).toBool();
}

void SettingsManager::setCustomFontEnabled(bool enabled) {
    m_settings->setValue("Font/CustomEnabled", enabled);
    saveSettings();
}

QString SettingsManager::getCustomFontFamily() const {
    return m_settings->value("Font/Family", "Segoe UI").toString();
}

void SettingsManager::setCustomFontFamily(const QString &family) {
    m_settings->setValue("Font/Family", family);
    saveSettings();
}

int SettingsManager::getCustomFontSize() const {
    return m_settings->value("Font/Size", 9).toInt();
}

void SettingsManager::setCustomFontSize(int sizePoint) {
    m_settings->setValue("Font/Size", sizePoint);
    saveSettings();
}

void SettingsManager::updateWindowsStartupRegistry(bool enable) {
    QSettings bootReg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appPath = QString("\"%1\"").arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));

    if (enable) {
        bootReg.setValue("RAMCleanerFlusher", appPath);
    } else {
        bootReg.remove("RAMCleanerFlusher");
    }
}
