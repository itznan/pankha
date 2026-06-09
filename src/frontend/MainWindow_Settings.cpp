#include "MainWindow.h"
#include <QSettings>
#include <QDir>
#include <QCoreApplication>

void MainWindow::saveSettings()
{
    QSettings settings("itznan", "Pankha");
    settings.setValue("theme", m_themeComboBox->currentIndex());
    settings.setValue("showRpm", m_showRpmStatusBarCheck->isChecked());
    settings.setValue("pollInterval", m_pollInterval);
    settings.setValue("minimizeToTray", m_minimizeToTrayCheck->isChecked());
    settings.setValue("startOnBoot", m_startOnBootCheck->isChecked());
}

void MainWindow::loadSettings()
{
    QSettings settings("itznan", "Pankha");

    int theme = settings.value("theme", 0).toInt();
    m_themeComboBox->setCurrentIndex(theme);
    loadStylesheet();

    bool showRpm = settings.value("showRpm", true).toBool();
    m_showRpmStatusBarCheck->setChecked(showRpm);

    int interval = settings.value("pollInterval", 2000).toInt();
    m_pollIntervalSpin->setValue(interval);
    m_pollInterval = interval;

    bool minToTray = settings.value("minimizeToTray", true).toBool();
    m_minimizeToTrayCheck->setChecked(minToTray);

    bool startBoot = settings.value("startOnBoot", false).toBool();
    m_startOnBootCheck->setChecked(startBoot);
}

void MainWindow::setStartOnBoot(bool enabled)
{
#ifdef Q_OS_WIN
    QSettings registrySettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enabled) {
        QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        registrySettings.setValue("PankhaFanControl", "\"" + appPath + "\" --startup");
    } else {
        registrySettings.remove("PankhaFanControl");
    }
#else
    Q_UNUSED(enabled);
#endif
}
