#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "update_manager.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMap>
#include <QList>

struct ProcessInstanceInfo {
    DWORD pid;
    QString procName;
    quint64 wsBytes;
    double wsMB;
    QString fullPath;
};

class ProcessTreeWidgetItem : public QTreeWidgetItem {
public:
    ProcessTreeWidgetItem(QTreeWidget *parent, const QStringList &strings)
        : QTreeWidgetItem(parent, strings) {}

    ProcessTreeWidgetItem(QTreeWidgetItem *parent, const QStringList &strings)
        : QTreeWidgetItem(parent, strings) {}

    void setSortValue(int column, double val) {
        m_sortValues[column] = val;
    }

    bool operator<(const QTreeWidgetItem &other) const override {
        int column = treeWidget() ? treeWidget()->sortColumn() : 0;
        const ProcessTreeWidgetItem *otherItem = dynamic_cast<const ProcessTreeWidgetItem*>(&other);
        if (otherItem && m_sortValues.contains(column) && otherItem->m_sortValues.contains(column)) {
            return m_sortValues.value(column, 0.0) < otherItem->m_sortValues.value(column, 0.0);
        }
        return QTreeWidgetItem::operator<(other);
    }

private:
    QMap<int, double> m_sortValues;
};

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QGroupBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDomDocument>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QSet>

void readAllTextInUI(QWidget *widget, const QString &fileName = "languages.xml")
{
    QSet<QString> ignoredObjectNames = {
        "lblHeaderTitle",
        "btnRowAction",
        "LogoTitle",
        "lblAppBanner",
    };

    QDomDocument doc;
    QDomElement root = doc.createElement("widgetInfo");
    doc.appendChild(root);

    auto addElement = [&](const QString &tagName, const QString &id, const QString &text) {
        if (id.isEmpty() || ignoredObjectNames.contains(id)) return;

        QDomElement elem = doc.createElement(tagName);
        elem.setAttribute("id", id);
        elem.appendChild(doc.createTextNode(text));
        root.appendChild(elem);
    };

    for (QLabel *label : widget->findChildren<QLabel *>()) {
        addElement("label", label->objectName(), label->text());
    }

    for (QPushButton *button : widget->findChildren<QPushButton *>()) {
        addElement("button", button->objectName(), button->text());
    }

    for (QRadioButton *radioButton : widget->findChildren<QRadioButton *>()) {
        addElement("radioButton", radioButton->objectName(), radioButton->text());
    }

    for (QCheckBox *checkBox : widget->findChildren<QCheckBox *>()) {
        addElement("checkBox", checkBox->objectName(), checkBox->text());
    }

    for (QLineEdit *lineEdit : widget->findChildren<QLineEdit *>()) {
        QString text = lineEdit->placeholderText().isEmpty() ? lineEdit->text() : lineEdit->placeholderText();
        addElement("lineEdit", lineEdit->objectName(), text);
    }

    for (QGroupBox *groupBox : widget->findChildren<QGroupBox *>()) {
        addElement("groupBox", groupBox->objectName(), groupBox->title());
    }

    for (QTabWidget *tabWidget : widget->findChildren<QTabWidget *>()) {
        for (int i = 0; i < tabWidget->count(); ++i) {
            QWidget *page = tabWidget->widget(i);
            if (page) {
                // Priority given to page objectName, fallback to indexed name if empty
                QString tabId = !page->objectName().isEmpty() ?
                                    page->objectName() : QString("%1_tab_%2").arg(tabWidget->objectName()).arg(i);
                addElement("tab", tabId, tabWidget->tabText(i));
            }
        }
    }

    for (QTreeWidget *treeWidget : widget->findChildren<QTreeWidget *>()) {
        if (treeWidget->headerItem()) {
            for (int col = 0; col < treeWidget->columnCount(); ++col) {
                QString colId = QString("%1_col_%2").arg(treeWidget->objectName()).arg(col);
                addElement("treeHeader", colId, treeWidget->headerItem()->text(col));
            }
        }
    }

    // Save into language_xml directory
    QDir dir("language_xml");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString fullPath = dir.filePath(fileName);
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << doc.toString();
        file.close();
        qDebug() << "Saved Done XML:" << fullPath;
    } else {
        qDebug() << "Failed to save XML:" << file.errorString();
    }
}

static QMap<QString, QString> g_xmlStringMap;

QString trXml(const QString &key, const QString &defaultVal) {
    if (g_xmlStringMap.contains(key)) {
        return g_xmlStringMap.value(key);
    }
    return defaultVal.isEmpty() ? key : defaultVal;
}

