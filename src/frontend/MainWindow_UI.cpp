#include "MainWindow.h"
#include "components/SmoothScrollFilter.h"
#include "components/FanCardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QApplication>
#include <QStatusBar>
#include <QStyle>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QMenu>

void MainWindow::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);

    setWindowTitle("Pankha – Fan Control");
    setMinimumSize(900, 580);
    resize(960, 640);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ==========================================
    // Left Pane (Sidebar)
    // ==========================================
    m_leftPane = new QFrame(m_centralWidget);
    m_leftPane->setObjectName("leftPane");
    m_leftPane->setFixedWidth(320);

    QVBoxLayout *leftLayout = new QVBoxLayout(m_leftPane);
    leftLayout->setContentsMargins(20, 20, 20, 16);
    leftLayout->setSpacing(12);

    // macOS traffic lights
    QHBoxLayout *trafficLightsLayout = new QHBoxLayout();
    trafficLightsLayout->setSpacing(8);
    trafficLightsLayout->setContentsMargins(2, 0, 0, 4);

    QPushButton *closeDot = new QPushButton(QString::fromUtf8("\xc3\x97"), m_leftPane);
    closeDot->setObjectName("macCloseDot");
    closeDot->setFixedSize(12, 12);
    closeDot->setCursor(Qt::PointingHandCursor);

    QPushButton *minimizeDot = new QPushButton(QString::fromUtf8("\xe2\x88\x92"), m_leftPane);
    minimizeDot->setObjectName("macMinimizeDot");
    minimizeDot->setFixedSize(12, 12);
    minimizeDot->setCursor(Qt::PointingHandCursor);

    QPushButton *maximizeDot = new QPushButton("+", m_leftPane);
    maximizeDot->setObjectName("macMaximizeDot");
    maximizeDot->setFixedSize(12, 12);
    maximizeDot->setCursor(Qt::PointingHandCursor);

    connect(closeDot, &QPushButton::clicked, this, &MainWindow::close);
    connect(minimizeDot, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(maximizeDot, &QPushButton::clicked, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });

    trafficLightsLayout->addWidget(closeDot);
    trafficLightsLayout->addWidget(minimizeDot);
    trafficLightsLayout->addWidget(maximizeDot);
    trafficLightsLayout->addStretch(1);
    leftLayout->addLayout(trafficLightsLayout);

    // Logo row
    QHBoxLayout *logoRow = new QHBoxLayout();
    logoRow->setSpacing(8);
    QLabel *logoLabel = new QLabel("PANKHA", m_leftPane);
    logoLabel->setObjectName("appNameLabel");
    logoRow->addWidget(logoLabel);

    QLabel *versionLabel = new QLabel("v1.0", m_leftPane);
    versionLabel->setObjectName("appVersionLabel");
    logoRow->addWidget(versionLabel);
    logoRow->addStretch(1);

    QLabel *authorLabel = new QLabel("by itznan", m_leftPane);
    authorLabel->setObjectName("appAuthorLabel");
    logoRow->addWidget(authorLabel);
    leftLayout->addLayout(logoRow);

    // Separator
    QFrame *headerSep = new QFrame(m_leftPane);
    headerSep->setObjectName("headerSeparator");
    headerSep->setFrameShape(QFrame::HLine);
    headerSep->setFixedHeight(1);
    leftLayout->addWidget(headerSep);

    // Scroll area for fan cards
    m_scrollArea = new QScrollArea(m_leftPane);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_listContainer = new QWidget(m_scrollArea);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 4, 0);
    m_listLayout->setSpacing(8);
    m_listLayout->addStretch(1);

    m_scrollArea->setWidget(m_listContainer);
    m_scrollArea->viewport()->installEventFilter(new SmoothScrollFilter(m_scrollArea->verticalScrollBar(), m_scrollArea));
    leftLayout->addWidget(m_scrollArea, 1);

    // Footer
    QHBoxLayout *leftFooter = new QHBoxLayout();
    leftFooter->setSpacing(8);
    m_connectionStatusLabel = new QLabel(m_leftPane);
    leftFooter->addWidget(m_connectionStatusLabel);
    leftFooter->addStretch(1);

    m_settingsButton = new QPushButton("Settings", m_leftPane);
    m_settingsButton->setObjectName("settingsButton");
    m_settingsButton->setToolTip("Open global application settings");
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    leftFooter->addWidget(m_settingsButton);

    m_rescanButton = new QPushButton("Rescan", m_leftPane);
    m_rescanButton->setObjectName("rescanButton");
    m_rescanButton->setToolTip("Query hardware controllers and update fan list");
    m_rescanButton->setCursor(Qt::PointingHandCursor);
    leftFooter->addWidget(m_rescanButton);
    leftLayout->addLayout(leftFooter);

    mainLayout->addWidget(m_leftPane);

    // ==========================================
    // Right Pane
    // ==========================================
    m_rightPane = new QFrame(m_centralWidget);
    m_rightPane->setObjectName("rightPane");

    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPane);
    rightLayout->setContentsMargins(28, 28, 28, 28);
    rightLayout->setSpacing(16);

    // Empty state
    m_emptyStateWidget = new QWidget(m_rightPane);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    QLabel *emptyIcon = new QLabel(m_emptyStateWidget);
    emptyIcon->setObjectName("emptyStateIcon");
    emptyIcon->setText(QString::fromUtf8("\xe2\x9b\x9f"));
    emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel = new QLabel("Select a fan from the list\nto monitor and configure controls.", m_emptyStateWidget);
    m_emptyStateLabel->setObjectName("emptyStateText");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addSpacing(8);
    emptyLayout->addWidget(m_emptyStateLabel);
    rightLayout->addWidget(m_emptyStateWidget, 1);

    // Detail scroll area
    m_detailScrollArea = new QScrollArea(m_rightPane);
    m_detailScrollArea->setWidgetResizable(true);
    m_detailScrollArea->setFrameShape(QFrame::NoFrame);
    m_detailScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_detailScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QGraphicsOpacityEffect *detailFadeEffect = new QGraphicsOpacityEffect(m_detailScrollArea);
    m_detailScrollArea->setGraphicsEffect(detailFadeEffect);

    // Detail widget
    m_detailWidget = new QWidget(m_detailScrollArea);
    m_detailWidget->setObjectName("detailWidgetContainer");
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detailWidget);
    detailLayout->setContentsMargins(0, 0, 4, 0);
    detailLayout->setSpacing(20);

    // Header
    QVBoxLayout *headerInfo = new QVBoxLayout();
    headerInfo->setSpacing(2);
    m_detailNameLabel = new QLabel("Fan Name", m_detailWidget);
    m_detailNameLabel->setObjectName("detailTitle");
    m_detailHardwareLabel = new QLabel("Hardware Controller", m_detailWidget);
    m_detailHardwareLabel->setObjectName("detailHardware");
    headerInfo->addWidget(m_detailNameLabel);
    headerInfo->addWidget(m_detailHardwareLabel);
    detailLayout->addLayout(headerInfo);

    // Telemetry group
    QGroupBox *telemetryGroup = new QGroupBox("TELEMETRY", m_detailWidget);
    QVBoxLayout *telemetryLayout = new QVBoxLayout(telemetryGroup);
    telemetryLayout->setSpacing(16);

    QHBoxLayout *statsRow = new QHBoxLayout();
    statsRow->setSpacing(24);

    QVBoxLayout *rpmStat = new QVBoxLayout();
    rpmStat->setSpacing(4);
    QLabel *rpmStatTitle = new QLabel("CURRENT SPEED", telemetryGroup);
    rpmStatTitle->setObjectName("telemetryStatTitle");
    m_rpmValueLabel = new QLabel("--- RPM", telemetryGroup);
    m_rpmValueLabel->setObjectName("rpmValueLabel");
    rpmStat->addWidget(rpmStatTitle);
    rpmStat->addWidget(m_rpmValueLabel);

    QVBoxLayout *dutyStat = new QVBoxLayout();
    dutyStat->setSpacing(4);
    QLabel *dutyStatTitle = new QLabel("DUTY CYCLE", telemetryGroup);
    dutyStatTitle->setObjectName("telemetryStatTitle");
    m_dutyValueLabel = new QLabel("--- %", telemetryGroup);
    m_dutyValueLabel->setObjectName("dutyValueLabel");
    dutyStat->addWidget(dutyStatTitle);
    dutyStat->addWidget(m_dutyValueLabel);

    statsRow->addLayout(rpmStat, 1);
    statsRow->addLayout(dutyStat, 1);
    telemetryLayout->addLayout(statsRow);

    m_progressBar = new QProgressBar(telemetryGroup);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    telemetryLayout->addWidget(m_progressBar);
    detailLayout->addWidget(telemetryGroup);

    // Control configuration group
    QGroupBox *ctrlGroup = new QGroupBox("CONTROL CONFIGURATION", m_detailWidget);
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlGroup);
    ctrlLayout->setSpacing(16);

    m_manualOverrideCheck = new QCheckBox("Enable Manual Override", ctrlGroup);
    m_manualOverrideCheck->setCursor(Qt::PointingHandCursor);
    ctrlLayout->addWidget(m_manualOverrideCheck);

    m_targetSpeedLabel = new QLabel("TARGET SPEED", ctrlGroup);
    m_targetSpeedLabel->setObjectName("sectionLabel");
    m_targetSpeedLabel->setEnabled(false);
    ctrlLayout->addWidget(m_targetSpeedLabel);

    QHBoxLayout *sliderRow = new QHBoxLayout();
    sliderRow->setSpacing(12);
    m_targetSpeedSlider = new QSlider(Qt::Horizontal, ctrlGroup);
    m_targetSpeedSlider->setRange(0, 100);
    m_targetSpeedSlider->setEnabled(false);
    m_targetSpeedSlider->setCursor(Qt::PointingHandCursor);

    m_targetSpeedSpin = new QSpinBox(ctrlGroup);
    m_targetSpeedSpin->setRange(0, 100);
    m_targetSpeedSpin->setSuffix(" %");
    m_targetSpeedSpin->setEnabled(false);
    m_targetSpeedSpin->setFixedWidth(80);
    sliderRow->addWidget(m_targetSpeedSlider, 1);
    sliderRow->addWidget(m_targetSpeedSpin);
    ctrlLayout->addLayout(sliderRow);

    // Action buttons row
    QHBoxLayout *actionRow = new QHBoxLayout();
    actionRow->setSpacing(12);

    m_applyButton = new QPushButton("Apply Speed", m_detailWidget);
    m_applyButton->setObjectName("applyButton");
    m_applyButton->setEnabled(false);
    m_applyButton->setCursor(Qt::PointingHandCursor);

    actionRow->addWidget(m_applyButton, 1);
    ctrlLayout->addLayout(actionRow);

    // Advanced toggle
    m_advancedLink = new QLabel("<a href=\"toggle\" style=\"text-decoration:none; color:#007aff; font-size:11px;\">Show advanced options</a>", ctrlGroup);
    m_advancedLink->setCursor(Qt::PointingHandCursor);
    ctrlLayout->addWidget(m_advancedLink);

    m_advancedPanel = new QWidget(ctrlGroup);
    QFormLayout *advForm = new QFormLayout(m_advancedPanel);
    advForm->setContentsMargins(8, 4, 8, 4);
    advForm->setSpacing(10);
    advForm->setLabelAlignment(Qt::AlignRight);

    m_hardwarePathEdit = new QLineEdit(m_advancedPanel);
    m_hardwarePathEdit->setReadOnly(true);
    advForm->addRow("Hardware Path:", m_hardwarePathEdit);

    m_minRpmSpin = new QSpinBox(m_advancedPanel);
    m_minRpmSpin->setRange(0, 10000);
    m_minRpmSpin->setReadOnly(true);
    m_minRpmSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    advForm->addRow("Min RPM:", m_minRpmSpin);

    m_maxRpmSpin = new QSpinBox(m_advancedPanel);
    m_maxRpmSpin->setRange(0, 10000);
    m_maxRpmSpin->setReadOnly(true);
    m_maxRpmSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    advForm->addRow("Max RPM:", m_maxRpmSpin);

    m_controlIdEdit = new QLineEdit(m_advancedPanel);
    m_controlIdEdit->setReadOnly(true);
    advForm->addRow("Control ID:", m_controlIdEdit);

    m_advancedPanel->setVisible(false);
    ctrlLayout->addWidget(m_advancedPanel);

    detailLayout->addWidget(ctrlGroup);
    detailLayout->addStretch(1);

    m_detailScrollArea->setWidget(m_detailWidget);
    m_detailScrollArea->viewport()->installEventFilter(new SmoothScrollFilter(m_detailScrollArea->verticalScrollBar(), m_detailScrollArea));
    rightLayout->addWidget(m_detailScrollArea, 1);
    m_detailScrollArea->setVisible(false);

    // ==========================================
    // Settings Page
    // ==========================================
    m_settingsScrollArea = new QScrollArea(m_rightPane);
    m_settingsScrollArea->setWidgetResizable(true);
    m_settingsScrollArea->setFrameShape(QFrame::NoFrame);
    m_settingsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_settingsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QGraphicsOpacityEffect *settingsFadeEffect = new QGraphicsOpacityEffect(m_settingsScrollArea);
    m_settingsScrollArea->setGraphicsEffect(settingsFadeEffect);

    m_settingsWidget = new QWidget(m_settingsScrollArea);
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsWidget);
    settingsLayout->setContentsMargins(0, 0, 4, 0);
    settingsLayout->setSpacing(20);

    QVBoxLayout *settingsHeaderInfo = new QVBoxLayout();
    settingsHeaderInfo->setSpacing(2);
    QLabel *settingsTitle = new QLabel("Settings", m_settingsWidget);
    settingsTitle->setObjectName("detailTitle");
    QLabel *settingsSubtitle = new QLabel("Global Preferences & Options", m_settingsWidget);
    settingsSubtitle->setObjectName("detailHardware");
    settingsHeaderInfo->addWidget(settingsTitle);
    settingsHeaderInfo->addWidget(settingsSubtitle);
    settingsLayout->addLayout(settingsHeaderInfo);

    // Appearance
    QGroupBox *appearanceGroup = new QGroupBox("APPEARANCE", m_settingsWidget);
    QFormLayout *appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(12);
    appearanceLayout->setLabelAlignment(Qt::AlignRight);

    m_themeComboBox = new QComboBox(appearanceGroup);
    m_themeComboBox->addItem("Light", 0);
    m_themeComboBox->addItem("Dark", 1);
    m_themeComboBox->setCurrentIndex(0);
    m_themeComboBox->setCursor(Qt::PointingHandCursor);
    appearanceLayout->addRow("Application Theme:", m_themeComboBox);
    settingsLayout->addWidget(appearanceGroup);

    // Status Bar
    QGroupBox *statusBarGroup = new QGroupBox("STATUS BAR", m_settingsWidget);
    QVBoxLayout *statusBarLayout = new QVBoxLayout(statusBarGroup);
    statusBarLayout->setSpacing(10);

    m_showRpmStatusBarCheck = new QCheckBox("Show current fan RPM in status bar", statusBarGroup);
    m_showRpmStatusBarCheck->setChecked(true);
    m_showRpmStatusBarCheck->setCursor(Qt::PointingHandCursor);
    statusBarLayout->addWidget(m_showRpmStatusBarCheck);
    settingsLayout->addWidget(statusBarGroup);

    // Polling
    QGroupBox *pollingGroup = new QGroupBox("POLLING & TELEMETRY", m_settingsWidget);
    QFormLayout *pollingLayout = new QFormLayout(pollingGroup);
    pollingLayout->setSpacing(12);
    pollingLayout->setLabelAlignment(Qt::AlignRight);

    m_pollIntervalSpin = new QSpinBox(pollingGroup);
    m_pollIntervalSpin->setRange(500, 10000);
    m_pollIntervalSpin->setSingleStep(500);
    m_pollIntervalSpin->setValue(2000);
    m_pollIntervalSpin->setSuffix(" ms");
    pollingLayout->addRow("Sensor Poll Interval:", m_pollIntervalSpin);
    settingsLayout->addWidget(pollingGroup);

    // System preferences
    QGroupBox *systemPrefsGroup = new QGroupBox("SYSTEM PREFERENCES", m_settingsWidget);
    QVBoxLayout *systemPrefsLayout = new QVBoxLayout(systemPrefsGroup);
    systemPrefsLayout->setSpacing(10);

    m_startOnBootCheck = new QCheckBox("Start application on Windows boot", systemPrefsGroup);
    m_startOnBootCheck->setCursor(Qt::PointingHandCursor);
    systemPrefsLayout->addWidget(m_startOnBootCheck);

    m_minimizeToTrayCheck = new QCheckBox("Minimize to system tray instead of closing", systemPrefsGroup);
    m_minimizeToTrayCheck->setChecked(true);
    m_minimizeToTrayCheck->setCursor(Qt::PointingHandCursor);
    systemPrefsLayout->addWidget(m_minimizeToTrayCheck);
    settingsLayout->addWidget(systemPrefsGroup);

    // About
    QGroupBox *aboutGroup = new QGroupBox("ABOUT", m_settingsWidget);
    QVBoxLayout *aboutLayout = new QVBoxLayout(aboutGroup);
    aboutLayout->setSpacing(6);

    QLabel *aboutTitle = new QLabel("Pankha Fan Control App", aboutGroup);
    aboutTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    QLabel *aboutVersion = new QLabel("Version 1.0.0", aboutGroup);
    QLabel *aboutAuthor = new QLabel("Developer: itznan", aboutGroup);
    QLabel *aboutEngine = new QLabel("Backend: LibreHardwareMonitor Service", aboutGroup);
    aboutLayout->addWidget(aboutTitle);
    aboutLayout->addWidget(aboutVersion);
    aboutLayout->addWidget(aboutAuthor);
    aboutLayout->addWidget(aboutEngine);
    settingsLayout->addWidget(aboutGroup);

    settingsLayout->addStretch(1);

    m_settingsScrollArea->setWidget(m_settingsWidget);
    m_settingsScrollArea->viewport()->installEventFilter(new SmoothScrollFilter(m_settingsScrollArea->verticalScrollBar(), m_settingsScrollArea));
    rightLayout->addWidget(m_settingsScrollArea, 1);
    m_settingsScrollArea->setVisible(false);

    mainLayout->addWidget(m_rightPane, 1);

    // Status bar
    m_statusLeftLabel = new QLabel("Initializing...", this);
    m_statusRightLabel = new QLabel("00:00:00", this);
    statusBar()->addWidget(m_statusLeftLabel, 1);
    statusBar()->addPermanentWidget(m_statusRightLabel);
    statusBar()->setSizeGripEnabled(true);

    // System tray
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/images/logo.png"));
    m_trayIcon->setToolTip("Pankha Fan Control");

    QMenu *trayMenu = new QMenu(this);
    QAction *showAction = new QAction("Show Window", this);
    QAction *quitAction = new QAction("Exit", this);
    trayMenu->addAction(showAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);
    m_trayIcon->setContextMenu(trayMenu);

    connect(showAction, &QAction::triggered, this, &MainWindow::showNormalAndActivate);
    connect(quitAction, &QAction::triggered, this, &MainWindow::onQuitActionTriggered);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->show();
}

