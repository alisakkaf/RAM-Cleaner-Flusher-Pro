#ifndef INSTALLER_MANAGER_H
#define INSTALLER_MANAGER_H

#include <QObject>
#include <QString>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QMouseEvent>
#include "settings_manager.h"

// ─── Sleek Frameless Installation / Update Progress Dialog ─────────────────
class InstallProgressDialog : public QDialog {
    Q_OBJECT
public:
    explicit InstallProgressDialog(bool isUpdate, QWidget *parent = nullptr);

    void setStatus(const QString &message, int percent);
    void setCompletedSuccess(const QString &message);
    void setCompletedError(const QString &errorMsg);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_dragPosition;
    SettingsManager m_settings;

    QLabel *m_lblTitle;
    QLabel *m_lblStatus;
    QProgressBar *m_progressBar;
    QPushButton *m_btnClose;
};

// ─── Win32 Self-Installer & Auto-Updater Controller ──────────────────────
class InstallerManager : public QObject {
    Q_OBJECT
public:
    static InstallerManager* instance();

    static QString getProgramFilesPath();
    static QString getDesktopShortcutPath();
    static QString getCurrentExePath();
    static bool isRunningFromProgramFiles();

    static bool createDesktopShortcut(const QString &targetExePath);
    static bool killRunningInstances();
    static bool grantFullFilePermissions(const QString &filePath);

    bool performSelfInstallation(QWidget *parentWidget = nullptr);
    bool performAppUpdate(const QString &zipOrExePath, QWidget *parentWidget = nullptr);

private:
    explicit InstallerManager(QObject *parent = nullptr);
    ~InstallerManager();
};

#endif // INSTALLER_MANAGER_H
