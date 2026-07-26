#include "update_manager.h"
#include "installer_manager.h"
#include "version.h"
#include "icon_provider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QUrlQuery>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

// ─── Semantic Version Comparison (e.g. "1.3.0" > "1.2.0") ───────────────────
bool UpdateManager::isVersionNewer(const QString &currentVer, const QString &serverVer) {
    QStringList currentList = currentVer.trimmed().split('.');
    QStringList serverList = serverVer.trimmed().split('.');

    int maxLen = qMax(currentList.size(), serverList.size());
    for (int i = 0; i < maxLen; ++i) {
        int curVal = (i < currentList.size()) ? currentList[i].toInt() : 0;
        int srvVal = (i < serverList.size()) ? serverList[i].toInt() : 0;

        if (srvVal > curVal) return true;
        if (srvVal < curVal) return false;
    }
    return false;
}

// ─── Sleek Frameless Update Available Dialog ───────────────────────────────
UpdateAvailableDialog::UpdateAvailableDialog(const UpdateInfo &info, QWidget *parent)
    : QDialog(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->resize(520, 400);

    bool isDark = m_settings.isDarkTheme();

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 10, 10, 10);

    QFrame *card = new QFrame(this);
    card->setObjectName("cardContainer");
    if (isDark) {
        card->setStyleSheet(
            "QFrame#cardContainer {"
            "    background-color: #18181b;"
            "    border: 1px solid #3f3f46;"
            "    border-radius: 12px;"
            "}"
            "QLabel { color: #f4f4f5; font-family: 'Segoe UI Variable Text', 'Segoe UI', sans-serif; font-size: 9pt; }"
        );
    } else {
        card->setStyleSheet(
            "QFrame#cardContainer {"
            "    background-color: #ffffff;"
            "    border: 1px solid #e4e4e7;"
            "    border-radius: 12px;"
            "}"
            "QLabel { color: #09090b; font-family: 'Segoe UI Variable Text', 'Segoe UI', sans-serif; font-size: 9pt; }"
        );
    }

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 12, 16, 16);
    cardLayout->setSpacing(10);

    // Custom Frameless Header Bar
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    topBarLayout->setSpacing(15);

    QLabel *lblIcon = new QLabel(card);
    lblIcon->setPixmap(IconProvider::getIcon(IconProvider::RamIcon).pixmap(24, 24));

    QLabel *lblHeader = new QLabel(QString("%1 • Software Updater").arg(APP_NAME), card);
    lblHeader->setStyleSheet("font-size: 9pt; font-weight: 600; color: #38bdf8;");

    QPushButton *btnClose = new QPushButton("X", card);
    btnClose->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: " + QString(isDark ? "#a1a1aa" : "#71717a") + ";"
        "    border: none;"
        "    font-size: 10pt;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ef4444;"
        "    color: #ffffff;"
        "}"
    );
    connect(btnClose, &QPushButton::clicked, this, [this]() { done(SkipUpdate); });

    topBarLayout->addWidget(lblIcon);
    topBarLayout->addWidget(lblHeader);
    topBarLayout->addStretch();
    topBarLayout->addWidget(btnClose);

    cardLayout->addLayout(topBarLayout);

    // Banner Details
    QHBoxLayout *infoLayout = new QHBoxLayout();
    infoLayout->setSpacing(12);

    QVBoxLayout *metaLayout = new QVBoxLayout();
    metaLayout->setSpacing(2);

    QLabel *lblTitle = new QLabel(QString("New Release Available: <b>Version %1</b>").arg(info.version), card);
    lblTitle->setStyleSheet("font-size: 10.5pt; font-weight: 600; color: " + QString(isDark ? "#ffffff" : "#0f172a") + ";");

    QLabel *lblSub = new QLabel(QString("Current Installed: %1 | Release Date: %2")
                                   .arg(APP_VERSION_STR).arg(info.releaseDate.isEmpty() ? "Latest" : info.releaseDate), card);
    lblSub->setStyleSheet("font-size: 8.5pt; color: " + QString(isDark ? "#a1a1aa" : "#71717a") + ";");

    metaLayout->addWidget(lblTitle);
    metaLayout->addWidget(lblSub);
    infoLayout->addLayout(metaLayout);
    infoLayout->addStretch();

    cardLayout->addLayout(infoLayout);

    // Changelog Text Area
    QTextBrowser *txtChangelog = new QTextBrowser(card);
    txtChangelog->setHtml(info.changelog);
    txtChangelog->setOpenExternalLinks(true);
    if (isDark) {
        txtChangelog->setStyleSheet(
            "QTextBrowser {"
            "    background-color: #09090b;"
            "    border: 1px solid #27272a;"
            "    border-radius: 8px;"
            "    padding: 8px;"
            "    font-size: 9pt;"
            "    color: #e4e4e7;"
            "}"
        );
    } else {
        txtChangelog->setStyleSheet(
            "QTextBrowser {"
            "    background-color: #f8fafc;"
            "    border: 1px solid #e2e8f0;"
            "    border-radius: 8px;"
            "    padding: 8px;"
            "    font-size: 9pt;"
            "    color: #0f172a;"
            "}"
        );
    }
    cardLayout->addWidget(txtChangelog);

    // Action Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QPushButton *btnDownload = new QPushButton("Update Automatically", card);
    btnDownload->setIcon(IconProvider::getIcon(IconProvider::RocketIcon));
    btnDownload->setStyleSheet(
        "QPushButton {"
        "    background-color: #0078d4;"
        "    color: #ffffff;"
        "    font-weight: 600;"
        "    font-size: 9pt;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 6px 14px;"
        "}"
        "QPushButton:hover { background-color: #106ebe; }"
    );
    connect(btnDownload, &QPushButton::clicked, this, [this]() { done(DownloadNow); });

    QPushButton *btnBrowser = new QPushButton("Open GitHub", card);
    btnBrowser->setIcon(IconProvider::getIcon(IconProvider::GithubIcon));
    btnBrowser->setStyleSheet(
        "QPushButton {"
        "    background-color: " + QString(isDark ? "#27272a" : "#f1f5f9") + ";"
        "    color: " + QString(isDark ? "#f4f4f5" : "#0f172a") + ";"
        "    font-weight: 500;"
        "    font-size: 9pt;"
        "    border: 1px solid " + QString(isDark ? "#3f3f46" : "#cbd5e1") + ";"
        "    border-radius: 6px;"
        "    padding: 6px 14px;"
        "}"
        "QPushButton:hover { background-color: " + QString(isDark ? "#3f3f46" : "#e2e8f0") + "; }"
    );
    connect(btnBrowser, &QPushButton::clicked, this, [this, info]() {
        QString targetUrl = info.directUrl.isEmpty() ? info.downloadUrl : info.directUrl;
        QDesktopServices::openUrl(QUrl(targetUrl));
        done(OpenBrowser);
    });

    QPushButton *btnSkip = new QPushButton("Later", card);
    btnSkip->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: " + QString(isDark ? "#a1a1aa" : "#64748b") + ";"
        "    font-weight: 500;"
        "    font-size: 9pt;"
        "    border: none;"
        "    padding: 6px 12px;"
        "}"
        "QPushButton:hover { color: " + QString(isDark ? "#ffffff" : "#000000") + "; }"
    );
    connect(btnSkip, &QPushButton::clicked, this, [this]() { done(SkipUpdate); });

    btnLayout->addWidget(btnDownload);
    btnLayout->addWidget(btnBrowser);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSkip);

    cardLayout->addLayout(btnLayout);
    outerLayout->addWidget(card);
}

void UpdateAvailableDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void UpdateAvailableDialog::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

// ─── Sleek Frameless Download Progress Dialog Implementation ───────────────
UpdateDownloadDialog::UpdateDownloadDialog(const UpdateInfo &info, QWidget *parent)
    : QDialog(parent)
    , m_info(info)
    , m_netManager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_file(nullptr)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->resize(460, 170);

    bool isDark = m_settings.isDarkTheme();

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 10, 10, 10);

    QFrame *card = new QFrame(this);
    card->setObjectName("cardContainer");
    if (isDark) {
        card->setStyleSheet(
            "QFrame#cardContainer {"
            "    background-color: #18181b;"
            "    border: 1px solid #3f3f46;"
            "    border-radius: 12px;"
            "}"
            "QLabel { color: #f4f4f5; font-family: 'Segoe UI Variable Text', 'Segoe UI', sans-serif; font-size: 9pt; }"
        );
    } else {
        card->setStyleSheet(
            "QFrame#cardContainer {"
            "    background-color: #ffffff;"
            "    border: 1px solid #e4e4e7;"
            "    border-radius: 12px;"
            "}"
            "QLabel { color: #09090b; font-family: 'Segoe UI Variable Text', 'Segoe UI', sans-serif; font-size: 9pt; }"
        );
    }

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 10, 14, 14);
    cardLayout->setSpacing(8);

    // Top Frameless Bar
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    QLabel *lblHeader = new QLabel(QString("Downloading Update %1...").arg(info.version), card);
    lblHeader->setStyleSheet("font-size: 9.5pt; font-weight: 600; color: #38bdf8;");

    QPushButton *btnClose = new QPushButton("✕", card);
    btnClose->setFixedSize(24, 24);
    btnClose->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: " + QString(isDark ? "#a1a1aa" : "#71717a") + ";"
        "    border: none;"
        "    font-size: 10.5pt;"
        "    font-weight: bold;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #ef4444;"
        "    color: #ffffff;"
        "}"
    );
    connect(btnClose, &QPushButton::clicked, this, &UpdateDownloadDialog::onCancelClicked);

    topBarLayout->addWidget(lblHeader);
    topBarLayout->addStretch();
    topBarLayout->addWidget(btnClose);

    cardLayout->addLayout(topBarLayout);

    // Download ProgressBar
    m_progressBar = new QProgressBar(card);
    m_progressBar->setObjectName("progressDownload");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setStyleSheet(
        "QProgressBar#progressDownload {"
        "    border: 1px solid " + QString(isDark ? "#3f3f46" : "#cbd5e1") + ";"
        "    border-radius: 6px;"
        "    text-align: center;"
        "    color: #ffffff;"
        "    font-size: 8.5pt;"
        "    font-weight: bold;"
        "    background-color: " + QString(isDark ? "#09090b" : "#f1f5f9") + ";"
        "}"
        "QProgressBar#progressDownload::chunk {"
        "    background-color: #0078d4;"
        "    border-radius: 5px;"
        "}"
    );

    m_lblMetrics = new QLabel("Connecting to update server...", card);
    m_lblMetrics->setStyleSheet("font-size: 8.5pt; color: " + QString(isDark ? "#a1a1aa" : "#64748b") + ";");

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnCancel = new QPushButton("Cancel", card);
    m_btnCancel->setStyleSheet(
        "QPushButton {"
        "    background-color: " + QString(isDark ? "#27272a" : "#f1f5f9") + ";"
        "    color: " + QString(isDark ? "#f4f4f5" : "#0f172a") + ";"
        "    font-size: 8.5pt;"
        "    font-weight: 500;"
        "    border: 1px solid " + QString(isDark ? "#3f3f46" : "#cbd5e1") + ";"
        "    border-radius: 5px;"
        "    padding: 4px 12px;"
        "}"
        "QPushButton:hover { background-color: #ef4444; color: #ffffff; border-color: #ef4444; }"
    );
    connect(m_btnCancel, &QPushButton::clicked, this, &UpdateDownloadDialog::onCancelClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCancel);

    cardLayout->addWidget(m_progressBar);
    cardLayout->addWidget(m_lblMetrics);
    cardLayout->addLayout(btnLayout);

    outerLayout->addWidget(card);

    // Temp download path
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_destinationPath = QDir(tempDir).filePath("RAM_Cleaner_Update.zip");
}

