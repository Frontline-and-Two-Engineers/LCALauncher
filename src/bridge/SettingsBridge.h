#pragma once

#include <QObject>
#include <QSettings>

namespace LCA::Java {
class JavaDownloader;
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
    Q_PROPERTY(QString modpackUrl READ modpackUrl WRITE setModpackUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString gameDir READ gameDir WRITE setGameDir NOTIFY settingsChanged)
    Q_PROPERTY(QString detectedJavaInfo READ detectedJavaInfo NOTIFY javaInfoChanged)
    Q_PROPERTY(bool isDownloadingJava READ isDownloadingJava NOTIFY javaDownloadingChanged)
    Q_PROPERTY(double javaDownloadProgress READ javaDownloadProgress NOTIFY javaDownloadingChanged)

public:
    explicit SettingsBridge(QObject* parent = nullptr);

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

    QString modpackUrl() const;
    void setModpackUrl(const QString& url);

    QString gameDir() const;
    void setGameDir(const QString& dir);

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

    QSettings m_settings;
    int m_ramMb{4096};
    QString m_javaPath;
    QString m_serverAddress{"mc.myserver.com"};
    int m_serverPort{25565};
    bool m_autoConnect{true};
    QString m_authUrl{"https://auth.myserver.com/api/auth/login"};
    QString m_modpackUrl{"https://myserver.com/api/modpack/manifest.json"};
    QString m_gameDir;
    QString m_detectedJavaInfo;
    Java::JavaDownloader* m_javaDownloader{nullptr};
    bool m_isDownloadingJava{false};
    double m_javaDownloadProgress{0.0};
};

} // namespace LCA::Bridge
