#pragma once

#include <QObject>
#include <QSettings>

namespace LCA::Java {
class JavaDownloader;
}

namespace LCA::Auth {
class AuthService;
}

namespace LCA::Bridge {

class SettingsBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(int ramMb READ ramMb WRITE setRamMb NOTIFY settingsChanged)
    Q_PROPERTY(QString javaPath READ javaPath WRITE setJavaPath NOTIFY settingsChanged)
    Q_PROPERTY(QString serverAddress READ serverAddress WRITE setServerAddress NOTIFY settingsChanged)
    Q_PROPERTY(int serverPort READ serverPort WRITE setServerPort NOTIFY settingsChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY settingsChanged)
    Q_PROPERTY(QString authUrl READ authUrl WRITE setAuthUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString telegramBotUsername READ telegramBotUsername WRITE setTelegramBotUsername NOTIFY settingsChanged)
    Q_PROPERTY(QString modpackUrl READ modpackUrl WRITE setModpackUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString clientBaseUrl READ clientBaseUrl WRITE setClientBaseUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString gameDir READ gameDir WRITE setGameDir NOTIFY settingsChanged)
    Q_PROPERTY(bool isDevMode READ isDevMode CONSTANT)
    Q_PROPERTY(QString detectedJavaInfo READ detectedJavaInfo NOTIFY javaInfoChanged)
    Q_PROPERTY(bool isDownloadingJava READ isDownloadingJava NOTIFY javaDownloadingChanged)
    Q_PROPERTY(double javaDownloadProgress READ javaDownloadProgress NOTIFY javaDownloadingChanged)

public:
    explicit SettingsBridge(Auth::AuthService* authService = nullptr, QObject* parent = nullptr);

    int ramMb() const;
    void setRamMb(int mb);

    QString javaPath() const;
    void setJavaPath(const QString& path);

    QString serverAddress() const;
    void setServerAddress(const QString& address);

    int serverPort() const;
    void setServerPort(int port);

    bool autoConnect() const;
    void setAutoConnect(bool enable);

    QString authUrl() const;
    void setAuthUrl(const QString& url);

    QString telegramBotUsername() const;
    void setTelegramBotUsername(const QString& username);

    QString modpackUrl() const;
    void setModpackUrl(const QString& url);

    QString clientBaseUrl() const;
    void setClientBaseUrl(const QString& url);

    QString gameDir() const;
    void setGameDir(const QString& dir);

    bool isDevMode() const;

    QString detectedJavaInfo() const;
    bool isDownloadingJava() const;
    double javaDownloadProgress() const;

    Q_INVOKABLE void autoDetectJava();
    Q_INVOKABLE void downloadJava21();
    Q_INVOKABLE void resetToDefaults();

signals:
    void settingsChanged();
    void javaInfoChanged();
    void javaDownloadingChanged();
    void javaDownloadFinished(bool success, const QString& message);

private:
    void load();
    void save();

    Auth::AuthService* m_authService{nullptr};
    QSettings m_settings;
    int m_ramMb{4096};
    QString m_javaPath;
    QString m_serverAddress{"lightcameraaction.frontlineand2engineers.dev"};
    int m_serverPort{25565};
    bool m_autoConnect{true};
    QString m_authUrl{"https://apilightcameraaction.frontlineand2engineers.dev"};
    QString m_telegramBotUsername{"@lightcameraaction_game"};
    QString m_modpackUrl{"https://apilightcameraaction.frontlineand2engineers.dev/api/modpack/manifest.json"};
    QString m_clientBaseUrl{"https://apilightcameraaction.frontlineand2engineers.dev/api/modpack/files/client-base.zip"};
    QString m_gameDir;
    bool m_isDevMode{false};
    QString m_detectedJavaInfo;
    Java::JavaDownloader* m_javaDownloader{nullptr};
    bool m_isDownloadingJava{false};
    double m_javaDownloadProgress{0.0};
};

} // namespace LCA::Bridge
