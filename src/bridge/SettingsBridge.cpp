#include "SettingsBridge.h"
#include "core/utils/FileUtils.h"
#include "core/utils/UrlUtils.h"
#include "core/java/JavaDetector.h"
#include "core/java/JavaDownloader.h"
#include "core/auth/AuthService.h"

namespace LCA::Bridge {

SettingsBridge::SettingsBridge(Auth::AuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
    , m_settings("LCAProject", "LCALauncher")
    , m_javaDownloader(new Java::JavaDownloader(this))
{
    connect(m_javaDownloader, &Java::JavaDownloader::progressChanged, this, [this](double percent, qint64, qint64) {
        m_javaDownloadProgress = percent;
        emit javaDownloadingChanged();
    });

    connect(m_javaDownloader, &Java::JavaDownloader::statusChanged, this, [this](const QString& status) {
        m_detectedJavaInfo = status;
        emit javaInfoChanged();
    });

    connect(m_javaDownloader, &Java::JavaDownloader::downloadFinished, this, [this](bool success, const QString& path, const QString& err) {
        m_isDownloadingJava = false;
        m_javaDownloadProgress = 0.0;
        emit javaDownloadingChanged();

        if (success) {
            setJavaPath(path);
            emit javaDownloadFinished(true, "Java 21 успешно установлена!");
        } else {
            m_detectedJavaInfo = "Ошибка установки Java: " + err;
            emit javaInfoChanged();
            emit javaDownloadFinished(false, err);
        }
    });

    m_gameDir = Utils::FileUtils::defaultGameDir();
    load();

    if (m_authService) {
        m_authService->setBaseUrl(m_authUrl);
        m_authService->setTelegramBotUsername(m_telegramBotUsername);
    }

    if (m_javaPath.isEmpty()) {
        autoDetectJava();
    } else {
        auto info = Java::JavaDetector::inspectJava(m_javaPath);
        m_detectedJavaInfo = info.isValid ? QString("Java %1").arg(info.majorVersion)
                                          : "Выбранный путь Java невалиден";
    }
}

int SettingsBridge::ramMb() const { return m_ramMb; }
void SettingsBridge::setRamMb(int mb) {
    if (m_ramMb != mb) {
        m_ramMb = mb;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::javaPath() const { return m_javaPath; }
void SettingsBridge::setJavaPath(const QString& path) {
    if (m_javaPath != path) {
        m_javaPath = path;
        auto info = Java::JavaDetector::inspectJava(m_javaPath);
        m_detectedJavaInfo = info.isValid ? QString("Java %1").arg(info.majorVersion)
                                          : "Выбранный путь Java невалиден";
        save();
        emit settingsChanged();
        emit javaInfoChanged();
    }
}

QString SettingsBridge::serverAddress() const { return m_serverAddress; }
void SettingsBridge::setServerAddress(const QString& address) {
    if (m_serverAddress != address) {
        m_serverAddress = address;
        save();
        emit settingsChanged();
    }
}

int SettingsBridge::serverPort() const { return m_serverPort; }
void SettingsBridge::setServerPort(int port) {
    if (m_serverPort != port) {
        m_serverPort = port;
        save();
        emit settingsChanged();
    }
}

bool SettingsBridge::autoConnect() const { return m_autoConnect; }
void SettingsBridge::setAutoConnect(bool enable) {
    if (m_autoConnect != enable) {
        m_autoConnect = enable;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::authUrl() const { return m_authUrl; }
void SettingsBridge::setAuthUrl(const QString& url) {
    QString cleanUrl = Utils::UrlUtils::sanitizeBaseUrl(url);
    if (m_authUrl != cleanUrl) {
        m_authUrl = cleanUrl;
        if (m_authService) {
            m_authService->setBaseUrl(cleanUrl);
        }
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::telegramBotUsername() const { return m_telegramBotUsername; }
void SettingsBridge::setTelegramBotUsername(const QString& username) {
    QString cleanBot = Utils::UrlUtils::sanitizeTelegramBot(username);
    if (m_telegramBotUsername != cleanBot) {
        m_telegramBotUsername = cleanBot;
        if (m_authService) {
            m_authService->setTelegramBotUsername(cleanBot);
        }
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::modpackUrl() const { return m_modpackUrl; }
void SettingsBridge::setModpackUrl(const QString& url) {
    QString cleanModpack = Utils::UrlUtils::sanitizeModpackUrl(url, m_authUrl);
    if (m_modpackUrl != cleanModpack) {
        m_modpackUrl = cleanModpack;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::clientBaseUrl() const { return m_clientBaseUrl; }
void SettingsBridge::setClientBaseUrl(const QString& url) {
    QString cleanClient = Utils::UrlUtils::sanitizeClientBaseUrl(url, m_authUrl);
    if (m_clientBaseUrl != cleanClient) {
        m_clientBaseUrl = cleanClient;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::gameDir() const { return m_gameDir; }
void SettingsBridge::setGameDir(const QString& dir) {
    if (m_gameDir != dir) {
        m_gameDir = dir;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::detectedJavaInfo() const {
    return m_detectedJavaInfo;
}

bool SettingsBridge::isDownloadingJava() const {
    return m_isDownloadingJava;
}

double SettingsBridge::javaDownloadProgress() const {
    return m_javaDownloadProgress;
}

void SettingsBridge::downloadJava21() {
    if (m_isDownloadingJava) return;
    m_isDownloadingJava = true;
    m_javaDownloadProgress = 0.0;
    emit javaDownloadingChanged();
    m_javaDownloader->startDownload();
}

void SettingsBridge::autoDetectJava() {
    auto info = Java::JavaDetector::detectBestJava();
    if (info.isValid) {
        m_javaPath = info.path;
        m_detectedJavaInfo = QString("Java %1").arg(info.majorVersion);
        save();
        emit settingsChanged();
        emit javaInfoChanged();
    } else {
        m_detectedJavaInfo = "Java 21 не найдена в системе. Укажите вручную.";
        emit javaInfoChanged();
    }
}

void SettingsBridge::resetToDefaults() {
    m_ramMb = 4096;
    m_serverAddress = "mc.myserver.com";
    m_serverPort = 25565;
    m_autoConnect = true;
    m_authUrl = "http://127.0.0.1:8080";
    m_telegramBotUsername = "@LCAMinecraftBot";
    m_modpackUrl = "http://127.0.0.1:8080/api/modpack/manifest.json";
    m_clientBaseUrl = "http://127.0.0.1:8080/api/modpack/files/client-base.zip";
    m_gameDir = Utils::FileUtils::defaultGameDir();

    if (m_authService) {
        m_authService->setBaseUrl(m_authUrl);
        m_authService->setTelegramBotUsername(m_telegramBotUsername);
    }

    autoDetectJava();
    save();
    emit settingsChanged();
}

void SettingsBridge::load() {
    m_ramMb = m_settings.value("settings/ramMb", m_ramMb).toInt();
    m_javaPath = m_settings.value("settings/javaPath", m_javaPath).toString();
    m_serverAddress = m_settings.value("settings/serverAddress", m_serverAddress).toString();
    m_serverPort = m_settings.value("settings/serverPort", m_serverPort).toInt();
    m_autoConnect = m_settings.value("settings/autoConnect", m_autoConnect).toBool();
    m_authUrl = Utils::UrlUtils::sanitizeBaseUrl(m_settings.value("settings/authUrl", m_authUrl).toString());
    m_telegramBotUsername = Utils::UrlUtils::sanitizeTelegramBot(m_settings.value("settings/telegramBotUsername", m_telegramBotUsername).toString());
    m_modpackUrl = Utils::UrlUtils::sanitizeModpackUrl(m_settings.value("settings/modpackUrl", m_modpackUrl).toString(), m_authUrl);
    m_clientBaseUrl = Utils::UrlUtils::sanitizeClientBaseUrl(m_settings.value("settings/clientBaseUrl", m_clientBaseUrl).toString(), m_authUrl);
    m_gameDir = m_settings.value("settings/gameDir", m_gameDir).toString();
}

void SettingsBridge::save() {
    m_settings.setValue("settings/ramMb", m_ramMb);
    m_settings.setValue("settings/javaPath", m_javaPath);
    m_settings.setValue("settings/serverAddress", m_serverAddress);
    m_settings.setValue("settings/serverPort", m_serverPort);
    m_settings.setValue("settings/autoConnect", m_autoConnect);
    m_settings.setValue("settings/authUrl", m_authUrl);
    m_settings.setValue("settings/telegramBotUsername", m_telegramBotUsername);
    m_settings.setValue("settings/modpackUrl", m_modpackUrl);
    m_settings.setValue("settings/clientBaseUrl", m_clientBaseUrl);
    m_settings.setValue("settings/gameDir", m_gameDir);
    m_settings.sync();
}

} // namespace LCA::Bridge