void replaceTextFromXML(QWidget *widget, const QString &xmlPath)
{
    QSet<QString> ignoredObjectNames = {
        "lblHeaderTitle"
    };

    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback to disk path if Qt resource is not recompiled yet
        QString diskPath = xmlPath;
        if (diskPath.startsWith(":/")) {
            diskPath = diskPath.mid(2);
        }
        file.setFileName(diskPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open XML file:" << xmlPath << "and" << diskPath;
            return;
        }
    }

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;

    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        qDebug() << "XML Parse error:" << errorMsg << "at line" << errorLine;
        file.close();
        return;
    }
    file.close();

    // Cache all XML elements with an 'id' attribute for trXml lookup
    g_xmlStringMap.clear();
    QDomNodeList allNodes = doc.elementsByTagName("str");
    for (int i = 0; i < allNodes.count(); ++i) {
        QDomElement elem = allNodes.at(i).toElement();
        if (elem.hasAttribute("id")) {
            g_xmlStringMap[elem.attribute("id")] = elem.text();
        }
    }

    QDomNodeList labels = doc.elementsByTagName("label");
    for (int i = 0; i < labels.count(); ++i) {
        QDomElement elem = labels.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QLabel *lbl = widget->findChild<QLabel *>(id)) {
            lbl->setText(elem.text());
        }
    }

    QDomNodeList buttons = doc.elementsByTagName("button");
    for (int i = 0; i < buttons.count(); ++i) {
        QDomElement elem = buttons.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QPushButton *btn = widget->findChild<QPushButton *>(id)) {
            btn->setText(elem.text());
        }
    }

    QDomNodeList radioButtons = doc.elementsByTagName("radioButton");
    for (int i = 0; i < radioButtons.count(); ++i) {
        QDomElement elem = radioButtons.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QRadioButton *radio = widget->findChild<QRadioButton *>(id)) {
            radio->setText(elem.text());
        }
    }

    QDomNodeList checkBoxes = doc.elementsByTagName("checkBox");
    for (int i = 0; i < checkBoxes.count(); ++i) {
        QDomElement elem = checkBoxes.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QCheckBox *chk = widget->findChild<QCheckBox *>(id)) {
            chk->setText(elem.text());
        }
    }

    QDomNodeList lineEdits = doc.elementsByTagName("lineEdit");
    for (int i = 0; i < lineEdits.count(); ++i) {
        QDomElement elem = lineEdits.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QLineEdit *txt = widget->findChild<QLineEdit *>(id)) {
            txt->setPlaceholderText(elem.text());
        }
    }

    QDomNodeList groupBoxes = doc.elementsByTagName("groupBox");
    for (int i = 0; i < groupBoxes.count(); ++i) {
        QDomElement elem = groupBoxes.at(i).toElement();
        QString id = elem.attribute("id");
        if (ignoredObjectNames.contains(id)) continue;

        if (QGroupBox *grp = widget->findChild<QGroupBox *>(id)) {
            grp->setTitle(elem.text());
        }
    }

    // --- QTabWidget Replacing (Fixed for Qt Layout Hierarchy) ---
    QDomNodeList tabs = doc.elementsByTagName("tab");
    for (int i = 0; i < tabs.count(); ++i) {
        QDomElement elem = tabs.at(i).toElement();
        QString tabId = elem.attribute("id");
        if (ignoredObjectNames.contains(tabId)) continue;

        if (QWidget *page = widget->findChild<QWidget *>(tabId)) {
            QTabWidget *tabWidget = nullptr;
            QObject *parentObj = page->parent();

            while (parentObj) {
                tabWidget = qobject_cast<QTabWidget *>(parentObj);
                if (tabWidget) break;
                parentObj = parentObj->parent(); 
            }

            if (tabWidget) {
                int index = tabWidget->indexOf(page);
                if (index != -1) {
                    tabWidget->setTabText(index, elem.text());
                }
            }
        }
        else if (tabId.contains("_tab_")) {
            QString parentName = tabId.section("_tab_", 0, 0);
            int index = tabId.section("_tab_", 1, 1).toInt();
            if (QTabWidget *tabWidget = widget->findChild<QTabWidget *>(parentName)) {
                if (index >= 0 && index < tabWidget->count()) {
                    tabWidget->setTabText(index, elem.text());
                }
            }
        }
    }

    QDomNodeList treeHeaders = doc.elementsByTagName("treeHeader");
    for (int i = 0; i < treeHeaders.count(); ++i) {
        QDomElement elem = treeHeaders.at(i).toElement();
        QString id = elem.attribute("id");

        QString treeName = id.section("_col_", 0, 0);
        int colIndex = id.section("_col_", 1, 1).toInt();

        if (ignoredObjectNames.contains(treeName)) continue;

        if (QTreeWidget *tree = widget->findChild<QTreeWidget *>(treeName)) {
            if (tree->headerItem() && colIndex < tree->columnCount()) {
                tree->headerItem()->setText(colIndex, elem.text());
            }
        }
    }

    qDebug() << "Successfully loaded UI text from:" << xmlPath;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION_DISPLAY));

    // Set Window Icon
    this->setWindowIcon(IconProvider::getIcon(IconProvider::RamIcon));

    // Setup About Logo (100x100)
    ui->lblAboutLogo->setPixmap(IconProvider::getIcon(IconProvider::RamIcon).pixmap(100, 100));

    // Setup Tree Widget Properties & Interactivity
    ui->treeProcesses->setIconSize(QSize(22, 22));
    ui->treeProcesses->setAnimated(true);
    ui->treeProcesses->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeProcesses, &QTreeWidget::customContextMenuRequested, this, &MainWindow::on_treeProcesses_customContextMenuRequested);
    connect(ui->treeProcesses, &QTreeWidget::itemDoubleClicked, this, &MainWindow::on_treeProcesses_itemDoubleClicked);

    // Header Column Setup (Interactive resizing for all columns, zero right whitespace gap)
    ui->treeProcesses->headerItem()->setText(0, "Process Name");
    ui->treeProcesses->headerItem()->setText(1, "PID");
    ui->treeProcesses->headerItem()->setText(2, "RAM Working Set");
    ui->treeProcesses->headerItem()->setText(3, "Actions");

    QHeaderView *header = ui->treeProcesses->header();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);

    int totalW = ui->treeProcesses->width();
    if (totalW < 800) totalW = 880;

    ui->treeProcesses->setColumnWidth(1, 110); // PID / Instances
    ui->treeProcesses->setColumnWidth(2, 140); // RAM Working Set
    ui->treeProcesses->setColumnWidth(3, 60);  // Actions Column (Compact)
    ui->treeProcesses->setColumnWidth(0, qMax(350, totalW - 330)); // Process Name fills remaining container width!
    ui->treeProcesses->setSortingEnabled(true);

    // Setup Tab & Social Button Icons
    setupTabIcons();

    // Setup System Tray
    setupSystemTray();

    // Initialize UI settings & load language translations FIRST
    loadSettingsToUI();

    // Initialize UI theme
    applyTheme(m_settings.isDarkTheme());

    // Setup monitoring timer (updates RAM status every 1.5 seconds)
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &MainWindow::updateMemoryStatusUI);
    m_monitorTimer->start(1500);

    // Setup auto-clean scheduler timer (checks every 10 seconds)
    m_schedulerTimer = new QTimer(this);
    connect(m_schedulerTimer, &QTimer::timeout, this, &MainWindow::checkAutoCleanRules);
    m_schedulerTimer->start(10000);

    // Initial refreshes
    updateMemoryStatusUI();
    refreshProcessTree();

    appendLog(trXml("logAppInit", "%1 initialized successfully.").arg(APP_NAME));
    appendLog(trXml("logDevInfo", "Developer: %1 (%2)").arg(APP_AUTHOR).arg(APP_GITHUB));

    // Trigger Silent Background Update Check (5 seconds after launch)
    QTimer::singleShot(5000, this, [this]() {
        UpdateManager::instance()->checkForUpdates(true /* silent */, this);
    });

    connect(UpdateManager::instance(), &UpdateManager::updateReadyForInstall, this, [this](const QString &zipPath) {
        appendLog(trXml("logUpdateReady", "Update downloaded and ready for installation: %1").arg(zipPath));
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupTabIcons() {
    ui->tabWidget->setTabIcon(0, IconProvider::getIcon(IconProvider::DashboardIcon));
    ui->tabWidget->setTabIcon(1, IconProvider::getIcon(IconProvider::ProcessesIcon));
    ui->tabWidget->setTabIcon(2, IconProvider::getIcon(IconProvider::ExclusionIcon));
    ui->tabWidget->setTabIcon(3, IconProvider::getIcon(IconProvider::BoosterIcon));
    ui->tabWidget->setTabIcon(4, IconProvider::getIcon(IconProvider::SettingsIcon));
    ui->tabWidget->setTabIcon(5, IconProvider::getIcon(IconProvider::AboutIcon));

    // Social buttons large icons & centered text
    ui->btnGithub->setIcon(IconProvider::getIcon(IconProvider::GithubIcon));
    ui->btnGithub->setIconSize(QSize(42, 42));
    ui->btnGithub->setStyleSheet("text-align: center;");

    ui->btnFacebook->setIcon(IconProvider::getIcon(IconProvider::FacebookIcon));
    ui->btnFacebook->setIconSize(QSize(42, 42));
    ui->btnFacebook->setStyleSheet("text-align: center;");

    ui->btnWebsite->setIcon(IconProvider::getIcon(IconProvider::WebsiteIcon));
    ui->btnWebsite->setIconSize(QSize(42, 42));
    ui->btnWebsite->setStyleSheet("text-align: center;");


    ui->btnRefreshProcesses->setIcon(IconProvider::getIcon(IconProvider::RefreshIcon));
    ui->btnClearLog->setIcon(IconProvider::getIcon(IconProvider::TrashIcon));

    ui->btnAddExclusion->setIcon(IconProvider::getIcon(IconProvider::ShieldIcon));
    ui->btnBrowseExclusion->setIcon(IconProvider::getIcon(IconProvider::ProcessesIcon));
    ui->btnPickRunningProcess->setIcon(IconProvider::getIcon(IconProvider::DashboardIcon));
    ui->btnRemoveExclusion->setIcon(IconProvider::getIcon(IconProvider::TrashIcon));

    ui->btnLaunchBoostedApp->setIcon(IconProvider::getIcon(IconProvider::RocketIcon));
    ui->btnLaunchBoostedApp->setStyleSheet("text-align: center;");

}

#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

void MainWindow::applyCustomFont() {
    // Strip any existing dynamic custom font block from stylesheet
    QString currentStyle = this->styleSheet();
    int fontIdx = currentStyle.indexOf("/* DYNAMIC_CUSTOM_FONT_START */");
    if (fontIdx != -1) {
        currentStyle = currentStyle.left(fontIdx).trimmed();
    }

    if (m_settings.isCustomFontEnabled()) {
        QString family = m_settings.getCustomFontFamily();
        int size = m_settings.getCustomFontSize();

        QString fontRule = QString("\n/* DYNAMIC_CUSTOM_FONT_START */\n"
                                   "QWidget, QLabel, QPushButton, QComboBox, QSpinBox, QLineEdit, QTreeWidget, QListWidget, QGroupBox, QTabBar::tab {\n"
                                   "    font-family: '%1' !important;\n"
                                   "    font-size: %2pt !important;\n"
                                   "}\n").arg(family).arg(size);

        this->setStyleSheet(currentStyle + fontRule);
        qApp->setFont(QFont(family, size));
    } else {
        // Strip custom font override block without forcing hardcoded font resets
        this->setStyleSheet(currentStyle);
    }
}

void MainWindow::changeLanguage(const QString &langCode) {
    QString xmlPath = ":/language_xml/languages.xml";
    bool isRTL = false;

    if (langCode == "ar") {
        xmlPath = ":/language_xml/languages_ar.xml";
        isRTL = true;
    } else if (langCode == "es") {
        xmlPath = ":/language_xml/languages_es.xml";
        isRTL = false;
    } else { // "en"
        xmlPath = ":/language_xml/languages.xml";
        isRTL = false;
    }

    // Replace text from XML for all UI widgets
    replaceTextFromXML(this, xmlPath);

    // Dynamic templates for RAM status labels loaded from XML
    if (!ui->lblRamLoadVal->text().isEmpty()) m_tmplRamLoad = ui->lblRamLoadVal->text();
    if (!ui->lblUsedFreeVal->text().isEmpty()) m_tmplUsedFree = ui->lblUsedFreeVal->text();
    if (!ui->lblStandbyVal->text().isEmpty()) m_tmplStandby = ui->lblStandbyVal->text();
    updateMemoryStatusUI();

    // Apply RTL or LTR layout direction & alignments to app and controls
    Qt::LayoutDirection dir = isRTL ? Qt::RightToLeft : Qt::LeftToRight;
    Qt::Alignment align = isRTL ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter);

    qApp->setLayoutDirection(dir);

    ui->spinThreshold->setLayoutDirection(dir);
    ui->spinThreshold->setAlignment(align);

    // ui->spinInterval->setLayoutDirection(dir);
    // ui->spinInterval->setAlignment(align);

    ui->spinFontSize->setLayoutDirection(dir);
    ui->spinFontSize->setAlignment(align);

    ui->comboFontFamily->setLayoutDirection(dir);
    ui->comboTheme->setLayoutDirection(dir);
    ui->comboLanguage->setLayoutDirection(dir);
    ui->comboPriority->setLayoutDirection(dir);

    // Translate comboTheme items dynamically
    bool isDark = m_settings.isDarkTheme();
    int currentThemeIdx = ui->comboTheme->currentIndex();
    ui->comboTheme->blockSignals(true);
    ui->comboTheme->clear();
    if (langCode == "ar") {
        ui->comboTheme->addItem(" الوضع الداكن");
        ui->comboTheme->addItem(" الوضع الفاتح");
    } else if (langCode == "es") {
        ui->comboTheme->addItem(" Tema Oscuro");
        ui->comboTheme->addItem(" Tema Claro");
    } else {
        ui->comboTheme->addItem(" Dark Theme");
        ui->comboTheme->addItem(" Light Theme");
    }
    ui->comboTheme->setCurrentIndex(currentThemeIdx >= 0 ? currentThemeIdx : (isDark ? 0 : 1));
    ui->comboTheme->blockSignals(false);

    // Update btnToggleTheme text based on current theme and language
    if (isDark) {
        if (langCode == "ar") ui->btnToggleTheme->setText("الوضع الفاتح");
        else if (langCode == "es") ui->btnToggleTheme->setText("Tema Claro");
        else ui->btnToggleTheme->setText("Light Theme");
        ui->btnToggleTheme->setIcon(IconProvider::getIcon(IconProvider::SunIcon));
    } else {
        if (langCode == "ar") ui->btnToggleTheme->setText("الوضع الداكن");
        else if (langCode == "es") ui->btnToggleTheme->setText("Tema Oscuro");
        else ui->btnToggleTheme->setText("Dark Theme");
        ui->btnToggleTheme->setIcon(IconProvider::getIcon(IconProvider::MoonIcon));
    }

    // Translate spinInterval suffix dynamically
    if (langCode == "ar") {
        ui->spinInterval->setSuffix(" دقائق");
    } else if (langCode == "es") {
        ui->spinInterval->setSuffix(" minutos");
    } else {
        ui->spinInterval->setSuffix(" minutes");
    }

    // Update System Tray menu actions dynamically if created
    if (m_trayMenu) {
        QList<QAction*> actions = m_trayMenu->actions();
        if (actions.size() >= 5) {
            actions[0]->setText(trXml("trayShow", "Show Dashboard"));
            actions[1]->setText(trXml("trayMax", "Maximum Optimization"));
            actions[2]->setText(trXml("trayQuick", "Quick Trim"));
            // index 3 is separator
            actions[4]->setText(trXml("trayToggleTheme", "Toggle Theme"));
            // index 5 is separator
            if (actions.size() >= 7) actions[6]->setText(trXml("trayExit", "Exit Program"));
        }
    }

    // Save language setting
    m_settings.setLanguage(langCode);

    // Refresh process tree with new language
    refreshProcessTree();
}

void MainWindow::applyTheme(bool isDark) {
    m_settings.setDarkTheme(isDark);
    m_settings.saveSettings();

    HWND hwnd = reinterpret_cast<HWND>(this->winId());
    BOOL darkMode = isDark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    QString resPath = isDark ? ":/styles/dark_theme.qss" : ":/styles/light_theme.qss";

    QFile file(resPath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        this->setStyleSheet(QString::fromUtf8(file.readAll()));
        file.close();
    } else {
        QString diskPath = isDark ? QDir(QCoreApplication::applicationDirPath()).filePath("styles/dark_theme.qss")
                                  : QDir(QCoreApplication::applicationDirPath()).filePath("styles/light_theme.qss");
        QFile diskFile(diskPath);
        if (diskFile.open(QFile::ReadOnly | QFile::Text)) {
            this->setStyleSheet(QString::fromUtf8(diskFile.readAll()));
            diskFile.close();
        }
    }

    QString currentLang = m_settings.getLanguage();
    if (isDark) {
        if (currentLang == "ar") ui->btnToggleTheme->setText("الوضع الفاتح");
        else if (currentLang == "es") ui->btnToggleTheme->setText("Tema Claro");
        else ui->btnToggleTheme->setText("Light Theme");
        ui->btnToggleTheme->setIcon(IconProvider::getIcon(IconProvider::SunIcon));
        ui->comboTheme->blockSignals(true);
        ui->comboTheme->setCurrentIndex(0);
        ui->comboTheme->blockSignals(false);
    } else {
        if (currentLang == "ar") ui->btnToggleTheme->setText("الوضع الداكن");
        else if (currentLang == "es") ui->btnToggleTheme->setText("Tema Oscuro");
        else ui->btnToggleTheme->setText("Dark Theme");
        ui->btnToggleTheme->setIcon(IconProvider::getIcon(IconProvider::MoonIcon));
        ui->comboTheme->blockSignals(true);
        ui->comboTheme->setCurrentIndex(1);
        ui->comboTheme->blockSignals(false);
    }

    QString themeName = isDark ? trXml("themeDark", "Dark") : trXml("themeLight", "Light");
    appendLog(trXml("logSwitchedTheme", "Switched application theme to: %1").arg(themeName));

    // Re-apply custom font override over new theme stylesheet
    applyCustomFont();
}

void MainWindow::loadSettingsToUI() {
    ui->chkAutoThreshold->setChecked(m_settings.isAutoCleanEnabled());
    ui->spinThreshold->setValue(m_settings.getAutoCleanThreshold());

    ui->chkTimerClean->setChecked(m_settings.isTimerCleanEnabled());
    ui->spinInterval->setValue(m_settings.getTimerCleanIntervalMinutes());

    ui->chkStartWithWindows->setChecked(m_settings.isStartWithWindows());
    ui->chkMinimizeToTray->setChecked(m_settings.isMinimizeToTray());
    ui->chkStartMinimized->setChecked(m_settings.isStartMinimized());

    ui->txtBoostAppPath->setText(m_settings.getBoostAppPath());
    ui->comboPriority->setCurrentIndex(m_settings.getBoostPriority());

    // Select current language in comboLanguage and apply language translations
    QString lang = m_settings.getLanguage();
    int langIdx = 0; // "ar"
    if (lang == "en") langIdx = 1;
    else if (lang == "es") langIdx = 2;

    ui->comboLanguage->blockSignals(true);
    ui->comboLanguage->setCurrentIndex(langIdx);
    ui->comboLanguage->blockSignals(false);

    changeLanguage(lang);

    // Configure comboFontFamily: Non-editable sleek combo & Scalable vector fonts only (prevents OpenType console warnings)
    ui->comboFontFamily->setFontFilters(QFontComboBox::ScalableFonts);
    ui->comboFontFamily->setEditable(false);

    // Font settings UI state
    bool customFontEnabled = m_settings.isCustomFontEnabled();
    ui->chkCustomFont->blockSignals(true);
    ui->chkCustomFont->setChecked(customFontEnabled);
    ui->chkCustomFont->blockSignals(false);

    ui->comboFontFamily->blockSignals(true);
    ui->comboFontFamily->setCurrentFont(QFont(m_settings.getCustomFontFamily()));
    ui->comboFontFamily->setEnabled(customFontEnabled);
    ui->comboFontFamily->blockSignals(false);

    ui->spinFontSize->blockSignals(true);
    ui->spinFontSize->setValue(m_settings.getCustomFontSize());
    ui->spinFontSize->setEnabled(customFontEnabled);
    ui->spinFontSize->blockSignals(false);

    applyCustomFont();

    // Load exclusions with real icons
    ui->listExclusions->clear();
    for (const QString &proc : m_settings.getExclusionList()) {
        QListWidgetItem *item = new QListWidgetItem(IconProvider::getProcessIcon(0, proc), proc);
        ui->listExclusions->addItem(item);
    }
}

void MainWindow::setupSystemTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(IconProvider::getIcon(IconProvider::RamIcon));
    m_trayIcon->setToolTip("RAM Cleaner & Flusher Pro Suite");

    m_trayMenu = new QMenu(this);

    QAction *actShow = m_trayMenu->addAction(trXml("trayShow", "Show Dashboard"));
    connect(actShow, &QAction::triggered, this, &MainWindow::showNormal);

    QAction *actMax = m_trayMenu->addAction(trXml("trayMax", "Maximum Optimization"));
    connect(actMax, &QAction::triggered, this, &MainWindow::on_btnMaxOpt_clicked);

    QAction *actQuick = m_trayMenu->addAction(trXml("trayQuick", "Quick Trim"));
    connect(actQuick, &QAction::triggered, this, &MainWindow::on_btnQuickOpt_clicked);

    m_trayMenu->addSeparator();

    QAction *actToggleTheme = m_trayMenu->addAction(trXml("trayToggleTheme", "Toggle Theme"));
    connect(actToggleTheme, &QAction::triggered, this, &MainWindow::on_btnToggleTheme_clicked);

    m_trayMenu->addSeparator();

    QAction *actExit = m_trayMenu->addAction(trXml("trayExit", "Exit Program"));
    connect(actExit, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_trayIcon->show();
}

void MainWindow::updateMemoryStatusUI() {
    if (m_isOptimizing) return; // Do not interrupt background optimization thread updates

    MemoryMetrics metrics = m_cleaner.getMemoryMetrics();

    ui->progressRam->setValue(metrics.memoryLoadPercent);
    ui->lblRamLoadVal->setText(QString(m_tmplRamLoad).arg(metrics.memoryLoadPercent));

    if (metrics.memoryLoadPercent >= 85) {
        ui->lblRamLoadVal->setStyleSheet("color: #ef4444; font-weight: bold;");
    } else if (metrics.memoryLoadPercent >= 65) {
        ui->lblRamLoadVal->setStyleSheet("color: #f59e0b; font-weight: bold;");
    } else {
        ui->lblRamLoadVal->setStyleSheet("color: #10b981; font-weight: bold;");
    }

    ui->lblUsedFreeVal->setText(QString(m_tmplUsedFree)
                                    .arg(metrics.usedPhysGB, 0, 'f', 2)
                                    .arg(metrics.availPhysGB, 0, 'f', 2)
                                    .arg(metrics.totalPhysGB, 0, 'f', 2));

    ui->lblStandbyVal->setText(QString(m_tmplStandby)
                                   .arg(metrics.standbyCacheGB, 0, 'f', 2));
}

void MainWindow::refreshProcessTree() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return;
    }

    // Group processes by executable name (lower-case key)
    QMap<QString, QList<ProcessInstanceInfo>> groupedProcesses;

    do {
        if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4) continue;

        QString procName = QString::fromWCharArray(pe32.szExeFile);
        QString keyName = procName.toLower();

        quint64 wsBytes = 0;
        QString fullPath;

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
        if (hProc) {
            PROCESS_MEMORY_COUNTERS pmc = { 0 };
            if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                wsBytes = pmc.WorkingSetSize;
            }
            wchar_t pathBuf[MAX_PATH] = { 0 };
            DWORD sz = MAX_PATH;
            if (GetModuleFileNameExW(hProc, NULL, pathBuf, MAX_PATH) > 0) {
                fullPath = QString::fromWCharArray(pathBuf);
            }
            CloseHandle(hProc);
        }

        ProcessInstanceInfo info;
        info.pid = pe32.th32ProcessID;
        info.procName = procName;
        info.wsBytes = wsBytes;
        info.wsMB = wsBytes / (1024.0 * 1024.0);
        info.fullPath = fullPath;

        groupedProcesses[keyName].append(info);
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    ui->treeProcesses->setSortingEnabled(false);
    ui->treeProcesses->clear();

    // Populate TreeWidget
    for (auto it = groupedProcesses.begin(); it != groupedProcesses.end(); ++it) {
        const QList<ProcessInstanceInfo> &instances = it.value();
        if (instances.isEmpty()) continue;

        QString displayName = instances.first().procName;
        QIcon procIcon = IconProvider::getProcessIcon(instances.first().pid, displayName);

        if (instances.size() == 1) {
            // Single Instance Process Row
            const ProcessInstanceInfo &inst = instances.first();

            // Check Win32 Service Name
            QString serviceDesc = IconProvider::getProcessServiceDescription(inst.pid);
            QString procTitle = serviceDesc.isEmpty() ? inst.procName : QString("%1 [%2]").arg(inst.procName).arg(serviceDesc);

            QStringList colTexts;
            colTexts << procTitle
                     << QString::number(inst.pid)
                     << QString("%1 MB").arg(inst.wsMB, 0, 'f', 2)
                     << "";

            ProcessTreeWidgetItem *item = new ProcessTreeWidgetItem(ui->treeProcesses, colTexts);
            item->setIcon(0, procIcon);
            item->setSortValue(1, static_cast<double>(inst.pid));
            item->setSortValue(2, static_cast<double>(inst.wsBytes));

            QString tooltip = QString("Process: %1\nPID: %2\nRAM: %3 MB\nService: %4\nPath: %5")
                                  .arg(inst.procName).arg(inst.pid).arg(inst.wsMB, 0, 'f', 2).arg(serviceDesc.isEmpty() ? "N/A" : serviceDesc).arg(inst.fullPath.isEmpty() ? "N/A" : inst.fullPath);
            item->setToolTip(0, tooltip);
            item->setToolTip(1, tooltip);
            item->setToolTip(2, tooltip);

            // Column 3: Styled Action Box Button (⋮)
            QPushButton *btnOptions = new QPushButton("⋮");
            btnOptions->setObjectName("btnRowAction");
            btnOptions->setToolTip("Process Options (Protect, Kill, File Location, Diagnostics)");
            btnOptions->setFixedSize(36, 22);
            btnOptions->setProperty("procName", inst.procName);
            btnOptions->setProperty("pid", static_cast<qulonglong>(inst.pid));
            btnOptions->setProperty("fullPath", inst.fullPath);
            btnOptions->setProperty("isGroup", false);
            connect(btnOptions, &QPushButton::clicked, this, &MainWindow::onOptionsButtonClicked);

            ui->treeProcesses->setItemWidget(item, 3, btnOptions);

        } else {
            // Grouped Multi-Instance Parent Node
            quint64 totalWsBytes = 0;
            QList<DWORD> pidsList;
            for (const auto &inst : instances) {
                totalWsBytes += inst.wsBytes;
                pidsList.append(inst.pid);
            }
            double totalWsMB = totalWsBytes / (1024.0 * 1024.0);

            QString parentTitle = trXml("treeGroupTitle", "%1 (%2 processes)").arg(displayName).arg(instances.size());
            QStringList colTexts;
            colTexts << parentTitle
                     << trXml("treeInstancesText", "%1 Instances").arg(instances.size())
                     << QString("%1 MB").arg(totalWsMB, 0, 'f', 2)
                     << "";

            ProcessTreeWidgetItem *parentItem = new ProcessTreeWidgetItem(ui->treeProcesses, colTexts);
            parentItem->setIcon(0, procIcon);
            parentItem->setSortValue(1, static_cast<double>(instances.size()));
            parentItem->setSortValue(2, static_cast<double>(totalWsBytes));

            QString parentTooltip = QString("Process Group: %1\nTotal Instances: %2\nTotal Combined RAM: %3 MB")
                                        .arg(displayName).arg(instances.size()).arg(totalWsMB, 0, 'f', 2);
            parentItem->setToolTip(0, parentTooltip);
            parentItem->setToolTip(1, parentTooltip);
            parentItem->setToolTip(2, parentTooltip);

            // Group Options Button (⋮)
            QPushButton *btnGroupOptions = new QPushButton("⋮");
            btnGroupOptions->setObjectName("btnRowAction");
            btnGroupOptions->setToolTip("Group Options");
            btnGroupOptions->setFixedSize(36, 22);
            btnGroupOptions->setProperty("procName", displayName);
            btnGroupOptions->setProperty("fullPath", instances.first().fullPath);
            btnGroupOptions->setProperty("isGroup", true);

            QVariantList pidsVarList;
            for (DWORD p : pidsList) pidsVarList.append(static_cast<qulonglong>(p));
            btnGroupOptions->setProperty("pidsList", pidsVarList);

            connect(btnGroupOptions, &QPushButton::clicked, this, &MainWindow::onOptionsButtonClicked);

            ui->treeProcesses->setItemWidget(parentItem, 3, btnGroupOptions);

            // Add Child Nodes for Each Process Instance
            for (const auto &inst : instances) {
                QString serviceDesc = IconProvider::getProcessServiceDescription(inst.pid);
                QString childTitle = serviceDesc.isEmpty() ? inst.procName : QString("%1 [%2]").arg(inst.procName).arg(serviceDesc);

                QStringList childTexts;
                childTexts << childTitle
                           << QString::number(inst.pid)
                           << QString("%1 MB").arg(inst.wsMB, 0, 'f', 2)
                           << "";

                ProcessTreeWidgetItem *childItem = new ProcessTreeWidgetItem(parentItem, childTexts);
                childItem->setIcon(0, procIcon);
                childItem->setSortValue(1, static_cast<double>(inst.pid));
                childItem->setSortValue(2, static_cast<double>(inst.wsBytes));

                QString childTooltip = QString("Process: %1\nPID: %2\nRAM: %3 MB\nService: %4\nPath: %5")
                                           .arg(inst.procName).arg(inst.pid).arg(inst.wsMB, 0, 'f', 2).arg(serviceDesc.isEmpty() ? "N/A" : serviceDesc).arg(inst.fullPath.isEmpty() ? "N/A" : inst.fullPath);
                childItem->setToolTip(0, childTooltip);
                childItem->setToolTip(1, childTooltip);
                childItem->setToolTip(2, childTooltip);

                QPushButton *btnChildOptions = new QPushButton("⋮");
                btnChildOptions->setObjectName("btnRowAction");
                btnChildOptions->setToolTip("Process Options");
                btnChildOptions->setFixedSize(36, 22);
                btnChildOptions->setProperty("procName", inst.procName);
                btnChildOptions->setProperty("pid", static_cast<qulonglong>(inst.pid));
                btnChildOptions->setProperty("fullPath", inst.fullPath);
                btnChildOptions->setProperty("isGroup", false);
                connect(btnChildOptions, &QPushButton::clicked, this, &MainWindow::onOptionsButtonClicked);

                ui->treeProcesses->setItemWidget(childItem, 3, btnChildOptions);
            }
        }
    }

    // Default sorting by Column 2 (RAM Working Set) Descending
    ui->treeProcesses->setSortingEnabled(true);
    ui->treeProcesses->sortItems(2, Qt::DescendingOrder);

    // Apply current search filter
    on_txtSearchProcess_textChanged(ui->txtSearchProcess->text());
}

