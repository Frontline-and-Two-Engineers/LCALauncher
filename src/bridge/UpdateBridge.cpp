#include "UpdateBridge.h"
#include "core/utils/ArchiveUtils.h"
#include "core/utils/FileUtils.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>
#include <QtConcurrent>

namespace {

const char* kLatestReleaseApiUrl = "https://api.github.com/repos/Frontline-and-Two-Engineers/LCALauncher/releases/latest";
const char* kReleasesListApiUrl = "https://api.github.com/repos/Frontline-and-Two-Engineers/LCALauncher/releases?per_page=10";
const char* kFallbackReleasesUrl = "https://github.com/Frontline-and-Two-Engineers/LCALauncher/releases";

QString stripVersionPrefix(QString tag) {
    if (tag.startsWith('v', Qt::CaseInsensitive)) {
        tag.remove(0, 1);
    }
    return tag.trimmed();
}

int compareVersions(const QString& remoteStr, const QString& localStr, bool* valid = nullptr) {
    qsizetype rIdx = 0;
    qsizetype lIdx = 0;
    const QVersionNumber rVer = QVersionNumber::fromString(remoteStr, &rIdx);
    const QVersionNumber lVer = QVersionNumber::fromString(localStr, &lIdx);
    if (rVer.isNull() || lVer.isNull()) {
        if (valid) *valid = false;
        return 0;
    }
    if (valid) *valid = true;

    const int numCmp = QVersionNumber::compare(rVer, lVer);
    if (numCmp != 0) {
        return numCmp;
    }

    const QString rSuffix = remoteStr.mid(rIdx).trimmed();
    const QString lSuffix = localStr.mid(lIdx).trimmed();
    if (rSuffix == lSuffix) return 0;
    if (rSuffix.isEmpty()) return 1;
    if (lSuffix.isEmpty()) return -1;
    return QString::compare(rSuffix, lSuffix, Qt::CaseInsensitive);
}

bool assetMatchesCurrentPlatform(const QString& name) {
#if defined(Q_OS_WIN)
    return name.compare("LCALauncher-Windows-x64.zip", Qt::CaseInsensitive) == 0 ||
           (name.contains("Windows", Qt::CaseInsensitive) && name.endsWith(".zip", Qt::CaseInsensitive));
#elif defined(Q_OS_MACOS)
#if defined(Q_PROCESSOR_X86_64)
    return name.compare("LCALauncher-macOS-x86_64.dmg", Qt::CaseInsensitive) == 0 ||
           (name.contains("macOS", Qt::CaseInsensitive) &&
            (name.contains("x86_64", Qt::CaseInsensitive) || name.contains("intel", Qt::CaseInsensitive)) &&
            name.endsWith(".dmg", Qt::CaseInsensitive));
#else
    return name.compare("LCALauncher-macOS.dmg", Qt::CaseInsensitive) == 0 ||
           name.compare("LCALauncher-macOS-arm64.dmg", Qt::CaseInsensitive) == 0 ||
           (name.contains("macOS", Qt::CaseInsensitive) &&
            !name.contains("x86_64", Qt::CaseInsensitive) &&
            !name.contains("intel", Qt::CaseInsensitive) &&
            name.endsWith(".dmg", Qt::CaseInsensitive));
#endif
#else
    return name.compare("LCALauncher-Linux-x86_64.tar.gz", Qt::CaseInsensitive) == 0 ||
           (name.contains("Linux", Qt::CaseInsensitive) && name.endsWith(".tar.gz", Qt::CaseInsensitive));
#endif
}

QNetworkRequest makeGitHubRequest(const QUrl& url, const QString& appVersion, bool isApi) {
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", QString("LCALauncher/%1").arg(appVersion).toUtf8());
    if (isApi) {
        req.setRawHeader("Accept", "application/vnd.github+json");
    }
    req.setTransferTimeout(20000);
    return req;
}

bool writeScriptFile(const QString& path, const QString& content, bool executable) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    if (executable) {
        return QFile::setPermissions(
            path,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
            QFileDevice::ReadGroup | QFileDevice::ExeGroup
        );
    }
    return true;
}

} // namespace

