#include "AuthBridge.h"

namespace LCA::Bridge {

AuthBridge::AuthBridge(Auth::AuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
{
    connect(m_authService, &Auth::AuthService::loginStarted, this, [this]() {
        m_isBusy = true;
        m_errorMessage.clear();
        emit busyChanged();
    });

    connect(m_authService, &Auth::AuthService::loginSuccess, this, [this](const Auth::UserSession&) {
        m_isBusy = false;
        m_errorMessage.clear();
        emit busyChanged();
        emit sessionChanged();
        emit loginSucceeded();
    });

    connect(m_authService, &Auth::AuthService::loginFailed, this, [this](const QString& err) {
        m_isBusy = false;
        m_errorMessage = err;
        emit busyChanged();
        emit errorOccurred(err);
    });

    connect(m_authService, &Auth::AuthService::logoutFinished, this, [this]() {
        emit sessionChanged();
    });
}

QString AuthBridge::username() const {
    return m_authService->currentSession().username;
}

QString AuthBridge::uuid() const {
    return m_authService->currentSession().uuid;
}

QString AuthBridge::userType() const {
    return m_authService->currentSession().userType;
}

bool AuthBridge::isLoggedIn() const {
    return m_authService->currentSession().isLoggedIn;
}

bool AuthBridge::isBusy() const {
    return m_isBusy;
}

QString AuthBridge::errorMessage() const {
    return m_errorMessage;
}

void AuthBridge::loginWithCredentials(const QString& username, const QString& password) {
    m_authService->loginWithCredentials(username, password);
}

void AuthBridge::loginOffline(const QString& username) {
    m_authService->loginOffline(username);
}

void AuthBridge::logout() {
    m_authService->logout();
}

} // namespace LCA::Bridge
