#include "JavaDownloader.h"
#include "JavaDetector.h"
#include "core/utils/FileUtils.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QSysInfo>
#include <QFileInfo>
#include <QStandardPaths>

namespace LCA::Java {

JavaDownloader::JavaDownloader(QObject* parent)
    : QObject(parent)
{
}

JavaDownloader::~JavaDownloader() {
    cancel();
}

bool JavaDownloader::isDownloading() const {
    return m_isDownloading;
}

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
    if (cpu == "arm64" || cpu == "aarch64") {
        arch = "aarch64";
    }

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
    // Follow HTTP 307 redirects to CDN/mirrors automatically
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_currentReply = m_network.get(request);
    m_isDownloading = true;

    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &JavaDownloader::onDownloadProgress);
    connect(m_currentReply, &QIODevice::readyRead, this, [this]() {
        if (m_outputFile && m_currentReply) {
            m_outputFile->write(m_currentReply->readAll());
        }
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

    // Extraction step
    emit statusChanged("Распаковка Java Runtime...");
    emit progressChanged(1.0, 100, 100);

    // Prepare clean destination directory
    Utils::FileUtils::removeDir(m_extractDir);
    Utils::FileUtils::ensureDir(m_extractDir);

    bool extracted = extractArchive(m_tempArchivePath, m_extractDir);
    Utils::FileUtils::removeFile(m_tempArchivePath); // Delete archive after unpacking

    if (!extracted) {
        emit downloadFinished(false, "", "Не удалось распаковать архив Java");
        return;
    }

    // Locate the java binary
    QString javaExe = findJavaExecutable(m_extractDir);
    if (javaExe.isEmpty() || !QFile::exists(javaExe)) {
        emit downloadFinished(false, "", "Исполняемый файл java не найден в распакованном каталоге");
        return;
    }

    // Set executable permissions on macOS / Linux
#if !defined(Q_OS_WIN)
    QFile::setPermissions(javaExe, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                   QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                   QFileDevice::ReadOther | QFileDevice::ExeOther);
#endif

    // Validate version
    JavaInfo info = JavaDetector::inspectJava(javaExe);
    if (!info.isValid || info.majorVersion < 21) {
        emit downloadFinished(false, "", QString("Установленная Java не прошла проверку (версия: %1)").arg(info.majorVersion));
        return;
    }

    emit statusChanged(QString("Java 21 готова к работе (%1)").arg(info.versionString));
    emit downloadFinished(true, javaExe, "");
}

bool JavaDownloader::extractArchive(const QString& archivePath, const QString& destinationDir) {
    QProcess process;

#if defined(Q_OS_WIN)
    // Try built-in tar on Windows 10/11 first
    QString tarPath = QStandardPaths::findExecutable("tar.exe");
    if (!tarPath.isEmpty()) {
        process.start(tarPath, {"-xf", archivePath, "-C", destinationDir});
        if (process.waitForFinished(60000) && process.exitCode() == 0) {
            return true;
        }
    }

    // Fallback: PowerShell Expand-Archive
    QString powershell = QStandardPaths::findExecutable("powershell.exe");
    if (!powershell.isEmpty()) {
        QString cmd = QString("Expand-Archive -Force -Path '%1' -DestinationPath '%2'")
                          .arg(archivePath, destinationDir);
        process.start(powershell, {"-NoProfile", "-NonInteractive", "-Command", cmd});
        if (process.waitForFinished(90000) && process.exitCode() == 0) {
            return true;
        }
    }
    return false;
#else
    // macOS and Linux tar
    process.start("tar", {"-xzf", archivePath, "-C", destinationDir});
    return process.waitForFinished(60000) && process.exitCode() == 0;
#endif
}

QString JavaDownloader::findJavaExecutable(const QString& rootDir) const {
#if defined(Q_OS_WIN)
    const QString targetName = "java.exe";
#else
    const QString targetName = "java";
#endif

    // Recursive search in rootDir for target executable
    QDirIterator it(rootDir, QStringList() << targetName, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        // Skip jspawnhelper or other binaries, ensure it's in a /bin directory
        if (path.contains("/bin/") || path.contains("\\bin\\")) {
            return path;
        }
    }
    return {};
}

} // namespace LCA::Java