namespace LCA::Bridge {

UpdateBridge::UpdateBridge(QObject* parent)
    : QObject(parent)
    , m_releaseUrl(QString::fromLatin1(kFallbackReleasesUrl)) {
    const QString savedChannel = QSettings().value("updater/channel", "main").toString();
    m_updateChannel = (savedChannel == "dev") ? QStringLiteral("dev") : QStringLiteral("main");

    // Check for updates shortly after startup so splash animation is not delayed
    QTimer::singleShot(2500, this, [this]() {
        checkForUpdates(true);
    });
}

UpdateBridge::~UpdateBridge() {
    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
    }
    cleanupDownloadReply();
}

QString UpdateBridge::currentVersion() const {
    return QCoreApplication::applicationVersion();
}

void UpdateBridge::setUpdateChannel(const QString& channel) {
    const QString normalized = (channel == "dev") ? QStringLiteral("dev") : QStringLiteral("main");
    if (m_updateChannel == normalized) {
        return;
    }
    if (m_isDownloading || m_isInstalling) {
        return;
    }

    m_updateChannel = normalized;
    QSettings().setValue("updater/channel", m_updateChannel);

    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
        m_isChecking = false;
    }

    m_updateAvailable = false;
    m_dismissed = false;
    m_errorText.clear();
    m_statusText.clear();
    emit channelChanged();
    emit stateChanged();

    checkForUpdates(false);
}

void UpdateBridge::checkForUpdates(bool silent) {
    if (m_isChecking || m_isDownloading || m_isInstalling) {
        return;
    }

    m_silentCheck = silent;
    m_isChecking = true;
    m_errorText.clear();
    if (!silent) {
        m_statusText = tr("Проверка обновлений (%1)...").arg(m_updateChannel);
    }
    emit stateChanged();

    const char* apiUrl = (m_updateChannel == "dev") ? kReleasesListApiUrl : kLatestReleaseApiUrl;
    const QNetworkRequest req = makeGitHubRequest(QUrl(QString::fromLatin1(apiUrl)), currentVersion(), true);
    m_checkReply = m_network.get(req);
    connect(m_checkReply, &QNetworkReply::finished, this, &UpdateBridge::onCheckReplyFinished);
}

