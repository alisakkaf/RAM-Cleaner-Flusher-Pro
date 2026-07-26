#include "icon_provider.h"
#include "qapplication.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

typedef BOOL(WINAPI *pfnQueryFullProcessImageNameW)(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD pdwSize);

static QPixmap HIconToQPixmap(HICON hIcon) {
    if (!hIcon) return QPixmap();

    ICONINFO iconInfo = { 0 };
    if (!GetIconInfo(hIcon, &iconInfo)) {
        return QPixmap();
    }

    BITMAP bmp = { 0 };
    GetObject(iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    if (!iconInfo.hbmColor) height /= 2;

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, iconInfo.hbmColor);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    GetDIBits(hdcScreen, iconInfo.hbmColor, 0, height, image.bits(), &bmi, DIB_RGB_COLORS);

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

    return QPixmap::fromImage(image);
}

QString IconProvider::ResolveExePath(const QString &exeName) {
    if (exeName.isEmpty()) return QString();
    if (QFileInfo::exists(exeName) && QFileInfo(exeName).isFile()) return exeName;

    QString cleanName = exeName;
    if (!cleanName.endsWith(".exe", Qt::CaseInsensitive)) {
        cleanName += ".exe";
    }

    // 1. Search active processes snapshot
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32 = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnap, &pe32)) {
            do {
                QString name = QString::fromWCharArray(pe32.szExeFile);
                if (name.compare(cleanName, Qt::CaseInsensitive) == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                    if (hProc) {
                        wchar_t buf[MAX_PATH] = { 0 };
                        DWORD sz = MAX_PATH;
                        if (GetModuleFileNameExW(hProc, NULL, buf, MAX_PATH) > 0) {
                            CloseHandle(hProc);
                            CloseHandle(hSnap);
                            return QString::fromWCharArray(buf);
                        }
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(hSnap, &pe32));
        }
        CloseHandle(hSnap);
    }

    // 2. Query Windows Registry App Paths
    HKEY hKey;
    std::wstring regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" + cleanName.toStdWString();
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t pathBuf[MAX_PATH] = { 0 };
        DWORD bufSize = sizeof(pathBuf);
        if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)pathBuf, &bufSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            QString resolved = QString::fromWCharArray(pathBuf).trimmed();
            if (resolved.startsWith("\"") && resolved.endsWith("\"")) {
                resolved = resolved.mid(1, resolved.length() - 2);
            }
            if (QFileInfo::exists(resolved)) return resolved;
        }
        RegCloseKey(hKey);
    }

    // 3. Fallback system directory check
    QString sys32Path = "C:\\Windows\\System32\\" + cleanName;
    if (QFileInfo::exists(sys32Path)) return sys32Path;

    return QString();
}

QList<WinServiceInfo> IconProvider::getServicesForPid(DWORD pid) {
    QList<WinServiceInfo> list;
    if (pid == 0 || pid == 4) return list;

    SC_HANDLE hSCM = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
    if (!hSCM) return list;

    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;

    EnumServicesStatusExW(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                          NULL, 0, &bytesNeeded, &servicesReturned, NULL, NULL);

    if (bytesNeeded > 0) {
        BYTE *buffer = new BYTE[bytesNeeded];
        DWORD resumeHandle = 0;
        if (EnumServicesStatusExW(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                                  buffer, bytesNeeded, &bytesNeeded, &servicesReturned, &resumeHandle, NULL)) {
            ENUM_SERVICE_STATUS_PROCESSW *services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer);
            for (DWORD i = 0; i < servicesReturned; ++i) {
                if (services[i].ServiceStatusProcess.dwProcessId == pid) {
                    WinServiceInfo info;
                    info.serviceName = QString::fromWCharArray(services[i].lpServiceName);
                    info.displayName = QString::fromWCharArray(services[i].lpDisplayName);
                    list.append(info);
                }
            }
        }
        delete[] buffer;
    }

    CloseServiceHandle(hSCM);
    return list;
}

