#include "ClientDownloader.h"
#include "core/utils/FileUtils.h"
#include "core/utils/ArchiveUtils.h"
#include <QDir>
#include <QDirIterator>
#include <QUrl>
#include <QNetworkRequest>

namespace LCA::Client {

ClientDownloader::ClientDownloader(QObject* parent)
    : QObject(parent)
{
}

ClientDownloader::~ClientDownloader() {
    cancel();
}

bool ClientDownloader::isClientInstalled(const QString& launcherDataDir) {
    if (launcherDataDir.isEmpty()) return false;

    // 1. Check for client jar (either NeoForge or Vanilla 1.21.1)
    bool hasClientJar = QFile::exists(launcherDataDir + "/versions/1.21.1-neoforge/1.21.1-neoforge.jar") ||
                        QFile::exists(launcherDataDir + "/versions/1.21.1/1.21.1.jar");
    if (!hasClientJar) return false;

    // 2. Check for libraries directory containing .jar files
    QString libDir = launcherDataDir + "/libraries";
    if (!QDir(libDir).exists()) return false;

    QDirIterator it(libDir, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
    if (!it.hasNext()) {
        return false; // Libraries directory is empty
    }

    return true;
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

    // Clean up previous temp archive if any
    Utils::FileUtils::removeFile(m_tempZipPath);

    m_outputFile = new QFile(m_tempZipPath, this);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        delete m_outputFile;
        m_outputFile = nullptr;
        emit downloadFinished(false, "Не удалось создать временный файл: " + m_tempZipPath);
        return;
    }

    emit statusChanged("Подключение к серверу для загрузки клиента 1.21.1 NeoForge...");
    emit progressChanged(0.0, 0, 0);

    QNetworkRequest request{QUrl(m_downloadUrl)};
    request.setRawHeader("User-Agent", "LCALauncher/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_currentReply = m_network.get(request);
    m_isDownloading = true;

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &ClientDownloader::onDownloadProgress);
    connect(m_currentReply, &QIODevice::readyRead, this, [this]() {
        if (m_outputFile && m_currentReply) {
            m_outputFile->write(m_currentReply->readAll());
        }
    });
    connect(m_currentReply, &QNetworkReply::finished, this, &ClientDownloader::onReplyFinished);
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
    if (total > 0) {
        double percent = static_cast<double>(received) / static_cast<double>(total);
        double mbReceived = static_cast<double>(received) / (1024.0 * 1024.0);
        double mbTotal = static_cast<double>(total) / (1024.0 * 1024.0);
        emit statusChanged(QString("Загрузка клиента Minecraft 1.21.1 NeoForge: %1 МБ из %2 МБ (%3%)")
                               .arg(mbReceived, 0, 'f', 1)
                               .arg(mbTotal, 0, 'f', 1)
                               .arg(static_cast<int>(percent * 100)));
        emit progressChanged(percent, received, total);
    } else {
        double mbReceived = static_cast<double>(received) / (1024.0 * 1024.0);
        emit statusChanged(QString("Загрузка клиента Minecraft 1.21.1 NeoForge: %1 МБ...")
                               .arg(mbReceived, 0, 'f', 1));
    }
}

void ClientDownloader::onReplyFinished() {
    if (!m_isDownloading || !m_currentReply) return;

    m_isDownloading = false;
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

    if (error != QNetworkReply::NoError || (httpCode >= 400 && httpCode < 600)) {
        Utils::FileUtils::removeFile(m_tempZipPath);
        QString msg = (httpCode == 404)
                          ? QString("Файл базового клиента не найден (404) по адресу:\n%1").arg(m_downloadUrl)
                          : QString("Ошибка загрузки базового клиента: %1 (HTTP %2)").arg(errorStr).arg(httpCode);
        emit downloadFinished(false, msg);
        return;
    }

    // Extraction step
    emit statusChanged("Распаковка клиента Minecraft 1.21.1 NeoForge...");
    emit progressChanged(1.0, 100, 100);

    bool extracted = Utils::ArchiveUtils::extractZip(m_tempZipPath, m_launcherDataDir);
    Utils::FileUtils::removeFile(m_tempZipPath);

    if (!extracted) {
        emit downloadFinished(false, "Не удалось распаковать архив client-base.zip");
        return;
    }

    // Verify presence
    if (!isClientInstalled(m_launcherDataDir)) {
        emit downloadFinished(false, "Распакованный архив не содержит versions/ или libraries/");
        return;
    }

    emit statusChanged("Клиент Minecraft 1.21.1 NeoForge успешно установлен");
    emit downloadFinished(true, "");
}

} // namespace LCA::Client
