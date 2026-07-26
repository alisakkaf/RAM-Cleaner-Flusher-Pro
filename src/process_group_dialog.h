#ifndef PROCESS_GROUP_DIALOG_H
#define PROCESS_GROUP_DIALOG_H

#include "win_clean_includes.h"
#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>

class ProcessGroupInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProcessGroupInfoDialog(const QString &groupName, const QList<DWORD> &pids, QWidget *parent = nullptr);

signals:
    void protectRequested(const QString &procName);
    void killGroupRequested(const QString &procName, const QList<DWORD> &pids);
    void trimGroupRequested(const QString &procName, const QList<DWORD> &pids);

private slots:
    void refreshGroupDiagnostics();

private:
    QString m_groupName;
    QList<DWORD> m_pids;

    QLabel *m_lblIcon;
    QLabel *m_lblTitle;
    QLabel *m_lblSummary;

    QTableWidget *m_tableInstances;

    QPushButton *m_btnProtect;
    QPushButton *m_btnTrim;
    QPushButton *m_btnKill;
    QPushButton *m_btnClose;
};

#endif // PROCESS_GROUP_DIALOG_H