void MainWindow::onOptionsButtonClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    bool isGroup = btn->property("isGroup").toBool();
    QString procName = btn->property("procName").toString();
    QString fullPath = btn->property("fullPath").toString();
    DWORD pid = static_cast<DWORD>(btn->property("pid").toULongLong());

    QList<DWORD> pidsList;
    if (isGroup) {
        QVariantList varList = btn->property("pidsList").toList();
        for (const QVariant &v : varList) {
            pidsList.append(static_cast<DWORD>(v.toULongLong()));
        }
    }

    QMenu optionsMenu(this);
    QAction *actInspect = optionsMenu.addAction(IconProvider::getIcon(IconProvider::DashboardIcon), isGroup ? trXml("actInspectGroup", "Inspect Group Diagnostics (%1 Processes)").arg(pidsList.size()) : trXml("actInspectSingle", "Inspect Detailed Diagnostics"));
    optionsMenu.addSeparator();

    QAction *actProtect = optionsMenu.addAction(IconProvider::getIcon(IconProvider::ShieldIcon), trXml("actProtect", "Protect (Add to Exclusion List)"));
    QAction *actKill = optionsMenu.addAction(IconProvider::getIcon(IconProvider::TrashIcon), isGroup ? trXml("actKillGroup", "Task Kill (Terminate All %1 Processes)").arg(pidsList.size()) : trXml("actKillSingle", "Task Kill (Terminate Process)"));
    QAction *actOpenLocation = optionsMenu.addAction(IconProvider::getIcon(IconProvider::ProcessesIcon), trXml("actOpenLocation", "Open File Location"));

    QAction *selected = optionsMenu.exec(btn->mapToGlobal(QPoint(0, btn->height())));

    if (selected == actInspect) {
        if (isGroup) {
            openProcessGroupInspector(procName, pidsList);
        } else if (pid > 0) {
            openProcessInspector(pid, procName);
        }
    } else if (selected == actProtect) {
        m_settings.addExclusion(procName);
        loadSettingsToUI();
        appendLog(trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
        QMessageBox::information(this, trXml("actProtect", "Protect (Add to Exclusion List)"), trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
    } else if (selected == actKill) {
        if (isGroup) {
            killProcessGroup(procName, pidsList);
        } else {
            killSingleProcess(pid, procName);
        }
    } else if (selected == actOpenLocation) {
        openFileLocation(fullPath.isEmpty() ? procName : fullPath);
    }
}

void MainWindow::openProcessGroupInspector(const QString &groupName, const QList<DWORD> &pids) {
    appendLog(trXml("logOpeningGroupDiag", "Opening Group Diagnostics for '%1' (%2 Instances)...").arg(groupName).arg(pids.size()));
    ProcessGroupInfoDialog dlg(groupName, pids, this);

    connect(&dlg, &ProcessGroupInfoDialog::protectRequested, this, [this](const QString &name) {
        m_settings.addExclusion(name);
        loadSettingsToUI();
        appendLog(trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(name));
    });

    connect(&dlg, &ProcessGroupInfoDialog::killGroupRequested, this, &MainWindow::killProcessGroup);

    connect(&dlg, &ProcessGroupInfoDialog::trimGroupRequested, this, [this](const QString &name, const QList<DWORD> &pids) {
        int trimmed = 0;
        for (DWORD pid : pids) {
            HANDLE hProc = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pid);
            if (hProc) {
                if (EmptyWorkingSet(hProc) || SetProcessWorkingSetSize(hProc, (SIZE_T)-1, (SIZE_T)-1)) {
                    trimmed++;
                }
                CloseHandle(hProc);
            }
        }
        appendLog(QString("Trimmed Working Set for %1 processes in group '%2'.").arg(trimmed).arg(name));
        refreshProcessTree();
        updateMemoryStatusUI();
    });

    dlg.exec();
}

