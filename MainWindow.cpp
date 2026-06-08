#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include <QApplication>
#include <QDebug>
#include <QStatusBar>
#include <QStyle>

// ==========================================================
// FanCardWidget Implementation
// ==========================================================

FanCardWidget::FanCardWidget(const QJsonObject &fan, QWidget *parent)
    : QFrame(parent)
    , m_isSelected(false)
{
    setObjectName("fanCard");
    setProperty("class", "FanCard");
    setProperty("selected", false);

    m_fanId = fan["Id"].toString();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    QHBoxLayout *topRow = new QHBoxLayout();
    m_nameLabel = new QLabel(fan["Name"].toString(), this);
    m_nameLabel->setObjectName("fanCardName");
    topRow->addWidget(m_nameLabel);
    topRow->addStretch(1);

    m_modeBadge = new QLabel(this);
    topRow->addWidget(m_modeBadge);
    layout->addLayout(topRow);

    QHBoxLayout *midRow = new QHBoxLayout();
    m_hardwareLabel = new QLabel(fan["HardwareName"].toString(), this);
    m_hardwareLabel->setObjectName("fanCardHardware");
    midRow->addWidget(m_hardwareLabel);
    midRow->addStretch(1);

    int rpm = fan["Rpm"].toDouble();
    m_rpmLabel = new QLabel(QString("%1 RPM").arg(rpm), this);
    m_rpmLabel->setObjectName("fanCardRpm");
    midRow->addWidget(m_rpmLabel);
    layout->addLayout(midRow);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);

    updateData(fan);
}

void FanCardWidget::updateData(const QJsonObject &fan)
{
    int rpm = fan["Rpm"].toDouble();
    int minRpm = fan["Min"].toDouble();
    int maxRpm = fan["Max"].toDouble();
    int currentDuty = fan["SpeedPercent"].toDouble();

    m_rpmLabel->setText(QString("%1 RPM").arg(rpm));

    // Update progress bar
    int progressVal = 0;
    if (maxRpm > minRpm && maxRpm > 0) {
        progressVal = qBound(0, (rpm - minRpm) * 100 / (maxRpm - minRpm), 100);
    } else if (rpm > 0) {
        progressVal = qBound(0, rpm * 100 / 3000, 100);
    }
    m_progressBar->setValue(progressVal);

    // Update badge mode
    QString mode = fan["Mode"].toString().toLower();
    QString controlId = fan["ControlId"].toString();
    if (controlId.isEmpty()) {
        m_modeBadge->setText("MONITOR ONLY");
        m_modeBadge->setObjectName("badgeMonitor");
    } else if (mode == "software") {
        m_modeBadge->setText("MANUAL");
        m_modeBadge->setObjectName("badgeManual");
    } else {
        m_modeBadge->setText("AUTO");
        m_modeBadge->setObjectName("badgeAuto");
    }

    // Refresh styling since objectName might have changed
    m_modeBadge->style()->unpolish(m_modeBadge);
    m_modeBadge->style()->polish(m_modeBadge);
}

void FanCardWidget::setSelectedState(bool selected)
{
    m_isSelected = selected;
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
}

void FanCardWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked(m_fanId);
    QFrame::mousePressEvent(event);
}

