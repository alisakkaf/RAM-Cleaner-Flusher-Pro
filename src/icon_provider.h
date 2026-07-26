#ifndef ICON_PROVIDER_H
#define ICON_PROVIDER_H

#include "win_clean_includes.h"

#include <QIcon>
#include <QPixmap>
#include <QColor>
#include <QString>
#include <QList>

struct WinServiceInfo {
    QString serviceName; // e.g. "DnsCache"
    QString displayName; // e.g. "DNS Client"
};

class IconProvider {
public:
    enum IconType {
        RamIcon,
        DashboardIcon,
        ProcessesIcon,
        ExclusionIcon,
        BoosterIcon,
        SettingsIcon,
        AboutIcon,
        GithubIcon,
        FacebookIcon,
        WebsiteIcon,
        SunIcon,
        MoonIcon,
        RefreshIcon,
        TrashIcon,
        CheckIcon,
        ShieldIcon,
        RocketIcon,
        TerminalIcon
    };

    static QIcon getIcon(IconType type, const QColor &customColor = QColor());
    static QIcon getProcessIcon(DWORD processId, const QString &exePath = QString());
    static QString ResolveExePath(const QString &exeName);
    static QList<WinServiceInfo> getServicesForPid(DWORD pid);
    static QString getProcessServiceDescription(DWORD pid);
};

#endif // ICON_PROVIDER_H