void UpdateBridge::onCheckReplyFinished() {
    QNetworkReply* reply = m_checkReply;
    m_checkReply = nullptr;
    if (!reply) return;

    m_isChecking = false;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            emit stateChanged();
            return;
        }
        if (!m_silentCheck) {
            m_errorText = tr("Не удалось проверить обновления: %1").arg(reply->errorString());
            m_statusText.clear();
        }
        emit stateChanged();
        return;
    }

    const QByteArray payload = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(payload);

    QJsonObject obj;
    bool foundRelease = false;

    if (m_updateChannel == "dev" && doc.isArray()) {
        const QJsonArray releases = doc.array();
        for (const QJsonValue& val : releases) {
            const QJsonObject candidate = val.toObject();
            if (!candidate.value("draft").toBool()) {
                obj = candidate;
                foundRelease = true;
                break;
            }
        }
        if (!foundRelease) {
            m_updateAvailable = false;
            if (!m_silentCheck) {
                m_statusText = tr("Релизы в ветке dev не найдены");
            }
            emit stateChanged();
            return;
        }
    } else if (doc.isObject()) {
        obj = doc.object();
        foundRelease = true;
    }

    if (!foundRelease) {
        if (!m_silentCheck) {
            m_errorText = tr("Некорректный ответ сервера обновлений");
            m_statusText.clear();
        }
        emit stateChanged();
        return;
    }

    if (obj.value("draft").toBool() || (m_updateChannel != "dev" && obj.value("prerelease").toBool())) {
        m_updateAvailable = false;
        if (!m_silentCheck) {
            m_statusText = tr("Установлена актуальная версия");
        }
        emit stateChanged();
        return;
    }

    const QString remoteVersionStr = stripVersionPrefix(obj.value("tag_name").toString());
    const QString localVersionStr = stripVersionPrefix(currentVersion());
    bool versionValid = false;
    const int cmp = compareVersions(remoteVersionStr, localVersionStr, &versionValid);

    if (!versionValid) {
        if (!m_silentCheck) {
            m_errorText = tr("Не удалось определить версию релиза");
            m_statusText.clear();
        }
        emit stateChanged();
        return;
    }

    const QString htmlUrl = obj.value("html_url").toString();
    if (!htmlUrl.isEmpty()) {
        m_releaseUrl = htmlUrl;
    }

    if (cmp <= 0) {
        m_updateAvailable = false;
        m_latestVersion = remoteVersionStr;
        if (!m_silentCheck) {
            m_statusText = tr("Установлена актуальная версия (%1, %2)").arg(currentVersion(), m_updateChannel);
        }
        emit stateChanged();
        return;
    }

    // Find asset for current OS
    QString foundDownloadUrl;
    QString foundAssetName;
    qint64 foundAssetSize = 0;

    const QJsonArray assets = obj.value("assets").toArray();
    for (const QJsonValue& val : assets) {
        const QJsonObject asset = val.toObject();
        const QString name = asset.value("name").toString();
        if (assetMatchesCurrentPlatform(name)) {
            foundAssetName = name;
            foundDownloadUrl = asset.value("browser_download_url").toString();
            foundAssetSize = static_cast<qint64>(asset.value("size").toDouble());
            break;
        }
    }

    if (foundDownloadUrl.isEmpty()) {
        if (!m_silentCheck) {
            m_errorText = tr("Вышла версия %1, но архив для вашей ОС не найден в релизе").arg(remoteVersionStr);
            m_statusText.clear();
        }
        emit stateChanged();
        return;
    }

    m_latestVersion = remoteVersionStr;
    m_releaseNotes = obj.value("body").toString().trimmed();
    m_downloadUrl = foundDownloadUrl;
    m_assetName = foundAssetName;
    m_assetSize = foundAssetSize;
    m_updateAvailable = true;
    m_dismissed = false;
    m_errorText.clear();
    m_statusText = tr("Доступна новая версия: %1").arg(m_latestVersion);
    emit stateChanged();
}

void UpdateBridge::startUpdate() {
    if (!m_updateAvailable || m_downloadUrl.isEmpty() || m_isDownloading || m_isInstalling) {
        return;
    }

    m_dismissed = false;
    m_errorText.clear();
    m_downloadProgress = 0.0;
    m_isDownloading = true;
    m_statusText = tr("Загрузка обновления %1...").arg(m_latestVersion);
    emit stateChanged();
    emit progressChanged();

    m_stagingDir = QDir::cleanPath(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/LCALauncherUpdate"
    );
    QDir(m_stagingDir).removeRecursively();
    if (!Utils::FileUtils::ensureDir(m_stagingDir)) {
        failUpdate(tr("Не удалось создать временную папку для обновления"));
        return;
    }

    m_archivePath = m_stagingDir + "/" + m_assetName;
    m_downloadFile = new QFile(m_archivePath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        failUpdate(tr("Не удалось создать файл обновления: %1").arg(m_downloadFile->errorString()));
        return;
    }

    QNetworkRequest req = makeGitHubRequest(QUrl(m_downloadUrl), currentVersion(), false);
    req.setTransferTimeout(120000);
    m_downloadReply = m_network.get(req);

    connect(m_downloadReply, &QNetworkReply::readyRead, this, &UpdateBridge::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateBridge::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateBridge::onDownloadFinished);
}

void UpdateBridge::onDownloadReadyRead() {
    if (m_downloadReply && m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->write(m_downloadReply->readAll());
    }
}

void UpdateBridge::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    const qint64 total = bytesTotal > 0 ? bytesTotal : m_assetSize;
    if (total > 0) {
        m_downloadProgress = qBound(0.0, static_cast<double>(bytesReceived) / static_cast<double>(total), 1.0);
        emit progressChanged();
    }
}