// ==========================================================
// MainWindow Implementation
// ==========================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_backendLauncher(new BackendLauncher(this))
    , m_apiClient(new FanApiClient(this))
    , m_pollTimer(new QTimer(this))
    , m_clockTimer(new QTimer(this))
    , m_isScanning(false)
    , m_consecutiveErrors(0)
    , m_pendingManualChange(false)
{
    setupUI();
    setupConnections();
    loadStylesheet();

    // Start elapsed clock
    m_elapsedTimer.start();
    m_clockTimer->start(1000);

    // Initial loading state
    setConnectionStatus("Connecting...", "#EAB308");
    m_statusLeftLabel->setText("Starting hardware backend...");

    // Start the C# background process
    m_backendLauncher->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Pankha – Fan Control");
    setMinimumSize(860, 560);
    resize(920, 620);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ==========================================
    // Left Pane (Sidebar List)
    // ==========================================
    m_leftPane = new QFrame(m_centralWidget);
    m_leftPane->setObjectName("leftPane");
    m_leftPane->setFixedWidth(320);
    
    QVBoxLayout *leftLayout = new QVBoxLayout(m_leftPane);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);

    // Logo / Header
    QHBoxLayout *logoRow = new QHBoxLayout();
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

    // Thin separator line under header
    QFrame *headerSep = new QFrame(m_leftPane);
    headerSep->setObjectName("headerSeparator");
    headerSep->setFrameShape(QFrame::HLine);
    headerSep->setFixedHeight(1);
    leftLayout->addWidget(headerSep);

    // Scroll Area for fan cards
    m_scrollArea = new QScrollArea(m_leftPane);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_listContainer = new QWidget(m_scrollArea);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(10);
    m_listLayout->addStretch(1); // keeps cards pushed to the top
    
    m_scrollArea->setWidget(m_listContainer);
    leftLayout->addWidget(m_scrollArea, 1);

    // Left pane footer
    QHBoxLayout *leftFooter = new QHBoxLayout();
    m_connectionStatusLabel = new QLabel(m_leftPane);
    leftFooter->addWidget(m_connectionStatusLabel);
    leftFooter->addStretch(1);
    
    m_rescanButton = new QPushButton("Rescan Sensors", m_leftPane);
    m_rescanButton->setObjectName("rescanButton");
    m_rescanButton->setToolTip("Query the hardware controllers and update the channel lists.");
    leftFooter->addWidget(m_rescanButton);
    leftLayout->addLayout(leftFooter);

    mainLayout->addWidget(m_leftPane);

    // ==========================================
    // Right Pane (Details & Control)
    // ==========================================
    m_rightPane = new QFrame(m_centralWidget);
    m_rightPane->setObjectName("rightPane");
    
    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPane);
    rightLayout->setContentsMargins(24, 24, 24, 24);
    rightLayout->setSpacing(16);

    // Empty state widget
    m_emptyStateWidget = new QWidget(m_rightPane);
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    m_emptyStateLabel = new QLabel("Select a fan from the list\nto monitor and configure controls.", m_emptyStateWidget);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setStyleSheet("font-size: 14px; color: #2a3d58; font-weight: 500;");
    emptyLayout->addStretch(1);
    emptyLayout->addWidget(m_emptyStateLabel);
    emptyLayout->addStretch(1);
    rightLayout->addWidget(m_emptyStateWidget, 1);

    // Detail widget
    m_detailWidget = new QWidget(m_rightPane);
    QVBoxLayout *detailLayout = new QVBoxLayout(m_detailWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(16);

    // Header info
    QVBoxLayout *headerInfo = new QVBoxLayout();
    headerInfo->setSpacing(4);
    m_detailNameLabel = new QLabel("Fan Name", m_detailWidget);
    m_detailNameLabel->setObjectName("detailTitle");
    m_detailHardwareLabel = new QLabel("Hardware Controller", m_detailWidget);
    m_detailHardwareLabel->setObjectName("detailHardware");
    headerInfo->addWidget(m_detailNameLabel);
    headerInfo->addWidget(m_detailHardwareLabel);
    detailLayout->addLayout(headerInfo);

    // Telemetry group (visual readings)
    QGroupBox *telemetryGroup = new QGroupBox("TELEMETRY", m_detailWidget);
    QVBoxLayout *telemetryLayout = new QVBoxLayout(telemetryGroup);
    telemetryLayout->setSpacing(14);

    QHBoxLayout *statsRow = new QHBoxLayout();
    
    QVBoxLayout *rpmStat = new QVBoxLayout();
    QLabel *rpmStatTitle = new QLabel("CURRENT SPEED", telemetryGroup);
    rpmStatTitle->setStyleSheet("font-size: 10px; color: #3b5070; font-weight: 700; letter-spacing: 0.5px;");
    m_rpmValueLabel = new QLabel("--- RPM", telemetryGroup);
    m_rpmValueLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: #60a5fa;");
    rpmStat->addWidget(rpmStatTitle);
    rpmStat->addWidget(m_rpmValueLabel);

    QVBoxLayout *dutyStat = new QVBoxLayout();
    QLabel *dutyStatTitle = new QLabel("DUTY CYCLE", telemetryGroup);
    dutyStatTitle->setStyleSheet("font-size: 10px; color: #3b5070; font-weight: 700; letter-spacing: 0.5px;");
    m_dutyValueLabel = new QLabel("--- %", telemetryGroup);
    m_dutyValueLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: #34d399;");
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
    ctrlLayout->setSpacing(14);

    m_manualOverrideCheck = new QCheckBox("Enable Manual Override", ctrlGroup);
    ctrlLayout->addWidget(m_manualOverrideCheck);

    // Target Speed label
    m_targetSpeedLabel = new QLabel("TARGET SPEED", ctrlGroup);
    m_targetSpeedLabel->setObjectName("sectionLabel");
    m_targetSpeedLabel->setEnabled(false);
    ctrlLayout->addWidget(m_targetSpeedLabel);

    // Target Speed Slider row
    QHBoxLayout *sliderRow = new QHBoxLayout();
    m_targetSpeedSlider = new QSlider(Qt::Horizontal, ctrlGroup);
    m_targetSpeedSlider->setRange(0, 100);
    m_targetSpeedSlider->setEnabled(false);

    m_targetSpeedSpin = new QSpinBox(ctrlGroup);
    m_targetSpeedSpin->setRange(0, 100);
    m_targetSpeedSpin->setSuffix(" %");
    m_targetSpeedSpin->setEnabled(false);
    m_targetSpeedSpin->setFixedWidth(80);

    sliderRow->addWidget(m_targetSpeedSlider, 1);
    sliderRow->addWidget(m_targetSpeedSpin);
    ctrlLayout->addLayout(sliderRow);

    // Options
    m_showRpmStatusBarCheck = new QCheckBox("Show RPM in status bar", ctrlGroup);
    m_showRpmStatusBarCheck->setChecked(true);
    ctrlLayout->addWidget(m_showRpmStatusBarCheck);

    m_resetAutoExitCheck = new QCheckBox("Reset to Auto on exit", ctrlGroup);
    m_resetAutoExitCheck->setChecked(true);
    ctrlLayout->addWidget(m_resetAutoExitCheck);

    // Advanced section (Collapsible)
    m_advancedLink = new QLabel("<a href=\"toggle\" style=\"text-decoration:none; color:#3b82f6; font-size:11px;\">▸ Show advanced options</a>", ctrlGroup);
    m_advancedLink->setCursor(Qt::PointingHandCursor);
    ctrlLayout->addWidget(m_advancedLink);

    m_advancedPanel = new QWidget(ctrlGroup);
    QFormLayout *advForm = new QFormLayout(m_advancedPanel);
    advForm->setContentsMargins(10, 4, 10, 4);
    advForm->setSpacing(8);

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

    // Apply & Reset footer
    QHBoxLayout *actionRow = new QHBoxLayout();
    m_resetAutoButton = new QPushButton("Reset to Auto", m_detailWidget);
    m_resetAutoButton->setEnabled(false);
    
    m_applyButton = new QPushButton("Apply Speed", m_detailWidget);
    m_applyButton->setObjectName("applyButton");
    m_applyButton->setEnabled(false);

    actionRow->addWidget(m_resetAutoButton);
    actionRow->addWidget(m_applyButton);
    detailLayout->addLayout(actionRow);

    rightLayout->addWidget(m_detailWidget, 1);
    m_detailWidget->setVisible(false); // Hide until selected

    mainLayout->addWidget(m_rightPane, 1);

    // Status Bar setup
    m_statusLeftLabel = new QLabel("Initializing...", this);
    m_statusRightLabel = new QLabel("00:00:00", this);
    statusBar()->addWidget(m_statusLeftLabel, 1);
    statusBar()->addPermanentWidget(m_statusRightLabel);
}

