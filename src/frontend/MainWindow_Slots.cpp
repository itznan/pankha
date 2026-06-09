#include "MainWindow.h"
#include "components/FanCardWidget.h"
#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>

void MainWindow::onBackendStarted()
{
    QTimer::singleShot(500, this, &MainWindow::onRescanClicked);
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
    m_pollTimer->stop();

    setConnectionStatus("Scanning...", "#EAB308");
    m_statusLeftLabel->setText("Scanning hardware sensors...");
    m_rescanButton->setEnabled(false);

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

    disconnect(m_apiClient, &FanApiClient::fansReceived, this, &MainWindow::onScanResponse);
    connect(m_apiClient, &FanApiClient::fansReceived, this, &MainWindow::onPollResponse);

    for (auto card : m_fanCards.values()) {
        m_listLayout->removeWidget(card);
        delete card;
    }
    m_fanCards.clear();

    QLayoutItem *child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        delete child;
    }
    m_listLayout->addStretch(1);

    if (fans.isEmpty()) {
        showEmptyState("No fan sensors found.\nTry running as Administrator.");
        setConnectionStatus("Online", "#10B981");
        m_statusLeftLabel->setText("No fan sensors found.");
        return;
    }

    for (const QJsonValue &val : fans) {
        QJsonObject fan = val.toObject();
        QString fanId = fan["Id"].toString();

        FanCardWidget *card = new FanCardWidget(fan, m_listContainer);
        m_listLayout->insertWidget(m_listLayout->count() - 1, card);
        m_fanCards.insert(fanId, card);

        connect(card, &FanCardWidget::clicked, this, &MainWindow::onFanCardClicked);
    }

    if (m_settingsScrollArea->isVisible()) {
        m_selectedFanId = "";
        m_selectedControlId = "";
    } else if (!m_selectedFanId.isEmpty() && m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(true);
        showDetailsPanel();
        updateUIWithSelectedFan(true);
    } else {
        m_selectedFanId = "";
        m_selectedControlId = "";
        showEmptyState("Select a fan from the list\nto monitor and configure controls.");
    }

    setConnectionStatus("Online", "#10B981");
    m_pollTimer->start(m_pollInterval);
}

void MainWindow::onFanCardClicked(const QString &fanId)
{
    if (m_selectedFanId == fanId) return;

    if (!m_selectedFanId.isEmpty() && m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(false);
    }

    m_selectedFanId = fanId;

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

    for (const QJsonValue &val : fans) {
        QJsonObject fan = val.toObject();
        QString fanId = fan["Id"].toString();
        if (m_fanCards.contains(fanId)) {
            m_fanCards[fanId]->updateData(fan);
        }
    }

    updateUIWithSelectedFan(false);
}

void MainWindow::onApiError(const QString &message)
{
    m_consecutiveErrors++;
    qDebug() << "API Error (" << m_consecutiveErrors << "):" << message;

    if (m_isScanning) {
        m_isScanning = false;
        m_rescanButton->setEnabled(true);
    }

    if (m_consecutiveErrors >= 3) {
        setConnectionStatus("Offline", "#EF4444");
        m_statusLeftLabel->setText("Connection lost. Click \"Rescan\" to retry.");
        m_applyButton->setEnabled(false);
    }
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
    m_pendingManualChange = true;
    m_targetSpeedLabel->setEnabled(checked);
    m_targetSpeedSlider->setEnabled(checked);
    m_targetSpeedSpin->setEnabled(checked);
    m_applyButton->setEnabled(checked && m_consecutiveErrors < 3);

    if (!checked && !m_selectedControlId.isEmpty()) {
        disconnect(m_apiClient, &FanApiClient::controlReset, nullptr, nullptr);
        connect(m_apiClient, &FanApiClient::controlReset, this, &MainWindow::onControlReset);
        m_apiClient->setAuto(m_selectedControlId);
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
        if (res == QMessageBox::No) return;
    }

    m_pendingManualChange = true;
    m_applyButton->setEnabled(false);

    disconnect(m_apiClient, &FanApiClient::controlApplied, nullptr, nullptr);
    connect(m_apiClient, &FanApiClient::controlApplied, this, &MainWindow::onControlApplied);

    m_apiClient->setManualSpeed(m_selectedControlId, targetSpeed, false);
}

void MainWindow::onControlApplied(const QString &id, bool success)
{
    Q_UNUSED(id);
    disconnect(m_apiClient, &FanApiClient::controlApplied, this, &MainWindow::onControlApplied);
    m_pendingManualChange = false;

    if (success) {
        statusBar()->showMessage("Override applied successfully.", 3000);
        m_apiClient->getFans();
    } else {
        QMessageBox::critical(this, "Operation Failed", "Could not apply manual override to this fan channel.");
    }
}

void MainWindow::onControlReset(const QString &id, bool success)
{
    Q_UNUSED(id);
    disconnect(m_apiClient, &FanApiClient::controlReset, this, &MainWindow::onControlReset);
    m_pendingManualChange = false;

    if (success) {
        statusBar()->showMessage("Fan returned to automatic control.", 3000);
        m_apiClient->getFans();
    } else {
        QMessageBox::critical(this, "Operation Failed", "Could not reset this fan to automatic control.");
        m_manualOverrideCheck->blockSignals(true);
        m_manualOverrideCheck->setChecked(true);
        m_manualOverrideCheck->blockSignals(false);
        m_targetSpeedLabel->setEnabled(true);
        m_targetSpeedSlider->setEnabled(true);
        m_targetSpeedSpin->setEnabled(true);
        m_applyButton->setEnabled(true);
    }
}

void MainWindow::onAdvancedToggleClicked(const QString &link)
{
    Q_UNUSED(link);
    if (m_advancedPanel->isVisible()) {
        m_advancedPanel->setVisible(false);
        m_advancedLink->setText("<a href=\"toggle\" style=\"text-decoration:none; color:#007aff; font-size:11px;\">Show advanced options</a>");
    } else {
        m_advancedPanel->setVisible(true);
        m_advancedLink->setText("<a href=\"toggle\" style=\"text-decoration:none; color:#007aff; font-size:11px;\">Hide advanced options</a>");
    }
}

void MainWindow::onThemeChanged(int index)
{
    Q_UNUSED(index);
    loadStylesheet();
    saveSettings();
}

void MainWindow::onSettingsClicked()
{
    if (!m_selectedFanId.isEmpty() && m_fanCards.contains(m_selectedFanId)) {
        m_fanCards[m_selectedFanId]->setSelectedState(false);
    }
    m_selectedFanId = "";
    m_selectedControlId = "";

    m_emptyStateWidget->setVisible(false);
    m_detailScrollArea->setVisible(false);
    m_settingsScrollArea->setVisible(true);
    animateFadeIn(m_settingsScrollArea);
}

void MainWindow::onPollIntervalChanged(int value)
{
    m_pollInterval = value;
    if (m_pollTimer->isActive()) {
        m_pollTimer->start(m_pollInterval);
    }
    saveSettings();
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
