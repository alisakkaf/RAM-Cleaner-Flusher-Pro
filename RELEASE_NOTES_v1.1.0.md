# RAM Cleaner & Flusher Pro Suite v1.1.0 Feature Release

We are excited to announce the release of **RAM Cleaner & Flusher Pro Suite v1.1.0** — featuring a complete multi-language XML localization engine, real-time font and typography customization, instant RTL alignment for Arabic and right-to-left languages, interactive process group context menus, and enhanced Win32 Kernel memory trimming stability!

---

## 🌟 What's New in Version 1.1.0

### 🌐 1. Dynamic Multi-Language XML Translation Engine (`trXml`)
- **Full XML-Driven Localization:** The entire application interface, popups, inspector dialogs, system tray menus, activity logs, and HTML memory reports are now driven by localized XML files (`languages.xml`, `languages_ar.xml`, `languages_es.xml`).
- **Complete RTL Support (Arabic):** Automatically applies right-to-left layout direction (`Qt::RightToLeft`) for Arabic, aligning all spinboxes, comboboxes, tree headers, and context menus.
- **Open-Source Localization System:** Added a comprehensive translator guide (`language_xml/README.md`) allowing community members to contribute new translations (French, German, Russian, Japanese, etc.) by creating custom ISO 639-1 XML files without modifying C++ code.

### 🎨 2. Real-Time Font & Typography Customization
- **System Font Scanner:** Integrated a non-editable, scalable vector font selector (`comboFontFamily`) that filters out non-scalable legacy bitmap fonts to prevent OpenType console warnings.
- **Instant UI Typography Override:** Font family and font size changes apply instantly across all tabs, buttons, dialogs, and popups in real-time using dynamic CSS injection (`!important`) without requiring an application restart.

### 🔍 3. Interactive Context Menus & Multi-Instance Process Group Diagnostics
- **Context Menu Options (`⋮`):** Right-clicking or clicking the action menu on any process or group provides options for:
  - **Inspect Group Diagnostics / Single Process Inspector**
  - **Protect (Add to Exclusion List)**
  - **Task Kill (Terminate All Instances / Single Process)**
  - **Open File Location in Windows Explorer**
- **Process Selection Dialog (`ProcessSelectionDialog`):** Allows users to search active processes by name or PID and add them directly to the Protection Exclusion list.

### ⚡ 4. Enhanced Performance & Kernel Memory Reporting
- **Detailed Win32 Kernel Optimization Reports:** Formatted HTML activity log reports show initial RAM state, process working sets trimmed, protected count, system skipped count, settled RAM state, and total net physical RAM reclaimed in **GB** and **MB**.
- **System Tray Quick Controls:** Dynamic localized System Tray context menu for 1-click Quick Trim, Maximum Optimization, and Theme toggling.

---

## 🖥️ Operating System Compatibility

- **Windows 11:** 21H2, 22H2, 23H2, 24H2, 25H1, 26H1 (Insider/Canary/Enterprise/Pro/Home)
- **Windows 10:** 1507 through 22H2 (LTSC/Enterprise/Pro/Home)
- **Windows 8.1 / 8 / 7 (SP1):** All editions (x86 32-bit & x64 64-bit)
- **Windows Server:** 2025, 2022, 2019, 2016, 2012 R2, 2012, 2008 R2 (SP1)

---

## 📥 Download Assets

Download the official release archive below:
- **`RAM_Cleaner_Flusher_Pro_v1.1.0.zip`** (Contains `RAM_Cleaner_Pro.exe` with Self-Installing & Portable support)
