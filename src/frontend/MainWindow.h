#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMap>
#include <QLabel>
#include <QProgressBar>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QComboBox>
#include <QMouseEvent>
#include <QPoint>
#include <QSystemTrayIcon>
#include <QMenu>
#include "FanApiClient.h"
#include "BackendLauncher.h"

class FanCardWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(bool startMinimized = false, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onScanResponse(const QJsonArray &fans);
    void onPollResponse(const QJsonArray &fans);
    void onControlApplied(const QString &id, bool success);
    void onControlReset(const QString &id, bool success);
    void onApiError(const QString &message);

    void onBackendStarted();
    void onBackendError(const QString &message);

    void onRescanClicked();
    void onFanCardClicked(const QString &fanId);
    void onManualOverrideToggled(bool checked);
    void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(int value);
    void onApplyClicked();
    void onResetToAutoClicked();
    void onAdvancedToggleClicked(const QString &link);
    void onThemeChanged(int index);
    void onSettingsClicked();
    void onPollIntervalChanged(int value);
    void onMinimizeToTrayToggled(bool checked);
    void onStartOnBootToggled(bool checked);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onQuitActionTriggered();
    void showNormalAndActivate();

    void onPollTimerTick();
    void onClockTimerTick();
    void updateStatusBar();

private:
    void setupUI();
    void setupConnections();
    void loadStylesheet();
    void setConnectionStatus(const QString &status, const QString &color);
    void updateUIWithSelectedFan(bool fullRefresh);
    void showEmptyState(const QString &message);
    void showDetailsPanel();
    void animateFadeIn(QWidget *widget);
    void saveSettings();
    void loadSettings();
    void setStartOnBoot(bool enabled);

    // UI Structure
    QWidget *m_centralWidget;

    // Left Pane
    QFrame *m_leftPane;
    QScrollArea *m_scrollArea;
    QWidget *m_listContainer;
    QVBoxLayout *m_listLayout;
    QPushButton *m_rescanButton;
    QLabel *m_connectionStatusLabel;
    QPushButton *m_settingsButton;

    // Right Pane
    QFrame *m_rightPane;
    QWidget *m_emptyStateWidget;
    QLabel *m_emptyStateLabel;

    QScrollArea *m_detailScrollArea;
    QWidget *m_detailWidget;
    QLabel *m_detailNameLabel;
    QLabel *m_detailHardwareLabel;

    QLabel *m_rpmValueLabel;
    QLabel *m_dutyValueLabel;
    QProgressBar *m_progressBar;

    QCheckBox *m_manualOverrideCheck;
    QSlider *m_targetSpeedSlider;
    QSpinBox *m_targetSpeedSpin;
    QLabel *m_targetSpeedLabel;

    QLabel *m_advancedLink;
    QWidget *m_advancedPanel;
    QLineEdit *m_hardwarePathEdit;
    QSpinBox *m_minRpmSpin;
    QSpinBox *m_maxRpmSpin;
    QLineEdit *m_controlIdEdit;

    QPushButton *m_applyButton;
    QPushButton *m_resetButton;

    // Settings
    QCheckBox *m_showRpmStatusBarCheck;
    QCheckBox *m_minimizeToTrayCheck;
    QCheckBox *m_startOnBootCheck;

    // Status bar
    QLabel *m_statusLeftLabel;
    QLabel *m_statusRightLabel;
    QSystemTrayIcon *m_trayIcon;

    // Settings Page
    QScrollArea *m_settingsScrollArea;
    QWidget *m_settingsWidget;
    QComboBox *m_themeComboBox;
    QSpinBox *m_pollIntervalSpin;
    int m_pollInterval;

    // Controllers
    BackendLauncher *m_backendLauncher;
    FanApiClient *m_apiClient;
    QTimer *m_pollTimer;
    QTimer *m_clockTimer;
    QElapsedTimer m_elapsedTimer;

    // State
    QJsonArray m_fansCache;
    QMap<QString, FanCardWidget*> m_fanCards;
    QString m_selectedFanId;
    QString m_selectedControlId;
    bool m_isScanning;
    int m_consecutiveErrors;
    bool m_pendingManualChange;
    bool m_forceClose;

    // Window dragging
    bool m_dragActive;
    QPoint m_dragPosition;
};

#endif // MAINWINDOW_H