void MainWindow::openFileLocation(const QString &exePath) {
    QString resolved = exePath;
    if (resolved.isEmpty() || !QFileInfo::exists(resolved)) {
        resolved = IconProvider::ResolveExePath(QFileInfo(exePath).fileName());
    }

    if (resolved.isEmpty() || !QFileInfo::exists(resolved)) {
        QMessageBox::warning(this, "File Location Error", QString("Executable path for '%1' could not be resolved on disk.").arg(exePath));
        return;
    }

    QString nativePath = QDir::toNativeSeparators(resolved);
    std::wstring wParams = L"/select,\"" + nativePath.toStdWString() + L"\"";
    ShellExecuteW(NULL, L"open", L"explorer.exe", wParams.c_str(), NULL, SW_SHOWNORMAL);
    appendLog(QString("Opened file location for '%1' in Windows Explorer.").arg(QFileInfo(resolved).fileName()));
}

void MainWindow::killProcessGroup(const QString &procName, const QList<DWORD> &pids) {
    if (pids.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        trXml("actKillGroup", "Task Kill (Terminate All %1 Processes)").arg(pids.size()),
        QString("Are you sure you want to terminate ALL %1 instances of process group '%2'?").arg(pids.size()).arg(procName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        int killed = 0;
        for (DWORD pid : pids) {
            if (pid == 0 || pid == 4 || pid == GetCurrentProcessId()) continue;
            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProc) {
                if (TerminateProcess(hProc, 1)) killed++;
                CloseHandle(hProc);
            }
        }
        appendLog(trXml("logTerminatedGroup", "Terminated %1 out of %2 instances of process group '%3'.").arg(killed).arg(pids.size()).arg(procName));
        refreshProcessTree();
    }
}

