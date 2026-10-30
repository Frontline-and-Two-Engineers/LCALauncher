#include "AuthBridge.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>

namespace LCA::Bridge {

AuthBridge::AuthBridge(Auth::AuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
{
    connect(m_authService, &Auth::AuthService::loginStarted, this, [this]() {
        m_isBusy = true;
        m_errorMessage.clear();
        m_isCodeExpired = false; // reset expired flag on new request
        emit busyChanged();
        emit expiredChanged();
    });

    connect(m_authService, &Auth::AuthService::codeReceived, this, [this](const QString& code, int expiresIn) {
        m_isBusy = false;
        m_authCode = code;
        m_remainingSeconds = expiresIn;
        m_isPolling = true;
        m_isCodeExpired = false;
        m_errorMessage.clear();

        emit busyChanged();
        emit codeChanged();
        emit timerChanged();
        emit pollingChanged();
        emit expiredChanged();
    });

    connect(m_authService, &Auth::AuthService::codeTick, this, [this](int remaining) {
        m_remainingSeconds = remaining;
        emit timerChanged();
    });

    connect(m_authService, &Auth::AuthService::codeExpired, this, [this]() {
        m_isPolling = false;
        m_isCodeExpired = true;
        emit pollingChanged();
        emit expiredChanged();
    });

    connect(m_authService, &Auth::AuthService::loginSuccess, this, [this](const Auth::UserSession&) {
        m_isBusy = false;
        m_isPolling = false;
        m_errorMessage.clear();
        emit busyChanged();
        emit pollingChanged();
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
        m_authCode.clear();
        m_remainingSeconds = 0;
        m_isPolling = false;
        m_isCodeExpired = false;
        emit codeChanged();
        emit timerChanged();
        emit pollingChanged();
        emit expiredChanged();
        emit sessionChanged();
    });
}

QString AuthBridge::username() const { return m_authService->currentSession().username; }
QString AuthBridge::uuid() const { return m_authService->currentSession().uuid; }
QString AuthBridge::userType() const { return m_authService->currentSession().userType; }
bool AuthBridge::isLoggedIn() const { return m_authService->currentSession().isLoggedIn; }
bool AuthBridge::isBusy() const { return m_isBusy; }
QString AuthBridge::errorMessage() const { return m_errorMessage; }
QString AuthBridge::authCode() const { return m_authCode; }

QString AuthBridge::formattedAuthCode() const {
    if (m_authCode.length() == 6) return m_authCode.left(3) + " " + m_authCode.mid(3);
    return m_authCode;
}

int AuthBridge::remainingSeconds() const { return m_remainingSeconds; }

QString AuthBridge::formattedTime() const {
    int minutes = m_remainingSeconds / 60;
    int seconds = m_remainingSeconds % 60;
    return QString::asprintf("%02d:%02d", minutes, seconds);
}

bool AuthBridge::isPolling() const { return m_isPolling; }
bool AuthBridge::isCodeExpired() const { return m_isCodeExpired; }

QString AuthBridge::telegramBotUsername() const {
    return m_authService->telegramBotUsername();
}

void AuthBridge::requestCode() { m_authService->requestCode(); }

void AuthBridge::cancelCode() {
    m_authService->cancelCodePolling();
    m_isPolling = false;
    emit pollingChanged();
}

void AuthBridge::copyCodeToClipboard() {
    if (!m_authCode.isEmpty()) QGuiApplication::clipboard()->setText(m_authCode);
}

void AuthBridge::openTelegramBot() {
    QString bot = m_authService->telegramBotUsername();
    bot.remove('@');
    if (!bot.isEmpty()) {
        QDesktopServices::openUrl(QUrl("https://t.me/" + bot));
    }
}

void AuthBridge::logout() { m_authService->logout(); }

} // namespace LCA::Bridge