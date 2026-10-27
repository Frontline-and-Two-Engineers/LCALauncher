#pragma once

#include <QObject>
#include <QStringList>
#include "core/auth/AuthService.h"
#include "core/sync/SyncEngine.h"
#include "core/launch/GameProcess.h"
#include "SettingsBridge.h"

namespace LCA::Bridge {

class LauncherBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(double progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY progressChanged)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool isGameRunning READ isGameRunning NOTIFY gameRunningChanged)
    Q_PROPERTY(QStringList consoleLogs READ consoleLogs NOTIFY logsChanged)

public:
    explicit LauncherBridge(Auth::AuthService* authService, SettingsBridge* settings, QObject* parent = nullptr);

    QString statusText() const;
    double progressPercent() const;
    QString currentFile() const;
    bool isBusy() const;
    bool isGameRunning() const;
    QStringList consoleLogs() const;

    Q_INVOKABLE void launchGame();
    Q_INVOKABLE void killGame();
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void copyLogsToClipboard();

signals:
    void statusChanged();
    void progressChanged();
    void busyChanged();
    void gameRunningChanged();
    void logsChanged();
    void logAdded(const QString& line);
    void requestLoginView();

private slots:
    void onSyncFinished(bool success, const QString& message);

private:
    void appendLog(const QString& line);
    void checkJavaAndProceed();
    void checkClientAndProceed();
    void checkNativesAndProceed();
    void startModpackSync();
    void doActualGameLaunch();

    Auth::AuthService* m_authService;
    SettingsBridge* m_settings;
    Sync::SyncEngine* m_syncEngine;
    Launch::GameProcess* m_gameProcess;

    QString m_statusText{"Готов к запуску"};
    double m_progressPercent{0.0};
    QString m_currentFile;
    bool m_isBusy{false};
    QStringList m_consoleLogs;
};

} // namespace LCA::Bridge
