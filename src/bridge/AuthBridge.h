#pragma once

#include <QObject>
#include "core/auth/AuthService.h"

namespace LCA::Bridge {

class AuthBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString username READ username NOTIFY sessionChanged)
    Q_PROPERTY(QString uuid READ uuid NOTIFY sessionChanged)
    Q_PROPERTY(QString userType READ userType NOTIFY sessionChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY sessionChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorOccurred)

    // Telegram Code Auth properties
    Q_PROPERTY(QString authCode READ authCode NOTIFY codeChanged)
    Q_PROPERTY(QString formattedAuthCode READ formattedAuthCode NOTIFY codeChanged)
    Q_PROPERTY(int remainingSeconds READ remainingSeconds NOTIFY timerChanged)
    Q_PROPERTY(QString formattedTime READ formattedTime NOTIFY timerChanged)
    Q_PROPERTY(bool isPolling READ isPolling NOTIFY pollingChanged)
    Q_PROPERTY(bool isCodeExpired READ isCodeExpired NOTIFY expiredChanged)
    Q_PROPERTY(QString telegramBotUsername READ telegramBotUsername NOTIFY botUsernameChanged)

public:
    explicit AuthBridge(Auth::AuthService* authService, QObject* parent = nullptr);

    QString username() const;
    QString uuid() const;
    QString userType() const;
    bool isLoggedIn() const;
    bool isBusy() const;
    QString errorMessage() const;

    QString authCode() const;
    QString formattedAuthCode() const;
    int remainingSeconds() const;
    QString formattedTime() const;
    bool isPolling() const;
    bool isCodeExpired() const;
    QString telegramBotUsername() const;

    Q_INVOKABLE void requestCode();
    Q_INVOKABLE void cancelCode();
    Q_INVOKABLE void copyCodeToClipboard();
    Q_INVOKABLE void openTelegramBot();
    Q_INVOKABLE void logout();

signals:
    void sessionChanged();
    void busyChanged();
    void errorOccurred(const QString& error);
    void loginSucceeded();
    void codeChanged();
    void timerChanged();
    void pollingChanged();
    void expiredChanged();
    void botUsernameChanged();

private:
    Auth::AuthService* m_authService;
    bool m_isBusy{false};
    QString m_errorMessage;
    QString m_authCode;
    int m_remainingSeconds{0};
    bool m_isPolling{false};
    bool m_isCodeExpired{false};
};

} // namespace LCA::Bridge
