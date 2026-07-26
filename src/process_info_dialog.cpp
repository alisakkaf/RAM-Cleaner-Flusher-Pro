#include "process_info_dialog.h"
#include "icon_provider.h"
#include "mainwindow.h"
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>

ProcessInfoDialog::ProcessInfoDialog(DWORD pid, const QString &procName, QWidget *parent)
    : QDialog(parent)
    , m_pid(pid)
    , m_procName(procName)
{
    setupUI();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &ProcessInfoDialog::refreshProcessDetails);
    m_refreshTimer->start(1000);

    refreshProcessDetails();
}

ProcessInfoDialog::~ProcessInfoDialog() {
}

void ProcessInfoDialog::setupUI() {
    this->setWindowTitle(trXml("dlgSingleWinTitle", "Process Inspector - %1 (PID: %2)").arg(m_procName).arg(m_pid));
    this->setMinimumSize(520, 420);
    this->setLayoutDirection(qApp->layoutDirection());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Header Card
    QGroupBox *headerGroup = new QGroupBox(trXml("dlgSingleCardTitle", "Process Identity"), this);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerGroup);
    headerLayout->setContentsMargins(12, 10, 12, 10);
    headerLayout->setSpacing(12);

    m_lblIcon = new QLabel(this);
    m_lblIcon->setFixedSize(48, 48);
    m_lblIcon->setScaledContents(true);
    m_lblIcon->setPixmap(IconProvider::getProcessIcon(m_pid, m_procName).pixmap(48, 48));

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);

    m_lblTitle = new QLabel(m_procName, this);
    m_lblTitle->setStyleSheet("font-size: 12pt; font-weight: bold; color: #0284c7;");

    m_lblPid = new QLabel(trXml("dlgSinglePidLabel", "Process ID (PID): %1").arg(m_pid), this);
    m_lblPid->setStyleSheet("font-size: 9pt; color: #64748b;");

    headerTextLayout->addWidget(m_lblTitle);
    headerTextLayout->addWidget(m_lblPid);

    headerLayout->addWidget(m_lblIcon);
    headerLayout->addLayout(headerTextLayout);
    headerLayout->addStretch();

    mainLayout->addWidget(headerGroup);

    // Diagnostics Group
    QGroupBox *detailsGroup = new QGroupBox(trXml("dlgSingleDiagTitle", "Real-Time Diagnostics & System Info"), this);
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    detailsLayout->setSpacing(6);
    detailsLayout->setContentsMargins(12, 12, 12, 12);

    m_lblPath = new QLabel(this);
    m_lblPath->setWordWrap(true);

    m_lblWorkingSet = new QLabel(this);
    m_lblPeakWorkingSet = new QLabel(this);
    m_lblPagefile = new QLabel(this);
    m_lblPriority = new QLabel(this);
    m_lblHandles = new QLabel(this);
    m_lblStartTime = new QLabel(this);

    detailsLayout->addWidget(m_lblPath);
    detailsLayout->addWidget(m_lblWorkingSet);
    detailsLayout->addWidget(m_lblPeakWorkingSet);
    detailsLayout->addWidget(m_lblPagefile);
    detailsLayout->addWidget(m_lblPriority);
    detailsLayout->addWidget(m_lblHandles);
    detailsLayout->addWidget(m_lblStartTime);

    mainLayout->addWidget(detailsGroup);

    // Actions Bar
    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(8);

    m_btnProtect = new QPushButton(trXml("btnProtectProcess", "Protect Process"), this);
    m_btnProtect->setIcon(IconProvider::getIcon(IconProvider::ShieldIcon));
    connect(m_btnProtect, &QPushButton::clicked, this, &ProcessInfoDialog::onProtectClicked);

    m_btnTrim = new QPushButton(trXml("btnTrimWorkingSet", "Trim Working Set"), this);
    m_btnTrim->setIcon(IconProvider::getIcon(IconProvider::RefreshIcon));
    connect(m_btnTrim, &QPushButton::clicked, this, &ProcessInfoDialog::onTrimClicked);

    m_btnKill = new QPushButton(trXml("btnTaskKill", "Task Kill"), this);
    m_btnKill->setIcon(IconProvider::getIcon(IconProvider::TrashIcon));
    m_btnKill->setStyleSheet("QPushButton { background-color: #991b1b; color: #ffffff; border-color: #ef4444; padding: 6px 12px; } QPushButton:hover { background-color: #dc2626; }");
    connect(m_btnKill, &QPushButton::clicked, this, &ProcessInfoDialog::onKillClicked);

    m_btnClose = new QPushButton(trXml("btnClose", "Close"), this);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    actionsLayout->addWidget(m_btnProtect);
    actionsLayout->addWidget(m_btnTrim);
    actionsLayout->addWidget(m_btnKill);
    actionsLayout->addStretch();
    actionsLayout->addWidget(m_btnClose);

    mainLayout->addLayout(actionsLayout);
}