void MainWindow::setupConnections()
{
    // Backend process connection
    connect(m_backendLauncher, &BackendLauncher::started, this, &MainWindow::onBackendStarted);
    connect(m_backendLauncher, &BackendLauncher::backendError, this, &MainWindow::onBackendError);

    // UI callbacks
    connect(m_rescanButton, &QPushButton::clicked, this, &MainWindow::onRescanClicked);
    connect(m_manualOverrideCheck, &QCheckBox::toggled, this, &MainWindow::onManualOverrideToggled);
    connect(m_resetAutoExitCheck, &QCheckBox::toggled, this, &MainWindow::onResetAutoExitToggled);
    connect(m_showRpmStatusBarCheck, &QCheckBox::clicked, this, &MainWindow::updateStatusBar);
    
    // Collapsible advanced panel
    connect(m_advancedLink, &QLabel::linkActivated, this, &MainWindow::onAdvancedToggleClicked);

    // Slider and spinbox sync
    connect(m_targetSpeedSlider, &QSlider::valueChanged, this, &MainWindow::onSliderValueChanged);
    connect(m_targetSpeedSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onSpinBoxValueChanged);

    // Footer actions
    connect(m_applyButton, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
    connect(m_resetAutoButton, &QPushButton::clicked, this, &MainWindow::onResetAutoClicked);

    // Timers
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimerTick);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::onClockTimerTick);
}