void MainWindow::setupConnections()
{
    connect(m_backendLauncher, &BackendLauncher::started, this, &MainWindow::onBackendStarted);
    connect(m_backendLauncher, &BackendLauncher::backendError, this, &MainWindow::onBackendError);

    connect(m_rescanButton, &QPushButton::clicked, this, &MainWindow::onRescanClicked);
    connect(m_manualOverrideCheck, &QCheckBox::toggled, this, &MainWindow::onManualOverrideToggled);
    connect(m_showRpmStatusBarCheck, &QCheckBox::clicked, this, &MainWindow::updateStatusBar);

    connect(m_advancedLink, &QLabel::linkActivated, this, &MainWindow::onAdvancedToggleClicked);

    connect(m_targetSpeedSlider, &QSlider::valueChanged, this, &MainWindow::onSliderValueChanged);
    connect(m_targetSpeedSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onSpinBoxValueChanged);

    connect(m_applyButton, &QPushButton::clicked, this, &MainWindow::onApplyClicked);

    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimerTick);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::onClockTimerTick);

    connect(m_themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onThemeChanged);

    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_pollIntervalSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onPollIntervalChanged);
    connect(m_startOnBootCheck, &QCheckBox::toggled, this, &MainWindow::onStartOnBootToggled);
    connect(m_minimizeToTrayCheck, &QCheckBox::toggled, this, &MainWindow::onMinimizeToTrayToggled);
    connect(m_showRpmStatusBarCheck, &QCheckBox::clicked, this, &MainWindow::saveSettings);
}

