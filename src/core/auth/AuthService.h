#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include "UserSession.h"

namespace LCA::Auth {

class AuthService : public QObject {
    Q_OBJECT

public:
    explicit AuthService(QObject* parent = nullptr);

    void setAuthServerUrl(const QString& url);
    QString authServerUrl() const;

    void loginWithCredentials(const QString& username, const QString& password);
    void loginOffline(const QString& username);
    void logout();

    bool hasSavedSession() const;
    UserSession currentSession() const;
    void restoreSavedSession();

signals:
    void loginStarted();
    void loginSuccess(const UserSession& session);
    void loginFailed(const QString& error);
    void logoutFinished();

private slots:
    void onNetworkReplyFinished(QNetworkReply* reply);

private:
    void saveSession(const UserSession& session);
    void clearSavedSession();

    QNetworkAccessManager m_network;
    QString m_authUrl{"https://auth.myserver.com/api/auth/login"}; // Default, configurable in Settings
    UserSession m_session;
    QSettings m_settings;
};

} // namespace LCA::Auth
