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
#include "FanApiClient.h"
#include "BackendLauncher.h"

// Forward declarations
class FanCardWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // API Callbacks
    void onScanResponse(const QJsonArray &fans);
    void onPollResponse(const QJsonArray &fans);
    void onControlApplied(const QString &id, bool success);
    void onControlReset(const QString &id, bool success);
    void onApiError(const QString &message);

    // Backend launcher callbacks
    void onBackendStarted();
    void onBackendError(const QString &message);

    // UI Trigger actions
    void onRescanClicked();
    void onFanCardClicked(const QString &fanId);
    void onManualOverrideToggled(bool checked);
    void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(int value);
    void onApplyClicked();
    void onResetAutoClicked();
    void onResetAutoExitToggled(bool checked);
    void onAdvancedToggleClicked(const QString &link);
    void onThemeChanged(int index);
    void onSettingsClicked();
    void onPollIntervalChanged(int value);
    
    // Timer handlers
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

    // UI Structure
    QWidget *m_centralWidget;
    
    // Left Pane (List)
    QFrame *m_leftPane;
    QScrollArea *m_scrollArea;
    QWidget *m_listContainer;
    QVBoxLayout *m_listLayout;
    QPushButton *m_rescanButton;
    QLabel *m_connectionStatusLabel;
    QPushButton *m_settingsButton;

    // Right Pane (Details)
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
    
    QCheckBox *m_showRpmStatusBarCheck;
    QCheckBox *m_resetAutoExitCheck;

    QLabel *m_targetSpeedLabel;

    QLabel *m_advancedLink;
    QWidget *m_advancedPanel;
    QLineEdit *m_hardwarePathEdit;
    QSpinBox *m_minRpmSpin;
    QSpinBox *m_maxRpmSpin;
    QLineEdit *m_controlIdEdit;

    QPushButton *m_resetAutoButton;
    QPushButton *m_applyButton;

    // Status bar labels
    QLabel *m_statusLeftLabel;
    QLabel *m_statusRightLabel;

    // Global Settings Page
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
    // True when the user has toggled manual override locally but hasn't applied yet.
    // Prevents the background poll from overwriting the checkbox state.
    bool m_pendingManualChange;
};

// FanCardWidget displays individual fan information in the list
class FanCardWidget : public QFrame
{
    Q_OBJECT
public:
    FanCardWidget(const QJsonObject &fan, QWidget *parent = nullptr);
    void updateData(const QJsonObject &fan);
    void setSelectedState(bool selected);
    QString fanId() const { return m_fanId; }

signals:
    void clicked(const QString &fanId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_fanId;
    QLabel *m_nameLabel;
    QLabel *m_hardwareLabel;
    QLabel *m_rpmLabel;
    QLabel *m_modeBadge;
    QProgressBar *m_progressBar;
    bool m_isSelected;
};

#endif // MAINWINDOW_H