void MainWindow::killSingleProcess(DWORD pid, const QString &procName) {
    if (pid == 0 || pid == GetCurrentProcessId()) {
        QMessageBox::warning(this, "Termination Error", "Cannot terminate this critical system process.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        trXml("actKillSingle", "Task Kill (Terminate Process)"),
        QString("Are you sure you want to terminate process '%1' (PID: %2)?").arg(procName).arg(pid),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc) {
            if (TerminateProcess(hProc, 1)) {
                appendLog(trXml("logTerminatedSingle", "Terminated process '%1' (PID: %2).").arg(procName).arg(pid));
            } else {
                appendLog(QString("Failed to terminate process '%1' (Access Denied).").arg(procName), false);
            }
            CloseHandle(hProc);
        } else {
            appendLog(QString("Failed to open process '%1' for termination.").arg(procName), false);
        }
        refreshProcessTree();
    }
}

void MainWindow::on_btnRefreshProcesses_clicked() {
    refreshProcessTree();
    appendLog(trXml("logRefreshedProcesses", "Refreshed running processes tree."));
}

void MainWindow::on_txtSearchProcess_textChanged(const QString &text) {
    int topCount = ui->treeProcesses->topLevelItemCount();
    for (int i = 0; i < topCount; ++i) {
        QTreeWidgetItem *parentItem = ui->treeProcesses->topLevelItem(i);
        if (!parentItem) continue;

        bool parentMatch = text.isEmpty() ||
                           parentItem->text(0).contains(text, Qt::CaseInsensitive) ||
                           parentItem->text(1).contains(text);

        bool childMatch = false;
        int childCount = parentItem->childCount();
        for (int c = 0; c < childCount; ++c) {
            QTreeWidgetItem *childItem = parentItem->child(c);
            bool m = text.isEmpty() ||
                     childItem->text(0).contains(text, Qt::CaseInsensitive) ||
                     childItem->text(1).contains(text);
            childItem->setHidden(!m);
            if (m) childMatch = true;
        }

        parentItem->setHidden(!parentMatch && !childMatch);
        if (childMatch && !text.isEmpty()) {
            parentItem->setExpanded(true);
        }
    }
}

