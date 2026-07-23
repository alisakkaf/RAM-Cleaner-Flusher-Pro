#include "installer_manager.h"
#include "version.h"
#include "icon_provider.h"
#include "settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QMessageBox>
#include <QFrame>
#include <QDebug>

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// ─── Sleek Frameless Installation / Update Progress Dialog ─────────────────
InstallProgressDialog::InstallProgressDialog(bool isUpdate, QWidget *parent)
    : QDialog(parent)
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

    // Top Frameless Header
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    QLabel *lblHeader = new QLabel(isUpdate ? QString("%1 • Applying Update...").arg(APP_NAME) : QString("%1 • Self-Installing...").arg(APP_NAME), card);
    lblHeader->setStyleSheet("font-size: 9.5pt; font-weight: 600; color: #38bdf8;");

    m_btnClose = new QPushButton("x", card);
    m_btnClose->setEnabled(false);
    m_btnClose->setStyleSheet(
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
        "QPushButton:disabled { color: " + QString(isDark ? "#3f3f46" : "#cbd5e1") + "; }"
    );
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    topBarLayout->addWidget(lblHeader);
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_btnClose);

    cardLayout->addLayout(topBarLayout);

    // Install ProgressBar
    m_progressBar = new QProgressBar(card);
    m_progressBar->setObjectName("progressInstall");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(20);
    m_progressBar->setStyleSheet(
        "QProgressBar#progressInstall {"
        "    border: 1px solid " + QString(isDark ? "#3f3f46" : "#cbd5e1") + ";"
        "    border-radius: 6px;"
        "    text-align: center;"
        "    color: #ffffff;"
        "    font-size: 8.5pt;"
        "    font-weight: bold;"
        "    background-color: " + QString(isDark ? "#09090b" : "#f1f5f9") + ";"
        "}"
        "QProgressBar#progressInstall::chunk {"
        "    background-color: #0078d4;"
        "    border-radius: 5px;"
        "}"
    );

    m_lblStatus = new QLabel("Initializing setup environment...", card);
    m_lblStatus->setStyleSheet("font-size: 8.5pt; color: " + QString(isDark ? "#a1a1aa" : "#64748b") + ";");

    cardLayout->addWidget(m_progressBar);
    cardLayout->addWidget(m_lblStatus);

    outerLayout->addWidget(card);
}

void InstallProgressDialog::setStatus(const QString &message, int percent) {
    m_lblStatus->setText(message);
    m_progressBar->setValue(percent);
    QCoreApplication::processEvents();
}

void InstallProgressDialog::setCompletedSuccess(const QString &message) {
    m_lblStatus->setStyleSheet("font-size: 8.5pt; color: #10b981; font-weight: bold;");
    m_lblStatus->setText(message);
    m_progressBar->setValue(100);
    m_btnClose->setEnabled(true);
    QCoreApplication::processEvents();
}

void InstallProgressDialog::setCompletedError(const QString &errorMsg) {
    m_lblStatus->setStyleSheet("font-size: 8.5pt; color: #ef4444; font-weight: bold;");
    m_lblStatus->setText(QString("Error: %1").arg(errorMsg));
    m_btnClose->setEnabled(true);
    QCoreApplication::processEvents();
}

void InstallProgressDialog::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void InstallProgressDialog::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

// ─── Win32 Installer Manager Implementation ──────────────────────────────────
InstallerManager::InstallerManager(QObject *parent)
    : QObject(parent)
{
}

InstallerManager::~InstallerManager() {
}

InstallerManager* InstallerManager::instance() {
    static InstallerManager s_instance;
    return &s_instance;
}

QString InstallerManager::getProgramFilesPath() {
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, szPath))) {
        return QDir(QString::fromWCharArray(szPath)).filePath(APP_INSTALL_DIR_NAME);
    }
    return QString("C:\\Program Files\\%1").arg(APP_INSTALL_DIR_NAME);
}

QString InstallerManager::getDesktopShortcutPath() {
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, szPath))) {
        return QDir(QString::fromWCharArray(szPath)).filePath(QString("%1.lnk").arg(APP_NAME));
    }
    return QString();
}

QString InstallerManager::getCurrentExePath() {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        return QString::fromWCharArray(szPath);
    }
    return QCoreApplication::applicationFilePath();
}

bool InstallerManager::isRunningFromProgramFiles() {
    QString currentExe = QDir::toNativeSeparators(getCurrentExePath()).toLower();
    QString targetInstallDir = QDir::toNativeSeparators(getProgramFilesPath()).toLower();
    return currentExe.startsWith(targetInstallDir);
}

