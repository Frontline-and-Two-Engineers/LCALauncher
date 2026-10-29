#include "ChannelPostsBridge.h"
#include "SettingsBridge.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

namespace LCA::Bridge {

ChannelPostsBridge::ChannelPostsBridge(SettingsBridge* settingsBridge, QObject* parent)
    : QObject(parent)
    , m_settingsBridge(settingsBridge)
{
    if (m_settingsBridge) {
        connect(m_settingsBridge, &SettingsBridge::settingsChanged, this, [this]() {
            refresh();
        });
    }

    refresh();
}

QVariantList ChannelPostsBridge::posts() const {
    return m_posts;
}

bool ChannelPostsBridge::isLoading() const {
    return m_isLoading;
}

QString ChannelPostsBridge::errorMessage() const {
    return m_errorMessage;
}

QString ChannelPostsBridge::channelUsername() const {
    return m_channelUsername;
}

void ChannelPostsBridge::refresh() {
    if (!m_settingsBridge) return;

    QString baseUrl = m_settingsBridge->authUrl();
    if (baseUrl.isEmpty()) {
        baseUrl = "http://127.0.0.1:8080";
    }

    QUrl requestUrl(baseUrl + "/api/channel/posts?limit=3");
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCALauncher/1.0");

    m_isLoading = true;
    emit loadingChanged();

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ChannelPostsBridge::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    m_isLoading = false;
    emit loadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_errorMessage = reply->errorString();
        qWarning() << "[ChannelPostsBridge] Failed to fetch channel posts:" << m_errorMessage;
        emit errorChanged();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        m_errorMessage = "Invalid JSON response from backend";
        qWarning() << "[ChannelPostsBridge]" << m_errorMessage;
        emit errorChanged();
        return;
    }

    QJsonObject root = doc.object();
    if (root.contains("channel")) {
        m_channelUsername = root["channel"].toString();
        emit channelChanged();
    }

    QJsonArray postsArray = root["posts"].toArray();
    QVariantList parsedPosts;

    for (const QJsonValue& val : postsArray) {
        if (!val.isObject()) continue;
        QJsonObject postObj = val.toObject();

        QVariantMap postMap;
        postMap["id"] = postObj["id"].toInt();
        postMap["date"] = postObj["date"].toString();
        postMap["formatted_date"] = postObj["formatted_date"].toString();
        postMap["text"] = postObj["text"].toString();
        postMap["image_url"] = postObj["image_url"].toString();
        postMap["views"] = postObj["views"].toVariant();
        postMap["link"] = postObj["link"].toString();

        parsedPosts.append(postMap);
    }

    m_posts = parsedPosts;
    m_errorMessage.clear();
    emit postsChanged();
    emit errorChanged();
    qInfo() << "[ChannelPostsBridge] Loaded" << m_posts.size() << "posts from channel @" << m_channelUsername;
}

} // namespace LCA::Bridge