UpdateDownloadDialog::~UpdateDownloadDialog() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
    if (m_file) {
        if (m_file->isOpen()) m_file->close();
        delete m_file;
    }
}

void UpdateDownloadDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void UpdateDownloadDialog::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void UpdateDownloadDialog::startDownload() {
    if (QFile::exists(m_destinationPath)) {
        QFile::remove(m_destinationPath);
    }

    m_file = new QFile(m_destinationPath, this);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Download Error", QString("Failed to create temporary file:\n%1").arg(m_destinationPath));
        reject();
        return;
    }

    QUrl url(m_info.downloadUrl);
    QUrlQuery query(url);
    query.addQueryItem("t", QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setRawHeader("User-Agent", "RAMCleanerFlusherPro-Updater/1.2");
    request.setRawHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request.setRawHeader("Pragma", "no-cache");
    request.setRawHeader("Expires", "0");

    m_reply = m_netManager->get(request);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &UpdateDownloadDialog::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateDownloadDialog::onDownloadFinished);

    m_speedTimer.start();
    m_lastBytesReceived = 0;
}

void UpdateDownloadDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (m_reply && m_file && m_file->isOpen()) {
        m_file->write(m_reply->readAll());
    }

    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressBar->setValue(percent);

        double receivedMB = bytesReceived / (1024.0 * 1024.0);
        double totalMB = bytesTotal / (1024.0 * 1024.0);

        qint64 elapsedMs = m_speedTimer.elapsed();
        double speedBytesPerSec = (elapsedMs > 0) ? (bytesReceived * 1000.0 / elapsedMs) : 0.0;

        QString speedStr;
        if (speedBytesPerSec < (1024.0 * 1024.0)) {
            double speedKBps = speedBytesPerSec / 1024.0;
            speedStr = QString("%1 KB/s").arg(speedKBps, 0, 'f', 1);
        } else {
            double speedMBps = speedBytesPerSec / (1024.0 * 1024.0);
            speedStr = QString("%1 MB/s").arg(speedMBps, 0, 'f', 2);
        }

        qint64 remainingBytes = bytesTotal - bytesReceived;
        int remainingSec = (speedBytesPerSec > 0) ? static_cast<int>(remainingBytes / speedBytesPerSec) : 0;

        int mins = remainingSec / 60;
        int secs = remainingSec % 60;

        m_lblMetrics->setText(QString("%1 MB / %2 MB (%3) — %4:%5 remaining")
                                  .arg(receivedMB, 0, 'f', 2)
                                  .arg(totalMB, 0, 'f', 2)
                                  .arg(speedStr)
                                  .arg(mins, 2, 10, QChar('0'))
                                  .arg(secs, 2, 10, QChar('0')));
    }
}

