#include "LauncherBridge.h"
#include "core/utils/FileUtils.h"
#include "core/java/JavaDetector.h"
#include "core/java/JavaDownloader.h"
#include "core/client/ClientDownloader.h"
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>

namespace LCA::Bridge {

LauncherBridge::LauncherBridge(Auth::AuthService* authService, SettingsBridge* settings, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
    , m_settings(settings)
    , m_syncEngine(new Sync::SyncEngine(m_settings->gameDir(), this))
    , m_gameProcess(new Launch::GameProcess(this))
{
    // Sync signals
    connect(m_syncEngine, &Sync::SyncEngine::statusChanged, this, [this](const QString& status) {
        m_statusText = status;
        emit statusChanged();
        appendLog("[Синхронизация] " + status);
    });

    connect(m_syncEngine, &Sync::SyncEngine::progressChanged, this, [this](qint64, qint64, double percent, const QString& currentItem) {
        m_progressPercent = percent;
        m_currentFile = currentItem;
        emit progressChanged();
    });

    connect(m_syncEngine, &Sync::SyncEngine::syncFinished, this, &LauncherBridge::onSyncFinished);

    // Process signals
    connect(m_gameProcess, &Launch::GameProcess::gameStarted, this, [this]() {
        m_isBusy = false;
        m_statusText = "Игра запущена";
        emit busyChanged();
        emit gameRunningChanged();
        emit statusChanged();
        appendLog("[Процесс] Minecraft успешно запущен!");
    });

    connect(m_gameProcess, &Launch::GameProcess::gameExited, this, [this](int exitCode, const QString& reason) {
        m_statusText = QString("Игра закрыта (код %1)").arg(exitCode);
        emit gameRunningChanged();
        emit statusChanged();
        appendLog(QString("[Процесс] %1").arg(reason));
    });

    connect(m_gameProcess, &Launch::GameProcess::logReceived, this, &LauncherBridge::appendLog);

    connect(m_gameProcess, &Launch::GameProcess::launchError, this, [this](const QString& err) {
        m_isBusy = false;
        m_statusText = "Ошибка запуска: " + err;
        emit busyChanged();
        emit statusChanged();
        appendLog("[Ошибка запуска] " + err);
    });
}

QString LauncherBridge::statusText() const { return m_statusText; }
double LauncherBridge::progressPercent() const { return m_progressPercent; }
QString LauncherBridge::currentFile() const { return m_currentFile; }
bool LauncherBridge::isBusy() const { return m_isBusy; }
bool LauncherBridge::isGameRunning() const { return m_gameProcess->isRunning(); }
QStringList LauncherBridge::consoleLogs() const { return m_consoleLogs; }

void LauncherBridge::appendLog(const QString& line) {
    m_consoleLogs.append(line);
    if (m_consoleLogs.size() > 5000) {
        m_consoleLogs.removeFirst();
    }
    emit logAdded(line);
    emit logsChanged();
}

void LauncherBridge::clearLogs() {
    m_consoleLogs.clear();
    emit logsChanged();
}

void LauncherBridge::killGame() {
    m_gameProcess->kill();
}

void LauncherBridge::launchGame() {
    if (m_isBusy || isGameRunning()) return;

    // 1. Check Auth
    if (!m_authService->currentSession().isLoggedIn) {
        m_statusText = "Требуется авторизация!";
        emit statusChanged();
        emit requestLoginView();
        return;
    }

    m_isBusy = true;
    m_progressPercent = 0.0;
    emit busyChanged();
    emit progressChanged();

    appendLog(QString("[Launcher] Подготовка к запуску. Игрок: %1 (%2)")
                  .arg(m_authService->currentSession().username, m_authService->currentSession().userType));

    // 2. Validate Java 21
    checkJavaAndProceed();
}

void LauncherBridge::checkJavaAndProceed() {
    QString java = m_settings->javaPath();
    auto javaInfo = Java::JavaDetector::inspectJava(java);
    if (!javaInfo.isValid) {
        m_settings->autoDetectJava();
        java = m_settings->javaPath();
        javaInfo = Java::JavaDetector::inspectJava(java);
    }

    if (!javaInfo.isValid) {
        m_statusText = "Java 21 не найдена. Начинается автозагрузка Adoptium Temurin 21...";
        emit statusChanged();
        appendLog("[Java] Java 21 не найдена. Автоматическое скачивание Eclipse Temurin JRE 21...");

        auto javaDownloader = new Java::JavaDownloader(this);
        connect(javaDownloader, &Java::JavaDownloader::progressChanged, this, [this](double percent, qint64, qint64) {
            m_progressPercent = percent;
            emit progressChanged();
        });
        connect(javaDownloader, &Java::JavaDownloader::statusChanged, this, [this](const QString& status) {
            m_statusText = status;
            emit statusChanged();
            appendLog("[Java] " + status);
        });
        connect(javaDownloader, &Java::JavaDownloader::downloadFinished, this, [this, javaDownloader](bool success, const QString& path, const QString& err) {
            javaDownloader->deleteLater();
            if (!success) {
                m_isBusy = false;
                m_statusText = "Ошибка загрузки Java 21: " + err;
                emit busyChanged();
                emit statusChanged();
                appendLog("[Java ОШИБКА] " + err);
                return;
            }

            m_settings->setJavaPath(path);
            appendLog("[Java] Успешно установлена Java 21: " + path);

            // Proceed to Step 3: Check base client
            checkClientAndProceed();
        });

        javaDownloader->startDownload();
        return;
    }

    appendLog(QString("[Launcher] Используется Java 21: %1").arg(javaInfo.path));
    checkClientAndProceed();
}

void LauncherBridge::checkClientAndProceed() {
    QString dataDir = Utils::FileUtils::defaultLauncherDataDir();

    if (!Client::ClientDownloader::isClientInstalled(dataDir)) {
        m_progressPercent = 0.0;
        emit progressChanged();
        m_statusText = "Клиент 1.21.1 NeoForge не найден. Загрузка базового клиента...";
        emit statusChanged();
        appendLog("[Клиент] Базовый клиент 1.21.1 NeoForge не обнаружен. Начинается загрузка client-base.zip...");

        auto clientDownloader = new Client::ClientDownloader(this);
        connect(clientDownloader, &Client::ClientDownloader::progressChanged, this, [this](double percent, qint64, qint64) {
            m_progressPercent = percent;
            emit progressChanged();
        });
        connect(clientDownloader, &Client::ClientDownloader::statusChanged, this, [this](const QString& status) {
            m_statusText = status;
            emit statusChanged();
            appendLog("[Клиент] " + status);
        });
        connect(clientDownloader, &Client::ClientDownloader::downloadFinished, this, [this, clientDownloader](bool success, const QString& err) {
            clientDownloader->deleteLater();
            if (!success) {
                m_isBusy = false;
                m_statusText = "Ошибка установки базового клиента: " + err;
                emit busyChanged();
                emit statusChanged();
                appendLog("[Клиент ОШИБКА] " + err);
                return;
            }

            appendLog("[Клиент] Базовый клиент 1.21.1 NeoForge успешно распакован и готов!");
            startModpackSync();
        });

        clientDownloader->startDownload(m_settings->clientBaseUrl(), dataDir);
        return;
    }

    appendLog("[Клиент] Базовый клиент 1.21.1 NeoForge проверен и готов.");
    startModpackSync();
}

void LauncherBridge::startModpackSync() {
    m_progressPercent = 0.0;
    emit progressChanged();
    m_statusText = "Синхронизация модов и конфигов сервера...";
    emit statusChanged();
    appendLog(QString("[Синхронизация] Подключение к манифесту сборки: %1").arg(m_settings->modpackUrl()));

    m_syncEngine->setManifestUrl(m_settings->modpackUrl());
    m_syncEngine->startSync();
}

void LauncherBridge::onSyncFinished(bool success, const QString& message) {
    if (!success) {
        m_isBusy = false;
        m_statusText = "Ошибка синхронизации: " + message;
        emit busyChanged();
        emit statusChanged();
        appendLog("[Синхронизация ОШИБКА] " + message);
        return;
    }

    appendLog("[Синхронизация] " + message);
    doActualGameLaunch();
}

void LauncherBridge::doActualGameLaunch() {
    m_statusText = "Сборка аргументов запуска NeoForge 1.21.1...";
    emit statusChanged();

    QString baseDataDir = Utils::FileUtils::defaultLauncherDataDir();

    Launch::LaunchConfig config;
    config.javaPath = m_settings->javaPath();
    config.ramMb = m_settings->ramMb();
    config.gameDir = m_settings->gameDir();
    config.assetsDir = baseDataDir + "/assets";
    config.nativesDir = baseDataDir + "/natives/1.21.1";
    config.assetIndex = "17";
    config.versionId = "1.21.1-neoforge";
    config.session = m_authService->currentSession();
    config.autoConnect = m_settings->autoConnect();
    config.serverAddress = m_settings->serverAddress();
    config.serverPort = m_settings->serverPort();

    // Dynamically locate version jar, version id and version json
    QString versionsDir = baseDataDir + "/versions";
    QString clientJar;
    QString versionJson;

    QDir vDir(versionsDir);
    if (vDir.exists()) {
        QStringList subdirs = vDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& sub : subdirs) {
            QDir subDir(vDir.filePath(sub));
            QString candidateJar = subDir.filePath(sub + ".jar");
            if (QFile::exists(candidateJar)) {
                clientJar = candidateJar;
                config.versionId = sub;
                versionJson = subDir.filePath(sub + ".json");
                break;
            }
            QStringList jars = subDir.entryList(QStringList() << "*.jar", QDir::Files);
            if (!jars.isEmpty()) {
                clientJar = subDir.filePath(jars.first());
                config.versionId = sub;
                QStringList jsons = subDir.entryList(QStringList() << "*.json", QDir::Files);
                if (!jsons.isEmpty()) {
                    versionJson = subDir.filePath(jsons.first());
                }
                break;
            }
        }
    }

    // Authlib injector path
    QString authlibPath = baseDataDir + "/authlib-injector.jar";
    if (!QFile::exists(authlibPath)) {
        QDirIterator aIt(baseDataDir, QStringList() << "authlib-injector*.jar", QDir::Files, QDirIterator::Subdirectories);
        if (aIt.hasNext()) {
            authlibPath = aIt.next();
        }
    }
    config.authlibInjectorPath = authlibPath;
    config.authServerYggdrasilUrl = m_settings->authUrl();

    // Default NeoForge main class
    config.mainClass = "cpw.mods.bootstraplauncher.BootstrapLauncher";

    // Read version JSON if present to get exact mainClass and assetIndex
    if (!versionJson.isEmpty() && QFile::exists(versionJson)) {
        QFile file(versionJson);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("mainClass")) {
                    config.mainClass = obj["mainClass"].toString();
                }
                if (obj.contains("assetIndex") && obj["assetIndex"].isObject()) {
                    config.assetIndex = obj["assetIndex"].toObject()["id"].toString("17");
                } else if (obj.contains("assets")) {
                    config.assetIndex = obj["assets"].toString("17");
                }
            }
        }
    }

    // Gather classpath and module-path
    QString librariesDir = baseDataDir + "/libraries";
    QDir libDir(librariesDir);
    if (libDir.exists()) {
        QDirIterator it(librariesDir, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString jar = it.next();
            config.classpath.append(jar);

            // Categorize NeoForge Java 21 module path dependencies
            if (jar.contains("modlauncher") || 
                jar.contains("bootstraplauncher") || 
                jar.contains("securejarhandler") || 
                jar.contains("asm-") || 
                jar.contains("neoforge-")) {
                config.modulePath.append(jar);
            }
        }
    }

    // Append client jar if found
    if (!clientJar.isEmpty() && QFile::exists(clientJar)) {
        config.classpath.append(clientJar);
    }

    // Launch!
    m_statusText = "Запуск Minecraft...";
    emit statusChanged();
    m_gameProcess->launch(config);
}

} // namespace LCA::Bridge
