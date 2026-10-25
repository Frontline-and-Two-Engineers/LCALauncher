#include "LauncherBridge.h"
#include "core/utils/FileUtils.h"
#include "core/java/JavaDetector.h"
#include <QDir>
#include <QDirIterator>

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

    // 2. Validate Java 21
    QString java = m_settings->javaPath();
    auto javaInfo = Java::JavaDetector::inspectJava(java);
    if (!javaInfo.isValid) {
        // Try auto detect
        m_settings->autoDetectJava();
        java = m_settings->javaPath();
        javaInfo = Java::JavaDetector::inspectJava(java);
    }

    if (!javaInfo.isValid) {
        m_statusText = "Ошибка: не найдена Java 21!";
        emit statusChanged();
        appendLog("[Ошибка] Для Minecraft 1.21.1 требуется Java 21. Укажите путь в настройках.");
        return;
    }

    m_isBusy = true;
    m_progressPercent = 0.0;
    emit busyChanged();
    emit progressChanged();

    appendLog(QString("[Launcher] Подготовка к запуску. Игрок: %1 (%2)")
                  .arg(m_authService->currentSession().username, m_authService->currentSession().userType));
    appendLog(QString("[Launcher] Используется Java: %1").arg(javaInfo.path));

    // 3. Start Sync with server
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

    Launch::LaunchConfig config;
    config.javaPath = m_settings->javaPath();
    config.ramMb = m_settings->ramMb();
    config.gameDir = m_settings->gameDir();
    config.assetsDir = Utils::FileUtils::defaultLauncherDataDir() + "/assets";
    config.nativesDir = Utils::FileUtils::defaultLauncherDataDir() + "/natives/1.21.1";
    config.assetIndex = "17";
    config.versionId = "1.21.1-neoforge";
    config.session = m_authService->currentSession();
    config.autoConnect = m_settings->autoConnect();
    config.serverAddress = m_settings->serverAddress();
    config.serverPort = m_settings->serverPort();

    // Authlib injector path
    config.authlibInjectorPath = Utils::FileUtils::defaultLauncherDataDir() + "/authlib-injector.jar";
    config.authServerYggdrasilUrl = m_settings->authUrl();

    // Gather classpath: all libraries in libraries/ + client.jar
    QString librariesDir = Utils::FileUtils::defaultLauncherDataDir() + "/libraries";
    QDir libDir(librariesDir);
    if (libDir.exists()) {
        QDirIterator it(librariesDir, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            config.classpath.append(it.next());
        }
    }

    QString clientJar = Utils::FileUtils::defaultLauncherDataDir() + "/versions/1.21.1/1.21.1.jar";
    if (QFile::exists(clientJar)) {
        config.classpath.append(clientJar);
    }

    // Launch!
    m_statusText = "Запуск Minecraft...";
    emit statusChanged();
    m_gameProcess->launch(config);
}

} // namespace LCA::Bridge
