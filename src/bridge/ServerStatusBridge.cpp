#include "ServerStatusBridge.h"

namespace LCA::Bridge {

ServerStatusBridge::ServerStatusBridge(SettingsBridge* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_pinger(new Server::MinecraftPinger(this))
{
    connect(m_pinger, &Server::MinecraftPinger::pingSuccess, this, &ServerStatusBridge::onPingSuccess);
    connect(m_pinger, &Server::MinecraftPinger::pingFailed, this, &ServerStatusBridge::onPingFailed);

    if (m_settings) {
        connect(m_settings, &SettingsBridge::settingsChanged, this, [this]() {
            if (m_lastHost != m_settings->serverAddress() || m_lastPort != m_settings->serverPort()) {
                refresh();
            }
        });
    }

    // Auto refresh every 25 seconds
    m_refreshTimer.setInterval(25000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &ServerStatusBridge::refresh);
    m_refreshTimer.start();

    // Initial ping after 300ms to allow UI to load
    QTimer::singleShot(300, this, &ServerStatusBridge::refresh);
}

ServerStatusBridge::~ServerStatusBridge() {
    m_refreshTimer.stop();
}

QString ServerStatusBridge::serverAddress() const {
    return m_settings ? m_settings->serverAddress() : "";
}

int ServerStatusBridge::serverPort() const {
    return m_settings ? m_settings->serverPort() : 25565;
}

QString ServerStatusBridge::playersText() const {
    if (m_isChecking && !m_isOnline) {
        return "...";
    }
    if (!m_isOnline) {
        return "— / —";
    }
    return QString("%1 / %2").arg(m_playersOnline).arg(m_playersMax);
}

QString ServerStatusBridge::statusText() const {
    if (m_isChecking) {
        return "ПРОВЕРКА...";
    }
    if (m_isOnline) {
        return "СЕРВЕР ОНЛАЙН";
    }
    return "СЕРВЕР ОФФЛАЙН";
}

QString ServerStatusBridge::statusColor() const {
    if (m_isChecking) {
        return "#FFB300"; // Yellow / Orange
    }
    if (m_isOnline) {
        return "#00FFA3"; // Green / Teal
    }
    return "#FF5252"; // Red
}

void ServerStatusBridge::refresh() {
    if (!m_settings) return;

    QString host = m_settings->serverAddress().trimmed();
    int port = m_settings->serverPort();

    m_lastHost = host;
    m_lastPort = port;

    if (host.isEmpty()) {
        m_isOnline = false;
        m_isChecking = false;
        emit statusChanged();
        return;
    }

    m_isChecking = true;
    emit statusChanged();

    m_pinger->ping(host, static_cast<quint16>(port), 3500);
}

void ServerStatusBridge::onPingSuccess(const LCA::Server::ServerInfo& info) {
    m_isChecking = false;
    m_isOnline = info.isOnline;
    m_playersOnline = info.playersOnline;
    m_playersMax = info.playersMax;
    m_pingMs = info.pingMs;
    m_motd = info.motd;
    m_version = info.versionName;
    m_favicon = info.favicon;

    emit statusChanged();
}

void ServerStatusBridge::onPingFailed(const QString& /*reason*/) {
    m_isChecking = false;
    m_isOnline = false;
    m_playersOnline = 0;
    m_playersMax = 0;
    m_pingMs = 0;
    m_motd = "";
    m_favicon = "";

    emit statusChanged();
}

} // namespace LCA::Bridge
