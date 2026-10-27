#include "AuthService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QNetworkRequest>

namespace LCA::Auth {

AuthService::AuthService(QObject* parent)
    : QObject(parent)
    , m_settings("LCAProject", "LCALauncher")
    , m_pollTimer(new QTimer(this))
    , m_countdownTimer(new QTimer(this))
{
    m_baseUrl = m_settings.value("auth/baseUrl", m_baseUrl).toString();
    m_telegramBotUsername = m_settings.value("auth/telegramBotUsername", m_telegramBotUsername).toString();

    connect(m_pollTimer, &QTimer::timeout, this, &AuthService::pollCodeStatus);
    connect(m_countdownTimer, &QTimer::timeout, this, &AuthService::onTimerTick);
}

AuthService::~AuthService() {
    cancelCodePolling();
}

void AuthService::setBaseUrl(const QString& url) {
    QString trimmed = url.trimmed();
    if (trimmed.endsWith('/')) {
        trimmed.chop(1);
    }
    m_baseUrl = trimmed;
    m_settings.setValue("auth/baseUrl", m_baseUrl);
}

QString AuthService::baseUrl() const {
    return m_baseUrl;
}

void AuthService::setTelegramBotUsername(const QString& botUsername) {
    m_telegramBotUsername = botUsername.trimmed();
    if (!m_telegramBotUsername.isEmpty() && !m_telegramBotUsername.startsWith('@')) {
        m_telegramBotUsername.prepend('@');
    }
    m_settings.setValue("auth/telegramBotUsername", m_telegramBotUsername);
}

QString AuthService::telegramBotUsername() const {
    return m_telegramBotUsername;
}

void AuthService::requestCode() {
    cancelCodePolling();
    emit loginStarted();

    QUrl requestUrl(m_baseUrl + "/api/auth/request-code");
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "LCALauncher/1.0");

    QNetworkReply* reply = m_network.post(request, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        requestCodeReplyFinished(reply);
    });
}

void AuthService::requestCodeReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(QString("Не удалось получить код: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit loginFailed("Неверный формат ответа сервера при запросе кода");
        return;
    }

    QJsonObject obj = doc.object();
    m_currentCode = obj["code"].toString();
    m_remainingSeconds = obj["expires_in"].toInt(300);

    if (m_currentCode.isEmpty()) {
        emit loginFailed("Сервер вернул пустой код авторизации");
        return;
    }

    m_isPolling = true;
    emit codeReceived(m_currentCode, m_remainingSeconds);

    m_countdownTimer->start(1000);
    m_pollTimer->start(2000);
}

void AuthService::onTimerTick() {
    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
        emit codeTick(m_remainingSeconds);

        if (m_remainingSeconds == 0) {
            cancelCodePolling();
            emit codeExpired();
        }
    }
}

void AuthService::pollCodeStatus() {
    if (!m_isPolling || m_currentCode.isEmpty()) {
        return;
    }

    QUrl pollUrl(QString("%1/api/auth/poll/%2").arg(m_baseUrl, m_currentCode));
    QNetworkRequest request(pollUrl);
    request.setRawHeader("User-Agent", "LCALauncher/1.0");

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        pollReplyFinished(reply);
    });
}

void AuthService::pollReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            cancelCodePolling();
            emit loginFailed("Код авторизации не найден или истёк");
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString status = obj["status"].toString();

    if (status == "confirmed") {
        cancelCodePolling();

        m_session.username = obj["username"].toString();
        m_session.uuid = obj["uuid"].toString();
        m_session.accessToken = obj["accessToken"].toString();
        m_session.userType = "custom";
        m_session.isLoggedIn = true;

        if (m_session.username.isEmpty() || m_session.uuid.isEmpty()) {
            emit loginFailed("Сервер не вернул имя игрока или UUID");
            return;
        }

        saveSession(m_session);
        emit loginSuccess(m_session);
    } else if (status == "expired") {
        cancelCodePolling();
        emit codeExpired();
    }
}

void AuthService::cancelCodePolling() {
    m_isPolling = false;
    m_pollTimer->stop();
    m_countdownTimer->stop();
    m_remainingSeconds = 0;
}

void AuthService::loginOffline(const QString& username) {
    if (username.trimmed().isEmpty()) {
        emit loginFailed("Имя пользователя не может быть пустым");
        return;
    }

    cancelCodePolling();
    emit loginStarted();

    m_session.username = username.trimmed();
    m_session.uuid = UserSession::generateOfflineUuid(m_session.username);
    m_session.accessToken = "offline_token_" + m_session.uuid;
    m_session.userType = "offline";
    m_session.isLoggedIn = true;

    saveSession(m_session);
    emit loginSuccess(m_session);
}

void AuthService::validateSession() {
    if (!m_session.isLoggedIn || m_session.accessToken.isEmpty()) {
        return;
    }

    if (m_session.userType == "offline" || m_session.accessToken.startsWith("offline_token_")) {
        emit loginSuccess(m_session);
        return;
    }

    QUrl validateUrl(m_baseUrl + "/api/auth/validate");
    QNetworkRequest request(validateUrl);
    request.setRawHeader("User-Agent", "LCALauncher/1.0");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_session.accessToken).toUtf8());

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        validateReplyFinished(reply);
    });
}

void AuthService::validateReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 200) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj["valid"].toBool(false)) {
                if (obj.contains("username")) m_session.username = obj["username"].toString();
                if (obj.contains("uuid")) m_session.uuid = obj["uuid"].toString();
                saveSession(m_session);
                emit loginSuccess(m_session);
                return;
            }
        }
    }

    // Token expired or invalid
    m_session = UserSession{};
    clearSavedSession();
    emit loginFailed("Сессия истекла. Пожалуйста, выполните вход через бота.");
}

void AuthService::logout() {
    cancelCodePolling();

    if (m_session.userType == "custom" && !m_session.accessToken.isEmpty()) {
        QUrl logoutUrl(m_baseUrl + "/api/auth/logout");
        QNetworkRequest request(logoutUrl);
        request.setRawHeader("User-Agent", "LCALauncher/1.0");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_session.accessToken).toUtf8());
        m_network.post(request, QByteArray("{}"));
    }

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

    validateSession();
}

QString AuthService::currentCode() const {
    return m_currentCode;
}

int AuthService::remainingSeconds() const {
    return m_remainingSeconds;
}

bool AuthService::isPolling() const {
    return m_isPolling;
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