void UpdateDownloadDialog::onDownloadFinished() {
    if (!m_reply) return;

    if (m_reply->error() == QNetworkReply::NoError) {
        if (m_file && m_file->isOpen()) {
            m_file->write(m_reply->readAll());
            m_file->close();
        }

        m_progressBar->setValue(100);
        m_lblMetrics->setStyleSheet("font-size: 8.5pt; color: #10b981; font-weight: bold;");
        m_lblMetrics->setText("Download completed successfully!");

        emit downloadCompleted(m_destinationPath);
        accept();
    } else {
        if (m_file && m_file->isOpen()) m_file->close();
        QFile::remove(m_destinationPath);

        QMessageBox::critical(this, "Download Failed", QString("Network error occurred while downloading update:\n%1").arg(m_reply->errorString()));
        reject();
    }
}

void UpdateDownloadDialog::onCancelClicked() {
    if (m_reply) {
        m_reply->abort();
    }
    if (m_file && m_file->isOpen()) {
        m_file->close();
        QFile::remove(m_destinationPath);
    }
    emit downloadCancelled();
    reject();
}

// ─── Central Update Manager Implementation ──────────────────────────────────
UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_netManager(new QNetworkAccessManager(this))
    , m_checkReply(nullptr)
{
}

UpdateManager::~UpdateManager() {
    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
    }
}

