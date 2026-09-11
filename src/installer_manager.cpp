#include "installer_manager.h"
#include "version.h"
#include "icon_provider.h"
#include "settings_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QProcess>
#include <QCoreApplication>
#include <QMessageBox>
#include <QFrame>
#include <QDebug>

#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shldisp.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// XML Helper declaration
QString trXml(const QString &key, const QString &defaultVal = QString());

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
    QLabel *lblHeader = new QLabel(isUpdate ? trXml("dlgInstallTitleUpdate", "%1 • Applying Update...").arg(APP_NAME) : trXml("dlgInstallTitleSelf", "%1 • Self-Installing...").arg(APP_NAME), card);
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

    m_lblStatus = new QLabel(trXml("lblInitSetupEnv", "Initializing setup environment..."), card);
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
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (pe.th32ProcessID != currentPid && pe.th32ProcessID != 0 && pe.th32ProcessID != 4) {
                QString procName = QString::fromWCharArray(pe.szExeFile);
                if (procName.compare(APP_EXE_NAME, Qt::CaseInsensitive) == 0 ||
                    procName.compare(QString(APP_EXE_NAME) + ".exe", Qt::CaseInsensitive) == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return true;
}

bool InstallerManager::grantFullFilePermissions(const QString &filePath) {
    if (filePath.isEmpty()) return false;
    QFileDevice::Permissions p = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                  QFileDevice::ReadUser  | QFileDevice::WriteUser  | QFileDevice::ExeUser  |
                                  QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                  QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    return QFile::setPermissions(filePath, p);
}

bool InstallerManager::performSelfInstallation(QWidget *parentWidget) {
    if (isRunningFromProgramFiles()) {
        return true;
    }

    InstallProgressDialog dlg(false, parentWidget);
    dlg.show();

    dlg.setStatus(trXml("lblStepTermBg", "Step 1/5: Terminating background instances..."), 15);
    killRunningInstances();
    Sleep(400);

    QString installDir = getProgramFilesPath();
    dlg.setStatus(trXml("lblStepCreateInstallDir", "Step 2/5: Creating installation folder: %1").arg(installDir), 35);
    QDir().mkpath(installDir);

    QString targetExePath = QDir(installDir).filePath(APP_EXE_NAME);
    QString currentExe = getCurrentExePath();

    dlg.setStatus(trXml("lblStepDeployProgFiles", "Step 3/5: Deploying executable to Program Files..."), 55);
    if (QFile::exists(targetExePath)) {
        QFile::remove(targetExePath);
    }
    if (!QFile::copy(currentExe, targetExePath)) {
        dlg.setCompletedError(trXml("msgErrCopyBin", "Failed to copy binary to %1").arg(targetExePath));
        dlg.exec();
        return false;
    }

    dlg.setStatus(trXml("lblStepGrantPerms", "Step 4/5: Granting Win32 file permissions..."), 75);
    grantFullFilePermissions(installDir);

    dlg.setStatus(trXml("lblStepCreateShortcut", "Step 5/5: Creating Desktop Shortcut..."), 90);
    createDesktopShortcut(targetExePath);

    dlg.setCompletedSuccess(trXml("msgSelfInstallSuccess", "Installation completed! Launching app..."));
    // Sleep(10);

    // Launch newly installed app from Program Files
    std::wstring wExe = QDir::toNativeSeparators(targetExePath).toStdWString();
    ShellExecuteW(NULL, L"open", wExe.c_str(), NULL, NULL, SW_SHOWNORMAL);

    QCoreApplication::quit();
    exit(0);
    return true;
}

// ─── Extract .exe from .zip using official Win32 COM Shell API (no PowerShell) ─
// Uses IShellDispatch / Folder / FolderItems — the standard Windows ZIP extraction
// method available natively on Windows 7, 8, 10, 11 without external libraries.
static bool extractExeFromZip(const QString &zipPath, const QString &destDir, const QString &exeName) {
    HRESULT hr = CoInitialize(NULL);
    bool coWasInit = SUCCEEDED(hr);  // track if we initialized COM here

    bool success = false;

    do {
        // ── 1. Create a Shell COM instance ──────────────────────────────────
        IShellDispatch *pISD = nullptr;
        hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                              IID_IShellDispatch,
                              reinterpret_cast<void **>(&pISD));
        if (FAILED(hr) || !pISD) break;

        // ── 2. Open the ZIP file as a Shell Folder namespace ─────────────────
        VARIANT vZip;
        VariantInit(&vZip);
        V_VT(&vZip)   = VT_BSTR;
        V_BSTR(&vZip) = SysAllocString(QDir::toNativeSeparators(zipPath).toStdWString().c_str());

        Folder *pZipFolder = nullptr;
        hr = pISD->NameSpace(vZip, &pZipFolder);
        VariantClear(&vZip);

        if (FAILED(hr) || !pZipFolder) { pISD->Release(); break; }

        // ── 3. Open the destination folder as a Shell Folder namespace ───────
        VARIANT vDest;
        VariantInit(&vDest);
        V_VT(&vDest)   = VT_BSTR;
        V_BSTR(&vDest) = SysAllocString(QDir::toNativeSeparators(destDir).toStdWString().c_str());

        Folder *pDestFolder = nullptr;
        hr = pISD->NameSpace(vDest, &pDestFolder);
        VariantClear(&vDest);

        if (FAILED(hr) || !pDestFolder) {
            pZipFolder->Release();
            pISD->Release();
            break;
        }

        // ── 4. Enumerate all items inside the ZIP ────────────────────────────
        FolderItems *pItems = nullptr;
        hr = pZipFolder->Items(&pItems);
        if (FAILED(hr) || !pItems) {
            pDestFolder->Release();
            pZipFolder->Release();
            pISD->Release();
            break;
        }

        // ── 5. CopyHere into destination ─────────────────────────────────────
        // Flags: 4  = no progress dialog
        //        16 = respond Yes to all dialogs (overwrite)
        //        512= no confirmation dialog for file ops
        //        1024= no error UI
        VARIANT vItems;
        VariantInit(&vItems);
        V_VT(&vItems)       = VT_DISPATCH;
        V_DISPATCH(&vItems) = pItems;
        pItems->AddRef();

        VARIANT vOptions;
        VariantInit(&vOptions);
        V_VT(&vOptions) = VT_I4;
        V_I4(&vOptions) = 4 | 16 | 512 | 1024;

        hr = pDestFolder->CopyHere(vItems, vOptions);
        VariantClear(&vItems);

        // COM Shell extraction is asynchronous — poll until the exe appears
        if (SUCCEEDED(hr)) {
            QString targetExe = QDir(destDir).filePath(exeName);
            for (int wait = 0; wait < 30; ++wait) {   // up to 15 seconds
                if (QFile::exists(targetExe) && QFileInfo(targetExe).size() > 1024) {
                    success = true;
                    break;
                }
                Sleep(500);
                QCoreApplication::processEvents();
            }
        }

        pItems->Release();
        pDestFolder->Release();
        pZipFolder->Release();
        pISD->Release();

    } while (false);

    if (coWasInit) CoUninitialize();

    // ── Fallback: search subdirectories if exe landed in a sub-folder ────────
    if (!success) {
        QString directPath = QDir(destDir).filePath(exeName);
        if (QFile::exists(directPath)) {
            success = true;
        } else {
            QDirIterator it(destDir, QStringList() << exeName,
                            QDir::Files, QDirIterator::Subdirectories);
            if (it.hasNext()) {
                it.next();
                // Move it to the expected flat location
                if (QFile::rename(it.filePath(), directPath))
                    success = true;
            }
        }
    }

    return success;
}

