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

public:
    explicit AuthBridge(Auth::AuthService* authService, QObject* parent = nullptr);

    QString username() const;
    QString uuid() const;
    QString userType() const;
    bool isLoggedIn() const;
    bool isBusy() const;
    QString errorMessage() const;

    Q_INVOKABLE void loginWithCredentials(const QString& username, const QString& password);
    Q_INVOKABLE void loginOffline(const QString& username);
    Q_INVOKABLE void logout();

signals:
    void sessionChanged();
    void busyChanged();
    void errorOccurred(const QString& error);
    void loginSucceeded();

private:
    Auth::AuthService* m_authService;
    bool m_isBusy{false};
    QString m_errorMessage;
};

} // namespace LCA::Bridge
