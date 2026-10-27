#include "LauncherBridge.h"
#include "core/utils/FileUtils.h"
#include "core/java/JavaDetector.h"
#include "core/java/JavaDownloader.h"
#include "core/client/ClientDownloader.h"
#include "core/downloader/DownloadManager.h"
#include <QSysInfo>
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QClipboard>
#include <QSet>

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

void LauncherBridge::copyLogsToClipboard() {
    QGuiApplication::clipboard()->setText(m_consoleLogs.join("\n"));
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
            checkNativesAndProceed();
        });

        clientDownloader->startDownload(m_settings->clientBaseUrl(), dataDir);
        return;
    }

    appendLog("[Клиент] Базовый клиент 1.21.1 NeoForge проверен и готов.");
    checkNativesAndProceed();
}

void LauncherBridge::checkNativesAndProceed() {
    QString librariesDir = Utils::FileUtils::defaultLauncherDataDir() + "/libraries";

    struct NativeItem {
        QString relPath;
        QString url;
    };

    QList<NativeItem> requiredItems;
    const QStringList modules = {
        "lwjgl", "lwjgl-glfw", "lwjgl-jemalloc", "lwjgl-openal",
        "lwjgl-opengl", "lwjgl-stb", "lwjgl-tinyfd", "lwjgl-freetype"
    };

#if defined(Q_OS_WIN)
    const QString classifier = "natives-windows";
    for (const QString& mod : modules) {
        QString rel = QString("org/lwjgl/%1/3.3.3/%1-3.3.3-%2.jar").arg(mod, classifier);
        requiredItems.append(NativeItem{rel, "https://libraries.minecraft.net/" + rel});
    }
#elif defined(Q_OS_MACOS) || defined(Q_OS_APPLE)
    QString arch = QSysInfo::currentCpuArchitecture().toLower();
    QString classifier = (arch == "arm64" || arch == "aarch64") ? "natives-macos-arm64" : "natives-macos";
    for (const QString& mod : modules) {
        QString rel = QString("org/lwjgl/%1/3.3.3/%1-3.3.3-%2.jar").arg(mod, classifier);
        requiredItems.append(NativeItem{rel, "https://libraries.minecraft.net/" + rel});
    }
    requiredItems.append(NativeItem{
        "ca/weblite/java-objc-bridge/1.1/java-objc-bridge-1.1.jar",
        "https://libraries.minecraft.net/ca/weblite/java-objc-bridge/1.1/java-objc-bridge-1.1.jar"
    });
#else
    // Linux
    const QString classifier = "natives-linux";
    for (const QString& mod : modules) {
        QString rel = QString("org/lwjgl/%1/3.3.3/%1-3.3.3-%2.jar").arg(mod, classifier);
        requiredItems.append(NativeItem{rel, "https://libraries.minecraft.net/" + rel});
    }
#endif

    QList<Downloader::DownloadTask> tasks;
    for (const auto& item : requiredItems) {
        QString fullPath = librariesDir + "/" + item.relPath;
        if (!QFile::exists(fullPath)) {
            Downloader::DownloadTask t;
            t.url = QUrl(item.url);
            t.destinationPath = fullPath;
            t.description = QFileInfo(fullPath).fileName();
            tasks.append(t);
        }
    }

    if (tasks.isEmpty()) {
        startModpackSync();
        return;
    }

    appendLog(QString("[Клиент] Обнаружены отсутствующие нативные библиотеки для ОС (%1 шт.). Загрузка с Mojang CDN...")
                  .arg(tasks.size()));
    m_statusText = "Загрузка нативных библиотек платформы...";
    emit statusChanged();

    auto nativeDownloader = new Downloader::DownloadManager(4, this);
    connect(nativeDownloader, &Downloader::DownloadManager::allFinished, this, [this, nativeDownloader]() {
        nativeDownloader->deleteLater();
        appendLog("[Клиент] Нативные библиотеки платформы успешно установлены.");
        startModpackSync();
    });
    connect(nativeDownloader, &Downloader::DownloadManager::taskFailed, this, [this](const QString& dest, const QString& err) {
        appendLog(QString("[Клиент ПРЕДУПРЕЖДЕНИЕ] Не удалось загрузить %1: %2").arg(dest, err));
    });

    nativeDownloader->addTasks(tasks);
    nativeDownloader->start();
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
    config.librariesDir = baseDataDir + "/libraries";

    // 1. Dynamically locate version jar, version id and version json
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

    // 2. Authlib injector path
    QString authlibPath = baseDataDir + "/authlib-injector.jar";
    if (!QFile::exists(authlibPath)) {
        QDirIterator aIt(baseDataDir, QStringList() << "authlib-injector*.jar", QDir::Files, QDirIterator::Subdirectories);
        if (aIt.hasNext()) {
            authlibPath = aIt.next();
        }
    }
    config.authlibInjectorPath = authlibPath;
    config.authServerYggdrasilUrl = m_settings->authUrl();

    // 3. Default NeoForge main class
    config.mainClass = "cpw.mods.bootstraplauncher.BootstrapLauncher";

    // 4. Read version JSON if present
    QSet<QString> downloadOnlyNames;
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

                // Check libraries for downloadOnly
                if (obj.contains("libraries") && obj["libraries"].isArray()) {
                    for (const auto& lVal : obj["libraries"].toArray()) {
                        QJsonObject lObj = lVal.toObject();
                        if (lObj["downloadOnly"].toBool(false)) {
                            QString name = lObj["name"].toString();
                            QStringList parts = name.split(':');
                            if (parts.size() >= 3) {
                                QString artifact = parts[1];
                                QString ver = parts[2];
                                QString classifier = parts.size() > 3 ? ("-" + parts[3]) : "";
                                downloadOnlyNames.insert(artifact + "-" + ver + classifier);
                                downloadOnlyNames.insert(artifact);
                            }
                        }
                    }
                }

                // Check extra game arguments (like --fml.neoForgeVersion, --launchTarget, etc.)
                if (obj.contains("arguments") && obj["arguments"].isObject()) {
                    QJsonObject argsObj = obj["arguments"].toObject();
                    if (argsObj.contains("game") && argsObj["game"].isArray()) {
                        QJsonArray gameArr = argsObj["game"].toArray();
                        for (int i = 0; i < gameArr.size(); ++i) {
                            QString val;
                            if (gameArr[i].isString()) val = gameArr[i].toString();
                            else if (gameArr[i].isObject()) val = gameArr[i].toObject()["value"].toString();

                            if (val.startsWith("--fml.") || val == "--launchTarget") {
                                config.extraGameArgs.append(val);
                                if (i + 1 < gameArr.size()) {
                                    QString nextVal;
                                    if (gameArr[i + 1].isString()) nextVal = gameArr[i + 1].toString();
                                    else if (gameArr[i + 1].isObject()) nextVal = gameArr[i + 1].toObject()["value"].toString();
                                    if (!nextVal.isEmpty()) {
                                        config.extraGameArgs.append(nextVal);
                                        ++i;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Fallback if --launchTarget was not in JSON
    if (!config.extraGameArgs.contains("--launchTarget")) {
        config.extraGameArgs << "--launchTarget" << "forgeclient";
    }

    // 5. Gather module-path and classpath
    // In NeoForge 1.21.1:
    // Module-path MUST ONLY contain: bootstraplauncher, securejarhandler, asm, JarJarFileSystems
    // Everything else goes to Classpath (EXCEPT downloadOnly jars like universal or srg)
    QSet<QString> seenClasspath;
    auto addClasspath = [&](const QString& jar) {
        if (!jar.isEmpty() && QFile::exists(jar) && !seenClasspath.contains(jar)) {
            seenClasspath.insert(jar);
            config.classpath.append(jar);
        }
    };

    QString librariesDir = config.librariesDir;
    QDir libDir(librariesDir);
    if (libDir.exists()) {
        QDirIterator it(librariesDir, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString jar = it.next();
            QString fileName = QFileInfo(jar).fileName();

            // Module path jars: strictly only the 4 boot module components
            if (jar.contains("bootstraplauncher") ||
                jar.contains("securejarhandler") ||
                jar.contains("JarJarFileSystems") ||
                (jar.contains("asm") && (jar.contains("/org/ow2/asm/") || jar.contains("\\org\\ow2\\asm\\")))) {
                config.modulePath.append(jar);
            }

            // Check if this jar is downloadOnly / mod jar (should NOT be on classpath)
            bool isDownloadOnly = false;
            if (jar.contains("neoforge-") && jar.contains("universal")) {
                isDownloadOnly = true;
            } else if (jar.contains("client-") && (jar.contains("-srg") || jar.contains("-slim") || jar.contains("-extra"))) {
                isDownloadOnly = true;
            } else {
                for (const QString& dName : downloadOnlyNames) {
                    if (fileName.contains(dName)) {
                        isDownloadOnly = true;
                        break;
                    }
                }
            }

            if (!isDownloadOnly) {
                addClasspath(jar);
            }
        }
    }

    // Append client jar if found
    if (!clientJar.isEmpty() && QFile::exists(clientJar)) {
        addClasspath(clientJar);
    }

    // Launch!
    m_statusText = "Запуск Minecraft...";
    emit statusChanged();
    m_gameProcess->launch(config);
}

} // namespace LCA::Bridge