void UpdateBridge::onDownloadFinished() {
    QNetworkReply* reply = m_downloadReply;
    m_downloadReply = nullptr;
    if (!reply) return;

    if (m_downloadFile) {
        if (m_downloadFile->isOpen()) {
            m_downloadFile->write(reply->readAll());
            m_downloadFile->flush();
            m_downloadFile->close();
        }
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }

    const auto netErr = reply->error();
    const QString errStr = reply->errorString();
    reply->deleteLater();

    if (netErr != QNetworkReply::NoError) {
        if (netErr == QNetworkReply::OperationCanceledError) {
            m_isDownloading = false;
            m_downloadProgress = 0.0;
            m_statusText = tr("Загрузка отменена");
            emit stateChanged();
            emit progressChanged();
            return;
        }
        failUpdate(tr("Ошибка загрузки обновления: %1").arg(errStr));
        return;
    }

    const QFileInfo archiveInfo(m_archivePath);
    if (!archiveInfo.exists() || archiveInfo.size() < 100 * 1024 ||
        (m_assetSize > 0 && archiveInfo.size() != m_assetSize)) {
        failUpdate(tr("Загруженный файл обновления поврежден или неполный"));
        return;
    }

    m_isDownloading = false;
    m_isInstalling = true;
    m_downloadProgress = 1.0;
    m_statusText = tr("Распаковка и подготовка обновления...");
    emit progressChanged();
    emit stateChanged();

    if (!m_prepareWatcher) {
        m_prepareWatcher = new QFutureWatcher<PrepareResult>(this);
        connect(m_prepareWatcher, &QFutureWatcher<PrepareResult>::finished, this, &UpdateBridge::onPrepareFinished);
    }

    const QString archivePath = m_archivePath;
    const QString stagingDir = m_stagingDir;
    m_prepareWatcher->setFuture(QtConcurrent::run([this, archivePath, stagingDir]() {
        return prepareUpdateInstall(archivePath, stagingDir);
    }));
}

