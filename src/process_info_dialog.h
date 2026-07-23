#ifndef PROCESS_INFO_DIALOG_H
#define PROCESS_INFO_DIALOG_H

#include "win_clean_includes.h"

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>

class ProcessInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProcessInfoDialog(DWORD pid, const QString &procName, QWidget *parent = nullptr);
    ~ProcessInfoDialog();

signals:
    void protectRequested(const QString &procName);
    void killRequested(DWORD pid, const QString &procName);
    void trimRequested(DWORD pid, const QString &procName);

private slots:
    void refreshProcessDetails();
    void onProtectClicked();
    void onKillClicked();
    void onTrimClicked();

private:
    DWORD m_pid;
    QString m_procName;
    QTimer *m_refreshTimer;

    QLabel *m_lblIcon;
    QLabel *m_lblTitle;
    QLabel *m_lblPid;
    QLabel *m_lblPath;
    QLabel *m_lblWorkingSet;
    QLabel *m_lblPeakWorkingSet;
    QLabel *m_lblPagefile;
    QLabel *m_lblPriority;
    QLabel *m_lblHandles;
    QLabel *m_lblStartTime;

    QPushButton *m_btnProtect;
    QPushButton *m_btnKill;
    QPushButton *m_btnTrim;
    QPushButton *m_btnClose;

    void setupUI();
};

#endif // PROCESS_INFO_DIALOG_H