void MainWindow::loadStylesheet()
{
    QFile file(":/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(file.readAll());
        file.close();
    }
}

void MainWindow::setConnectionStatus(const QString &status, const QString &color)
{
    m_connectionStatusLabel->setText(QString("<span style='color:%1; font-size:14px; font-weight:bold;'>●</span> %2")
                                     .arg(color).arg(status));
}

void MainWindow::onBackendStarted()
{
    // Backend successfully started, trigger initial scan
    onRescanClicked();
}

void MainWindow::onBackendError(const QString &message)
{
    m_pollTimer->stop();
    setConnectionStatus("Offline", "#EF4444");
    m_statusLeftLabel->setText("Backend offline.");
    
    m_rescanButton->setEnabled(true);
    showEmptyState("Backend offline.\n" + message);

    QMessageBox::critical(this, "Backend Error", message);
}

void MainWindow::onRescanClicked()
{
    m_isScanning = true;
    m_pollTimer->stop(); // halt polling during scan

    setConnectionStatus("Scanning...", "#EAB308");
    m_statusLeftLabel->setText("Scanning hardware sensors...");
    m_rescanButton->setEnabled(false);

    // Re-connect API callbacks specifically for the scan request
    disconnect(m_apiClient, &FanApiClient::fansReceived, nullptr, nullptr);
    disconnect(m_apiClient, &FanApiClient::apiError, nullptr, nullptr);

    connect(m_apiClient, &FanApiClient::fansReceived, this, &MainWindow::onScanResponse);
    connect(m_apiClient, &FanApiClient::apiError, this, &MainWindow::onApiError);

    m_apiClient->getFans();
}

