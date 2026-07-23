# Contributing to RAM Cleaner & Flusher Pro Suite

First off, thank you for considering contributing to **RAM Cleaner & Flusher Pro Suite**! It's contributions like yours that make this utility an open-source success.

---

## 📜 Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md). Please report any unacceptable behavior on our [Facebook Page](https://www.facebook.com/AliSakkaf.Dev/) or via [GitHub Issues](https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro/issues).

---

## 🛠️ How Can I Contribute?

### 1. Reporting Bugs
Before creating a bug report, please check existing issues to avoid duplicates.
When creating an issue, please use the **Bug Report Template** and include:
* **Operating System Version:** (e.g., Windows 11 Build 24H2 / Windows 10 / Windows Server 2022)
* **RAM Cleaner Version:** (e.g., v1.0.0)
* **Steps to Reproduce:** Exact sequence of steps.
* **Expected vs Actual Behavior:** What happened vs what should have happened.
* **Log Snippets:** Relevant non-sensitive logs from the Activity Log panel.

### 2. Suggesting Features
Enhancements and feature requests are welcome!
Please use the **Feature Request Template** and explain:
* **The Problem:** What inconvenience or limitation does this feature solve?
* **The Proposed Solution:** Describe how you envision the feature working.
* **Alternatives Considered:** Any alternative approaches.

### 3. Submitting Pull Requests
1. **Fork the Repository:** Create your own fork on GitHub.
2. **Clone & Branch:**
   ```bash
   git clone https://github.com/YOUR_USERNAME/RAM-Cleaner-Flusher-Pro.git
   cd RAM-Cleaner-Flusher-Pro
   git checkout -b feature/my-new-feature
   ```
3. **Coding Guidelines:**
   * Strictly adhere to pure C++14 / Qt 5 standards.
   * Do **NOT** use external shell calls (`cmd`, `powershell`, `wmic`).
   * Preserve Win32 Kernel safety checks (`IsRunningAsAdmin`, zero memory leak allocations).
   * Maintain high-contrast UI theme compatibility for both Dark and Light QSS stylesheets.
4. **Test Thoroughly:** Ensure clean compilation with zero warnings on MinGW or MSVC.
5. **Submit PR:** Open a Pull Request against the `main` branch with a clear description.

---

## 👨‍💻 Developer Contact

* **Author:** AliSakkaf (By AliSakkaf)
* **Website:** [https://alisakkaf.com](https://alisakkaf.com)
* **Facebook:** [https://www.facebook.com/AliSakkaf.Dev/](https://www.facebook.com/AliSakkaf.Dev/)
* **GitHub:** [@alisakkaf](https://github.com/alisakkaf)
