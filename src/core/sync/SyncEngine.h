#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include "core/downloader/DownloadManager.h"

namespace LCA::Sync {

struct SyncItem {
    QString relativePath;
    QUrl downloadUrl;
    QString sha256;
    qint64 size{0};
};

class SyncEngine : public QObject {
    Q_OBJECT

public:
    explicit SyncEngine(const QString& gameDir, QObject* parent = nullptr);

    void setManifestUrl(const QString& url);
    void startSync();
    void cancel();

signals:
    void statusChanged(const QString& statusText);
    void progressChanged(qint64 current, qint64 total, double percent, const QString& currentFile);
    void syncFinished(bool success, const QString& message);

private slots:
    void onManifestDownloaded(QNetworkReply* reply);

private:
    void verifyAndDownloadFiles(const QList<SyncItem>& items);
    void cleanObsoleteMods(const QSet<QString>& allowedRelativePaths);

    QString m_gameDir;
    QString m_manifestUrl{"https://myserver.com/api/modpack/manifest.json"};
    QNetworkAccessManager m_network;
    Downloader::DownloadManager* m_downloader{nullptr};
};

} // namespace LCA::Sync
