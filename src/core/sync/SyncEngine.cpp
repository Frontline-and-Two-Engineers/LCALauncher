#include "SyncEngine.h"
#include "core/utils/FileUtils.h"
#include "core/utils/HashUtils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>

namespace LCA::Sync {

SyncEngine::SyncEngine(const QString& gameDir, QObject* parent)
    : QObject(parent)
    , m_gameDir(gameDir)
    , m_downloader(new Downloader::DownloadManager(6, this))
{
    connect(m_downloader, &Downloader::DownloadManager::progressChanged,
            this, &SyncEngine::progressChanged);

    connect(m_downloader, &Downloader::DownloadManager::allFinished,
            this, &SyncEngine::onDownloaderFinished);

    connect(m_downloader, &Downloader::DownloadManager::taskFailed,
            this, [this](const QString& file, const QString& error) {
        // Collect, do not fail immediately — allow remaining files to finish
        m_pendingErrors.append(QString("%1: %2").arg(QFileInfo(file).fileName(), error));
        emit statusChanged(QString("Ошибка загрузки: %1").arg(QFileInfo(file).fileName()));
    });
}

void SyncEngine::setManifestUrl(const QString& url) { m_manifestUrl = url; }

bool SyncEngine::isSafeRelativePath(const QString& path) {
    if (path.isEmpty()) return false;
    if (path.startsWith('/') || path.startsWith('\\')) return false;
    if (path.contains("..")) return false;
    // Reject Windows drive letters (C:)
    if (path.length() >= 2 && path[1] == ':') return false;
    return true;
}

void SyncEngine::startSync() {
    emit statusChanged("Получение манифеста сервера...");
    Utils::FileUtils::ensureDir(m_gameDir);
    Utils::FileUtils::ensureDir(m_gameDir + "/mods");
    Utils::FileUtils::ensureDir(m_gameDir + "/config");

    m_pendingErrors.clear();
    m_pendingTotal = 0;

    if (m_manifestUrl.isEmpty() || m_manifestUrl.startsWith("file://")) {
        emit statusChanged("Локальный режим: пропуск синхронизации с сервером");
        emit syncFinished(true, "Готово");
        return;
    }

    QNetworkRequest request{QUrl(m_manifestUrl)};
    request.setRawHeader("User-Agent", "LCALauncher/1.0");
    request.setTransferTimeout(15000);

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onManifestDownloaded(reply);
    });
}

void SyncEngine::cancel() { m_downloader->cancel(); }

void SyncEngine::onManifestDownloaded(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        emit statusChanged("Сервер синхронизации недоступен. Проверка локальных файлов...");
        emit syncFinished(true, "Сервер недоступен, запуск с текущими локальными модами");
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit syncFinished(false, "Неверный формат манифеста сервера");
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray files = root["files"].toArray();

    QList<SyncItem> items;
    QSet<QString> allowedPaths;
    int rejected = 0;

    for (const auto& val : files) {
        QJsonObject f = val.toObject();
        SyncItem item;
        item.relativePath = f["path"].toString();
        item.downloadUrl = QUrl(f["url"].toString());
        item.sha256 = f["sha256"].toString();
        item.size = f["size"].toInteger(0);

        // Security: reject path-traversal / absolute paths
        if (!isSafeRelativePath(item.relativePath)) { ++rejected; continue; }
        // Reject non-http(s) URLs
        const QString scheme = item.downloadUrl.scheme();
        if (scheme != "http" && scheme != "https") { ++rejected; continue; }

        items.append(item);
        allowedPaths.insert(item.relativePath);
    }
    if (rejected > 0) {
        emit statusChanged(QString("Отклонено небезопасных путей в манифесте: %1").arg(rejected));
    }

    emit statusChanged("Проверка чистоты папки модов...");
    cleanObsoleteMods(allowedPaths);

    emit statusChanged(QString("Проверка %1 файлов сервера...").arg(items.size()));
    verifyAndDownloadFiles(items);
}

void SyncEngine::cleanObsoleteMods(const QSet<QString>& allowedRelativePaths) {
    QDir modsDir(m_gameDir + "/mods");
    if (!modsDir.exists()) return;

    QStringList files = modsDir.entryList(QStringList() << "*.jar", QDir::Files);
    for (const QString& file : files) {
        QString relPath = "mods/" + file;
        if (!allowedRelativePaths.contains(relPath)) modsDir.remove(file);
    }
}

void SyncEngine::verifyAndDownloadFiles(const QList<SyncItem>& items) {
    QList<Downloader::DownloadTask> tasks;

    for (const auto& item : items) {
        QString fullPath = m_gameDir + "/" + item.relativePath;
        bool needsDownload = true;
        if (QFile::exists(fullPath) && Utils::HashUtils::verifySha256(fullPath, item.sha256)) {
            needsDownload = false;
        }
        if (needsDownload) {
            Downloader::DownloadTask task;
            task.url = item.downloadUrl;
            task.destinationPath = fullPath;
            task.expectedSha256 = item.sha256;
            task.size = item.size;
            task.description = item.relativePath;
            tasks.append(task);
        }
    }

    if (tasks.isEmpty()) {
        emit statusChanged("Все моды и конфиги актуальны");
        emit syncFinished(true, "Файлы сервера в актуальном состоянии");
        return;
    }

    m_pendingTotal = tasks.size();
    emit statusChanged(QString("Загрузка обновлений (%1 файлов)...").arg(tasks.size()));
    m_downloader->addTasks(tasks);
    m_downloader->start();
}

void SyncEngine::onDownloaderFinished() {
    if (m_pendingErrors.isEmpty()) {
        emit statusChanged("Синхронизация завершена");
        emit syncFinished(true, "Файлы сервера проверены и обновлены");
    } else {
        const int failed = m_pendingErrors.size();
        const QString summary = QString("Не удалось загрузить %1 из %2 файлов:\n%3")
                                    .arg(failed)
                                    .arg(m_pendingTotal)
                                    .arg(m_pendingErrors.mid(0, 5).join("\n")
                                         + (failed > 5 ? QString("\n... и ещё %1").arg(failed - 5) : QString()));
        emit syncFinished(false, summary);
    }
}

} // namespace LCA::Sync