void MainWindow::onScanResponse(const QJsonArray &fans)
{
    m_isScanning = false;
    m_rescanButton->setEnabled(true);
    m_consecutiveErrors = 0;
    m_fansCache = fans;

    // Disconnect scan handlers
    disconnect(m_apiClient, &FanApiClient::fansReceived, this, &MainWindow::onScanResponse);

    // Connect standard poll handlers
    connect(m_apiClient, &FanApiClient::fansReceived, this, &MainWindow::onPollResponse);

    // Clear old card map and widgets
    for (auto card : m_fanCards.values()) {
        m_listLayout->removeWidget(card);
        delete card;
    }
    m_fanCards.clear();
    
    // Clear layout completely (except the stretch)
    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        delete child;
    }
    // Re-add stretch at the bottom
    m_listLayout->addStretch(1);

    if (fans.isEmpty()) {
        showEmptyState("No fan sensors found.\nTry running as Administrator.");
        setConnectionStatus("Online", "#10B981");
        m_statusLeftLabel->setText("No fan sensors found.");
        return;
    }

    // Populate new cards
    for (const QJsonValue &val : fans) {
        QJsonObject fan = val.toObject();
        QString fanId = fan["Id"].toString();
        
        FanCardWidget *card = new FanCardWidget(fan, m_listContainer);
        // Insert card widget BEFORE the stretch at the bottom!
        m_listLayout->insertWidget(m_listLayout->count() - 1, card);
        m_fanCards.insert(fanId, card);

        // Connect card click signal
        connect(card, &FanCardWidget::clicked, this, &MainWindow::onFanCardClicked);
    }

    // Re-select previously selected fan if it still exists
    if (!m_selectedFanId.isEmpty() && m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(true);
        showDetailsPanel();
        updateUIWithSelectedFan(true);
    } else {
        m_selectedFanId = "";
        showEmptyState("Select a fan from the list\nto monitor and configure controls.");
    }

    // Start live 2-second updates
    setConnectionStatus("Online", "#10B981");
    m_pollTimer->start(2000);
}

void MainWindow::onFanCardClicked(const QString &fanId)
{
    if (m_selectedFanId == fanId) return;

    // Deselect old card
    if (!m_selectedFanId.isEmpty() && m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(false);
    }

    m_selectedFanId = fanId;

    // Select new card
    if (m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(true);
        showDetailsPanel();
        updateUIWithSelectedFan(true);
    }
}

void MainWindow::onPollTimerTick()
{
    m_apiClient->getFans();
}

void MainWindow::onPollResponse(const QJsonArray &fans)
{
    m_consecutiveErrors = 0;
    m_fansCache = fans;
    setConnectionStatus("Online", "#10B981");

    // Update each card in-place (no recreations)
    for (const QJsonValue &val : fans) {
        QJsonObject fan = val.toObject();
        QString fanId = fan["Id"].toString();
        if (m_fanCards.contains(fanId)) {
            m_fanCards[fanId]->updateData(fan);
        }
    }

    // Perform an in-place values-only update for the details panel
    updateUIWithSelectedFan(false);
}

void MainWindow::onApiError(const QString &message)
{
    m_consecutiveErrors++;
    qDebug() << "API Error (" << m_consecutiveErrors << "):" << message;

    if (m_consecutiveErrors >= 3) {
        setConnectionStatus("Offline", "#EF4444");
        m_statusLeftLabel->setText("Connection lost. Reconnecting...");
        
        m_applyButton->setEnabled(false);
        m_resetAutoButton->setEnabled(false);
    }
}

void MainWindow::showEmptyState(const QString &message)
{
    m_emptyStateLabel->setText(message);
    m_emptyStateWidget->setVisible(true);
    m_detailWidget->setVisible(false);
}

void MainWindow::showDetailsPanel()
{
    m_emptyStateWidget->setVisible(false);
    m_detailWidget->setVisible(true);
}

