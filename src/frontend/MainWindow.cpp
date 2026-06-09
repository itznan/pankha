#include "MainWindow.h"
#include <QCloseEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPainterPath>
#include <QRegion>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#endif

MainWindow::MainWindow(bool startMinimized, QWidget *parent)
    : QMainWindow(parent)
    , m_backendLauncher(new BackendLauncher(this))
    , m_apiClient(new FanApiClient(this))
    , m_pollTimer(new QTimer(this))
    , m_clockTimer(new QTimer(this))
    , m_isScanning(false)
    , m_consecutiveErrors(0)
    , m_pendingManualChange(false)
    , m_pollInterval(2000)
    , m_dragActive(false)
    , m_forceClose(false)
    , m_useNativeRoundedCorners(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI();
    setupConnections();
    loadSettings();
    applyRoundedCorners();

    m_elapsedTimer.start();
    m_clockTimer->start(1000);

    setConnectionStatus("Connecting...", "#EAB308");
    m_statusLeftLabel->setText("Starting hardware backend...");

    m_backendLauncher->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_minimizeToTrayCheck->isChecked() && !m_forceClose) {
        hide();
        event->ignore();
    } else {
        m_pollTimer->stop();
        m_clockTimer->stop();
        m_trayIcon->hide();
        m_backendLauncher->stop();
        event->accept();
        // Ensure the application fully exits after the window closes
        QApplication::quit();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->position().y() < 60) {
        m_dragActive = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    } else {
        QMainWindow::mousePressEvent(event);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragActive && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    } else {
        QMainWindow::mouseMoveEvent(event);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragActive = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::animateFadeIn(QWidget *widget)
{
    QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (effect) {
        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", widget);
        anim->setDuration(200);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void MainWindow::onMinimizeToTrayToggled(bool checked)
{
    Q_UNUSED(checked);
    saveSettings();
}

void MainWindow::onStartOnBootToggled(bool checked)
{
    setStartOnBoot(checked);
    saveSettings();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormalAndActivate();
        }
    }
}

void MainWindow::onQuitActionTriggered()
{
    m_forceClose = true;
    m_pollTimer->stop();
    m_clockTimer->stop();
    m_trayIcon->hide();
    m_backendLauncher->stop();
    QApplication::quit();
}

void MainWindow::showNormalAndActivate()
{
    showNormal();
    activateWindow();
    raise();
}

void MainWindow::applyRoundedCorners()
{
#ifdef Q_OS_WIN
    // Try native Windows 11 rounded corners via DWM API
    HWND hwnd = reinterpret_cast<HWND>(winId());
    int preference = DWMWCP_ROUND;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
    if (SUCCEEDED(hr)) {
        m_useNativeRoundedCorners = true;
        return;
    }
#endif
    // Fallback: apply a QPainterPath region mask (works on Windows 10)
    m_useNativeRoundedCorners = false;
    updateRoundedMask();
}

void MainWindow::updateRoundedMask()
{
    const int radius = 12;
    QPainterPath path;
    path.addRoundedRect(rect(), radius, radius);
    setMask(path.toFillPolygon().toPolygon());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!m_useNativeRoundedCorners) {
        updateRoundedMask();
    }
}
