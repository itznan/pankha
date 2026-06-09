#include "BackendLauncher.h"
#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <windows.h>
#include <tlhelp32.h>

static bool isProcessNameRunning(const QString &processName) {
    bool exists = false;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                QString name = QString::fromWCharArray(entry.szExeFile);
                if (name.compare(processName, Qt::CaseInsensitive) == 0) {
                    exists = true;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return exists;
}

BackendLauncher::BackendLauncher(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isShuttingDown(false)
    , m_retryCount(0)
{
    connect(m_process, &QProcess::finished, this, &BackendLauncher::handleFinished);
    connect(m_process, &QProcess::errorOccurred, this, &BackendLauncher::handleError);
}

BackendLauncher::~BackendLauncher()
{
    stop();
}

bool BackendLauncher::isBackendRunning()
{
    return isProcessNameRunning("FanControlBackend.exe");
}

void BackendLauncher::start()
{
    m_isShuttingDown = false;

    if (isBackendRunning()) {
        qDebug() << "FanControlBackend.exe is already running. Using existing instance.";
        emit started();
        return;
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString backendPath = appDir + "/FanControlBackend.exe";

    if (!QFile::exists(backendPath)) {
        // Fallback for development
        backendPath = appDir + "/../bin/Release/net8.0/win-x64/publish/FanControlBackend.exe";
    }

    if (!QFile::exists(backendPath)) {
        backendPath = appDir + "/FanControlBackend/FanControlBackend.exe";
    }

    qDebug() << "Starting backend from:" << backendPath;

    if (!QFile::exists(backendPath)) {
        emit backendError(QString("Backend executable not found at: %1").arg(backendPath));
        return;
    }

    m_process->start(backendPath);
    if (m_process->waitForStarted(2000)) {
        emit started();
    } else {
        emit backendError("Timed out waiting for backend to start.");
    }
}

void BackendLauncher::stop()
{
    m_isShuttingDown = true;

    // First, try graceful shutdown of our managed process
    if (m_process && m_process->state() != QProcess::NotRunning) {
        // Close standard input to trigger clean C# backend exit
        m_process->closeWriteChannel();
        
        m_process->terminate();
        if (!m_process->waitForFinished(2000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }

    // Then, kill ANY remaining FanControlBackend.exe processes by name
    // This handles orphaned processes or pre-existing instances
    killAllBackendProcesses();
}

void BackendLauncher::killAllBackendProcesses()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            QString name = QString::fromWCharArray(entry.szExeFile);
            if (name.compare("FanControlBackend.exe", Qt::CaseInsensitive) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess) {
                    qDebug() << "Terminating orphan backend process PID:" << entry.th32ProcessID;
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
}

void BackendLauncher::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Backend finished with code" << exitCode << "status" << exitStatus;
    if (m_isShuttingDown) {
        emit finished();
        return;
    }

    if (exitCode == 1) {
        emit backendError("Backend failed to start: LibreHardwareMonitor requires Administrator privileges.");
        return;
    }

    if (exitCode == 3) {
        emit backendError("Backend failed to start: Port 5555 is already in use by another application.");
        return;
    }

    if (m_retryCount < m_maxRetries) {
        m_retryCount++;
        qDebug() << "Backend stopped unexpectedly. Restarting... Attempt" << m_retryCount;
        start();
    } else {
        emit backendError("Backend process crashed repeatedly and could not be restarted.");
    }
}

void BackendLauncher::handleError(QProcess::ProcessError error)
{
    qDebug() << "Backend QProcess error occurred:" << error;
    if (!m_isShuttingDown && error == QProcess::FailedToStart) {
        emit backendError("Failed to start backend process. Check permissions and paths.");
    }
}
