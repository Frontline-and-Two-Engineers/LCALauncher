#include "MinecraftPinger.h"
#include <QtEndian>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

namespace LCA::Server {

MinecraftPinger::MinecraftPinger(QObject* parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &MinecraftPinger::onTimeout);
}

MinecraftPinger::~MinecraftPinger() {
    cleanup();
}

void MinecraftPinger::ping(const QString& host, quint16 port, int timeoutMs) {
    cancel();

    m_currentHost = host.trimmed();
    m_currentPort = (port == 0) ? 25565 : port;

    if (m_currentHost.isEmpty()) {
        emit pingFailed("Адрес сервера не указан");
        return;
    }

    m_isPinging = true;
    m_receiveBuffer.clear();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &MinecraftPinger::onSocketConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MinecraftPinger::onSocketReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MinecraftPinger::onSocketError);

    m_elapsedTimer.start();
    m_timeoutTimer.start(timeoutMs);

    m_socket->connectToHost(m_currentHost, m_currentPort);
}

void MinecraftPinger::cancel() {
    m_timeoutTimer.stop();
    m_isPinging = false;
    cleanup();
}

void MinecraftPinger::cleanup() {
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void MinecraftPinger::onTimeout() {
    if (!m_isPinging) return;
    m_isPinging = false;
    cleanup();
    emit pingFailed("Превышено время ожидания ответа сервера (таймаут)");
}

void MinecraftPinger::onSocketConnected() {
    if (!m_socket || !m_isPinging) return;

    // Build Handshake Packet (ID = 0x00)
    // Fields:
    // - Packet ID (VarInt 0x00)
    // - Protocol Version (VarInt 767 for MC 1.21.1)
    // - Server Address (VarInt length + UTF-8 string)
    // - Server Port (Unsigned Short, Big Endian)
    // - Next State (VarInt 1 for status)
    QByteArray handshakeBody;
    writeVarInt(handshakeBody, 0x00);
    writeVarInt(handshakeBody, 767);

    QByteArray hostUtf8 = m_currentHost.toUtf8();
    writeVarInt(handshakeBody, hostUtf8.size());
    handshakeBody.append(hostUtf8);

    quint16 portBe = qToBigEndian(m_currentPort);
    handshakeBody.append(reinterpret_cast<const char*>(&portBe), sizeof(quint16));

    writeVarInt(handshakeBody, 1);

    // Prefix with length
    QByteArray handshakePacket;
    writeVarInt(handshakePacket, handshakeBody.size());
    handshakePacket.append(handshakeBody);

    // Build Status Request Packet (Length 1, ID 0x00)
    QByteArray requestPacket;
    writeVarInt(requestPacket, 1);
    writeVarInt(requestPacket, 0x00);

    // Send both packets
    m_socket->write(handshakePacket);
    m_socket->write(requestPacket);
    m_socket->flush();
}

void MinecraftPinger::onSocketReadyRead() {
    if (!m_socket || !m_isPinging) return;

    m_receiveBuffer.append(m_socket->readAll());

    int offset = 0;
    qint32 packetLength = 0;
    if (!readVarInt(m_receiveBuffer, offset, packetLength)) {
        return; // Need more data to read packet length
    }

    int headerSize = offset;
    if (m_receiveBuffer.size() < headerSize + packetLength) {
        return; // Full packet has not arrived yet
    }

    // Full packet received
    m_timeoutTimer.stop();
    m_isPinging = false;

    qint32 packetId = 0;
    if (!readVarInt(m_receiveBuffer, offset, packetId) || packetId != 0x00) {
        cleanup();
        emit pingFailed("Некорректный ID ответа сервера");
        return;
    }

    qint32 jsonLength = 0;
    if (!readVarInt(m_receiveBuffer, offset, jsonLength)) {
        cleanup();
        emit pingFailed("Некорректный заголовок JSON в ответе сервера");
        return;
    }

    if (m_receiveBuffer.size() < offset + jsonLength) {
        cleanup();
        emit pingFailed("Длина JSON ответа меньше ожидаемой");
        return;
    }

    QByteArray jsonBytes = m_receiveBuffer.mid(offset, jsonLength);
    cleanup();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit pingFailed("Не удалось декодировать JSON статуса сервера");
        return;
    }

    QJsonObject obj = doc.object();
    ServerInfo info;
    info.isOnline = true;
    info.pingMs = static_cast<int>(m_elapsedTimer.elapsed());

    // Players
    if (obj.contains("players") && obj["players"].isObject()) {
        QJsonObject pObj = obj["players"].toObject();
        info.playersOnline = pObj["online"].toInt(0);
        info.playersMax = pObj["max"].toInt(0);
    }

    // Version
    if (obj.contains("version") && obj["version"].isObject()) {
        QJsonObject vObj = obj["version"].toObject();
        info.versionName = vObj["name"].toString();
        info.protocolVersion = vObj["protocol"].toInt(0);
    }

    // Favicon
    if (obj.contains("favicon")) {
        info.favicon = obj["favicon"].toString();
    }

    // Description / MOTD
    if (obj.contains("description")) {
        QJsonValue dVal = obj["description"];
        if (dVal.isString()) {
            info.motd = dVal.toString();
        } else if (dVal.isObject()) {
            QJsonObject dObj = dVal.toObject();
            info.motd = dObj["text"].toString();
            if (dObj.contains("extra") && dObj["extra"].isArray()) {
                for (const auto& item : dObj["extra"].toArray()) {
                    if (item.isString()) info.motd += item.toString();
                    else if (item.isObject()) info.motd += item.toObject()["text"].toString();
                }
            }
        }
    }

    // Strip Minecraft color and formatting codes (§a, §l, etc.)
    info.motd.remove(QRegularExpression(R"(§[0-9a-fk-orA-FK-OR])"));
    info.motd = info.motd.trimmed();

    emit pingSuccess(info);
}

void MinecraftPinger::onSocketError(QAbstractSocket::SocketError) {
    if (!m_isPinging) return;
    m_isPinging = false;
    QString err = m_socket ? m_socket->errorString() : "Ошибка сокета";
    cleanup();
    emit pingFailed(err);
}

void MinecraftPinger::writeVarInt(QByteArray& buffer, qint32 value) {
    quint32 uValue = static_cast<quint32>(value);
    while (true) {
        if ((uValue & ~0x7Fu) == 0) {
            buffer.append(static_cast<char>(uValue));
            return;
        }
        buffer.append(static_cast<char>((uValue & 0x7F) | 0x80));
        uValue >>= 7;
    }
}

bool MinecraftPinger::readVarInt(const QByteArray& data, int& offset, qint32& outValue) {
    int numRead = 0;
    qint32 result = 0;
    quint8 byte = 0;
    do {
        if (offset >= data.size()) return false;
        byte = static_cast<quint8>(data[offset++]);
        qint32 val = (byte & 0x7F);
        result |= (val << (7 * numRead));
        numRead++;
        if (numRead > 5) return false;
    } while ((byte & 0x80) != 0);
    outValue = result;
    return true;
}

} // namespace LCA::Server