UpdateManager* UpdateManager::instance() {
    static UpdateManager s_instance;
    return &s_instance;
}

void UpdateManager::checkForUpdates(bool silent, QWidget *parentWidget) {
    m_silentCheck = silent;
    m_parentWidget = parentWidget;

    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
    }

    QUrl url(APP_UPDATE_URL);
    QUrlQuery query(url);
    query.addQueryItem("t", QString::number(QDateTime::currentMSecsSinceEpoch()));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setRawHeader("User-Agent", "RAMCleanerFlusherPro-Updater/1.2");
    request.setRawHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request.setRawHeader("Pragma", "no-cache");
    request.setRawHeader("Expires", "0");

    m_checkReply = m_netManager->get(request);
    connect(m_checkReply, &QNetworkReply::finished, this, &UpdateManager::onCheckFinished);
}

void UpdateManager::onCheckFinished() {
    if (!m_checkReply) return;

    if (m_checkReply->error() != QNetworkReply::NoError) {
        qWarning() << "[UpdateManager] Network check failed:" << m_checkReply->errorString();
        if (!m_silentCheck) {
            QMessageBox::warning(m_parentWidget, "Update Check Failed",
                                 QString("Unable to connect to update server:\n%1").arg(m_checkReply->errorString()));
        }
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
        return;
    }

    QByteArray responseData = m_checkReply->readAll();
    m_checkReply->deleteLater();
    m_checkReply = nullptr;

    QJsonParseError parseErr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "[UpdateManager] JSON parse error:" << parseErr.errorString();
        if (!m_silentCheck) {
            QMessageBox::warning(m_parentWidget, "Update Check Error", "Invalid update response received from server.");
        }
        return;
    }

    QJsonObject obj = jsonDoc.object();
    UpdateInfo info;
    info.version = obj.value("version").toString();
    info.releaseDate = obj.value("release_date").toString();
    info.downloadUrl = obj.value("download_url").toString();
    info.directUrl = obj.value("url").toString();
    info.changelog = obj.value("changelog").toString();
    info.isValid = !info.version.isEmpty() && !info.downloadUrl.isEmpty();

    if (!info.isValid) {
        if (!m_silentCheck) {
            QMessageBox::warning(m_parentWidget, "Update Check Error", "Update manifest contains incomplete data.");
        }
        return;
    }

    bool hasNewer = isVersionNewer(APP_VERSION_STR, info.version);
    emit updateCheckCompleted(hasNewer, info);

    if (hasNewer) {
        UpdateAvailableDialog dlg(info, m_parentWidget);
        int res = dlg.exec();

        if (res == UpdateAvailableDialog::DownloadNow) {
            UpdateDownloadDialog downloadDlg(info, m_parentWidget);
            downloadDlg.startDownload();
            if (downloadDlg.exec() == QDialog::Accepted) {
                QString zipPath = downloadDlg.getDownloadedFilePath();
                emit updateReadyForInstall(zipPath);
                InstallerManager::instance()->performAppUpdate(zipPath, m_parentWidget);
            }
        }
    } else {
        if (!m_silentCheck) {
            QMessageBox::information(m_parentWidget, "No Updates Available",
                                     QString("You are using the latest version of %1 (%2).").arg(APP_NAME).arg(APP_VERSION_STR));
        }
    }
}
