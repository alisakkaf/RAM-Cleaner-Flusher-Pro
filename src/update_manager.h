#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDialog>
#include <QLabel>
#include <QTextBrowser>
#include <QProgressBar>
#include <QPushButton>
#include <QElapsedTimer>
#include <QFile>
#include <QMouseEvent>
#include "settings_manager.h"

struct UpdateInfo {
    QString version;
    QString releaseDate;
    QString downloadUrl;
    QString changelog;
    QString directUrl;
    bool isValid{false};
};

// ─── Sleek Frameless Update Available Dialog ───────────────────────────────
class UpdateAvailableDialog : public QDialog {
    Q_OBJECT
public:
    explicit UpdateAvailableDialog(const UpdateInfo &info, QWidget *parent = nullptr);

    enum ResultCode {
        DownloadNow = 1,
        OpenBrowser = 2,
        SkipUpdate  = 0
    };

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_dragPosition;
    SettingsManager m_settings;
};

// ─── Sleek Frameless Download Progress Dialog ──────────────────────────────
class UpdateDownloadDialog : public QDialog {
    Q_OBJECT
public:
    explicit UpdateDownloadDialog(const UpdateInfo &info, QWidget *parent = nullptr);
    ~UpdateDownloadDialog();

    void startDownload();
    QString getDownloadedFilePath() const { return m_destinationPath; }

signals:
    void downloadCompleted(const QString &zipPath);
    void downloadCancelled();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onCancelClicked();

private:
    QPoint m_dragPosition;
    UpdateInfo m_info;
    QNetworkAccessManager *m_netManager;
    QNetworkReply *m_reply;
    QFile *m_file;
    QString m_destinationPath;
    SettingsManager m_settings;

    QLabel *m_lblTitle;
    QLabel *m_lblMetrics;
    QProgressBar *m_progressBar;
    QPushButton *m_btnCancel;

    QElapsedTimer m_speedTimer;
    qint64 m_lastBytesReceived{0};
};

// ─── Central Update Manager Singleton / Controller ────────────────────────
class UpdateManager : public QObject {
    Q_OBJECT
public:
    static UpdateManager* instance();

    void checkForUpdates(bool silent = true, QWidget *parentWidget = nullptr);
    static bool isVersionNewer(const QString &currentVer, const QString &serverVer);

signals:
    void updateCheckCompleted(bool hasUpdate, const UpdateInfo &info);
    void updateReadyForInstall(const QString &zipPath);

private slots:
    void onCheckFinished();

private:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager();
    SettingsManager m_settings;
    QNetworkAccessManager *m_netManager;
    QNetworkReply *m_checkReply;
    bool m_silentCheck{true};
    QWidget *m_parentWidget{nullptr};
};

#endif // UPDATE_MANAGER_H
