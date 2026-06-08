#include "FanApiClient.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

FanApiClient::FanApiClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_baseUrl("http://localhost:5555")
{
}

void FanApiClient::getFans()
{
    QUrl url(m_baseUrl + "/fans");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("API Connection Error: %1").arg(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            emit apiError(QString("JSON Parse Error: %1").arg(parseError.errorString()));
            return;
        }

        if (doc.isArray()) {
            emit fansReceived(doc.array());
        } else {
            emit apiError("Invalid JSON response: expected array.");
        }
    });
}

void FanApiClient::setManualSpeed(const QString &id, int speedPercent, bool resetOnExit)
{
    QUrl url(QString("%1/controls/%2").arg(m_baseUrl).arg(id));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["mode"] = "manual";
    body["speed"] = speedPercent;
    body["resetOnExit"] = resetOnExit;

    QByteArray postData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_nam->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("Failed to apply manual control: %1").arg(reply->errorString()));
            emit controlApplied(id, false);
            return;
        }
        emit controlApplied(id, true);
    });
}

void FanApiClient::setAuto(const QString &id)
{
    QUrl url(QString("%1/controls/%2/auto").arg(m_baseUrl).arg(id));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_nam->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit apiError(QString("Failed to reset to auto: %1").arg(reply->errorString()));
            emit controlReset(id, false);
            return;
        }
        emit controlReset(id, true);
    });
}
