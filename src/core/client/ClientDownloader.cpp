#include "ClientDownloader.h"
#include "core/utils/FileUtils.h"
#include "core/utils/ArchiveUtils.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QUrl>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>
#include <QMetaObject>

namespace LCA::Client {

namespace {

// Runs in worker thread: extract, unwrap nested dir, verify layout.
bool extractClientArchive(const QString& zipPath, const QString& dataDir) {
    if (!Utils::ArchiveUtils::extractZip(zipPath, dataDir)) return false;
    Utils::FileUtils::removeFile(zipPath);

    // Auto-unwrap if archive had an enclosing root folder
    if (!QDir(dataDir + "/versions").exists() || !QDir(dataDir + "/libraries").exists()) {
        QDir baseDir(dataDir);
        QStringList subdirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& sub : subdirs) {
            if (sub == "temp" || sub == "runtime" || sub == "game") continue;
            QString subPath = baseDir.filePath(sub);
            if (QDir(subPath + "/versions").exists() || QDir(subPath + "/libraries").exists()) {
                QDir nestedDir(subPath);
                QStringList entries = nestedDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QString& entry : entries) {
                    QString src = nestedDir.filePath(entry);
                    QString dst = baseDir.filePath(entry);
                    if (QFileInfo(src).isDir()) Utils::FileUtils::removeDir(dst);
                    else Utils::FileUtils::removeFile(dst);
                    QFile::rename(src, dst);
                }
                nestedDir.removeRecursively();
                break;
            }
        }
    }
    return ClientDownloader::isClientInstalled(dataDir);
}

} // namespace

ClientDownloader::ClientDownloader(QObject* parent)
    : QObject(parent)
{
}

ClientDownloader::~ClientDownloader() {
    cancel();
}

