# 📚 RAM Cleaner & Flusher Pro Suite - Official Technical Wiki & Operating Guide

Welcome to the official technical documentation and operating manual for **RAM Cleaner & Flusher Pro Suite** (v1.1.0 Release).

---

## 📌 Table of Contents
1. [Architecture Overview](#1-architecture-overview)
2. [Kernel Memory Optimization Mechanics](#2-kernel-memory-optimization-mechanics)
3. [Installation & Deployments](#3-installation--deployments)
4. [Comprehensive Tab-by-Tab User Manual](#4-comprehensive-tab-by-tab-user-manual)
5. [App Launch Game & Software Booster](#5-app-launch-game--software-booster)
6. [Svchost Windows Service Resolver](#6-svchost-windows-service-resolver)
7. [Exclusion Protection Engine](#7-exclusion-protection-engine)
8. [Automation & Background Rules](#8-automation--background-rules)
9. [Troubleshooting & Diagnostics](#9-troubleshooting--diagnostics)
10. [Frequently Asked Questions (FAQ)](#10-frequently-asked-questions-faq)

---

## 1. Architecture Overview

**RAM Cleaner & Flusher Pro Suite** is engineered strictly in native C++14 using direct Win32 Kernel APIs (`NtSetSystemInformation`, `EmptyWorkingSet`, `GlobalMemoryStatusEx`, `EnumServicesStatusExW`).

Unlike managed tools or PowerShell scripts that consume 80-150 MB RAM and cause CPU spikes, RAM Cleaner Pro operates with:
- **Active Memory Footprint:** ~14 MB RAM
- **Idle CPU Load:** 0.00%
- **Runtime Dependencies:** Zero (Standalone Win32 Binary)

---

## 2. Kernel Memory Optimization Mechanics

### 2.1 Standby Cache Memory Purge (`MemoryPurgeStandbyList`)
- **Syscall:** `NtSetSystemInformation` dynamically bound from `ntdll.dll`.
- **Information Class:** `SystemMemoryListInformation` (`0x50`).
- **Command Value:** `1` (`MemoryPurgeStandbyList`).
- **Function:** Purges Priority 0 through 7 Standby Memory pages instantly, converting cached standby RAM into free physical RAM for allocation.

### 2.2 Modified Page List Flush (`MemoryFlushModifiedList`)
- **Command Value:** `3` (`MemoryFlushModifiedList`).
- **Function:** Forces dirty modified pages to write back to disk or pagefile, reducing system commit charge.

### 2.3 Working Set Trimming (`EmptyWorkingSet`)
- **Win32 API:** `EmptyWorkingSet()` and `SetProcessWorkingSetSize(-1, -1)` via `psapi.dll`.
- **Function:** Instructs the Windows Memory Manager to trim idle page frames back to pagefile or free lists without terminating applications, achieving **40% to 60% physical RAM reclamation**.

---

## 3. Installation & Deployments

### Mode A: Standard Self-Installer via ZIP Archive (Recommended)
1. Download `RAM_Cleaner_Flusher_Pro_v1.1.0.zip` from [GitHub Releases](https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro/releases/latest).
2. Extract the `.zip` archive to any folder.
3. Run `RAM_Cleaner_Pro.exe`.
4. The app automatically deploys to `%ProgramFiles%\RAM Cleaner & Flusher Pro\RAM_Cleaner_Pro.exe`, generates a Win32 COM Desktop Shortcut, sets ACL permissions (`icacls`), and launches.

### Mode B: Portable Execution
Run from command prompt:
```cmd
RAM_Cleaner_Pro.exe --portable
```

---

## 4. Comprehensive Tab-by-Tab User Manual

### 4.1 Dashboard Tab
- **RAM Utilization Bar:** Linear progress bar with multi-stop color transitions (`#10b981` Green, `#f59e0b` Amber, `#ef4444` Red).
- **Physical RAM Cards:** Displays Usage %, Used RAM (GB), Free RAM (GB), Total RAM (GB), and Standby Cache (GB).
- **Manual Buttons:** `Purge Standby Cache`, `Trim Working Sets`, `Flush Modified Pages`, `Maximum Optimization`.
- **Activity Log Panel:** HTML formatted event log with `Export Log` and `Clear Log` buttons.

### 4.2 Running Processes Tab
- **Search Filter:** Real-time filtering by process name or PID.
- **Grouped Process Tree:** Hierarchical tree grouping multi-instance processes (e.g., `chrome.exe (27 processes)`).
- **`Inspect` Button:** Opens `ProcessInfoDialog` showing PID, RAM MB, handles, path, open location, single trim, and taskkill.
- **`Group Diagnostics` Button:** Opens `ProcessGroupInfoDialog` breaking down child PIDs, resolved Windows Service names, average RAM, group protection, group trim, and group kill.

---

## 5. App Launch Game & Software Booster

1. Select target `.exe` via file browser or active processes.
2. Specify optional launch arguments (e.g., `-high -novid`).
3. Set execution priority (`HIGH_PRIORITY_CLASS`, `REALTIME_PRIORITY_CLASS`).
4. Enable `Purge RAM Before Launch` for maximum pre-launch memory cleanup.
5. Click `Launch & Boost Application`.

---

## 6. Svchost Windows Service Resolver

- Queries `EnumServicesStatusExW` with `SC_MANAGER_ENUMERATE_SERVICE`.
- Maps generic `svchost.exe` process IDs to human-readable Windows Service names:
  - `svchost.exe [DNS Client - DnsCache]`
  - `svchost.exe [Windows Audio - AudioSrv]`
  - `svchost.exe [Windows Defender Service - MsMpEng]`

---

## 7. Exclusion Protection Engine

- Add critical applications to exclusion protection list to prevent working set trimming.
- Add by file browser or select running processes from `ProcessSelectionDialog`.
- Ideal for EasyAntiCheat, BattEye, Vanguard, Visual Studio, Docker, and database servers.

---

## 8. Automation & Background Rules

- **Auto Threshold Cleaning:** Triggers optimization when physical RAM load exceeds limit (50-98%, default 80%).
- **Periodic Timer Scheduler:** Automated background timer (5 to 1440 minutes).
- **Windows Autostart:** Registry key `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\RAMCleanerPro`.
- **System Tray Mode:** Minimize to tray on close and start hidden in system tray on boot.

---

## 9. Troubleshooting & Diagnostics

- **Permission Error (`Access Denied`):** Ensure RAM Cleaner Pro is launched as Administrator (`SE_INCREASE_QUOTA_NAME` privilege).
- **Network Update Failure:** Verify outbound HTTPS access to Gist update manifests over TLS 1.3.

---

## 10. Frequently Asked Questions (FAQ)

**Q: Is RAM Cleaner Pro safe with anti-cheat software in games?**  
*A: Yes! RAM Cleaner Pro operates strictly within official Win32 Kernel API boundaries and does not modify process memory spaces or inject DLLs.*

**Q: Does trimming working sets close my applications?**  
*A: No. It simply instructs the Windows Memory Manager to move unallocated idle pages to pagefile or free lists.*

---

*Documentation maintained by [AliSakkaf](https://www.facebook.com/AliSakkaf.Dev/).*
