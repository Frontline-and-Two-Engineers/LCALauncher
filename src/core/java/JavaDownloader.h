#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

namespace LCA::Java {

class JavaDownloader : public QObject {
    Q_OBJECT

public:
    explicit JavaDownloader(QObject* parent = nullptr);
    ~JavaDownloader() override;

    void startDownload();
    void cancel();

    bool isDownloading() const;

signals:
    void statusChanged(const QString& status);
    void progressChanged(double percent, qint64 receivedBytes, qint64 totalBytes);
    void downloadFinished(bool success, const QString& javaPath, const QString& error);

private slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onReplyFinished();

private:
    QString getAdoptiumUrl() const;
    QString getTargetArchiveExtension() const;
    bool extractArchive(const QString& archivePath, const QString& destinationDir);
    QString findJavaExecutable(const QString& rootDir) const;

    QNetworkAccessManager m_network;
    QNetworkReply* m_currentReply{nullptr};
    QFile* m_outputFile{nullptr};
    QString m_tempArchivePath;
    QString m_extractDir;
    bool m_isDownloading{false};
};

} // namespace LCA::Java