void MainWindow::updateUIWithSelectedFan(bool fullRefresh)
{
    if (m_selectedFanId.isEmpty()) {
        showEmptyState("Select a fan from the list\nto monitor and configure controls.");
        return;
    }

    // Find select object in cache
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

    // Live monitor metrics (always update on every tick)
    m_rpmValueLabel->setText(QString("%1 RPM").arg(rpm));
    m_dutyValueLabel->setText(QString("%1 %").arg(currentDuty));

    // Update progress bar
    int progressVal = 0;
    if (maxRpm > minRpm && maxRpm > 0) {
        progressVal = qBound(0, (rpm - minRpm) * 100 / (maxRpm - minRpm), 100);
    } else if (rpm > 0) {
        progressVal = qBound(0, rpm * 100 / 3000, 100);
    }
    m_progressBar->setValue(progressVal);

    bool hasControl = !m_selectedControlId.isEmpty();

    // Full refresh: only executed on user selection changes, NOT during background polls!
    if (fullRefresh) {
        // Switching to a new fan: clear any pending user change
        m_pendingManualChange = false;

        m_detailNameLabel->setText(selectedFan["Name"].toString() + (hasControl ? "" : " (Monitor Only)"));
        m_detailHardwareLabel->setText(selectedFan["HardwareName"].toString());
        
        QString displayPath = m_selectedFanId;
        m_hardwarePathEdit->setText(displayPath.replace('_', '/'));
        m_minRpmSpin->setValue(minRpm);
        m_maxRpmSpin->setValue(maxRpm);
        m_controlIdEdit->setText(m_selectedControlId);

        m_manualOverrideCheck->setEnabled(hasControl);
        m_resetAutoButton->setEnabled(hasControl);

        if (hasControl) {
            QString mode = selectedFan["Mode"].toString().toLower();
            bool isManual = (mode == "software");
            bool resetOnExit = selectedFan["ResetOnExit"].toBool(true);
            
            m_manualOverrideCheck->blockSignals(true);
            m_manualOverrideCheck->setChecked(isManual);
            m_manualOverrideCheck->blockSignals(false);

            m_resetAutoExitCheck->setEnabled(isManual);
            m_resetAutoExitCheck->blockSignals(true);
            m_resetAutoExitCheck->setChecked(resetOnExit);
            m_resetAutoExitCheck->blockSignals(false);

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
        // Poll refresh: only update settings if the user is NOT actively editing or focused on them!
        if (hasControl) {
            m_manualOverrideCheck->setEnabled(true);
            m_resetAutoButton->setEnabled(m_consecutiveErrors < 3);

            QString mode = selectedFan["Mode"].toString().toLower();
            bool isManual = (mode == "software");

            // Only sync the checkbox from the backend when there is no pending
            // local change (i.e. the user hasn't toggled the override manually
            // without applying it yet).
            if (!m_pendingManualChange) {
                m_manualOverrideCheck->blockSignals(true);
                m_manualOverrideCheck->setChecked(isManual);
                m_manualOverrideCheck->blockSignals(false);

                m_resetAutoExitCheck->setEnabled(isManual);
                bool resetOnExit = selectedFan["ResetOnExit"].toBool(true);
                m_resetAutoExitCheck->blockSignals(true);
                m_resetAutoExitCheck->setChecked(resetOnExit);
                m_resetAutoExitCheck->blockSignals(false);

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

    // Find item in cache
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
        leftText = QString("Fan: %1 | %2 RPM | %3").arg(selectedFan["Name"].toString()).arg(rpm).arg(mode);
    } else {
        leftText = QString("Fan: %1 | %2").arg(selectedFan["Name"].toString()).arg(mode);
    }
    m_statusLeftLabel->setText(leftText);
}

void MainWindow::onSliderValueChanged(int value)
{
    m_targetSpeedSpin->blockSignals(true);
    m_targetSpeedSpin->setValue(value);
    m_targetSpeedSpin->blockSignals(false);
}

void MainWindow::onSpinBoxValueChanged(int value)
{
    m_targetSpeedSlider->blockSignals(true);
    m_targetSpeedSlider->setValue(value);
    m_targetSpeedSlider->blockSignals(false);
}

void MainWindow::onManualOverrideToggled(bool checked)
{
    // Mark that the user has a pending local change so background polls
    // don't overwrite this checkbox state before the user applies/resets.
    m_pendingManualChange = true;
    m_targetSpeedLabel->setEnabled(checked);
    m_targetSpeedSlider->setEnabled(checked);
    m_targetSpeedSpin->setEnabled(checked);
    m_resetAutoExitCheck->setEnabled(checked);
    m_applyButton->setEnabled(checked && m_consecutiveErrors < 3);
}

void MainWindow::onResetAutoExitToggled(bool checked)
{
    Q_UNUSED(checked);
    if (m_manualOverrideCheck->isChecked()) {
        m_pendingManualChange = true;
        m_applyButton->setEnabled(m_consecutiveErrors < 3);
    }
}

void MainWindow::onApplyClicked()
{
    if (m_selectedControlId.isEmpty() || !m_manualOverrideCheck->isChecked()) return;

    int targetSpeed = m_targetSpeedSpin->value();

    if (targetSpeed == 0) {
        QMessageBox::StandardButton res = QMessageBox::warning(this, "Safety Warning",
            "Setting the fan speed to 0% stops airflow, which could overheat components.\n\nAre you sure you want to apply this speed?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (res == QMessageBox::No) {
            return;
        }
    }

    // Connect temporary handler to capture success feedback
    disconnect(m_apiClient, &FanApiClient::controlApplied, nullptr, nullptr);
    connect(m_apiClient, &FanApiClient::controlApplied, this, &MainWindow::onControlApplied);

    m_apiClient->setManualSpeed(m_selectedControlId, targetSpeed, m_resetAutoExitCheck->isChecked());
}

void MainWindow::onControlApplied(const QString &id, bool success)
{
    Q_UNUSED(id);
    disconnect(m_apiClient, &FanApiClient::controlApplied, this, &MainWindow::onControlApplied);
    m_pendingManualChange = false;

    if (success) {
        statusBar()->showMessage("Override applied successfully.", 3000);
        
        // Trigger immediate fetch to verify state on UI
        m_apiClient->getFans();
    } else {
        QMessageBox::critical(this, "Operation Failed", "Could not apply manual override to this fan channel.");
    }
}

void MainWindow::onResetAutoClicked()
{
    if (m_selectedControlId.isEmpty()) return;

    disconnect(m_apiClient, &FanApiClient::controlReset, nullptr, nullptr);
    connect(m_apiClient, &FanApiClient::controlReset, this, &MainWindow::onControlReset);

    m_apiClient->setAuto(m_selectedControlId);
}

void MainWindow::onControlReset(const QString &id, bool success)
{
    Q_UNUSED(id);
    disconnect(m_apiClient, &FanApiClient::controlReset, this, &MainWindow::onControlReset);
    m_pendingManualChange = false;

    if (success) {
        statusBar()->showMessage("Restored BIOS auto control.", 3000);
        m_manualOverrideCheck->setChecked(false);
        m_targetSpeedLabel->setEnabled(false);
        m_targetSpeedSlider->setEnabled(false);
        m_targetSpeedSpin->setEnabled(false);
        m_applyButton->setEnabled(false);

        // Fetch verification state
        m_apiClient->getFans();
    } else {
        QMessageBox::critical(this, "Operation Failed", "Could not restore automatic BIOS control for this channel.");
    }
}

void MainWindow::onAdvancedToggleClicked(const QString &link)
{
    Q_UNUSED(link);
    if (m_advancedPanel->isVisible()) {
        m_advancedPanel->setVisible(false);
        m_advancedLink->setText("<a href=\"toggle\" style=\"text-decoration:none; color:#3b82f6; font-size:11px;\">▸ Show advanced options</a>");
    } else {
        m_advancedPanel->setVisible(true);
        m_advancedLink->setText("<a href=\"toggle\" style=\"text-decoration:none; color:#3b82f6; font-size:11px;\">▾ Hide advanced options</a>");
    }
}

void MainWindow::onClockTimerTick()
{
    qint64 ms = m_elapsedTimer.elapsed();
    int secs = (ms / 1000) % 60;
    int mins = (ms / 60000) % 60;
    int hours = (ms / 3600000);
    
    m_statusRightLabel->setText(QString("%1:%2:%3")
                                .arg(hours, 2, 10, QChar('0'))
                                .arg(mins, 2, 10, QChar('0'))
                                .arg(secs, 2, 10, QChar('0')));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_pollTimer->stop();
    m_clockTimer->stop();

    if (m_resetAutoExitCheck->isChecked() && !m_selectedControlId.isEmpty()) {
        // Rely on backend shutdown cleaner (stdin closed),
        // which resets all software controls to auto automatically.
    }
    
    m_backendLauncher->stop();
    event->accept();
}
