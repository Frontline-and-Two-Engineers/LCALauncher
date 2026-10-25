#pragma once

#include <QString>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QUuid>

namespace LCA::Auth {

struct UserSession {
    QString username;
    QString uuid;
    QString accessToken;
    QString userType{"custom"}; // "custom" or "offline"
    bool isLoggedIn{false};

    static QString generateOfflineUuid(const QString& username) {
        // Minecraft offline UUID is MD5 of "OfflinePlayer:" + username, version 3 UUID format
        QByteArray input = QString("OfflinePlayer:%1").arg(username).toUtf8();
        QByteArray md5 = QCryptographicHash::hash(input, QCryptographicHash::Md5);
        
        // Fix version to 3 and variant to IETF
        md5[6] = static_cast<char>((md5[6] & 0x0f) | 0x30);
        md5[8] = static_cast<char>((md5[8] & 0x3f) | 0x80);

        QString hex = md5.toHex();
        return QString("%1-%2-%3-%4-%5")
            .arg(hex.mid(0, 8))
            .arg(hex.mid(8, 4))
            .arg(hex.mid(12, 4))
            .arg(hex.mid(16, 4))
            .arg(hex.mid(20, 12));
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["username"] = username;
        obj["uuid"] = uuid;
        obj["accessToken"] = accessToken;
        obj["userType"] = userType;
        obj["isLoggedIn"] = isLoggedIn;
        return obj;
    }

    static UserSession fromJson(const QJsonObject& obj) {
        UserSession session;
        session.username = obj["username"].toString();
        session.uuid = obj["uuid"].toString();
        session.accessToken = obj["accessToken"].toString();
        session.userType = obj["userType"].toString("custom");
        session.isLoggedIn = obj["isLoggedIn"].toBool(false);
        return session;
    }
};

} // namespace LCA::Auth
