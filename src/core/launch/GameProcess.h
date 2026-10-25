#pragma once

#include <QObject>
#include <QProcess>
#include "ArgumentBuilder.h"

namespace LCA::Launch {

class GameProcess : public QObject {
    Q_OBJECT

public:
    explicit GameProcess(QObject* parent = nullptr);
    ~GameProcess() override;

    bool launch(const LaunchConfig& config);
    void kill();

    bool isRunning() const;
    qint64 processId() const;

signals:
    void gameStarted();
    void logReceived(const QString& logLine);
    void gameExited(int exitCode, const QString& reason);
    void launchError(const QString& errorMessage);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess m_process;
};

} // namespace LCA::Launch
