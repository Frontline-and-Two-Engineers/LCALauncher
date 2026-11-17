#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QFutureWatcher>

namespace LCA::Bridge {

class UpdateBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY stateChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY stateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY stateChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY stateChanged)
    Q_PROPERTY(bool isInstalling READ isInstalling NOTIFY stateChanged)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(bool dismissed READ dismissed NOTIFY stateChanged)
    Q_PROPERTY(QString updateChannel READ updateChannel WRITE setUpdateChannel NOTIFY channelChanged)

public:
    explicit UpdateBridge(QObject* parent = nullptr);
    ~UpdateBridge() override;

    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseNotes() const { return m_releaseNotes; }
    QString releaseUrl() const { return m_releaseUrl; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool isChecking() const { return m_isChecking; }
    bool isDownloading() const { return m_isDownloading; }
    bool isInstalling() const { return m_isInstalling; }
    double downloadProgress() const { return m_downloadProgress; }
    QString statusText() const { return m_statusText; }
    QString errorText() const { return m_errorText; }
    bool dismissed() const { return m_dismissed; }
    QString updateChannel() const { return m_updateChannel; }
    void setUpdateChannel(const QString& channel);

    Q_INVOKABLE void checkForUpdates(bool silent = true);
    Q_INVOKABLE void startUpdate();
    Q_INVOKABLE void cancelUpdate();
    Q_INVOKABLE void dismiss();
    Q_INVOKABLE void openReleasePage();

signals:
    void stateChanged();
    void progressChanged();
    void channelChanged();

private slots:
    void onCheckReplyFinished();
    void onDownloadReadyRead();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onPrepareFinished();

private:
    struct PrepareResult {
        bool success{false};
        QString error;
        QString program;
        QStringList arguments;
        QString workingDir;
    };

    PrepareResult prepareUpdateInstall(const QString& archivePath, const QString& stagingDir);
    void failUpdate(const QString& message);
    void cleanupDownloadReply();

    QNetworkAccessManager m_network;
    QNetworkReply* m_checkReply{nullptr};
    QNetworkReply* m_downloadReply{nullptr};
    QFile* m_downloadFile{nullptr};
    QFutureWatcher<PrepareResult>* m_prepareWatcher{nullptr};

    QString m_latestVersion;
    QString m_releaseNotes;
    QString m_releaseUrl;
    QString m_downloadUrl;
    QString m_assetName;
    qint64 m_assetSize{0};
    QString m_stagingDir;
    QString m_archivePath;

    bool m_silentCheck{true};
    bool m_updateAvailable{false};
    bool m_isChecking{false};
    bool m_isDownloading{false};
    bool m_isInstalling{false};
    double m_downloadProgress{0.0};
    QString m_statusText;
    QString m_errorText;
    bool m_dismissed{false};
    QString m_updateChannel{"main"};
};

} // namespace LCA::Bridge
