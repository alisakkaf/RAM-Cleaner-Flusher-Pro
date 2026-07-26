# 🌐 RAM Cleaner & Flusher Pro Suite - Localization & Translation Guide

Welcome to the **RAM Cleaner & Flusher Pro Suite** localization directory! We welcome community contributions to help translate the application into all major world languages.

This guide provides step-by-step instructions for translators, developers, and open-source contributors on how to add or update language XML files.

---

## 📁 Directory Structure & File Naming Conventions

All translation files are stored in XML format within the `language_xml/` directory:

```
RAM_Cleaner_GUI/
└── language_xml/
    ├── README.md               # Localization documentation (this file)
    ├── languages.xml           # 🇬🇧 English Master Template (Primary Reference)
    ├── languages_ar.xml        # 🇸🇦 Arabic Localization (Complete RTL)
    └── languages_es.xml        # 🇪🇸 Spanish Localization
```

### 🏷️ Naming Convention Rule
When creating a new language translation, copy the base `languages.xml` file and append the 2-letter **ISO 639-1 language code** to the filename:

`languages_<lang_code>.xml`

#### Examples:
- **French**: `languages_fr.xml`
- **German**: `languages_de.xml`
- **Italian**: `languages_it.xml`
- **Turkish**: `languages_tr.xml`
- **Russian**: `languages_ru.xml`
- **Portuguese**: `languages_pt.xml`
- **Japanese**: `languages_ja.xml`
- **Chinese (Simplified)**: `languages_zh.xml`
- **Korean**: `languages_ko.xml`

---

## 🏗️ XML Structure & Element Types

The translation files use a clean, structured XML schema wrapped inside `<widgetInfo>...</widgetInfo>`. 

Every translatable item has a unique `id="..."` attribute. **Do NOT change the `id` attributes or the XML tag names.** Only translate the text contained *between* the opening and closing XML tags.

| XML Tag Element | Target UI Component | Example Code |
| :--- | :--- | :--- |
| `<label id="...">` | Static UI labels, descriptions, and credits | `<label id="lblDevCredit">Created &amp; Developed By: AliSakkaf</label>` |
| `<button id="...">` | Interactive action buttons | `<button id="btnMaxOpt">Maximum Optimization (All Actions)</button>` |
| `<checkBox id="...">` | Checkbox options | `<checkBox id="chkAutoThreshold">Enable Automatic Memory Flushing</checkBox>` |
| `<radioButton id="...">` | Option selection radio buttons | `<radioButton id="...">Option Text</radioButton>` |
| `<lineEdit id="...">` | Input box placeholder texts | `<lineEdit id="txtSearchProcess">Search process by name or PID...</lineEdit>` |
| `<groupBox id="...">` | UI Section frames & group titles | `<groupBox id="groupAutoClean">Automated Memory Cleaning Rules</groupBox>` |
| `<tab id="...">` | Top navigation tab titles | `<tab id="tabDashboard">Dashboard</tab>` |
| `<treeHeader id="...">` | Process tree table column headers | `<treeHeader id="treeProcesses_col_0">Process Name</treeHeader>` |
| `<str id="...">` | Context menus (`⋮`), inspector dialogs, logs, tray menu, priority levels | `<str id="actInspectGroup">Inspect Group Diagnostics (%1 Processes)</str>` |

---

## ⚠️ Critical Translation Rules

### 1. 🛑 Preserve Dynamic Placeholders (`%1`, `%2`, `%3`, `%4`, `%5`)
Many string entries contain dynamic formatting placeholders like `%1`, `%2`, `%3`, etc. These placeholders are replaced at runtime by the application engine with real-time numbers, process names, memory sizes, or PIDs.

- **DO NOT** delete, alter, or re-number the `%1`, `%2`, `%3` markers.
- You **MAY** change the order of words around the markers to match the natural grammar of your target language.

#### Example:
- **English Master (`languages.xml`)**:
  ```xml
  <str id="logSingleTrimDone">Successfully trimmed Working Set of process '%1' (PID: %2). Released: %3 MB RAM. Total RAM Load: %4% ➔ %5%.</str>
  ```
- **Arabic Translation (`languages_ar.xml`)**:
  ```xml
  <str id="logSingleTrimDone">تم تفريغ ذاكرة العملية '%1' (PID: %2) بنجاح. تم تحرير: %3 ميجابايت. مستوى الذاكرة: %4% ➔ %5%.</str>
  ```