void ProcessInfoDialog::refreshProcessDetails() {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_pid);
    if (!hProc) {
        hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);
    }

    if (!hProc) {
        m_lblWorkingSet->setText(trXml("lblRamWorkingSetFull", "RAM Working Set: Process Exited / Access Denied"));
        return;
    }

    // 1. Executable Path
    wchar_t pathBuf[MAX_PATH] = { 0 };
    if (GetModuleFileNameExW(hProc, NULL, pathBuf, MAX_PATH) > 0) {
        m_lblPath->setText(trXml("lblExecPath", "Executable Path: %1").arg(QString::fromWCharArray(pathBuf)));
    } else {
        m_lblPath->setText(trXml("lblExecPath", "Executable Path: %1").arg(m_procName));
    }

    // 2. Memory Counters
    PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };
    if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
        double wsMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
        double peakMB = pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
        double pageMB = pmc.PagefileUsage / (1024.0 * 1024.0);

        m_lblWorkingSet->setText(trXml("lblRamWorkingSetFull", "RAM Working Set: %1 MB (%2 bytes)").arg(wsMB, 0, 'f', 2).arg(pmc.WorkingSetSize));
        m_lblPeakWorkingSet->setText(trXml("lblPeakRam", "Peak RAM Usage: %1 MB").arg(peakMB, 0, 'f', 2));
        m_lblPagefile->setText(trXml("lblPagefile", "Pagefile Commitment: %1 MB").arg(pageMB, 0, 'f', 2));
    }

    // 3. Priority Class
    DWORD prio = GetPriorityClass(hProc);
    QString prioStr = trXml("prioNormal", "Normal");
    if (prio == HIGH_PRIORITY_CLASS) prioStr = trXml("prioHigh", "High Priority");
    else if (prio == ABOVE_NORMAL_PRIORITY_CLASS) prioStr = trXml("prioAboveNormal", "Above Normal");
    else if (prio == BELOW_NORMAL_PRIORITY_CLASS) prioStr = trXml("prioBelowNormal", "Below Normal");
    else if (prio == IDLE_PRIORITY_CLASS) prioStr = trXml("prioIdle", "Idle");
    else if (prio == REALTIME_PRIORITY_CLASS) prioStr = trXml("prioRealtime", "Realtime");

    m_lblPriority->setText(trXml("lblCpuPriority", "CPU Priority Class: %1").arg(prioStr));

    // 4. Handle Count
    DWORD handleCount = 0;
    if (GetProcessHandleCount(hProc, &handleCount)) {
        m_lblHandles->setText(trXml("lblOpenHandles", "Open Handles Count: %1").arg(handleCount));
    }

    // 5. Creation Time
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    if (GetProcessTimes(hProc, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
        SYSTEMTIME stUTC, stLocal;
        FileTimeToSystemTime(&ftCreate, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
        QDateTime dt(QDate(stLocal.wYear, stLocal.wMonth, stLocal.wDay), QTime(stLocal.wHour, stLocal.wMinute, stLocal.wSecond));
        m_lblStartTime->setText(trXml("lblStartTime", "Process Start Time: %1").arg(dt.toString("yyyy-MM-dd hh:mm:ss AP")));
    }

    CloseHandle(hProc);
}

void ProcessInfoDialog::onProtectClicked() {
    emit protectRequested(m_procName);
}

void ProcessInfoDialog::onKillClicked() {
    emit killRequested(m_pid, m_procName);
    this->accept();
}

void ProcessInfoDialog::onTrimClicked() {
    emit trimRequested(m_pid, m_procName);
}