bool ClientDownloader::isClientInstalled(const QString& launcherDataDir) {
    if (launcherDataDir.isEmpty()) return false;

    QString libDir = launcherDataDir + "/libraries";
    if (!QDir(libDir).exists()) return false;

    QDirIterator it(libDir, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
    if (!it.hasNext()) return false;

    QDir versionsDir(launcherDataDir + "/versions");
    if (!versionsDir.exists()) return false;

    QDirIterator vIt(versionsDir.path(), QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
    return vIt.hasNext();
}

void ClientDownloader::startDownload(const QString& downloadUrl, const QString& launcherDataDir) {
    if (m_isDownloading) return;

    m_downloadUrl = downloadUrl.trimmed();
    m_launcherDataDir = launcherDataDir;

    if (m_downloadUrl.isEmpty()) {
        emit downloadFinished(false, "URL для загрузки базового клиента не указан в настройках");
        return;
    }

    QString tempDir = m_launcherDataDir + "/temp";
    Utils::FileUtils::ensureDir(tempDir);
    m_tempZipPath = tempDir + "/client-base.zip";
    Utils::FileUtils::removeFile(m_tempZipPath);

    m_retryCount = 0;
    m_totalBytes = 0;
    m_resumeOffset = 0;
    m_isDownloading = true;

    emit statusChanged("Подключение к серверу для загрузки клиента 1.21.1 NeoForge...");
    emit progressChanged(0.0, 0, 0);

    beginAttempt();
}

void ClientDownloader::beginAttempt() {
    if (!m_isDownloading) return;

    const QFileInfo existingInfo(m_tempZipPath);
    m_resumeOffset = existingInfo.exists() ? existingInfo.size() : 0;
    m_headersChecked = false;

    m_outputFile = new QFile(m_tempZipPath, this);
    const QIODevice::OpenMode mode = (m_resumeOffset > 0)
        ? (QIODevice::ReadWrite | QIODevice::Append)
        : (QIODevice::WriteOnly | QIODevice::Truncate);
    if (!m_outputFile->open(mode)) {
        delete m_outputFile;
        m_outputFile = nullptr;
        m_isDownloading = false;
        emit downloadFinished(false, "Не удалось создать временный файл: " + m_tempZipPath);
        return;
    }

    QNetworkRequest request{QUrl(m_downloadUrl)};
    request.setRawHeader("User-Agent", "LCALauncher/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);
    if (m_retryCount > 0) {
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
        request.setRawHeader("Connection", "close");
    }
    if (m_resumeOffset > 0) {
        request.setRawHeader("Range", QString("bytes=%1-").arg(m_resumeOffset).toUtf8());
    }

    m_currentReply = m_network.get(request);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &ClientDownloader::onDownloadProgress);
    connect(m_currentReply, &QIODevice::readyRead, this, &ClientDownloader::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &ClientDownloader::onReplyFinished);
}

void ClientDownloader::handleReadyRead() {
    if (!m_outputFile || !m_currentReply) return;

    if (!m_headersChecked) {
        m_headersChecked = true;
        const int httpCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (m_resumeOffset > 0 && httpCode != 206) {
            m_resumeOffset = 0;
            m_outputFile->resize(0);
            m_outputFile->seek(0);
        }
    }

    m_outputFile->write(m_currentReply->readAll());
}

void ClientDownloader::cancel() {
    if (!m_isDownloading) return;
    m_isDownloading = false;
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    if (m_outputFile) {
        m_outputFile->close();
        m_outputFile->deleteLater();
        m_outputFile = nullptr;
        Utils::FileUtils::removeFile(m_tempZipPath);
    }
}

void ClientDownloader::onDownloadProgress(qint64 received, qint64 total) {
    const qint64 effectiveReceived = m_resumeOffset + received;
    const qint64 effectiveTotal = (total > 0) ? (m_resumeOffset + total) : m_totalBytes;
    if (effectiveTotal > m_totalBytes) {
        m_totalBytes = effectiveTotal;
    }

    if (effectiveTotal > 0) {
        double percent = qBound(0.0, static_cast<double>(effectiveReceived) / static_cast<double>(effectiveTotal), 1.0);
        double mbReceived = static_cast<double>(effectiveReceived) / (1024.0 * 1024.0);
        double mbTotal = static_cast<double>(effectiveTotal) / (1024.0 * 1024.0);
        emit statusChanged(QString("Загрузка клиента Minecraft 1.21.1 NeoForge: %1 МБ из %2 МБ (%3%)")
                               .arg(mbReceived, 0, 'f', 1)
                               .arg(mbTotal, 0, 'f', 1)
                               .arg(static_cast<int>(percent * 100)));
        emit progressChanged(percent, effectiveReceived, effectiveTotal);
    } else {
        double mbReceived = static_cast<double>(effectiveReceived) / (1024.0 * 1024.0);
        emit statusChanged(QString("Загрузка клиента Minecraft 1.21.1 NeoForge: %1 МБ...")
                               .arg(mbReceived, 0, 'f', 1));
    }
}

void ClientDownloader::onReplyFinished() {
    if (!m_isDownloading || !m_currentReply) return;

    QNetworkReply::NetworkError error = m_currentReply->error();
    QString errorStr = m_currentReply->errorString();
    int httpCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    if (m_outputFile) {
        m_outputFile->flush();
        m_outputFile->close();
        delete m_outputFile;
        m_outputFile = nullptr;
    }

    const bool canRetry = (error != QNetworkReply::NoError && httpCode != 404 && httpCode != 403 && httpCode != 401);
    if (canRetry && m_retryCount < 5) {
        m_retryCount++;
        m_network.clearConnectionCache();
        emit statusChanged(QString("Задержка ответа сервера, повторная попытка %1/5...").arg(m_retryCount));
        QTimer::singleShot(1500, this, &ClientDownloader::beginAttempt);
        return;
    }

    m_isDownloading = false;

    if (error != QNetworkReply::NoError || (httpCode >= 400 && httpCode < 600)) {
        Utils::FileUtils::removeFile(m_tempZipPath);
        if (error == QNetworkReply::OperationCanceledError) {
            errorStr = "Таймаут ожидания данных от сервера";
        }
        QString msg = (httpCode == 404)
                          ? QString("Файл базового клиента не найден (404) по адресу:\n%1").arg(m_downloadUrl)
                          : QString("Ошибка загрузки базового клиента: %1 (HTTP %2)").arg(errorStr).arg(httpCode);
        emit downloadFinished(false, msg);
        return;
    }

    emit statusChanged("Распаковка клиента Minecraft 1.21.1 NeoForge...");
    emit progressChanged(1.0, 100, 100);

    // Extract in worker thread; do NOT block the GUI.
    // Note: QThread::create() auto-deletes the thread object on finish — no manual setAutoDelete().
    const QString zip = m_tempZipPath;
    const QString dataDir = m_launcherDataDir;
    QThread* worker = QThread::create([this, zip, dataDir]() {
        bool ok = extractClientArchive(zip, dataDir);
        QMetaObject::invokeMethod(this, [this, ok]() {
            if (!ok) {
                emit downloadFinished(false, "Не удалось распаковать архив client-base.zip или он не содержит versions/ и libraries/");
                return;
            }
            emit statusChanged("Клиент Minecraft 1.21.1 NeoForge успешно установлен");
            emit downloadFinished(true, "");
        }, Qt::QueuedConnection);
    });
    worker->start();
}

} // namespace LCA::Client