#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <QTimer>
#include <QByteArray>
#include <QString>

namespace LCA::Server {

struct ServerInfo {
    bool isOnline{false};
    int playersOnline{0};
    int playersMax{0};
    int pingMs{0};
    QString motd;
    QString versionName;
    int protocolVersion{0};
    QString favicon;
};

class MinecraftPinger : public QObject {
    Q_OBJECT

public:
    explicit MinecraftPinger(QObject* parent = nullptr);
    ~MinecraftPinger() override;

    void ping(const QString& host, quint16 port, int timeoutMs = 4000);
    void cancel();

    bool isPinging() const { return m_isPinging; }

signals:
    void pingSuccess(const LCA::Server::ServerInfo& info);
    void pingFailed(const QString& reason);

private slots:
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onTimeout();

private:
    void cleanup();
    static void writeVarInt(QByteArray& buffer, qint32 value);
    static bool readVarInt(const QByteArray& data, int& offset, qint32& outValue);

    QTcpSocket* m_socket{nullptr};
    QElapsedTimer m_elapsedTimer;
    QTimer m_timeoutTimer;
    QByteArray m_receiveBuffer;
    QString m_currentHost;
    quint16 m_currentPort{25565};
    bool m_isPinging{false};
};

} // namespace LCA::Server