void MainWindow::on_treeProcesses_itemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    static bool s_isOpeningInspector = false;
    if (s_isOpeningInspector) return;

    if (!item) return;

    // If double clicked a parent group item (item has child nodes):
    if (item->childCount() > 0) {
        // Toggle expansion state cleanly, DO NOT open single inspector!
        item->setExpanded(!item->isExpanded());
        return;
    }

    // If double clicked a child or single-instance process:
    QString procName = item->text(0);
    bool ok = false;
    DWORD pid = static_cast<DWORD>(item->text(1).toULong(&ok));

    if (ok && pid > 0) {
        s_isOpeningInspector = true;
        openProcessInspector(pid, procName);
        s_isOpeningInspector = false;
    }
}

void MainWindow::on_treeProcesses_customContextMenuRequested(const QPoint &pos) {
    QTreeWidgetItem *item = ui->treeProcesses->itemAt(pos);
    if (!item) return;

    QString procName = item->text(0);
    bool isGroupNode = (item->childCount() > 0);
    bool ok = false;
    DWORD pid = static_cast<DWORD>(item->text(1).toULong(&ok));

    QList<DWORD> groupPids;
    if (isGroupNode) {
        for (int c = 0; c < item->childCount(); ++c) {
            bool cOk = false;
            DWORD cPid = static_cast<DWORD>(item->child(c)->text(1).toULong(&cOk));
            if (cOk) groupPids.append(cPid);
        }
    }

    QMenu contextMenu(this);
    QAction *actInspect = contextMenu.addAction(IconProvider::getIcon(IconProvider::DashboardIcon), isGroupNode ? trXml("actInspectGroup", "Inspect Group Diagnostics (%1 Processes)").arg(groupPids.size()) : trXml("actInspectSingle", "Inspect Detailed Diagnostics"));
    contextMenu.addSeparator();

    QAction *actProtect = contextMenu.addAction(IconProvider::getIcon(IconProvider::ShieldIcon), trXml("actProtect", "Protect (Add to Exclusion List)"));
    QAction *actKill = contextMenu.addAction(IconProvider::getIcon(IconProvider::TrashIcon), isGroupNode ? trXml("actKillGroup", "Task Kill (Terminate All %1 Processes)").arg(groupPids.size()) : trXml("actKillSingle", "Task Kill (Terminate Process)"));
    QAction *actOpenLocation = contextMenu.addAction(IconProvider::getIcon(IconProvider::ProcessesIcon), trXml("actOpenLocation", "Open File Location"));

    QAction *selected = contextMenu.exec(ui->treeProcesses->viewport()->mapToGlobal(pos));

    if (selected == actInspect) {
        if (isGroupNode) {
            openProcessGroupInspector(procName, groupPids);
        } else if (ok && pid > 0) {
            openProcessInspector(pid, procName);
        }
    } else if (selected == actProtect) {
        m_settings.addExclusion(procName);
        loadSettingsToUI();
        appendLog(trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
        QMessageBox::information(this, trXml("actProtect", "Protect (Add to Exclusion List)"), trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
    } else if (selected == actKill) {
        if (isGroupNode) {
            killProcessGroup(procName, groupPids);
        } else if (ok && pid > 0) {
            killSingleProcess(pid, procName);
        }
    } else if (selected == actOpenLocation) {
        openFileLocation(procName);
    }
}

void MainWindow::openProcessInspector(DWORD pid, const QString &procName) {
    appendLog(trXml("logOpeningProcDiag", "Opening Process Inspector Diagnostics for '%1' (PID: %2)...").arg(procName).arg(pid));
    ProcessInfoDialog dlg(pid, procName, this);

    connect(&dlg, &ProcessInfoDialog::protectRequested, this, [this](const QString &name) {
        m_settings.addExclusion(name);
        loadSettingsToUI();
        appendLog(trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(name));
        QMessageBox::information(this, trXml("actProtect", "Protect (Add to Exclusion List)"), trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(name));
    });

    connect(&dlg, &ProcessInfoDialog::killRequested, this, [this](DWORD p, const QString &name) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, p);
        if (hProc) {
            if (TerminateProcess(hProc, 1)) {
                appendLog(trXml("logTerminatedSingle", "Terminated process '%1' (PID: %2).").arg(name).arg(p));
            } else {
                appendLog(QString("Failed to terminate process '%1' (Access Denied).").arg(name), false);
            }
            CloseHandle(hProc);
        } else {
            appendLog(QString("Failed to open process '%1' for termination.").arg(name), false);
        }
        refreshProcessTree();
    });

    connect(&dlg, &ProcessInfoDialog::trimRequested, this, &MainWindow::trimSingleProcess);

    dlg.exec();
}

void MainWindow::trimSingleProcess(DWORD pid, const QString &procName) {
    if (!startWorker(OptimizationWorker::ActionTrimProcess, pid, procName)) return;
}

void MainWindow::onProcessProtectClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString procName = btn->property("procName").toString();
    if (!procName.isEmpty()) {
        m_settings.addExclusion(procName);
        loadSettingsToUI();
        appendLog(trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
        QMessageBox::information(this, trXml("actProtect", "Protect (Add to Exclusion List)"), trXml("logProtected", "Protected '%1' by adding to Exclusion List.").arg(procName));
    }
}

void MainWindow::onProcessKillClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString procName = btn->property("procName").toString();
    DWORD pid = static_cast<DWORD>(btn->property("pid").toULongLong());

    killSingleProcess(pid, procName);
}

void MainWindow::setUIButtonsEnabled(bool enabled) {
    ui->btnMaxOpt->setEnabled(enabled);
    ui->btnQuickOpt->setEnabled(enabled);
    ui->btnPurgeStandby->setEnabled(enabled);
    ui->btnFlushModified->setEnabled(enabled);
    ui->btnLaunchBoostedApp->setEnabled(enabled);

    // if (enabled) {
    //     ui->btnMaxOpt->setText(trXml("btnMaxOpt", "Maximum Optimization (All Actions)"));
    //     ui->btnQuickOpt->setText(trXml("btnQuickOpt", "Quick Trim (Process Working Sets)"));
    //     ui->btnPurgeStandby->setText(trXml("btnPurgeStandby", "Purge Standby Cache"));
    //     ui->btnFlushModified->setText(trXml("btnFlushModified", "Flush Modified Pages"));
    // } else {
    //     ui->btnMaxOpt->setText("...");
    //     ui->btnQuickOpt->setText("...");
    //     ui->btnPurgeStandby->setText("...");
    //     ui->btnFlushModified->setText("...");
    // }
}

bool MainWindow::startWorker(OptimizationWorker::ActionType type, DWORD pid, const QString &procName) {
    if (m_isOptimizing.exchange(true)) {
        return false; // Optimization already in progress; block re-entrancy safely!
    }

    ui->txtLog->clear();
    setUIButtonsEnabled(false);

    QString actionTitle = "Memory Optimization";
    if (type == OptimizationWorker::ActionMaximum) actionTitle = trXml("actTitleMax", "Maximum Optimization (Full System Memory Flush)");
    else if (type == OptimizationWorker::ActionQuick) actionTitle = trXml("actTitleQuick", "Quick Trim (Process Working Sets Only)");
    else if (type == OptimizationWorker::ActionPurgeStandby) actionTitle = trXml("actTitlePurge", "Purge Standby Memory Cache Lists");
    else if (type == OptimizationWorker::ActionFlushModified) actionTitle = trXml("actTitleFlush", "Flush Modified Memory Page Lists to Disk");
    else if (type == OptimizationWorker::ActionTrimProcess) actionTitle = trXml("actTitleTrimProc", "Trim Working Set for process '%1' (PID: %2)").arg(procName).arg(pid);

    appendLog(trXml("logStartingOp", "Starting Operation : %1 | Please Wait For OS Kernel Memory.").arg(actionTitle));

    OptimizationWorker *worker = new OptimizationWorker(type, m_settings.getExclusionList(), this);
    if (type == OptimizationWorker::ActionTrimProcess) {
        worker->setTargetProcess(pid, procName);
        connect(worker, &OptimizationWorker::singleProcessTrimCompleted, this, &MainWindow::onWorkerSingleProcessTrimCompleted);
    } else {
        connect(worker, &OptimizationWorker::optimizationCompleted, this, &MainWindow::onWorkerOptimizationCompleted);
    }

    connect(worker, &OptimizationWorker::finished, worker, &QObject::deleteLater);

    worker->start();
    return true;
}

