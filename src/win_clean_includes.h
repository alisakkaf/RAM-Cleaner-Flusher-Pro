#ifndef WIN_CLEAN_INCLUDES_H
#define WIN_CLEAN_INCLUDES_H

#if defined(_WIN32) || defined(WIN32)

// Target Windows 7 (0x0601) or higher for modern Win32 APIs
#ifndef WINVER
#define WINVER 0x0601
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Prevent macro collisions with min/max and lean Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <shlobj.h>

#endif // _WIN32

#endif // WIN_CLEAN_INCLUDES_H
