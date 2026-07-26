#ifndef VERSION_H
#define VERSION_H

// =========================================================================
//         RAM CLEANER & FLUSHER PRO SUITE - CENTRAL VERSION CONFIG
// =========================================================================
// Single source of truth for versioning, identity, publisher, and links.
// Editing this file automatically updates:
// 1. Windows File Properties & VersionInfo Resource (app.rc)
// 2. Qt Application Metadata (main.cpp)
// 3. User Interface Headers, About Dialogs, & Activity Logs (mainwindow.cpp)
// =========================================================================

// ─── Version Numbers ─────────────────────────────────────────────────────────
#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   1
#define APP_VERSION_PATCH   0
#define APP_VERSION_BUILD   0

// Comma-separated for VERSIONINFO resource in app.rc
#define APP_VERSION_RC      APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_PATCH,APP_VERSION_BUILD

// String versions
#define APP_VERSION_STR     "1.1.0"
#define APP_VERSION_RC_STR  "1.1.0.0"

// ─── Application Identity ────────────────────────────────────────────────────
#define APP_NAME            "RAM Cleaner & Flusher Pro Suite"
#define APP_EXE_NAME        "RAM_Cleaner_Pro.exe"
#define APP_INTERNAL_NAME   "RAM_Cleaner_Pro.exe"
#define APP_DESCRIPTION     "Professional Win32 Physical Memory & Standby Cache Optimizer"
#define APP_ORIGINAL_NAME   "RAM_Cleaner_Pro.exe"
#define APP_INSTALL_DIR_NAME "RAM Cleaner & Flusher Pro"

// ─── Publisher / Author ──────────────────────────────────────────────────────
#define APP_AUTHOR          "AliSakkaf"
#define APP_COMPANY         "AliSakkaf"
#define APP_COPYRIGHT       "Copyright © 2026 AliSakkaf. All Rights Reserved."
#define APP_COPYRIGHT_RC    "Copyright (C) 2026 AliSakkaf. All Rights Reserved."

// ─── Online Presence & Update Server ─────────────────────────────────────────
#define APP_WEBSITE         "https://alisakkaf.com"
#define APP_GITHUB          "https://github.com/alisakkaf"
#define APP_FACEBOOK        "https://www.facebook.com/AliSakkaf.Dev/"

// Permanent, stable Raw Gist URL for silent check & auto-updater
#define APP_UPDATE_URL      "https://gist.githubusercontent.com/alisakkaf/57417e7089a058e7200b0795a4802cc1/raw/RAM_Cleaner.json"

// ─── Display String (used in UI header & logs) ──────────────────────────────
#define APP_VERSION_DISPLAY "v1.1.0 | By AliSakkaf"

#endif // VERSION_H
