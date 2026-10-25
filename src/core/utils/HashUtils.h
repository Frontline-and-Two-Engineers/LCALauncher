#pragma once

#include <QString>
#include <QByteArray>
#include <QCryptographicHash>

namespace LCA::Utils {

class HashUtils {
public:
    static QString sha1File(const QString& filePath);
    static QString sha256File(const QString& filePath);
    static QString md5File(const QString& filePath);

    static QString sha1Data(const QByteArray& data);
    static QString sha256Data(const QByteArray& data);

    static bool verifySha1(const QString& filePath, const QString& expectedSha1);
    static bool verifySha256(const QString& filePath, const QString& expectedSha256);
};

} // namespace LCA::Utils
