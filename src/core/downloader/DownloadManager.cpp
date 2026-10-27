#include "DownloadManager.h"
#include "core/utils/HashUtils.h"
#include "core/utils/FileUtils.h"
#include <QFileInfo>
#include <QDir>

namespace LCA::Downloader {

DownloadManager::DownloadManager(int maxConcurrent, QObject* parent)
    : QObject(parent)
    , m_maxConcurrent(maxConcurrent)
{
}

void DownloadManager::addTask(const DownloadTask& task) {
    m_pendingTasks.enqueue(task);
    m_totalCount++;
}

void DownloadManager::addTasks(const QList<DownloadTask>& tasks) {
    for (const auto& task : tasks) {
        addTask(task);
    }
}

void DownloadManager::start() {
    if (m_isRunning) return;
    m_isRunning = true;
    m_completedCount = 0;

    if (m_pendingTasks.isEmpty()) {
        m_isRunning = false;
        emit allFinished();
        return;
    }

    processQueue();
}

void DownloadManager::cancel() {
    m_isRunning = false;
    for (auto it = m_activeTasks.begin(); it != m_activeTasks.end(); ++it) {
        it.key()->abort();
        it.key()->deleteLater();
    }
    m_activeTasks.clear();
    m_pendingTasks.clear();
    m_totalCount = 0;
    m_completedCount = 0;
}

bool DownloadManager::isRunning() const {
    return m_isRunning;
}

qint64 DownloadManager::totalTasksCount() const {
    return m_totalCount;
}

qint64 DownloadManager::completedTasksCount() const {
    return m_completedCount;
}

bool DownloadManager::checkExistingFileValid(const DownloadTask& task) {
    if (!QFile::exists(task.destinationPath)) {
        return false;
    }

    if (!task.expectedSha256.isEmpty()) {
        return Utils::HashUtils::verifySha256(task.destinationPath, task.expectedSha256);
    }

    if (!task.expectedSha1.isEmpty()) {
        return Utils::HashUtils::verifySha1(task.destinationPath, task.expectedSha1);
    }

    if (task.size > 0) {
        return QFileInfo(task.destinationPath).size() == task.size;
    }

    return true;
}

void DownloadManager::processQueue() {
    if (!m_isRunning) return;

    while (m_activeTasks.size() < m_maxConcurrent && !m_pendingTasks.isEmpty()) {
        DownloadTask task = m_pendingTasks.dequeue();

        // Check if file is already valid on disk
        if (checkExistingFileValid(task)) {
            m_completedCount++;
            double percent = m_totalCount > 0 ? (double)m_completedCount / m_totalCount : 1.0;
            emit progressChanged(m_completedCount, m_totalCount, percent, QFileInfo(task.destinationPath).fileName());
            continue;
        }

        // Prepare destination directory
        Utils::FileUtils::ensureDir(QFileInfo(task.destinationPath).absolutePath());

        QNetworkRequest request(task.url);
        request.setRawHeader("User-Agent", "LCALauncher/1.0");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply* reply = m_network.get(request);
        m_activeTasks.insert(reply, task);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            onReplyFinished(reply);
        });
    }

    if (m_activeTasks.isEmpty() && m_pendingTasks.isEmpty()) {
        m_isRunning = false;
        emit allFinished();
    }
}

void DownloadManager::onReplyFinished(QNetworkReply* reply) {
    if (!m_activeTasks.contains(reply)) {
        reply->deleteLater();
        return;
    }

    DownloadTask task = m_activeTasks.take(reply);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit taskFailed(task.destinationPath, reply->errorString());
    } else {
        QByteArray data = reply->readAll();
        QFile file(task.destinationPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();

            // Verify integrity
            bool valid = true;
            if (!task.expectedSha256.isEmpty()) {
                valid = Utils::HashUtils::verifySha256(task.destinationPath, task.expectedSha256);
            } else if (!task.expectedSha1.isEmpty()) {
                valid = Utils::HashUtils::verifySha1(task.destinationPath, task.expectedSha1);
            }

            if (!valid) {
                QFile::remove(task.destinationPath);
                emit taskFailed(task.destinationPath, "Контрольная сумма файла не совпадает");
            } else {
                emit fileDownloaded(task.destinationPath);
            }
        } else {
            emit taskFailed(task.destinationPath, "Не удалось сохранить файл на диск");
        }
    }

    m_completedCount++;
    double percent = m_totalCount > 0 ? (double)m_completedCount / m_totalCount : 1.0;
    emit progressChanged(m_completedCount, m_totalCount, percent, QFileInfo(task.destinationPath).fileName());

    processQueue();
}

} // namespace LCA::Downloader
