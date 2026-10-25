#include "SettingsBridge.h"
#include "core/utils/FileUtils.h"
#include "core/java/JavaDetector.h"
#include "core/java/JavaDownloader.h"

namespace LCA::Bridge {

SettingsBridge::SettingsBridge(QObject* parent)
    : QObject(parent)
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
    if (m_javaPath.isEmpty()) {
        autoDetectJava();
    } else {
        auto info = Java::JavaDetector::inspectJava(m_javaPath);
        m_detectedJavaInfo = info.isValid ? QString("Java %1 (%2)").arg(info.majorVersion).arg(info.path)
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
        m_detectedJavaInfo = info.isValid ? QString("Java %1 (%2)").arg(info.majorVersion).arg(info.path)
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
    if (m_authUrl != url) {
        m_authUrl = url;
        save();
        emit settingsChanged();
    }
}

QString SettingsBridge::modpackUrl() const { return m_modpackUrl; }
void SettingsBridge::setModpackUrl(const QString& url) {
    if (m_modpackUrl != url) {
        m_modpackUrl = url;
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
        m_detectedJavaInfo = QString("Java %1 (%2)").arg(info.majorVersion).arg(info.path);
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
    m_gameDir = Utils::FileUtils::defaultGameDir();
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
    m_authUrl = m_settings.value("settings/authUrl", m_authUrl).toString();
    m_modpackUrl = m_settings.value("settings/modpackUrl", m_modpackUrl).toString();
    m_gameDir = m_settings.value("settings/gameDir", m_gameDir).toString();
}

void SettingsBridge::save() {
    m_settings.setValue("settings/ramMb", m_ramMb);
    m_settings.setValue("settings/javaPath", m_javaPath);
    m_settings.setValue("settings/serverAddress", m_serverAddress);
    m_settings.setValue("settings/serverPort", m_serverPort);
    m_settings.setValue("settings/autoConnect", m_autoConnect);
    m_settings.setValue("settings/authUrl", m_authUrl);
    m_settings.setValue("settings/modpackUrl", m_modpackUrl);
    m_settings.setValue("settings/gameDir", m_gameDir);
    m_settings.sync();
}

} // namespace LCA::Bridge
