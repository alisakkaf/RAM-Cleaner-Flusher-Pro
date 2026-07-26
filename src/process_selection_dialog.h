#ifndef PROCESS_SELECTION_DIALOG_H
#define PROCESS_SELECTION_DIALOG_H

#include "win_clean_includes.h"
#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>

class ProcessSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProcessSelectionDialog(QWidget *parent = nullptr);
    QString selectedProcessName() const;

private slots:
    void refreshProcessList();
    void filterProcesses(const QString &text);
    void onItemSelected();

private:
    QLineEdit *m_txtSearch;
    QTableWidget *m_tableProcesses;
    QPushButton *m_btnSelect;
    QPushButton *m_btnCancel;
    QString m_selectedProcName;
};

#endif // PROCESS_SELECTION_DIALOG_H