UpdateBridge::PrepareResult UpdateBridge::prepareUpdateInstall(const QString& archivePath, const QString& stagingDir) {
    PrepareResult res;
    const QString pidStr = QString::number(QCoreApplication::applicationPid());

#if defined(Q_OS_WIN)
    const QString extractDir = stagingDir + "/extracted";
    if (!Utils::ArchiveUtils::extractZip(archivePath, extractDir)) {
        res.error = tr("Не удалось распаковать архив обновления");
        return res;
    }

    if (!QFileInfo::exists(extractDir + "/LCALauncher.exe")) {
        res.error = tr("В архиве обновления отсутствует LCALauncher.exe");
        return res;
    }

    QString powershell = QStandardPaths::findExecutable("powershell.exe");
    if (powershell.isEmpty() && QFileInfo::exists("C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe")) {
        powershell = "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe";
    }
    if (powershell.isEmpty()) {
        res.error = tr("Не найден PowerShell для установки обновления");
        return res;
    }

    const QString scriptPath = QDir::toNativeSeparators(stagingDir + "/apply_update.ps1");
    const QString psScript =
        "param(\n"
        "    [int]$TargetPid,\n"
        "    [string]$SrcDir,\n"
        "    [string]$DstDir,\n"
        "    [string]$StagingDir\n"
        ")\n"
        "$ErrorActionPreference = 'Stop'\n"
        "try {\n"
        "    $proc = Get-Process -Id $TargetPid -ErrorAction SilentlyContinue\n"
        "    if ($proc) { $proc.WaitForExit(15000) | Out-Null }\n"
        "} catch {}\n"
        "Start-Sleep -Milliseconds 400\n"
        "for ($i = 0; $i -lt 10; $i++) {\n"
        "    try {\n"
        "        Copy-Item -Path (Join-Path $SrcDir '*') -Destination $DstDir -Recurse -Force\n"
        "        break\n"
        "    } catch {\n"
        "        if ($i -eq 9) { throw }\n"
        "        Start-Sleep -Milliseconds 500\n"
        "    }\n"
        "}\n"
        "$exe = Join-Path $DstDir 'LCALauncher.exe'\n"
        "Start-Process -FilePath $exe -WorkingDirectory $DstDir\n"
        "Start-Sleep -Milliseconds 500\n"
        "Remove-Item -Path $StagingDir -Recurse -Force -ErrorAction SilentlyContinue\n";

    if (!writeScriptFile(scriptPath, psScript, false)) {
        res.error = tr("Не удалось создать скрипт установки обновления");
        return res;
    }

    res.success = true;
    res.program = powershell;
    res.arguments = {
        "-NoProfile", "-NonInteractive", "-WindowStyle", "Hidden", "-ExecutionPolicy", "Bypass",
        "-File", scriptPath,
        "-TargetPid", pidStr,
        "-SrcDir", QDir::toNativeSeparators(extractDir),
        "-DstDir", QDir::toNativeSeparators(QCoreApplication::applicationDirPath()),
        "-StagingDir", QDir::toNativeSeparators(stagingDir)
    };
    res.workingDir = QDir::toNativeSeparators(stagingDir);
    return res;

#elif defined(Q_OS_MACOS)
    const QString appBundlePath = QDir(QCoreApplication::applicationDirPath() + "/../..").canonicalPath();
    if (!appBundlePath.endsWith(".app", Qt::CaseInsensitive)) {
        res.error = tr("Автообновление доступно только при запуске из LCALauncher.app");
        return res;
    }

    const QString mountDir = stagingDir + "/mnt";
    if (!Utils::FileUtils::ensureDir(mountDir)) {
        res.error = tr("Не удалось создать точку монтирования DMG");
        return res;
    }

    QProcess hdiutil;
    hdiutil.start("hdiutil", {"attach", archivePath, "-nobrowse", "-readonly", "-mountpoint", mountDir});
    if (!hdiutil.waitForFinished(60000) || hdiutil.exitCode() != 0) {
        res.error = tr("Не удалось смонтировать образ DMG");
        return res;
    }

    if (!QFileInfo::exists(mountDir + "/LCALauncher.app")) {
        QProcess::execute("hdiutil", {"detach", mountDir, "-force"});
        res.error = tr("В образе DMG не найдено приложение LCALauncher.app");
        return res;
    }

    const QString scriptPath = stagingDir + "/apply_update.sh";
    const QString shScript =
        "#!/bin/sh\n"
        "TARGET_PID=\"$1\"\n"
        "MOUNT_DIR=\"$2\"\n"
        "APP_BUNDLE=\"$3\"\n"
        "STAGING_DIR=\"$4\"\n"
        "i=0\n"
        "while [ \"$i\" -lt 30 ]; do\n"
        "    if ! kill -0 \"$TARGET_PID\" 2>/dev/null; then\n"
        "        break\n"
        "    fi\n"
        "    sleep 0.5\n"
        "    i=$((i + 1))\n"
        "done\n"
        "BACKUP=\"${APP_BUNDLE}.bak\"\n"
        "rm -rf \"$BACKUP\"\n"
        "if mv \"$APP_BUNDLE\" \"$BACKUP\"; then\n"
        "    if ditto \"${MOUNT_DIR}/LCALauncher.app\" \"$APP_BUNDLE\"; then\n"
        "        rm -rf \"$BACKUP\"\n"
        "    else\n"
        "        mv \"$BACKUP\" \"$APP_BUNDLE\"\n"
        "        hdiutil detach \"$MOUNT_DIR\" -force >/dev/null 2>&1 || true\n"
        "        exit 1\n"
        "    fi\n"
        "else\n"
        "    hdiutil detach \"$MOUNT_DIR\" -force >/dev/null 2>&1 || true\n"
        "    exit 1\n"
        "fi\n"
        "xattr -cr \"$APP_BUNDLE\" >/dev/null 2>&1 || true\n"
        "hdiutil detach \"$MOUNT_DIR\" -force >/dev/null 2>&1 || true\n"
        "open -n \"$APP_BUNDLE\"\n"
        "rm -rf \"$STAGING_DIR\"\n";

    if (!writeScriptFile(scriptPath, shScript, true)) {
        QProcess::execute("hdiutil", {"detach", mountDir, "-force"});
        res.error = tr("Не удалось создать скрипт установки обновления");
        return res;
    }

    res.success = true;
    res.program = "/bin/sh";
    res.arguments = {scriptPath, pidStr, mountDir, appBundlePath, stagingDir};
    res.workingDir = stagingDir;
    return res;

#else
    const QString extractDir = stagingDir + "/extracted";
    if (!Utils::ArchiveUtils::extractTarGz(archivePath, extractDir)) {
        res.error = tr("Не удалось распаковать архив обновления");
        return res;
    }

    if (!QFileInfo::exists(extractDir + "/lcalauncher")) {
        res.error = tr("В архиве обновления отсутствует исполняемый файл lcalauncher");
        return res;
    }

    const QString scriptPath = stagingDir + "/apply_update.sh";
    const QString shScript =
        "#!/bin/sh\n"
        "TARGET_PID=\"$1\"\n"
        "SRC_DIR=\"$2\"\n"
        "DST_DIR=\"$3\"\n"
        "STAGING_DIR=\"$4\"\n"
        "i=0\n"
        "while [ \"$i\" -lt 30 ]; do\n"
        "    if ! kill -0 \"$TARGET_PID\" 2>/dev/null; then\n"
        "        break\n"
        "    fi\n"
        "    sleep 0.5\n"
        "    i=$((i + 1))\n"
        "done\n"
        "cp -rf \"${SRC_DIR}/.\" \"${DST_DIR}/\"\n"
        "chmod +x \"${DST_DIR}/lcalauncher\"\n"
        "nohup \"${DST_DIR}/lcalauncher\" >/dev/null 2>&1 &\n"
        "rm -rf \"$STAGING_DIR\"\n";

    if (!writeScriptFile(scriptPath, shScript, true)) {
        res.error = tr("Не удалось создать скрипт установки обновления");
        return res;
    }

    res.success = true;
    res.program = "/bin/sh";
    res.arguments = {scriptPath, pidStr, extractDir, QCoreApplication::applicationDirPath(), stagingDir};
    res.workingDir = stagingDir;
    return res;
#endif
}