void MainWindow::onWorkerOptimizationCompleted(const QString &actionTitle, const OptimizationResult &res) {
    logOptimizationReport(actionTitle, res);
    updateMemoryStatusUI();
    setUIButtonsEnabled(true);
    m_isOptimizing.store(false);

    if (m_trayIcon && m_trayIcon->isVisible() && (res.reclaimedGB >= 0.10)) {
        m_trayIcon->showMessage("RAM Optimization Completed",
                                QString("Released %1 GB RAM (RAM load reduced by %2%)")
                                    .arg(res.reclaimedGB, 0, 'f', 2).arg(res.loadDiffPercent),
                                QSystemTrayIcon::Information, 3000);
    }
}

void MainWindow::onWorkerSingleProcessTrimCompleted(DWORD pid, const QString &procName, bool success, double savedMB, int beforeLoad, int afterLoad) {
    if (success) {
        appendLog(trXml("logSingleTrimDone", "Successfully trimmed Working Set of process '%1' (PID: %2). Released: %3 MB RAM. Total RAM Load: %4% ➔ %5%.")
                      .arg(procName).arg(pid).arg(savedMB, 0, 'f', 2).arg(beforeLoad).arg(afterLoad));
    } else {
        appendLog(trXml("logSingleTrimFail", "Failed to trim Working Set of '%1' (Access Denied).").arg(procName), false);
    }

    refreshProcessTree();
    updateMemoryStatusUI();
    setUIButtonsEnabled(true);
    m_isOptimizing.store(false);
}

void MainWindow::checkAutoCleanRules() {
    if (m_isOptimizing) return;

    static QDateTime lastTimerCleanTime = QDateTime::currentDateTime();
    MemoryMetrics metrics = m_cleaner.getMemoryMetrics();

    // Rule 1: Threshold trigger
    if (m_settings.isAutoCleanEnabled()) {
        int threshold = m_settings.getAutoCleanThreshold();
        if (metrics.memoryLoadPercent >= threshold) {
            startWorker(OptimizationWorker::ActionMaximum);
            return;
        }
    }

    // Rule 2: Scheduled interval timer
    if (m_settings.isTimerCleanEnabled()) {
        int intervalMins = m_settings.getTimerCleanIntervalMinutes();
        if (lastTimerCleanTime.secsTo(QDateTime::currentDateTime()) >= (intervalMins * 60)) {
            lastTimerCleanTime = QDateTime::currentDateTime();
            startWorker(OptimizationWorker::ActionMaximum);
        }
    }
}

void MainWindow::on_btnMaxOpt_clicked() {
    startWorker(OptimizationWorker::ActionMaximum);
}

void MainWindow::on_btnQuickOpt_clicked() {
    startWorker(OptimizationWorker::ActionQuick);
}

void MainWindow::on_btnPurgeStandby_clicked() {
    startWorker(OptimizationWorker::ActionPurgeStandby);
}

void MainWindow::on_btnFlushModified_clicked() {
    startWorker(OptimizationWorker::ActionFlushModified);
}

void MainWindow::on_btnClearLog_clicked() {
    ui->txtLog->clear();
    appendLog(trXml("logActivityCleared", "Activity log cleared."));
}

void MainWindow::on_btnToggleTheme_clicked() {
    bool newDark = !m_settings.isDarkTheme();
    applyTheme(newDark);
}

void MainWindow::on_comboTheme_currentIndexChanged(int index) {
    applyTheme(index == 0);
}

void MainWindow::on_comboLanguage_currentIndexChanged(int index) {
    QString langCode = "ar";
    if (index == 1) langCode = "en";
    else if (index == 2) langCode = "es";

    changeLanguage(langCode);
    appendLog(trXml("logLanguageSwitched", "Language switched to: %1").arg(langCode.toUpper()));
}

void MainWindow::on_btnAddExclusion_clicked() {
    QString input = ui->txtExclusionInput->text().trimmed();
    if (input.isEmpty()) {
        on_btnPickRunningProcess_clicked();
        return;
    }

    QString procName = QFileInfo(input).fileName();
    if (!procName.endsWith(".exe", Qt::CaseInsensitive)) {
        procName += ".exe";
    }

    m_settings.addExclusion(procName);
    ui->txtExclusionInput->clear();

    loadSettingsToUI();
    appendLog(QString("Added '%1' to process exclusion list.").arg(procName));
}

void MainWindow::on_btnBrowseExclusion_clicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Executable File to Protect", "", "Executable Files (*.exe)");
    if (!file.isEmpty()) {
        QString procName = QFileInfo(file).fileName();
        m_settings.addExclusion(procName);
        loadSettingsToUI();
        appendLog(QString("Protected executable '%1' by adding to Exclusion List.").arg(procName));
    }
}

void MainWindow::on_btnPickRunningProcess_clicked() {
    ProcessSelectionDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString procName = dlg.selectedProcessName();
        if (!procName.isEmpty()) {
            m_settings.addExclusion(procName);
            loadSettingsToUI();
            appendLog(QString("Protected active process '%1' by adding to Exclusion List.").arg(procName));
        }
    }
}

void MainWindow::on_btnRemoveExclusion_clicked() {
    QListWidgetItem *item = ui->listExclusions->currentItem();
    if (!item) return;

    QString procName = item->text();
    m_settings.removeExclusion(procName);

    loadSettingsToUI();
    appendLog(trXml("logExclusionRemoved", "Removed '%1' from process exclusion list.").arg(procName));
}

