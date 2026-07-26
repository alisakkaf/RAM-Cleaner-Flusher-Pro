#include "process_group_dialog.h"
#include "icon_provider.h"
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>

ProcessGroupInfoDialog::ProcessGroupInfoDialog(const QString &groupName, const QList<DWORD> &pids, QWidget *parent)
    : QDialog(parent)
    , m_groupName(groupName)
    , m_pids(pids)
{
    this->setWindowTitle(trXml("dlgGroupWinTitle", "Process Group Diagnostics - %1 (%2 Instances)").arg(groupName).arg(pids.size()));
    this->resize(720, 540);
    this->setWindowIcon(IconProvider::getIcon(IconProvider::ProcessesIcon));
    this->setLayoutDirection(qApp->layoutDirection());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Group Header Identity Card
    QGroupBox *groupCard = new QGroupBox(trXml("dlgGroupCardTitle", "Process Group Identity"), this);
    QHBoxLayout *headerLayout = new QHBoxLayout(groupCard);

    m_lblIcon = new QLabel(groupCard);
    m_lblIcon->setFixedSize(48, 48);
    m_lblIcon->setPixmap(IconProvider::getProcessIcon(pids.isEmpty() ? 0 : pids.first(), groupName).pixmap(48, 48));

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    m_lblTitle = new QLabel(trXml("dlgGroupMainLabel", "<b>%1</b> (Group of %2 Active Processes)").arg(groupName).arg(pids.size()), groupCard);
    m_lblTitle->setStyleSheet("font-size: 11pt; color: #38bdf8;");

    m_lblSummary = new QLabel("...", groupCard);
    m_lblSummary->setStyleSheet("color: #a3acb5; font-size: 9pt;");

    titleLayout->addWidget(m_lblTitle);
    titleLayout->addWidget(m_lblSummary);

    headerLayout->addWidget(m_lblIcon);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    mainLayout->addWidget(groupCard);

    // Child Instances Table
    QGroupBox *groupInstances = new QGroupBox(trXml("dlgGroupInstancesTitle", "Active Process Instances & Windows Services"), this);
    QVBoxLayout *tableLayout = new QVBoxLayout(groupInstances);

    m_tableInstances = new QTableWidget(groupInstances);
    m_tableInstances->setColumnCount(4);
    QStringList headers;
    headers << trXml("colPid", "PID")
            << trXml("colWinService", "Windows Service / Description")
            << trXml("colRamWorkingSet", "RAM Working Set")
            << trXml("colOpenHandles", "Open Handles");
    m_tableInstances->setHorizontalHeaderLabels(headers);
    m_tableInstances->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableInstances->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableInstances->verticalHeader()->setVisible(false);

    QHeaderView *header = m_tableInstances->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);

    m_tableInstances->setColumnWidth(0, 75);  // PID
    m_tableInstances->setColumnWidth(2, 135); // RAM Working Set
    m_tableInstances->setColumnWidth(3, 100); // Open Handles

    tableLayout->addWidget(m_tableInstances);
    mainLayout->addWidget(groupInstances);

    // Action Buttons
    QHBoxLayout *actLayout = new QHBoxLayout();

    m_btnProtect = new QPushButton(trXml("btnProtectGroup", "Protect Group"), this);
    m_btnProtect->setIcon(IconProvider::getIcon(IconProvider::ShieldIcon));
    connect(m_btnProtect, &QPushButton::clicked, this, [this]() {
        emit protectRequested(m_groupName);
        QMessageBox::information(this, trXml("btnProtectGroup", "Protect Group"), trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(m_groupName));
    });

    m_btnTrim = new QPushButton(trXml("btnTrimGroup", "Trim Group RAM"), this);
    m_btnTrim->setIcon(IconProvider::getIcon(IconProvider::DashboardIcon));
    connect(m_btnTrim, &QPushButton::clicked, this, [this]() {
        emit trimGroupRequested(m_groupName, m_pids);
        this->accept();
    });

    m_btnKill = new QPushButton(trXml("btnKillGroup", "Task Kill Group"), this);
    m_btnKill->setIcon(IconProvider::getIcon(IconProvider::TrashIcon));
    m_btnKill->setStyleSheet("QPushButton { background-color: #991b1b; color: #ffffff; border-color: #ef4444; } QPushButton:hover { background-color: #dc2626; }");
    connect(m_btnKill, &QPushButton::clicked, this, [this]() {
        emit killGroupRequested(m_groupName, m_pids);
        this->accept();
    });

    m_btnClose = new QPushButton(trXml("btnClose", "Close"), this);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    actLayout->addWidget(m_btnProtect);
    actLayout->addWidget(m_btnTrim);
    actLayout->addWidget(m_btnKill);
    actLayout->addStretch();
    actLayout->addWidget(m_btnClose);

    mainLayout->addLayout(actLayout);

    refreshGroupDiagnostics();
}

void ProcessGroupInfoDialog::refreshGroupDiagnostics() {
    m_tableInstances->setRowCount(0);

    quint64 totalGroupWs = 0;
    DWORD totalHandles = 0;

    int row = 0;
    for (DWORD pid : m_pids) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProc) {
            hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        }

        quint64 wsBytes = 0;
        DWORD handleCount = 0;

        if (hProc) {
            PROCESS_MEMORY_COUNTERS pmc = { 0 };
            if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                wsBytes = pmc.WorkingSetSize;
            }
            GetProcessHandleCount(hProc, &handleCount);
            CloseHandle(hProc);
        }

        totalGroupWs += wsBytes;
        totalHandles += handleCount;

        double wsMB = wsBytes / (1024.0 * 1024.0);

        // Win32 Service resolution for svchost.exe & system services
        QString serviceDesc = IconProvider::getProcessServiceDescription(pid);
        if (serviceDesc.isEmpty()) {
            serviceDesc = trXml("dlgStdProcessDesc", "Standard Executable Process");
        }

        m_tableInstances->insertRow(row);
        m_tableInstances->setItem(row, 0, new QTableWidgetItem(QString::number(pid)));
        
        QTableWidgetItem *itemSvc = new QTableWidgetItem(serviceDesc);
        itemSvc->setToolTip(serviceDesc);
        m_tableInstances->setItem(row, 1, itemSvc);

        m_tableInstances->setItem(row, 2, new QTableWidgetItem(QString("%1 MB").arg(wsMB, 0, 'f', 2)));
        m_tableInstances->setItem(row, 3, new QTableWidgetItem(QString::number(handleCount)));

        row++;
    }

    double totalGroupMB = totalGroupWs / (1024.0 * 1024.0);
    double avgMB = m_pids.isEmpty() ? 0.0 : (totalGroupMB / m_pids.size());

    m_lblSummary->setText(trXml("dlgGroupSummaryTmpl", "Total Combined RAM: <b>%1 MB</b> | Avg per Instance: <b>%2 MB</b> | Total Open Handles: <b>%3</b>")
                              .arg(totalGroupMB, 0, 'f', 2).arg(avgMB, 0, 'f', 2).arg(totalHandles));
}
