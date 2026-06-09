#ifndef FANAPICLIENT_H
#define FANAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

class FanApiClient : public QObject
{
    Q_OBJECT
public:
    explicit FanApiClient(QObject *parent = nullptr);

    void getFans();
    void setManualSpeed(const QString &id, int speedPercent, bool resetOnExit);
    void setAuto(const QString &id);

signals:
    void fansReceived(const QJsonArray &fans);
    void controlApplied(const QString &id, bool success);
    void controlReset(const QString &id, bool success);
    void apiError(const QString &message);

private:
    QNetworkAccessManager *m_nam;
    QString m_baseUrl;
};

#endif // FANAPICLIENT_H
