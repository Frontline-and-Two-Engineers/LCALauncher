#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSettings>
#include "UserSession.h"

namespace LCA::Auth {

class AuthService : public QObject {
    Q_OBJECT

public:
    explicit AuthService(QObject* parent = nullptr);
    ~AuthService() override;

    void setBaseUrl(const QString& url);
    QString baseUrl() const;

    void setTelegramBotUsername(const QString& botUsername);
    QString telegramBotUsername() const;

    // Telegram Code Auth flow
    void requestCode();
    void cancelCodePolling();

    // Session management
    void validateSession();
    void logout();

    bool hasSavedSession() const;
    UserSession currentSession() const;
    void restoreSavedSession();

    QString currentCode() const;
    int remainingSeconds() const;
    bool isPolling() const;

signals:
    void codeReceived(const QString& code, int expiresInSeconds);
    void codeTick(int remainingSeconds);
    void codeExpired();
    void loginStarted();
    void loginSuccess(const UserSession& session);
    void loginFailed(const QString& error);
    void logoutFinished();

private slots:
    void pollCodeStatus();
    void onTimerTick();

private:
    void pollReplyFinished(QNetworkReply* reply);
    void requestCodeReplyFinished(QNetworkReply* reply);
    void validateReplyFinished(QNetworkReply* reply);

    void saveSession(const UserSession& session);
    void clearSavedSession();

    QNetworkAccessManager m_network;
    QString m_baseUrl{"http://localhost:8080"};
    QString m_telegramBotUsername{"@LCAMinecraftBot"};

    UserSession m_session;
    QSettings m_settings;

    QString m_currentCode;
    int m_remainingSeconds{0};
    QTimer* m_pollTimer{nullptr};
    QTimer* m_countdownTimer{nullptr};
    bool m_isPolling{false};
};

} // namespace LCA::Auth
