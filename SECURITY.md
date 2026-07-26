# Security Policy

## 🛡️ Supported Versions

We actively release security patches and updates for the following versions of **RAM Cleaner & Flusher Pro Suite**:

| Version | Supported          |
| ------- | ------------------ |
| v1.0.x  | :white_check_mark: |

---

## 🔒 Security Guarantees & Safety Commitments

**RAM Cleaner & Flusher Pro Suite** prioritizes OS stability, data integrity, and system safety:
1. **Zero Shell Execution / Zero Scripts:** The application does **NOT** execute `cmd.exe`, `powershell.exe`, or `wmic`. All operations invoke direct C++ Win32 API functions (`ntdll.dll`, `psapi.dll`, `kernel32.dll`, `advapi32.dll`).
2. **Zero Telemetry / Zero Tracking:** The application contains zero trackers, adware, analytics, or background data collection.
3. **Safe Working Set Trimming:** Memory trimming only releases idle process working set pages. It **NEVER** terminates active user applications, corrupts memory pages, or forces process crashes.
4. **Privilege Boundaries:** Requires elevation solely for NT Kernel Standby Cache flushing (`NtSetSystemInformation`).
5. **Verified Network Requests:** Update checks strictly connect over HTTPS to verified GitHub / Gist endpoints with no-cache validation.

---

## 📩 Reporting a Vulnerability

If you discover a security vulnerability or potential privilege elevation exploit:
* **DO NOT** create a public GitHub issue.
* Report security findings directly on our [Facebook Developer Page](https://www.facebook.com/AliSakkaf.Dev/) or through [GitHub Security Advisories / Issues](https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro/issues).
* Include detailed steps to reproduce the issue along with proof of concept if applicable.
* We acknowledge reports within **48 hours** and aim to release patch builds within **7 business days**.