void MainWindow::on_chkAutoThreshold_toggled(bool checked) {
    m_settings.setAutoCleanEnabled(checked);
    m_settings.saveSettings();
    appendLog(trXml("logSetAutoClean", "Settings Updated: Auto-Threshold Memory Clean %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_spinThreshold_valueChanged(int value) {
    m_settings.setAutoCleanThreshold(value);
    m_settings.saveSettings();
    appendLog(trXml("logSetAutoLimit", "Settings Updated: Auto-Threshold Limit set to %1%.").arg(value));
}

void MainWindow::on_chkTimerClean_toggled(bool checked) {
    m_settings.setTimerCleanEnabled(checked);
    m_settings.saveSettings();
    appendLog(trXml("logSetTimerClean", "Settings Updated: Scheduled Timer Clean %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_spinInterval_valueChanged(int value) {
    m_settings.setTimerCleanIntervalMinutes(value);
    m_settings.saveSettings();
    appendLog(trXml("logSetTimerInterval", "Settings Updated: Scheduled Timer Interval set to %1 minutes.").arg(value));
}

void MainWindow::on_chkStartWithWindows_toggled(bool checked) {
    m_settings.setStartWithWindows(checked);
    m_settings.saveSettings();
    appendLog(trXml("logSetStartup", "Settings Updated: Run on Windows Startup %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_chkMinimizeToTray_toggled(bool checked) {
    m_settings.setMinimizeToTray(checked);
    m_settings.saveSettings();
    appendLog(trXml("logSetMinTray", "Settings Updated: Minimize to System Tray %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_chkStartMinimized_toggled(bool checked) {
    m_settings.setStartMinimized(checked);
    m_settings.saveSettings();
    appendLog(trXml("logSetStartHidden", "Settings Updated: Start Hidden in System Tray %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_chkCustomFont_toggled(bool checked) {
    m_settings.setCustomFontEnabled(checked);
    ui->comboFontFamily->setEnabled(checked);
    ui->spinFontSize->setEnabled(checked);
    applyCustomFont();
    appendLog(trXml("logSetCustomFont", "Settings Updated: Custom Font %1.").arg(checked ? trXml("txtEnabled", "Enabled") : trXml("txtDisabled", "Disabled")));
}

void MainWindow::on_comboFontFamily_currentFontChanged(const QFont &font) {
    if (m_settings.isCustomFontEnabled()) {
        m_settings.setCustomFontFamily(font.family());
        applyCustomFont();
        appendLog(trXml("logSetFontFamily", "Settings Updated: Font Family set to '%1'.").arg(font.family()));
    }
}

void MainWindow::on_spinFontSize_valueChanged(int value) {
    if (m_settings.isCustomFontEnabled()) {
        m_settings.setCustomFontSize(value);
        applyCustomFont();
        appendLog(trXml("logSetFontSize", "Settings Updated: Font Size set to %1 pt.").arg(value));
    }
}

void MainWindow::on_btnBrowseBoostApp_clicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Application Executable", "", "Executable Files (*.exe)");
    if (!file.isEmpty()) {
        ui->txtBoostAppPath->setText(file);
        m_settings.setBoostAppPath(file);
        m_settings.saveSettings();
    }
}

void MainWindow::on_btnLaunchBoostedApp_clicked() {
    ui->txtLog->clear();
    QString appPath = ui->txtBoostAppPath->text().trimmed();
    if (appPath.isEmpty()) {
        QMessageBox::warning(this, "No Executable Selected", "Please select a target application executable (.exe) first.");
        return;
    }

    int priorityIdx = ui->comboPriority->currentIndex();
    m_settings.setBoostPriority(priorityIdx);
    m_settings.saveSettings();

    AppBooster::PriorityLevel prioLevel = static_cast<AppBooster::PriorityLevel>(priorityIdx);

    appendLog(trXml("logStartBooster", "🚀 Starting App Booster Pre-Flush & Launch for executable '%1'...").arg(QFileInfo(appPath).fileName()));

    OptimizationResult optRes;
    if (m_booster.boostAndLaunch(appPath, prioLevel, m_settings.getExclusionList(), &optRes)) {
        logOptimizationReport(QString("App Booster Pre-Flush for '%1'").arg(QFileInfo(appPath).fileName()), optRes);
        appendLog(trXml("logBoosterSuccess", "Application '%1' launched successfully with priority: %2.")
                      .arg(QFileInfo(appPath).fileName()).arg(AppBooster::priorityToString(prioLevel)));
    } else {
        appendLog(trXml("logBoosterFail", "Failed to launch application or elevate process priority."), false);
        QMessageBox::critical(this, "Launch Error", "Could not start the selected executable.");
    }
}

void MainWindow::on_btnGithub_clicked() {
    QDesktopServices::openUrl(QUrl(APP_GITHUB));
}

void MainWindow::on_btnFacebook_clicked() {
    QDesktopServices::openUrl(QUrl(APP_FACEBOOK));
}

void MainWindow::on_btnWebsite_clicked() {
    QDesktopServices::openUrl(QUrl(APP_WEBSITE));
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (this->isVisible()) {
            this->hide();
        } else {
            this->showNormal();
            this->activateWindow();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_settings.isMinimizeToTray() && m_trayIcon && m_trayIcon->isVisible()) {
        this->hide();
        m_trayIcon->showMessage("RAM Cleaner & Flusher Pro",
                                "Application running in background (System Tray).",
                                QSystemTrayIcon::Information, 2000);
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::appendLog(const QString &message, bool isSuccess) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    bool isDark = m_settings.isDarkTheme();

    QString color = isSuccess ? (isDark ? "#38bdf8" : "#0284c7") : (isDark ? "#ef4444" : "#dc2626");
    QString timeColor = isDark ? "#94a3b8" : "#64748b";

    QString html = QString("<span style='color:%1;'>[%2]</span> <span style='color:%3;'>%4</span>")
                       .arg(timeColor).arg(timestamp).arg(color).arg(message);
    ui->txtLog->append(html);
}

void MainWindow::logOptimizationReport(const QString &actionTitle, const OptimizationResult &res) {
    bool isDark = m_settings.isDarkTheme();

    QString headerColor = isDark ? "#38bdf8" : "#0284c7";  // Bright cyan / deep navy
    QString infoColor   = isDark ? "#f8fafc" : "#0f172a";  // High contrast body
    QString subColor    = isDark ? "#94a3b8" : "#64748b";  // Muted gray labels
    QString greenColor  = isDark ? "#34d399" : "#047857";  // Emerald accent
    QString yellowColor = isDark ? "#fbbf24" : "#b45309";  // Amber accent
    QString timeStr     = QDateTime::currentDateTime().toString("hh:mm:ss");

    QString html;
    html += QString("<div style='margin-top:6px; margin-bottom:6px; font-family:Segoe UI, Consolas, monospace; font-size:9.5pt;'>");
    html += QString("<b style='color:%1;'>======================================================================</b><br>").arg(headerColor);
    html += QString("<b style='color:%1;'>[%2] %3</b><br>").arg(headerColor).arg(timeStr).arg(trXml("reportActionInit", "ACTION INITIATED: %1").arg(actionTitle));
    html += QString("<b style='color:%1;'>----------------------------------------------------------------------</b><br>").arg(subColor);

    // Initial RAM State
    html += QString("<b style='color:%1;'>%2</b><br>").arg(yellowColor).arg(trXml("reportInitState", "INITIAL SYSTEM MEMORY STATE (BEFORE):"));
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4%</b><br>")
                .arg(infoColor).arg(trXml("reportPhysRamLoad", "Physical RAM Load:")).arg(res.before.memoryLoadPercent >= 80 ? "#ef4444" : greenColor).arg(res.before.memoryLoadPercent);
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4 GB</b> <span style='color:%5;'>(%6 %7 GB | %8 %9 GB)</span><br>")
                .arg(infoColor).arg(trXml("reportUsedMem", "Used Memory:")).arg(infoColor).arg(res.before.usedPhysGB, 0, 'f', 2).arg(subColor).arg(trXml("reportTotalInstalled", "Total Installed:")).arg(res.before.totalPhysGB, 0, 'f', 2).arg(trXml("reportAvailable", "Available:")).arg(res.before.availPhysGB, 0, 'f', 2);
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4 GB</b><br>")
                .arg(infoColor).arg(trXml("reportStandbyCache", "Standby Cache Memory:")).arg(infoColor).arg(res.before.standbyCacheGB, 0, 'f', 2);

    // Execution Details
    html += QString("<b style='color:%1;'>%2</b><br>").arg(yellowColor).arg(trXml("reportKernelSummary", "WIN32 KERNEL EXECUTION SUMMARY:"));
    html += QString("<span style='color:%1;'>   [1] %2 </span><b style='color:%3;'>%4 %5</b> <span style='color:%6;'>(%7 %8 | %9 %10)</span><br>")
                .arg(infoColor).arg(trXml("reportProcTrim", "Process Working Sets Trimming:")).arg(greenColor).arg(res.trimmedProcesses).arg(trXml("reportProcTrimmedCount", "Processes Trimmed")).arg(subColor).arg(trXml("reportProtectedCount", "Protected:")).arg(res.protectedProcesses).arg(trXml("reportSkippedCount", "System Skipped:")).arg(res.skippedProcesses);

    // Final RAM State
    html += QString("<b style='color:%1;'>%2</b><br>").arg(yellowColor).arg(trXml("reportAfterState", "SETTLED SYSTEM MEMORY STATE (AFTER):"));
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4%</b> <span style='color:%5;'>(%6 -%7%)</span><br>")
                .arg(infoColor).arg(trXml("reportPhysRamLoad", "Physical RAM Load:")).arg(greenColor).arg(res.after.memoryLoadPercent).arg(subColor).arg(trXml("reportLoadReduction", "Load Reduction:")).arg(res.loadDiffPercent);
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4 GB</b> <span style='color:%5;'>(%6 %7 GB)</span><br>")
                .arg(infoColor).arg(trXml("reportUsedMem", "Used Memory:")).arg(infoColor).arg(res.after.usedPhysGB, 0, 'f', 2).arg(subColor).arg(trXml("reportAvailable", "Available:")).arg(res.after.availPhysGB, 0, 'f', 2);
    html += QString("<span style='color:%1;'>   • %2 </span><b style='color:%3;'>%4 GB</b><br>")
                .arg(infoColor).arg(trXml("reportStandbyCache", "Standby Cache Memory:")).arg(infoColor).arg(res.after.standbyCacheGB, 0, 'f', 2);

    // Net Reclaimed Summary
    html += QString("<b style='color:%1;'>★ %2 </b><b style='color:%3;'>%4 GB (%5 MB)</b><br>")
                .arg(greenColor).arg(trXml("reportTotalReclaimed", "TOTAL RECLAIMED PHYSICAL MEMORY:")).arg(greenColor).arg(res.reclaimedGB, 0, 'f', 2).arg(res.reclaimedMB, 0, 'f', 2);
    html += QString("<b style='color:%1;'>======================================================================</b></div><br>").arg(headerColor);

    ui->txtLog->append(html);
}
