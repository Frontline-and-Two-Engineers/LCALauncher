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
    
    connect(m_downloader, &Downloader::DownloadManager::allFinished, this, [this]() {
        emit statusChanged("Синхронизация завершена");
        emit syncFinished(true, "Файлы сервера проверены и обновлены");
    });

    connect(m_downloader, &Downloader::DownloadManager::taskFailed, this, [this](const QString& file, const QString& error) {
        emit syncFinished(false, QString("Ошибка загрузки %1: %2").arg(QFileInfo(file).fileName(), error));
    });
}

void SyncEngine::setManifestUrl(const QString& url) {
    m_manifestUrl = url;
}

void SyncEngine::startSync() {
    emit statusChanged("Получение манифеста сервера...");
    Utils::FileUtils::ensureDir(m_gameDir);
    Utils::FileUtils::ensureDir(m_gameDir + "/mods");
    Utils::FileUtils::ensureDir(m_gameDir + "/config");

    // If manifest URL is empty or offline test mode
    if (m_manifestUrl.isEmpty() || m_manifestUrl.startsWith("file://")) {
        emit statusChanged("Локальный режим: пропуск синхронизации с сервером");
        emit syncFinished(true, "Готово");
        return;
    }

    QNetworkRequest request{QUrl(m_manifestUrl)};
    request.setRawHeader("User-Agent", "LCALauncher/1.0");

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onManifestDownloaded(reply);
    });
}

void SyncEngine::cancel() {
    m_downloader->cancel();
}

void SyncEngine::onManifestDownloaded(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        // If server is unreachable, we allow playing if local mods exist, but warn user
        emit statusChanged("Сервер синхронизации недоступен. Проверка локальных файлов...");
        emit syncFinished(true, "Сервер недоступен, запуск с текущими локальными модами");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit syncFinished(false, "Неверный формат манифеста сервера");
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray files = root["files"].toArray();

    QList<SyncItem> items;
    QSet<QString> allowedPaths;

    for (const auto& val : files) {
        QJsonObject f = val.toObject();
        SyncItem item;
        item.relativePath = f["path"].toString();
        item.downloadUrl = QUrl(f["url"].toString());
        item.sha256 = f["sha256"].toString();
        item.size = f["size"].toInteger(0);

        if (!item.relativePath.isEmpty() && item.downloadUrl.isValid()) {
            items.append(item);
            allowedPaths.insert(item.relativePath);
        }
    }

    // 1. Clean foreign mods in mods/
    emit statusChanged("Проверка чистоты папки модов...");
    cleanObsoleteMods(allowedPaths);

    // 2. Download missing or modified files
    emit statusChanged(QString("Проверка %1 файлов сервера...").arg(items.size()));
    verifyAndDownloadFiles(items);
}

void SyncEngine::cleanObsoleteMods(const QSet<QString>& allowedRelativePaths) {
    QDir modsDir(m_gameDir + "/mods");
    if (!modsDir.exists()) return;

    QStringList files = modsDir.entryList(QStringList() << "*.jar", QDir::Files);
    for (const QString& file : files) {
        QString relPath = "mods/" + file;
        if (!allowedRelativePaths.contains(relPath)) {
            modsDir.remove(file);
        }
    }
}

void SyncEngine::verifyAndDownloadFiles(const QList<SyncItem>& items) {
    QList<Downloader::DownloadTask> tasks;

    for (const auto& item : items) {
        QString fullPath = m_gameDir + "/" + item.relativePath;

        bool needsDownload = true;
        if (QFile::exists(fullPath)) {
            if (Utils::HashUtils::verifySha256(fullPath, item.sha256)) {
                needsDownload = false;
            }
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

    emit statusChanged(QString("Загрузка обновлений (%1 файлов)...").arg(tasks.size()));
    m_downloader->addTasks(tasks);
    m_downloader->start();
}

} // namespace LCA::Sync