QString IconProvider::getProcessServiceDescription(DWORD pid) {
    QList<WinServiceInfo> services = getServicesForPid(pid);
    if (services.isEmpty()) return QString();

    if (services.size() == 1) {
        return QString("%1 (%2)").arg(services.first().displayName).arg(services.first().serviceName);
    }

    QStringList names;
    for (const auto &s : services) {
        names.append(s.displayName.isEmpty() ? s.serviceName : s.displayName);
    }
    return names.join(", ");
}

QIcon IconProvider::getProcessIcon(DWORD processId, const QString &exePath) {
    HICON hIcon = NULL;

    QString resolvedPath = exePath;
    if (resolvedPath.isEmpty() || !QFileInfo::exists(resolvedPath)) {
        resolvedPath = ResolveExePath(exePath);
    }

    if (!resolvedPath.isEmpty()) {
        SHFILEINFOW sfi = { 0 };
        std::wstring wPath = resolvedPath.toStdWString();
        if (SHGetFileInfoW(wPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
            hIcon = sfi.hIcon;
        }
    }

    if (!hIcon && processId > 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (!hProcess) {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        }

        if (hProcess) {
            wchar_t pathBuf[MAX_PATH] = { 0 };
            DWORD size = MAX_PATH;
            BOOL gotPath = FALSE;

            HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
            if (hKernel32) {
                pfnQueryFullProcessImageNameW pQuery = (pfnQueryFullProcessImageNameW)GetProcAddress(hKernel32, "QueryFullProcessImageNameW");
                if (pQuery) {
                    gotPath = pQuery(hProcess, 0, pathBuf, &size);
                }
            }

            if (!gotPath) {
                if (GetModuleFileNameExW(hProcess, NULL, pathBuf, MAX_PATH) > 0) {
                    gotPath = TRUE;
                }
            }

            if (gotPath) {
                SHFILEINFOW sfi = { 0 };
                if (SHGetFileInfoW(pathBuf, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
                    hIcon = sfi.hIcon;
                }
            }
            CloseHandle(hProcess);
        }
    }

    if (hIcon) {
        QPixmap pix = HIconToQPixmap(hIcon);
        DestroyIcon(hIcon);
        if (!pix.isNull()) return QIcon(pix);
    }

    // Generic Executable Icon fallback
    SHFILEINFOW sfiGen = { 0 };
    if (SHGetFileInfoW(L"cmd.exe", FILE_ATTRIBUTE_NORMAL, &sfiGen, sizeof(sfiGen), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
        hIcon = sfiGen.hIcon;
        if (hIcon) {
            QPixmap pix = HIconToQPixmap(hIcon);
            DestroyIcon(hIcon);
            if (!pix.isNull()) return QIcon(pix);
        }
    }

    return getIcon(ProcessesIcon);
}

QIcon IconProvider::getIcon(IconType type, const QColor &customColor) {
    if (type == RamIcon) {
        QIcon appIcon(":/app_icon.png");
        if (!appIcon.isNull()) return appIcon;
        QString diskPng = QDir(QCoreApplication::applicationDirPath()).filePath("app_icon.png");
        if (QFileInfo::exists(diskPng)) return QIcon(diskPng);
    } else if (type == GithubIcon) {
        QIcon icon(":/github_icon.png");
        if (!icon.isNull()) return icon;
        QString diskPng = QDir(QCoreApplication::applicationDirPath()).filePath("github_icon.png");
        if (QFileInfo::exists(diskPng)) return QIcon(diskPng);
    } else if (type == FacebookIcon) {
        QIcon icon(":/facebook_icon.png");
        if (!icon.isNull()) return icon;
        QString diskPng = QDir(QCoreApplication::applicationDirPath()).filePath("facebook_icon.png");
        if (QFileInfo::exists(diskPng)) return QIcon(diskPng);
    } else if (type == WebsiteIcon) {
        QIcon icon(":/website_icon.png");
        if (!icon.isNull()) return icon;
        QString diskPng = QDir(QCoreApplication::applicationDirPath()).filePath("website_icon.png");
        if (QFileInfo::exists(diskPng)) return QIcon(diskPng);
    }

    int size = 128;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor mainCol = customColor.isValid() ? customColor : QColor(6, 182, 212); // Emerald Cyan default

    p.setRenderHint(QPainter::SmoothPixmapTransform,true);

    switch (type) {
    case DashboardIcon: {
        p.setPen(QPen(mainCol, 6, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(20, 20, 88, 88, -35 * 16, 250 * 16);

        for (int i = 0; i < 5; ++i) {
            p.save();
            p.translate(64, 64);
            p.rotate(-120 + i * 60);
            p.setPen(QPen(mainCol, 3, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(0, -34, 0, -28);
            p.restore();
        }

        QLinearGradient needleGrad(64, 64, 94, 34);
        needleGrad.setColorAt(0.0, QColor(245, 158, 11));
        needleGrad.setColorAt(1.0, QColor(239, 68, 68));

        p.setPen(QPen(QBrush(needleGrad), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(64, 64, 94, 34);

        p.setBrush(QColor(245, 158, 11));
        p.setPen(QPen(mainCol, 2));
        p.drawEllipse(56, 56, 16, 16);
        break;
    }

    case ProcessesIcon: {
        QLinearGradient bgGrad(20, 20, 108, 108);
        bgGrad.setColorAt(0.0, QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 35));
        bgGrad.setColorAt(1.0, QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 10));

        p.setBrush(bgGrad);
        p.setPen(QPen(mainCol, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawRoundedRect(18, 18, 92, 92, 14, 14);

        p.drawLine(18, 48, 110, 48);

        p.setBrush(mainCol);
        p.setPen(Qt::NoPen);
        p.drawEllipse(32, 32, 8, 8);
        p.drawEllipse(46, 32, 8, 8);
        p.drawEllipse(60, 32, 8, 8);

        p.setPen(QPen(mainCol, 4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(34, 68, 92, 68);
        p.drawLine(34, 84, 72, 84);
        break;
    }

    case ExclusionIcon:
    case ShieldIcon: {
        QPainterPath path;
        path.moveTo(64, 16);
        path.quadTo(106, 24, 106, 58);
        path.quadTo(106, 100, 64, 114);
        path.quadTo(22, 100, 22, 58);
        path.quadTo(22, 24, 64, 16);

        QLinearGradient shieldGrad(64, 16, 64, 114);
        shieldGrad.setColorAt(0.0, QColor(16, 185, 129, 65));
        shieldGrad.setColorAt(1.0, QColor(16, 185, 129, 15));

        p.setBrush(shieldGrad);
        p.setPen(QPen(QColor(16, 185, 129), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);

        p.setPen(QPen(QColor(16, 185, 129), 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(46, 64, 58, 76);
        p.drawLine(58, 76, 82, 50);
        break;
    }

    case BoosterIcon:
    case RocketIcon: {
        QPainterPath rocket;
        rocket.moveTo(64, 14);
        rocket.quadTo(94, 32, 86, 78);
        rocket.lineTo(72, 90);
        rocket.lineTo(56, 90);
        rocket.lineTo(42, 78);
        rocket.quadTo(34, 32, 64, 14);

        QLinearGradient rocketGrad(64, 14, 64, 90);
        rocketGrad.setColorAt(0.0, QColor(245, 158, 11, 70));
        rocketGrad.setColorAt(1.0, QColor(245, 158, 11, 20));

        p.setBrush(rocketGrad);
        p.setPen(QPen(QColor(245, 158, 11), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(rocket);

        QPainterPath leftFin, rightFin;
        leftFin.moveTo(42, 72);
        leftFin.lineTo(24, 92);
        leftFin.lineTo(46, 92);

        rightFin.moveTo(86, 72);
        rightFin.lineTo(104, 92);
        rightFin.lineTo(82, 92);

        p.setBrush(QColor(245, 158, 11));
        p.drawPath(leftFin);
        p.drawPath(rightFin);

        p.setBrush(QColor(255, 255, 255));
        p.setPen(QPen(QColor(245, 158, 11), 3));
        p.drawEllipse(57, 40, 14, 14);

        QPainterPath flame;
        flame.moveTo(52, 94);
        flame.quadTo(64, 118, 64, 118);
        flame.quadTo(64, 118, 76, 94);

        QLinearGradient flameGrad(64, 94, 64, 118);
        flameGrad.setColorAt(0.0, QColor(239, 68, 68));
        flameGrad.setColorAt(1.0, QColor(245, 158, 11));

        p.setBrush(flameGrad);
        p.setPen(QPen(QColor(239, 68, 68), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(flame);
        break;
    }

    case SettingsIcon: {
        p.setPen(QPen(mainCol, 5, Qt::SolidLine, Qt::RoundCap));

        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(64, 64);
            p.rotate(i * 45);
            p.drawLine(0, -28, 0, -42);
            p.restore();
        }

        p.setBrush(QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 30));
        p.drawEllipse(36, 36, 56, 56);
        p.drawEllipse(52, 52, 24, 24);
        break;
    }

    case AboutIcon: {
        QLinearGradient aboutGrad(20, 20, 108, 108);
        aboutGrad.setColorAt(0.0, QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 40));
        aboutGrad.setColorAt(1.0, QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 10));

        p.setBrush(aboutGrad);
        p.setPen(QPen(mainCol, 5, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(18, 18, 92, 92);

        p.setBrush(mainCol);
        p.setPen(Qt::NoPen);
        p.drawEllipse(60, 38, 8, 8);

        p.setPen(QPen(mainCol, 7, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(64, 56, 64, 88);
        break;
    }

    case SunIcon: {
        p.setPen(QPen(QColor(245, 158, 11), 5, Qt::SolidLine, Qt::RoundCap));

        QLinearGradient sunGrad(42, 42, 86, 86);
        sunGrad.setColorAt(0.0, QColor(245, 158, 11, 90));
        sunGrad.setColorAt(1.0, QColor(245, 158, 11, 30));

        p.setBrush(sunGrad);
        p.drawEllipse(40, 40, 48, 48);

        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(64, 64);
            p.rotate(i * 45);
            p.drawLine(0, -32, 0, -45);
            p.restore();
        }
        break;
    }

    case MoonIcon: {
        QPainterPath moon;
        moon.addEllipse(22, 22, 84, 84);

        QPainterPath cut;
        cut.addEllipse(44, 14, 76, 76);

        moon = moon.subtracted(cut);

        QLinearGradient moonGrad(22, 22, 106, 106);
        moonGrad.setColorAt(0.0, QColor(56, 189, 248, 80));
        moonGrad.setColorAt(1.0, QColor(56, 189, 248, 20));

        p.setBrush(moonGrad);
        p.setPen(QPen(QColor(56, 189, 248), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(moon);
        break;
    }

    case RefreshIcon: {
        p.setPen(QPen(mainCol, 6, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(22, 22, 84, 84, 30 * 16, 290 * 16);

        QPainterPath arrow;
        arrow.moveTo(76, 14);
        arrow.lineTo(96, 32);
        arrow.lineTo(72, 42);

        p.setBrush(mainCol);
        p.setPen(QPen(mainCol, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(arrow);
        break;
    }

    case TrashIcon: {
        QLinearGradient trashGrad(40, 46, 88, 104);
        trashGrad.setColorAt(0.0, QColor(239, 68, 68, 45));
        trashGrad.setColorAt(1.0, QColor(239, 68, 68, 10));

        p.setBrush(trashGrad);
        p.setPen(QPen(QColor(239, 68, 68), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        p.drawRoundedRect(38, 46, 52, 60, 8, 8);
        p.drawLine(24, 38, 104, 38);

        QPainterPath handle;
        handle.moveTo(50, 38);
        handle.lineTo(50, 26);
        handle.lineTo(78, 26);
        handle.lineTo(78, 38);
        p.drawPath(handle);

        p.drawLine(52, 60, 52, 90);
        p.drawLine(64, 60, 64, 90);
        p.drawLine(76, 60, 76, 90);
        break;
    }

    default: {
        p.setPen(QPen(mainCol, 5, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(32, 32, 64, 64);
        break;
    }
    }

    return QIcon(pix);
}
