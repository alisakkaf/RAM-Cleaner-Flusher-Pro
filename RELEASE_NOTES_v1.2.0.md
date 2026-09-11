# RAM Cleaner & Flusher Pro Suite v1.2.0 Release Notes

We are thrilled to announce **RAM Cleaner & Flusher Pro Suite v1.2.0** — featuring a new **Developer Mode** toggle, **10-Second Settling & Adaptive Memory Monitoring**, **Clean Extension-less Process Tree**, **Priority List Management**, **Bulletproof BiDi RTL/LTR Formatting**, and **Startup Cleanup**!

---

## 🌟 Key Features & Improvements in Version 1.2.0

### 🛠️ 1. Developer Mode (وضع المطور) & Dual-View Logging
- **Toggleable Developer Mode:** Added a `chkDeveloperMode` checkbox in the Settings tab to control log verbosity in real time without restarting the application.
- **Regular User Mode (Default):** Displays a compact, beautifully styled HTML card summary featuring reclaimed RAM, percentage load drop, and trimmed process counts with clean emojis.
- **Developer Mode (ON):** Outputs an exhaustive Win32 kernel execution report including initial/settled RAM states, Standby Cache lists, and trimmed/protected/skipped process diagnostics.

### ⏱️ 2. 10-Second Settling Observation & Live Progress Log
- **Kernel Memory Settling Phase:** After executing memory flush commands, the optimizer enters an asynchronous 10-second observation phase.
- **Live Progress Dots:** Appends live dots (`. . . . . . . . . .`) every second to the log waiting line (`يرجى الانتظار حتى استقرار الذاكرة`).
- **Minimum RAM Load Tracking:** Continuously samples memory metrics every second for 10 seconds and uses the **true lowest RAM load** observed as the final settled memory result.

### 🌳 3. Clean Process Tree & Priority List Rebranding
- **Extension-Less Process Display:** Strips `.exe` extensions from process names in the tree view, context menus, and logs for a clean, modern look (e.g. `chrome`, `Explorer`, `svchost`).
- **Priority List Management:** Rebranded "Protect" to "Priority" (قائمة الأولويات / Priority List) across English, Arabic, and Spanish interfaces.
- **Dynamic Context Actions:** Context menus dynamically switch between *Set Priority* and *Remove Priority* based on existing exclusion rules.
- **Single Confirmation Popup:** Adding a process to the Priority list prompts a single confirmation dialog (`Confirm Set Priority`) and updates the list and log without extra popups.
- **Process Search Box Fix:** Clearing the search input cleanly collapses process groups without leaving all top-level items expanded.

### 🌐 4. Perfect BiDi RTL/LTR Formatting & Typography
- **Unit Flipping Fix:** All numeric values, RAM units (`2.3 GB`, `236.1 MB`), and percentage transitions (`55% ➔ 56% (-1%)`) are wrapped in explicit `<span dir='ltr' style='unicode-bidi: embed;'>` elements to prevent unit flipping in Arabic RTL mode.
- **Clean Number Formatting:** Formats RAM values intelligently (e.g., `0 MB`, `12.3 MB`, `1.5 GB`) with rounded decimals.
- **Document Text Option Alignment:** `txtLog` alignment adapts dynamically to the active language direction (Right-Aligned for Arabic, Left-Aligned for English and Spanish) with zero trailing empty lines.

### 🧹 5. Startup File Cleanup & Win32 Shell COM Extraction
- **Startup `.old` Cleanup:** `main.cpp` automatically cleans up leftover `.old` executable backup files from previous auto-updates upon application launch.
- **Native Win32 COM Unzipping:** `InstallerManager` uses the native COM `IShellDispatch` interface for ZIP extraction and native `QFile::setPermissions()` for permission handling, ensuring safe locked-binary replacement.

### 🛡️ 6. Antivirus False Positive Elimination & PE Security Hardening
- **Manifest Table Correction (BitDefender Fix):** Removed redundant `RT_MANIFEST` resource embedding in `app.rc` and unified manifest handling under `qmake` (`MANIFEST += app.manifest`), completely eliminating the duplicate resource header pattern that triggered `Gen:Variant.Babar` in BitDefender and partner engines.
- **Native Win32 Process Termination (Defender Fix):** Replaced external `taskkill` CLI execution with native Win32 toolhelp API (`CreateToolhelp32Snapshot`, `Process32FirstW`, `TerminateProcess`), eliminating command-line process killer heuristic triggers (`Trojan:Win32/Wacatac.B!ml`, `MalwareX-gen`).
- **Native Qt Permission Management:** Removed external `icacls` command invocations in favor of native Qt `QFile::setPermissions()`, eliminating suspicious command strings.
- **Clean Standard Process Launching:** Removed `CREATE_NEW_CONSOLE` flag in `AppBooster` to launch accelerated applications cleanly under standard priority classes.
- **Binary Security Hardening (ASLR & DEP):** Enabled Windows standard security mitigations (`-Wl,--dynamicbase` for ASLR and `-Wl,--nxcompat` for DEP) and normalized resource entropy by removing aggressive compression flags.

---

## 🖥️ System Requirements & OS Support

- **Windows 11:** All editions (21H2 through 26H1 Canary/Enterprise/Pro/Home)
- **Windows 10:** All editions (1507 through 22H2 LTSC/Enterprise/Pro/Home)
- **Windows 8.1 / 8 / 7 (SP1):** All editions (32-bit & 64-bit)
- **Windows Server:** 2025, 2022, 2019, 2016, 2012 R2, 2012, 2008 R2 (SP1)

---

## 📥 Download Assets

Official Release Archive:
- **`RAM_Cleaner_Flusher_Pro_v1.2.0.zip`** (Executable with Portable & Self-Installing support)
