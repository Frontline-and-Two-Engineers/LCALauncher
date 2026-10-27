#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

namespace LCA::Client {

class ClientDownloader : public QObject {
    Q_OBJECT

public:
    explicit ClientDownloader(QObject* parent = nullptr);
    ~ClientDownloader() override;

    /**
     * @brief Checks if base client files (versions, libraries, authlib-injector) are present.
     */
    static bool isClientInstalled(const QString& launcherDataDir);

    /**
     * @brief Begins downloading client-base.zip and extracting it to launcherDataDir.
     */
    void startDownload(const QString& downloadUrl, const QString& launcherDataDir);

    /**
     * @brief Cancels any active download and cleans temporary files.
     */
    void cancel();

    bool isDownloading() const { return m_isDownloading; }

signals:
    void progressChanged(double percent, qint64 bytesReceived, qint64 bytesTotal);
    void statusChanged(const QString& status);
    void downloadFinished(bool success, const QString& errorMessage);

private slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onReplyFinished();

private:
    QNetworkAccessManager m_network;
    QNetworkReply* m_currentReply{nullptr};
    QFile* m_outputFile{nullptr};

    QString m_downloadUrl;
    QString m_launcherDataDir;
    QString m_tempZipPath;
    bool m_isDownloading{false};
};

} // namespace LCA::Client