void MainWindow::loadStylesheet()
{
    QString themeName = "light";
    if (m_themeComboBox) {
        themeName = (m_themeComboBox->currentIndex() == 1) ? "dark" : "light";
    }

    QFile file(QString(":/styles_%1.qss").arg(themeName));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}

void MainWindow::setConnectionStatus(const QString &status, const QString &color)
{
    m_connectionStatusLabel->setText(QString("<span style='color:%1; font-size:14px; font-weight:bold;'>\xe2\x97\x8f</span> %2")
                                     .arg(color).arg(status));
}

void MainWindow::showEmptyState(const QString &message)
{
    m_emptyStateLabel->setText(message);
    m_emptyStateWidget->setVisible(true);
    m_detailScrollArea->setVisible(false);
    m_settingsScrollArea->setVisible(false);
}

void MainWindow::showDetailsPanel()
{
    m_emptyStateWidget->setVisible(false);
    m_detailScrollArea->setVisible(true);
    m_settingsScrollArea->setVisible(false);
    animateFadeIn(m_detailScrollArea);
}

void MainWindow::updateUIWithSelectedFan(bool fullRefresh)
{
    if (m_selectedFanId.isEmpty()) {
        if (!m_settingsScrollArea->isVisible()) {
            showEmptyState("Select a fan from the list\nto monitor and configure controls.");
        }
        return;
    }

    QJsonObject selectedFan;
    bool found = false;
    for (const QJsonValue &val : m_fansCache) {
        QJsonObject fan = val.toObject();
        if (fan["Id"].toString() == m_selectedFanId) {
            selectedFan = fan;
            found = true;
            break;
        }
    }
    if (!found) return;

    m_selectedControlId = selectedFan["ControlId"].toString();

    int rpm = selectedFan["Rpm"].toDouble();
    int minRpm = selectedFan["Min"].toDouble();
    int maxRpm = selectedFan["Max"].toDouble();
    int currentDuty = selectedFan["SpeedPercent"].toDouble();

    m_rpmValueLabel->setText(QString("%1 RPM").arg(rpm));
    m_dutyValueLabel->setText(QString("%1 %").arg(currentDuty));

    int progressVal = 0;
    if (maxRpm > minRpm && maxRpm > 0) {
        progressVal = qBound(0, (rpm - minRpm) * 100 / (maxRpm - minRpm), 100);
    } else if (rpm > 0) {
        progressVal = qBound(0, rpm * 100 / 3000, 100);
    }

    if (fullRefresh) {
        m_progressBar->setValue(progressVal);
    } else {
        QPropertyAnimation *progressAnim = new QPropertyAnimation(m_progressBar, "value", this);
        progressAnim->setDuration(400);
        progressAnim->setStartValue(m_progressBar->value());
        progressAnim->setEndValue(progressVal);
        progressAnim->setEasingCurve(QEasingCurve::OutQuad);
        progressAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    bool hasControl = !m_selectedControlId.isEmpty();

    if (fullRefresh) {
        m_pendingManualChange = false;

        m_detailNameLabel->setText(selectedFan["Name"].toString() + (hasControl ? "" : " (Monitor Only)"));
        m_detailHardwareLabel->setText(selectedFan["HardwareName"].toString());

        QString displayPath = m_selectedFanId;
        m_hardwarePathEdit->setText(displayPath.replace('_', '/'));
        m_minRpmSpin->setValue(minRpm);
        m_maxRpmSpin->setValue(maxRpm);
        m_controlIdEdit->setText(m_selectedControlId);

        m_manualOverrideCheck->setEnabled(hasControl);

        if (hasControl) {
            QString mode = selectedFan["Mode"].toString().toLower();
            bool isManual = (mode == "software");

            m_manualOverrideCheck->blockSignals(true);
            m_manualOverrideCheck->setChecked(isManual);
            m_manualOverrideCheck->blockSignals(false);

            m_targetSpeedLabel->setEnabled(isManual);
            m_targetSpeedSlider->setEnabled(isManual);
            m_targetSpeedSpin->setEnabled(isManual);

            m_targetSpeedSlider->blockSignals(true);
            m_targetSpeedSlider->setValue(currentDuty);
            m_targetSpeedSlider->blockSignals(false);

            m_targetSpeedSpin->blockSignals(true);
            m_targetSpeedSpin->setValue(currentDuty);
            m_targetSpeedSpin->blockSignals(false);

            m_applyButton->setEnabled(isManual);
        } else {
            m_manualOverrideCheck->setChecked(false);
            m_targetSpeedLabel->setEnabled(false);
            m_targetSpeedSlider->setEnabled(false);
            m_targetSpeedSpin->setEnabled(false);
            m_targetSpeedSlider->setValue(0);
            m_targetSpeedSpin->setValue(0);
            m_applyButton->setEnabled(false);
        }
    } else {
        if (hasControl) {
            m_manualOverrideCheck->setEnabled(true);

            QString mode = selectedFan["Mode"].toString().toLower();
            bool isManual = (mode == "software");

            if (!m_pendingManualChange) {
                m_manualOverrideCheck->blockSignals(true);
                m_manualOverrideCheck->setChecked(isManual);
                m_manualOverrideCheck->blockSignals(false);

                m_targetSpeedLabel->setEnabled(isManual);
                m_targetSpeedSlider->setEnabled(isManual);
                m_targetSpeedSpin->setEnabled(isManual);
                m_applyButton->setEnabled(isManual && m_consecutiveErrors < 3);
            }

            if (!m_targetSpeedSlider->hasFocus() && !m_targetSpeedSpin->hasFocus()) {
                m_targetSpeedSlider->blockSignals(true);
                m_targetSpeedSlider->setValue(currentDuty);
                m_targetSpeedSlider->blockSignals(false);

                m_targetSpeedSpin->blockSignals(true);
                m_targetSpeedSpin->setValue(currentDuty);
                m_targetSpeedSpin->blockSignals(false);
            }
        }
    }

    updateStatusBar();
}

void MainWindow::updateStatusBar()
{
    if (m_selectedFanId.isEmpty()) {
        m_statusLeftLabel->setText("No fan selected");
        return;
    }

    QJsonObject selectedFan;
    bool found = false;
    for (const QJsonValue &val : m_fansCache) {
        QJsonObject fan = val.toObject();
        if (fan["Id"].toString() == m_selectedFanId) {
            selectedFan = fan;
            found = true;
            break;
        }
    }
    if (!found) return;

    int rpm = selectedFan["Rpm"].toDouble();
    bool hasControl = !selectedFan["ControlId"].toString().isEmpty();
    QString mode = !hasControl ? "Monitor Only" : (selectedFan["Mode"].toString().toLower() == "software" ? "Manual" : "Auto");

    QString leftText;
    if (m_showRpmStatusBarCheck->isChecked()) {
        leftText = QString("%1 | %2 RPM | %3").arg(selectedFan["Name"].toString()).arg(rpm).arg(mode);
    } else {
        leftText = QString("%1 | %2").arg(selectedFan["Name"].toString()).arg(mode);
    }
    m_statusLeftLabel->setText(leftText);
}
