#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QMap>
#include "DownloadTask.h"

namespace LCA::Downloader {

class DownloadManager : public QObject {
    Q_OBJECT

public:
    explicit DownloadManager(int maxConcurrent = 6, QObject* parent = nullptr);

    void addTask(const DownloadTask& task);
    void addTasks(const QList<DownloadTask>& tasks);
    void start();
    void cancel();

    bool isRunning() const;
    qint64 totalTasksCount() const;
    qint64 completedTasksCount() const;

signals:
    void progressChanged(qint64 completedCount, qint64 totalCount, double percent, const QString& currentItem);
    void fileDownloaded(const QString& destinationPath);
    void taskFailed(const QString& destinationPath, const QString& error);
    void allFinished();

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    void processQueue();
    bool checkExistingFileValid(const DownloadTask& task);

    QNetworkAccessManager m_network;
    int m_maxConcurrent{6};
    bool m_isRunning{false};

    QQueue<DownloadTask> m_pendingTasks;
    QMap<QNetworkReply*, DownloadTask> m_activeTasks;

    qint64 m_totalCount{0};
    qint64 m_completedCount{0};
};

} // namespace LCA::Downloader
