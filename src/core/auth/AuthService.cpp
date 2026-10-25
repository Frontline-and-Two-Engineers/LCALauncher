#include "AuthService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QNetworkRequest>

namespace LCA::Auth {

AuthService::AuthService(QObject* parent)
    : QObject(parent)
    , m_settings("LCAProject", "LCALauncher")
{
    m_authUrl = m_settings.value("auth/serverUrl", m_authUrl).toString();
    connect(&m_network, &QNetworkAccessManager::finished, this, &AuthService::onNetworkReplyFinished);
}

void AuthService::setAuthServerUrl(const QString& url) {
    m_authUrl = url;
    m_settings.setValue("auth/serverUrl", m_authUrl);
}

QString AuthService::authServerUrl() const {
    return m_authUrl;
}

void AuthService::loginOffline(const QString& username) {
    if (username.trimmed().isEmpty()) {
        emit loginFailed("Имя пользователя не может быть пустым");
        return;
    }

    emit loginStarted();

    m_session.username = username.trimmed();
    m_session.uuid = UserSession::generateOfflineUuid(m_session.username);
    m_session.accessToken = "offline_token_" + m_session.uuid;
    m_session.userType = "offline";
    m_session.isLoggedIn = true;

    saveSession(m_session);
    emit loginSuccess(m_session);
}

void AuthService::loginWithCredentials(const QString& username, const QString& password) {
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        emit loginFailed("Введите логин и пароль");
        return;
    }

    emit loginStarted();

    QNetworkRequest request{QUrl(m_authUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "LCALauncher/1.0");

    QJsonObject body;
    body["username"] = username.trimmed();
    body["password"] = password;

    m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void AuthService::onNetworkReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(QString("Ошибка сети: %1").arg(reply->errorString()));
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit loginFailed("Неверный ответ сервера авторизации");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("error")) {
        emit loginFailed(obj["errorMessage"].toString(obj["error"].toString("Ошибка авторизации")));
        return;
    }

    m_session.username = obj["username"].toString();
    m_session.uuid = obj["uuid"].toString();
    m_session.accessToken = obj["accessToken"].toString(obj["token"].toString());
    m_session.userType = "custom";
    m_session.isLoggedIn = true;

    if (m_session.uuid.isEmpty() || m_session.username.isEmpty()) {
        emit loginFailed("Сервер не вернул UUID или имя игрока");
        return;
    }

    saveSession(m_session);
    emit loginSuccess(m_session);
}

void AuthService::logout() {
    m_session = UserSession{};
    clearSavedSession();
    emit logoutFinished();
}

bool AuthService::hasSavedSession() const {
    return m_settings.contains("session/username") && m_settings.contains("session/uuid");
}

UserSession AuthService::currentSession() const {
    return m_session;
}

void AuthService::restoreSavedSession() {
    if (!hasSavedSession()) return;

    m_session.username = m_settings.value("session/username").toString();
    m_session.uuid = m_settings.value("session/uuid").toString();
    m_session.accessToken = m_settings.value("session/accessToken").toString();
    m_session.userType = m_settings.value("session/userType", "custom").toString();
    m_session.isLoggedIn = true;

    emit loginSuccess(m_session);
}

void AuthService::saveSession(const UserSession& session) {
    m_settings.setValue("session/username", session.username);
    m_settings.setValue("session/uuid", session.uuid);
    m_settings.setValue("session/accessToken", session.accessToken);
    m_settings.setValue("session/userType", session.userType);
    m_settings.sync();
}

void AuthService::clearSavedSession() {
    m_settings.remove("session");
    m_settings.sync();
}

} // namespace LCA::Auth