// ─── Safely remove a locked exe using rename + Win32 MoveFileEx fallback ────
static bool safeRemoveExe(const QString &exePath) {
    if (!QFile::exists(exePath))
        return true;

    // Step 1: Grant write permissions first using Qt native API
    QFile::setPermissions(exePath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadUser  | QFileDevice::WriteUser  | QFileDevice::ExeUser  |
        QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
        QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);

    // Step 2: Try direct removal with retry
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (QFile::remove(exePath))
            return true;
        Sleep(300);
    }

    // Step 3: Rename to .old so we can overwrite target path
    QString oldPath = exePath + ".old";
    QFile::remove(oldPath);  // remove stale .old if exists
    if (QFile::rename(exePath, oldPath)) {
        // Schedule .old for deletion on next reboot via Win32
        MoveFileExW(oldPath.toStdWString().c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        return true;  // target path is now free
    }

    return false;
}

bool InstallerManager::performAppUpdate(const QString &zipOrExePath, QWidget *parentWidget) {
    InstallProgressDialog dlg(true, parentWidget);
    dlg.show();

    // ── Step 1: Kill other running instances (keep current alive for now) ──
    dlg.setStatus(trXml("lblStepTermOther", "Step 1/5: Terminating other application instances..."), 15);
    killRunningInstances();
    Sleep(400);

    QString installDir   = getProgramFilesPath();
    QDir().mkpath(installDir);
    QString targetExePath = QDir(installDir).filePath(APP_EXE_NAME);
    QString tempExtractDir = QDir(QDir::tempPath()).filePath("RamCleanerUpdate_Extract");

    // ── Step 2: Extract the downloaded ZIP to temp folder ──
    dlg.setStatus(trXml("lblStepExtractZip", "Step 2/5: Extracting update archive..."), 35);
    QDir(tempExtractDir).removeRecursively();
    QDir().mkpath(tempExtractDir);

    bool isZip = zipOrExePath.endsWith(".zip", Qt::CaseInsensitive);
    QString newExePath;

    if (isZip) {
        if (!extractExeFromZip(zipOrExePath, tempExtractDir, APP_EXE_NAME)) {
            dlg.setCompletedError(trXml("msgErrExtractZip", "Failed to extract update archive. Please download manually."));
            dlg.exec();
            QDir(tempExtractDir).removeRecursively();
            return false;
        }
        newExePath = QDir(tempExtractDir).filePath(APP_EXE_NAME);
    } else {
        // Already an exe
        newExePath = zipOrExePath;
    }

    // Verify extracted exe exists and has non-zero size
    QFileInfo newExeInfo(newExePath);
    if (!newExeInfo.exists() || newExeInfo.size() < 1024) {
        dlg.setCompletedError(trXml("msgErrInvalidExtracted", "Extracted file appears invalid or corrupted."));
        dlg.exec();
        QDir(tempExtractDir).removeRecursively();
        return false;
    }

    // ── Step 3: Remove old exe safely (rename-on-reboot fallback) ──
    dlg.setStatus(trXml("lblStepReplaceExe", "Step 3/5: Replacing old executable..."), 55);
    if (!safeRemoveExe(targetExePath)) {
        dlg.setCompletedError(trXml("msgErrCannotReplaceLocked", "Cannot replace locked file:\n%1\nClose all instances and try again.").arg(targetExePath));
        dlg.exec();
        QDir(tempExtractDir).removeRecursively();
        return false;
    }

    // ── Step 4: Copy new exe to install dir ──
    dlg.setStatus(trXml("lblStepDeployBin", "Step 4/5: Deploying updated binary..."), 75);

    // Grant permissions on destination dir first
    grantFullFilePermissions(installDir);
    QFile::setPermissions(installDir,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadUser  | QFileDevice::WriteUser  | QFileDevice::ExeUser  |
        QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
        QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);

    bool copyOk = false;
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (QFile::copy(newExePath, targetExePath)) {
            copyOk = true;
            break;
        }
        Sleep(400);
    }

    QDir(tempExtractDir).removeRecursively();
    QFile::remove(zipOrExePath);  // clean up downloaded zip

    if (!copyOk) {
        dlg.setCompletedError(trXml("msgErrFailedDeploy", "Failed to deploy update to:\n%1").arg(targetExePath));
        dlg.exec();
        return false;
    }

    // Set full permissions on new exe
    QFile::setPermissions(targetExePath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadUser  | QFileDevice::WriteUser  | QFileDevice::ExeUser  |
        QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
        QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);
    grantFullFilePermissions(targetExePath);

    // ── Step 5: Shortcut + Relaunch ──
    dlg.setStatus(trXml("lblStepUpdateShortcut", "Step 5/5: Updating Desktop Shortcut & restarting..."), 92);
    createDesktopShortcut(targetExePath);

    dlg.setCompletedSuccess(trXml("msgUpdateSuccess", "Update applied successfully! Restarting..."));

    // Sleep(400);
    std::wstring wExe = QDir::toNativeSeparators(targetExePath).toStdWString();
    ShellExecuteW(NULL, L"open", wExe.c_str(), NULL, NULL, SW_SHOWNORMAL);

    QCoreApplication::quit();
    exit(0);
    return true;
}
