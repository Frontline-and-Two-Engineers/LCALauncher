#include "HashUtils.h"
#include <QFile>

namespace LCA::Utils {

static QString hashFile(const QString& filePath, QCryptographicHash::Algorithm algorithm) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(algorithm);
    if (!hash.addData(&file)) {
        return {};
    }

    return hash.result().toHex().toLower();
}

QString HashUtils::sha1File(const QString& filePath) {
    return hashFile(filePath, QCryptographicHash::Sha1);
}

QString HashUtils::sha256File(const QString& filePath) {
    return hashFile(filePath, QCryptographicHash::Sha256);
}

QString HashUtils::md5File(const QString& filePath) {
    return hashFile(filePath, QCryptographicHash::Md5);
}

QString HashUtils::sha1Data(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex().toLower();
}

QString HashUtils::sha256Data(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toLower();
}

bool HashUtils::verifySha1(const QString& filePath, const QString& expectedSha1) {
    if (expectedSha1.isEmpty()) return true;
    return QString::compare(sha1File(filePath), expectedSha1, Qt::CaseInsensitive) == 0;
}

bool HashUtils::verifySha256(const QString& filePath, const QString& expectedSha256) {
    if (expectedSha256.isEmpty()) return true;
    return QString::compare(sha256File(filePath), expectedSha256, Qt::CaseInsensitive) == 0;
}

} // namespace LCA::Utils