- **Spanish Translation (`languages_es.xml`)**:
  ```xml
  <str id="logSingleTrimDone">Conjunto de trabajo del proceso '%1' (PID: %2) reducido con éxito. Liberado: %3 MB RAM. Carga de RAM: %4% ➔ %5%.</str>
  ```

---

### 2. 🔤 Preserve XML Special Character Escaping
Because the files are XML documents, certain characters MUST be escaped using standard XML entities:

| Character | Must be written as | Notes |
| :---: | :---: | :--- |
| `&` | `&amp;` | Double `&amp;&amp;&amp;` is used in Qt buttons for shortcut mnemonics |
| `<` | `&lt;` | Used inside HTML tags embedded in XML summaries |
| `>` | `&gt;` | Used inside HTML tags embedded in XML summaries |
| `"` | `&quot;` | Used inside HTML attributes |

---

### 3. 🎨 Preserve HTML Formatting Tags
Some strings contain embedded HTML tags for styling (such as `<b>...</b>`, `<br>`, `<span>`). Preserve these tags so the formatted optimization reports display correctly.

#### Example:
```xml
<str id="dlgGroupSummaryTmpl">Total Combined RAM: &lt;b&gt;%1 MB&lt;/b&gt; | Avg per Instance: &lt;b&gt;%2 MB&lt;/b&gt; | Total Open Handles: &lt;b&gt;%3&lt;/b&gt;</str>
```

---

## 🚀 Step-by-Step Contribution Guide (GitHub Workflow)

Follow these steps to submit a new language translation or update an existing one:

### Step 1: Fork & Clone the Repository
First, fork the official repository on GitHub and clone it to your local system:

```bash
git clone https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro.git
cd RAM-Cleaner-Flusher-Pro/RAM_Cleaner_GUI
```

### Step 2: Create a New Branch
Create a new feature branch named after your target language:

```bash
git checkout -b feature/add-french-language
```

### Step 3: Create Your Translation File
Copy `languages.xml` to your target language filename in `RAM_Cleaner_GUI/language_xml/`:

```bash
cp language_xml/languages.xml language_xml/languages_fr.xml
```

### Step 4: Translate the File
Open `language_xml/languages_fr.xml` in your favorite code editor (e.g., VS Code, Notepad++, Sublime Text) and translate all element text.

### Step 5: Register the New File in Qt Resources (`resources.qrc`)
To bundle your new translation into the application build, open `RAM_Cleaner_GUI/resources.qrc` and add your new XML file:

```xml
<RCC>
    <qresource prefix="/">
        ...
        <file>language_xml/languages.xml</file>
        <file>language_xml/languages_ar.xml</file>
        <file>language_xml/languages_es.xml</file>
        <file>language_xml/languages_fr.xml</file>  <!-- ADD YOUR NEW FILE HERE -->
    </qresource>
</RCC>
```

### Step 6: Test Your Translation
Build and run the project using Qt Creator or CMake/QMake to verify that all UI labels, context menus, and logs display cleanly in your target language.

### Step 7: Commit & Push Changes
Stage, commit, and push your changes to your GitHub fork:

```bash
git add language_xml/languages_fr.xml resources.qrc
git commit -m "feat(i18n): Add French (fr) translation"
git push origin feature/add-french-language
```

### Step 8: Submit a Pull Request (PR)
1. Go to [https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro](https://github.com/alisakkaf/RAM-Cleaner-Flusher-Pro).
2. Click **New Pull Request**.
3. Select your branch and submit the PR with a brief description of your translation.

---

## 👨‍💻 Project Developer & Credits

- **Author & Lead Developer**: **AliSakkaf** (**علي السكاف**)
- **Official Website**: [https://alisakkaf.com](https://alisakkaf.com)
- **GitHub Profile**: [https://github.com/alisakkaf](https://github.com/alisakkaf)
- **Facebook Page**: [https://www.facebook.com/AliSakkaf.Dev](https://www.facebook.com/AliSakkaf.Dev)
- **License**: Open Source under the **MIT License**.

Thank you for contributing to **RAM Cleaner & Flusher Pro Suite** and helping make high-performance Windows memory optimization accessible to users worldwide! 🌍✨
