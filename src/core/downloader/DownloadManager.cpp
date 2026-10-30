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
    for (const auto& task : tasks) addTask(task);
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

    // Abort & cleanup active replies
    for (auto it = m_activeTasks.begin(); it != m_activeTasks.end(); ++it) {
        QNetworkReply* reply = it.key();
        ActiveTask& active = it.value();
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
        if (active.file) {
            active.file->close();
            active.file->remove();
            active.file->deleteLater();
        }
    }
    m_activeTasks.clear();

    // Drop pending queue
    m_pendingTasks.clear();
    m_totalCount = 0;
    m_completedCount = 0;

    // Notify listeners so they don't hang
    emit allFinished();
}

bool DownloadManager::isRunning() const { return m_isRunning; }
qint64 DownloadManager::totalTasksCount() const { return m_totalCount; }
qint64 DownloadManager::completedTasksCount() const { return m_completedCount; }

bool DownloadManager::checkExistingFileValid(const DownloadTask& task) {
    if (!QFile::exists(task.destinationPath)) return false;
    if (!task.expectedSha256.isEmpty())
        return Utils::HashUtils::verifySha256(task.destinationPath, task.expectedSha256);
    if (!task.expectedSha1.isEmpty())
        return Utils::HashUtils::verifySha1(task.destinationPath, task.expectedSha1);
    if (task.size > 0)
        return QFileInfo(task.destinationPath).size() == task.size;
    return true;
}

void DownloadManager::processQueue() {
    if (!m_isRunning) return;

    while (m_activeTasks.size() < m_maxConcurrent && !m_pendingTasks.isEmpty()) {
        DownloadTask task = m_pendingTasks.dequeue();

        if (checkExistingFileValid(task)) {
            m_completedCount++;
            double percent = m_totalCount > 0 ? (double)m_completedCount / m_totalCount : 1.0;
            emit progressChanged(m_completedCount, m_totalCount, percent, QFileInfo(task.destinationPath).fileName());
            continue;
        }

        Utils::FileUtils::ensureDir(QFileInfo(task.destinationPath).absolutePath());

        // Write to a .part file, then atomically rename on success
        const QString partPath = task.destinationPath + ".part";
        Utils::FileUtils::removeFile(partPath);

        QFile* file = new QFile(partPath);
        if (!file->open(QIODevice::WriteOnly)) {
            delete file;
            emit taskFailed(task.destinationPath, "Не удалось создать файл: " + partPath);
            m_completedCount++;
            continue;
        }

        QNetworkRequest request(task.url);
        request.setRawHeader("User-Agent", "LCALauncher/1.0");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setTransferTimeout(60000);

        QNetworkReply* reply = m_network.get(request);
        m_activeTasks.insert(reply, ActiveTask{task, file});

        // Stream to disk as chunks arrive
        connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
            if (file && file->isOpen()) file->write(reply->readAll());
        });
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
    if (!m_activeTasks.contains(reply)) { reply->deleteLater(); return; }

    ActiveTask active = m_activeTasks.take(reply);
    const DownloadTask task = active.task;
    const QNetworkReply::NetworkError err = reply->error();
    const QString errStr = reply->errorString();

    // Drain any remaining bytes, then close
    if (active.file) {
        if (err == QNetworkReply::NoError) {
            active.file->write(reply->readAll());
        }
        active.file->flush();
        active.file->close();
        active.file->deleteLater();
        active.file = nullptr;
    }
    reply->deleteLater();

    const QString partPath = task.destinationPath + ".part";

    if (err != QNetworkReply::NoError) {
        Utils::FileUtils::removeFile(partPath);
        emit taskFailed(task.destinationPath, errStr);
    } else {
        bool valid = true;
        if (!task.expectedSha256.isEmpty())
            valid = Utils::HashUtils::verifySha256(partPath, task.expectedSha256);
        else if (!task.expectedSha1.isEmpty())
            valid = Utils::HashUtils::verifySha1(partPath, task.expectedSha1);

        if (!valid) {
            Utils::FileUtils::removeFile(partPath);
            emit taskFailed(task.destinationPath, "Контрольная сумма файла не совпадает");
        } else {
            // Atomic replace: remove old, rename .part -> destination
            Utils::FileUtils::removeFile(task.destinationPath);
            if (QFile::rename(partPath, task.destinationPath)) {
                emit fileDownloaded(task.destinationPath);
            } else {
                Utils::FileUtils::removeFile(partPath);
                emit taskFailed(task.destinationPath, "Не удалось переименовать .part в финальный файл");
            }
        }
    }

    m_completedCount++;
    double percent = m_totalCount > 0 ? (double)m_completedCount / m_totalCount : 1.0;
    emit progressChanged(m_completedCount, m_totalCount, percent, QFileInfo(task.destinationPath).fileName());

    processQueue();
}

void DownloadManager::onReplyReadyRead() { /* handled via lambda above */ }

} // namespace LCA::Downloader