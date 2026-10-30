#include "JavaDownloader.h"
#include "JavaDetector.h"
#include "core/utils/FileUtils.h"
#include "core/utils/ArchiveUtils.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QSysInfo>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <QMetaObject>

namespace LCA::Java {

namespace {

// Runs in worker thread: extract, locate java binary, chmod, validate.
struct ExtractResult {
    bool success{false};
    QString javaPath;
    QString error;
};

ExtractResult extractAndValidateJava(const QString& archivePath, const QString& extractDir) {
    ExtractResult result;
    if (!Utils::ArchiveUtils::extractTarGz(archivePath, extractDir)
        && !Utils::ArchiveUtils::extractZip(archivePath, extractDir)) {
        result.error = "Не удалось распаковать архив Java";
        return result;
    }
    Utils::FileUtils::removeFile(archivePath);

#if defined(Q_OS_WIN)
    const QString targetName = "java.exe";
#else
    const QString targetName = "java";
#endif

    QString javaExe;
    QDirIterator it(extractDir, QStringList() << targetName, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        if (path.contains("/bin/") || path.contains("\\bin\\")) { javaExe = path; break; }
    }

    if (javaExe.isEmpty() || !QFile::exists(javaExe)) {
        result.error = "Исполняемый файл java не найден в распакованном каталоге";
        return result;
    }

#if !defined(Q_OS_WIN)
    QFile::setPermissions(javaExe,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadGroup | QFileDevice::ExeGroup |
        QFileDevice::ReadOther | QFileDevice::ExeOther);
#endif

    JavaInfo info = JavaDetector::inspectJava(javaExe);
    if (!info.isValid || info.majorVersion < 21) {
        result.error = QString("Установленная Java не прошла проверку (версия: %1)").arg(info.majorVersion);
        return result;
    }

    result.success = true;
    result.javaPath = javaExe;
    return result;
}

} // namespace

JavaDownloader::JavaDownloader(QObject* parent)
    : QObject(parent)
{
}

JavaDownloader::~JavaDownloader() { cancel(); }

bool JavaDownloader::isDownloading() const { return m_isDownloading; }

QString JavaDownloader::getAdoptiumUrl() const {
    QString os;
#if defined(Q_OS_WIN)
    os = "windows";
#elif defined(Q_OS_MACOS)
    os = "mac";
#elif defined(Q_OS_LINUX)
    os = "linux";
#else
    os = "linux";
#endif

    QString arch = "x64";
    QString cpu = QSysInfo::currentCpuArchitecture().toLower();
    if (cpu == "arm64" || cpu == "aarch64") arch = "aarch64";

    return QString("https://api.adoptium.net/v3/binary/latest/21/ga/%1/%2/jre/hotspot/normal/eclipse")
        .arg(os, arch);
}

QString JavaDownloader::getTargetArchiveExtension() const {
#if defined(Q_OS_WIN)
    return ".zip";
#else
    return ".tar.gz";
#endif
}

void JavaDownloader::startDownload() {
    if (m_isDownloading) return;

    QString dataDir = Utils::FileUtils::defaultLauncherDataDir();
    m_extractDir = dataDir + "/runtime/java-21";
    Utils::FileUtils::ensureDir(dataDir + "/runtime");

    m_tempArchivePath = dataDir + "/runtime/temurin-21" + getTargetArchiveExtension();
    Utils::FileUtils::removeFile(m_tempArchivePath);

    m_outputFile = new QFile(m_tempArchivePath, this);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        emit downloadFinished(false, "", "Не удалось создать временный файл для загрузки Java");
        delete m_outputFile;
        m_outputFile = nullptr;
        return;
    }

    QString downloadUrl = getAdoptiumUrl();
    emit statusChanged("Подключение к Adoptium API (Java 21 JRE)...");

    QNetworkRequest request{QUrl(downloadUrl)};
    request.setRawHeader("User-Agent", "LCALauncher/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(180000);

    m_currentReply = m_network.get(request);
    m_isDownloading = true;

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &JavaDownloader::onDownloadProgress);
    connect(m_currentReply, &QIODevice::readyRead, this, [this]() {
        if (m_outputFile && m_currentReply) m_outputFile->write(m_currentReply->readAll());
    });
    connect(m_currentReply, &QNetworkReply::finished, this, &JavaDownloader::onReplyFinished);
}

void JavaDownloader::cancel() {
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
        Utils::FileUtils::removeFile(m_tempArchivePath);
    }
}

void JavaDownloader::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        double percent = static_cast<double>(received) / static_cast<double>(total);
        double mbReceived = static_cast<double>(received) / (1024.0 * 1024.0);
        double mbTotal = static_cast<double>(total) / (1024.0 * 1024.0);
        emit statusChanged(QString("Загрузка Java 21: %1 МБ из %2 МБ (%3%)")
                               .arg(mbReceived, 0, 'f', 1)
                               .arg(mbTotal, 0, 'f', 1)
                               .arg(static_cast<int>(percent * 100)));
        emit progressChanged(percent, received, total);
    } else {
        emit statusChanged("Загрузка Java 21...");
    }
}

void JavaDownloader::onReplyFinished() {
    if (!m_isDownloading || !m_currentReply) return;

    m_isDownloading = false;
    QNetworkReply::NetworkError error = m_currentReply->error();
    QString errorStr = m_currentReply->errorString();

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    if (m_outputFile) {
        m_outputFile->flush();
        m_outputFile->close();
        delete m_outputFile;
        m_outputFile = nullptr;
    }

    if (error != QNetworkReply::NoError) {
        Utils::FileUtils::removeFile(m_tempArchivePath);
        emit downloadFinished(false, "", "Ошибка загрузки Java: " + errorStr);
        return;
    }

    emit statusChanged("Распаковка Java Runtime...");
    emit progressChanged(1.0, 100, 100);

    Utils::FileUtils::removeDir(m_extractDir);
    Utils::FileUtils::ensureDir(m_extractDir);

    // Extract in worker thread; do NOT block the GUI.
    // Note: QThread::create() auto-deletes the thread object on finish — no manual setAutoDelete().
    const QString archive = m_tempArchivePath;
    const QString dir = m_extractDir;
    QThread* worker = QThread::create([this, archive, dir]() {
        ExtractResult result = extractAndValidateJava(archive, dir);
        QMetaObject::invokeMethod(this, [this, result]() {
            if (!result.success) {
                emit downloadFinished(false, "", result.error);
                return;
            }
            emit statusChanged("Java 21 готова к работе");
            emit downloadFinished(true, result.javaPath, "");
        }, Qt::QueuedConnection);
    });
    worker->start();
}

bool JavaDownloader::extractArchive(const QString& archivePath, const QString& destinationDir) {
#if defined(Q_OS_WIN)
    return Utils::ArchiveUtils::extractZip(archivePath, destinationDir);
#else
    return Utils::ArchiveUtils::extractTarGz(archivePath, destinationDir);
#endif
}

QString JavaDownloader::findJavaExecutable(const QString& rootDir) const {
#if defined(Q_OS_WIN)
    const QString targetName = "java.exe";
#else
    const QString targetName = "java";
#endif
    QDirIterator it(rootDir, QStringList() << targetName, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        if (path.contains("/bin/") || path.contains("\\bin\\")) return path;
    }
    return {};
}

} // namespace LCA::Java