#include "process_selection_dialog.h"
#include "icon_provider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

class NumericTableWidgetItem : public QTableWidgetItem {
public:
    NumericTableWidgetItem(const QString &text, double val)
        : QTableWidgetItem(text), m_val(val) {}

    bool operator<(const QTableWidgetItem &other) const override {
        const NumericTableWidgetItem *otherNum = dynamic_cast<const NumericTableWidgetItem*>(&other);
        if (otherNum) {
            return m_val < otherNum->m_val;
        }
        return QTableWidgetItem::operator<(other);
    }
private:
    double m_val;
};

ProcessSelectionDialog::ProcessSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("Select Running Process to Protect");
    this->resize(580, 520);
    this->setWindowIcon(IconProvider::getIcon(IconProvider::ShieldIcon));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Search bar layout
    QHBoxLayout *searchLayout = new QHBoxLayout();
    m_txtSearch = new QLineEdit(this);
    m_txtSearch->setPlaceholderText("Search running process by name or PID...");
    connect(m_txtSearch, &QLineEdit::textChanged, this, &ProcessSelectionDialog::filterProcesses);
    searchLayout->addWidget(m_txtSearch);

    QPushButton *btnRefresh = new QPushButton("Refresh", this);
    btnRefresh->setIcon(IconProvider::getIcon(IconProvider::RefreshIcon));
    connect(btnRefresh, &QPushButton::clicked, this, &ProcessSelectionDialog::refreshProcessList);
    searchLayout->addWidget(btnRefresh);

    mainLayout->addLayout(searchLayout);

    // Process Table
    m_tableProcesses = new QTableWidget(this);
    m_tableProcesses->setColumnCount(4);
    QStringList headers;
    headers << "Icon" << "Process Name" << "PID" << "RAM Working Set";
    m_tableProcesses->setHorizontalHeaderLabels(headers);
    m_tableProcesses->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableProcesses->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableProcesses->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableProcesses->setIconSize(QSize(24, 24));
    m_tableProcesses->verticalHeader()->setVisible(false);

    QHeaderView *header = m_tableProcesses->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);

    m_tableProcesses->setColumnWidth(0, 50);
    m_tableProcesses->setColumnWidth(2, 90);
    m_tableProcesses->setColumnWidth(3, 140);

    connect(m_tableProcesses, &QTableWidget::itemDoubleClicked, this, &ProcessSelectionDialog::onItemSelected);

    mainLayout->addWidget(m_tableProcesses);

    // Action buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnSelect = new QPushButton("Protect Selected Process", this);
    m_btnSelect->setIcon(IconProvider::getIcon(IconProvider::ShieldIcon));
    m_btnSelect->setStyleSheet("QPushButton { font-weight: bold; padding: 6px 16px; }");
    connect(m_btnSelect, &QPushButton::clicked, this, &ProcessSelectionDialog::onItemSelected);

    m_btnCancel = new QPushButton("Cancel", this);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_btnSelect);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);

    refreshProcessList();
}

QString ProcessSelectionDialog::selectedProcessName() const {
    return m_selectedProcName;
}

void ProcessSelectionDialog::refreshProcessList() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return;
    }

    m_tableProcesses->setSortingEnabled(false);
    m_tableProcesses->setRowCount(0);

    int row = 0;
    do {
        if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4) continue;

        QString procName = QString::fromWCharArray(pe32.szExeFile);
        double pidVal = static_cast<double>(pe32.th32ProcessID);

        quint64 wsBytes = 0;
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
        if (hProc) {
            PROCESS_MEMORY_COUNTERS pmc = { 0 };
            if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                wsBytes = pmc.WorkingSetSize;
            }
            CloseHandle(hProc);
        }

        double wsMB = wsBytes / (1024.0 * 1024.0);

        m_tableProcesses->insertRow(row);

        QTableWidgetItem *itemIcon = new QTableWidgetItem();
        itemIcon->setIcon(IconProvider::getProcessIcon(pe32.th32ProcessID, procName));
        itemIcon->setTextAlignment(Qt::AlignCenter);
        m_tableProcesses->setItem(row, 0, itemIcon);

        QTableWidgetItem *itemName = new QTableWidgetItem(procName);
        m_tableProcesses->setItem(row, 1, itemName);

        NumericTableWidgetItem *itemPid = new NumericTableWidgetItem(QString::number(pe32.th32ProcessID), pidVal);
        m_tableProcesses->setItem(row, 2, itemPid);

        NumericTableWidgetItem *itemRam = new NumericTableWidgetItem(QString("%1 MB").arg(wsMB, 0, 'f', 2), wsMB);
        m_tableProcesses->setItem(row, 3, itemRam);

        row++;
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    m_tableProcesses->setSortingEnabled(true);
    m_tableProcesses->sortItems(3, Qt::DescendingOrder);

    filterProcesses(m_txtSearch->text());
}

void ProcessSelectionDialog::filterProcesses(const QString &text) {
    int rowCount = m_tableProcesses->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        QTableWidgetItem *nameItem = m_tableProcesses->item(i, 1);
        QTableWidgetItem *pidItem = m_tableProcesses->item(i, 2);
        if (!nameItem || !pidItem) continue;

        bool match = text.isEmpty() || 
                     nameItem->text().contains(text, Qt::CaseInsensitive) || 
                     pidItem->text().contains(text);

        m_tableProcesses->setRowHidden(i, !match);
    }
}

void ProcessSelectionDialog::onItemSelected() {
    int row = m_tableProcesses->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a running process from the list.");
        return;
    }

    QTableWidgetItem *nameItem = m_tableProcesses->item(row, 1);
    if (nameItem) {
        m_selectedProcName = nameItem->text();
        this->accept();
    }
}
