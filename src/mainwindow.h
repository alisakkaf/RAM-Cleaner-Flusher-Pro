#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "win_clean_includes.h"

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QTreeWidgetItem>
#include <atomic>

#include "memory_cleaner.h"
#include "app_booster.h"
#include "settings_manager.h"
#include "icon_provider.h"
#include "process_info_dialog.h"
#include "optimization_worker.h"
#include "process_selection_dialog.h"
#include "process_group_dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// XML Translation System Helpers
QString trXml(const QString &key, const QString &defaultVal = QString());
void replaceTextFromXML(QWidget *widget, const QString &xmlPath);

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Periodic monitoring & Auto-clean
    void updateMemoryStatusUI();
    void checkAutoCleanRules();

    // Process Tree Monitor & Inspector
    void refreshProcessTree();
    void on_btnRefreshProcesses_clicked();
    void on_txtSearchProcess_textChanged(const QString &text);
    void on_treeProcesses_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_treeProcesses_customContextMenuRequested(const QPoint &pos);
    void onOptionsButtonClicked();

    void onProcessProtectClicked();
    void onProcessKillClicked();

    void openProcessInspector(DWORD pid, const QString &procName);
    void openProcessGroupInspector(const QString &groupName, const QList<DWORD> &pids);
    void openFileLocation(const QString &exePath);
    void killProcessGroup(const QString &procName, const QList<DWORD> &pids);
    void killSingleProcess(DWORD pid, const QString &procName);

    void trimSingleProcess(DWORD pid, const QString &procName);

    // Dashboard Buttons
    void on_btnMaxOpt_clicked();
    void on_btnQuickOpt_clicked();
    void on_btnPurgeStandby_clicked();
    void on_btnFlushModified_clicked();
    void on_btnClearLog_clicked();

    // Asynchronous Worker Slots
    void onWorkerOptimizationCompleted(const QString &actionTitle, const OptimizationResult &res);
    void onWorkerSingleProcessTrimCompleted(DWORD pid, const QString &procName, bool success, double savedMB, int beforeLoad, int afterLoad);

    // Theme & UI Switcher
    void on_btnToggleTheme_clicked();
    void on_comboTheme_currentIndexChanged(int index);
    void on_comboLanguage_currentIndexChanged(int index);

    // Exclusion list actions
    void on_btnAddExclusion_clicked();
    void on_btnBrowseExclusion_clicked();
    void on_btnPickRunningProcess_clicked();
    void on_btnRemoveExclusion_clicked();

    // Settings slots
    void on_chkAutoThreshold_toggled(bool checked);
    void on_spinThreshold_valueChanged(int value);
    void on_chkTimerClean_toggled(bool checked);
    void on_spinInterval_valueChanged(int value);
    void on_chkStartWithWindows_toggled(bool checked);
    void on_chkMinimizeToTray_toggled(bool checked);
    void on_chkStartMinimized_toggled(bool checked);

    // Font settings slots
    void on_chkCustomFont_toggled(bool checked);
    void on_comboFontFamily_currentFontChanged(const QFont &font);
    void on_spinFontSize_valueChanged(int value);

    // Booster actions
    void on_btnBrowseBoostApp_clicked();
    void on_btnLaunchBoostedApp_clicked();

    // Social Links
    void on_btnGithub_clicked();
    void on_btnFacebook_clicked();
    void on_btnWebsite_clicked();

    // System Tray
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    Ui::MainWindow *ui;
    MemoryCleaner m_cleaner;
    AppBooster m_booster;
    SettingsManager m_settings;

    QTimer *m_monitorTimer{nullptr};
    QTimer *m_schedulerTimer{nullptr};
    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_trayMenu{nullptr};

    std::atomic<bool> m_isOptimizing{false};

    void setupSystemTray();
    void loadSettingsToUI();
    void setupTabIcons();
    void applyTheme(bool isDark);
    void applyCustomFont();
    void changeLanguage(const QString &langCode);
    void appendLog(const QString &message, bool isSuccess = true);
    void logOptimizationReport(const QString &actionTitle, const OptimizationResult &res);
    bool startWorker(OptimizationWorker::ActionType type, DWORD pid = 0, const QString &procName = QString());
    void setUIButtonsEnabled(bool enabled);

    QString m_tmplRamLoad{"%1% RAM Load"};
    QString m_tmplUsedFree{"Used: %1 GB | Free: %2 GB | Total: %3 GB"};
    QString m_tmplStandby{"Standby Cache Memory: %1 GB"};
};

#endif // MAINWINDOW_H
