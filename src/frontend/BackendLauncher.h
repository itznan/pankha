#ifndef BACKENDLAUNCHER_H
#define BACKENDLAUNCHER_H

#include <QObject>
#include <QProcess>
#include <QString>

class BackendLauncher : public QObject
{
    Q_OBJECT
public:
    explicit BackendLauncher(QObject *parent = nullptr);
    ~BackendLauncher();

    void start();
    void stop();
    bool isBackendRunning();

signals:
    void started();
    void backendError(const QString &message);
    void finished();

private slots:
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleError(QProcess::ProcessError error);

private:
    QProcess *m_process;
    bool m_isShuttingDown;
    int m_retryCount;
    const int m_maxRetries = 3;

    void killAllBackendProcesses();
};

#endif // BACKENDLAUNCHER_H
