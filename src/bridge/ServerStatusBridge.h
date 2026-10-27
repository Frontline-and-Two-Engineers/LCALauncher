#pragma once

#include <QObject>
#include <QTimer>
#include "bridge/SettingsBridge.h"
#include "core/server/MinecraftPinger.h"

namespace LCA::Bridge {

class ServerStatusBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isOnline READ isOnline NOTIFY statusChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY statusChanged)
    Q_PROPERTY(int playersOnline READ playersOnline NOTIFY statusChanged)
    Q_PROPERTY(int playersMax READ playersMax NOTIFY statusChanged)
    Q_PROPERTY(QString playersText READ playersText NOTIFY statusChanged)
    Q_PROPERTY(int pingMs READ pingMs NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString statusColor READ statusColor NOTIFY statusChanged)
    Q_PROPERTY(QString motd READ motd NOTIFY statusChanged)
    Q_PROPERTY(QString version READ version NOTIFY statusChanged)
    Q_PROPERTY(QString serverAddress READ serverAddress NOTIFY statusChanged)
    Q_PROPERTY(int serverPort READ serverPort NOTIFY statusChanged)
    Q_PROPERTY(QString favicon READ favicon NOTIFY statusChanged)

public:
    explicit ServerStatusBridge(SettingsBridge* settings, QObject* parent = nullptr);
    ~ServerStatusBridge() override;

    bool isOnline() const { return m_isOnline; }
    bool isChecking() const { return m_isChecking; }
    int playersOnline() const { return m_playersOnline; }
    int playersMax() const { return m_playersMax; }
    QString playersText() const;
    int pingMs() const { return m_pingMs; }
    QString statusText() const;
    QString statusColor() const;
    QString motd() const { return m_motd; }
    QString version() const { return m_version; }
    QString serverAddress() const;
    int serverPort() const;
    QString favicon() const { return m_favicon; }

    Q_INVOKABLE void refresh();

signals:
    void statusChanged();

private slots:
    void onPingSuccess(const LCA::Server::ServerInfo& info);
    void onPingFailed(const QString& reason);

private:
    SettingsBridge* m_settings{nullptr};
    Server::MinecraftPinger* m_pinger{nullptr};
    QTimer m_refreshTimer;

    bool m_isOnline{false};
    bool m_isChecking{false};
    int m_playersOnline{0};
    int m_playersMax{0};
    int m_pingMs{0};
    QString m_motd;
    QString m_version;
    QString m_favicon;
    QString m_lastHost;
    int m_lastPort{0};
};

} // namespace LCA::Bridge