bool InstallerManager::createDesktopShortcut(const QString &targetExePath) {
    HRESULT hr = CoInitialize(NULL);
    bool success = false;

    IShellLinkW *psl = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));
    if (SUCCEEDED(hr)) {
        psl->SetPath(targetExePath.toStdWString().c_str());
        psl->SetDescription(L"RAM Cleaner & Flusher Pro Suite");
        psl->SetWorkingDirectory(QFileInfo(targetExePath).absolutePath().toStdWString().c_str());
        psl->SetIconLocation(targetExePath.toStdWString().c_str(), 0);

        IPersistFile *ppf = nullptr;
        hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
        if (SUCCEEDED(hr)) {
            QString shortcutPath = getDesktopShortcutPath();
            if (!shortcutPath.isEmpty()) {
                hr = ppf->Save(shortcutPath.toStdWString().c_str(), TRUE);
                success = SUCCEEDED(hr);
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return success;
}

bool InstallerManager::killRunningInstances() {
    DWORD currentPid = GetCurrentProcessId();
    QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << APP_EXE_NAME << "/FI" << QString("PID ne %1").arg(currentPid));
    return true;
}

bool InstallerManager::grantFullFilePermissions(const QString &filePath) {
    QString nativePath = QDir::toNativeSeparators(filePath);
    QProcess::execute("icacls", QStringList() << nativePath << "/grant" << "Administrators:F" << "/grant" << "Users:F" << "/T");
    return true;
}

bool InstallerManager::performSelfInstallation(QWidget *parentWidget) {
    if (isRunningFromProgramFiles()) {
        return true;
    }

    InstallProgressDialog dlg(false, parentWidget);
    dlg.show();

    dlg.setStatus("Step 1/5: Terminating background instances...", 15);
    killRunningInstances();
    Sleep(400);

    QString installDir = getProgramFilesPath();
    dlg.setStatus(QString("Step 2/5: Creating installation folder: %1").arg(installDir), 35);
    QDir().mkpath(installDir);

    QString targetExePath = QDir(installDir).filePath(APP_EXE_NAME);
    QString currentExe = getCurrentExePath();

    dlg.setStatus("Step 3/5: Deploying executable to Program Files...", 55);
    if (QFile::exists(targetExePath)) {
        QFile::remove(targetExePath);
    }
    if (!QFile::copy(currentExe, targetExePath)) {
        dlg.setCompletedError(QString("Failed to copy binary to %1").arg(targetExePath));
        dlg.exec();
        return false;
    }

    dlg.setStatus("Step 4/5: Granting Win32 file permissions...", 75);
    grantFullFilePermissions(installDir);

    dlg.setStatus("Step 5/5: Creating Desktop Shortcut...", 90);
    createDesktopShortcut(targetExePath);

    dlg.setCompletedSuccess("Installation completed! Launching app...");
    Sleep(600);

    // Launch newly installed app from Program Files
    std::wstring wExe = QDir::toNativeSeparators(targetExePath).toStdWString();
    ShellExecuteW(NULL, L"runas", wExe.c_str(), NULL, NULL, SW_SHOWNORMAL);

    QCoreApplication::quit();
    exit(0);
    return true;
}

bool InstallerManager::performAppUpdate(const QString &zipOrExePath, QWidget *parentWidget) {
    InstallProgressDialog dlg(true, parentWidget);
    dlg.show();

    dlg.setStatus("Step 1/5: Terminating active application processes...", 20);
    killRunningInstances();
    Sleep(500);

    QString installDir = getProgramFilesPath();
    QDir().mkpath(installDir);
    QString targetExePath = QDir(installDir).filePath(APP_EXE_NAME);

    dlg.setStatus("Step 2/5: Replacing executable with updated binary...", 50);
    if (QFile::exists(targetExePath)) {
        QFile::remove(targetExePath);
    }

    if (!QFile::copy(zipOrExePath, targetExePath)) {
        dlg.setCompletedError(QString("Failed to copy update file to %1").arg(targetExePath));
        dlg.exec();
        return false;
    }

    dlg.setStatus("Step 3/5: Setting full file permissions...", 75);
    grantFullFilePermissions(installDir);

    dlg.setStatus("Step 4/5: Updating Desktop Shortcut...", 90);
    createDesktopShortcut(targetExePath);

    dlg.setCompletedSuccess("Update applied! Restarting RAM Cleaner Pro...");
    Sleep(600);

    // Relaunch updated process
    std::wstring wExe = QDir::toNativeSeparators(targetExePath).toStdWString();
    ShellExecuteW(NULL, L"runas", wExe.c_str(), NULL, NULL, SW_SHOWNORMAL);

    QCoreApplication::quit();
    exit(0);
    return true;
}