void UpdateBridge::onPrepareFinished() {
    if (!m_prepareWatcher) return;
    const PrepareResult res = m_prepareWatcher->result();

    if (!res.success) {
        failUpdate(res.error);
        return;
    }

    m_statusText = tr("Перезапуск лаунчера для завершения обновления...");
    emit stateChanged();

    if (!QProcess::startDetached(res.program, res.arguments, res.workingDir)) {
        failUpdate(tr("Не удалось запустить установщик обновления"));
        return;
    }

    QTimer::singleShot(200, QCoreApplication::instance(), &QCoreApplication::quit);
}

void UpdateBridge::cancelUpdate() {
    if (m_downloadReply) {
        m_downloadReply->abort();
    }
}

void UpdateBridge::dismiss() {
    if (m_isDownloading || m_isInstalling) {
        return;
    }
    m_dismissed = true;
    emit stateChanged();
}

void UpdateBridge::openReleasePage() {
    QDesktopServices::openUrl(QUrl(m_releaseUrl));
}

void UpdateBridge::failUpdate(const QString& message) {
    cleanupDownloadReply();
    m_isDownloading = false;
    m_isInstalling = false;
    m_downloadProgress = 0.0;
    m_errorText = message;
    m_statusText.clear();
    emit progressChanged();
    emit stateChanged();
}

void UpdateBridge::cleanupDownloadReply() {
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    if (m_downloadFile) {
        if (m_downloadFile->isOpen()) {
            m_downloadFile->close();
        }
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }
}

} // namespace LCA::Bridge
