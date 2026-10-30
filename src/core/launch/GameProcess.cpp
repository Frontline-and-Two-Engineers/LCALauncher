#include "GameProcess.h"
#include <QFileInfo>

namespace LCA::Launch {

GameProcess::GameProcess(QObject* parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &GameProcess::onReadyReadStdout);
    connect(&m_process, &QProcess::readyReadStandardError, this, &GameProcess::onReadyReadStderr);
    connect(&m_process, &QProcess::finished, this, &GameProcess::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &GameProcess::onProcessError);

    // Asynchronous start notification (non-blocking launch)
    connect(&m_process, &QProcess::started, this, [this]() {
        emit gameStarted();
    });
}

GameProcess::~GameProcess() {
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool GameProcess::launch(const LaunchConfig& config) {
    if (isRunning()) {
        emit launchError("Игра уже запущена!");
        return false;
    }

    if (!QFile::exists(config.javaPath)) {
        emit launchError(QString("Java не найдена по пути: %1").arg(config.javaPath));
        return false;
    }

    QStringList args = ArgumentBuilder::buildAllArguments(config);

    m_process.setProgram(config.javaPath);
    m_process.setArguments(args);
    m_process.setWorkingDirectory(config.gameDir);

    emit logReceived(QString("[Launcher] Запуск игры: %1 %2")
                         .arg(config.javaPath, args.join(" ")));

    // Start asynchronously — do NOT block the GUI thread with waitForStarted().
    // Outcomes are reported via signals:
    //   - gameStarted()    on QProcess::started
    //   - launchError(...) on QProcess::errorOccurred(FailedToStart)
    m_process.start();
    return true;
}

void GameProcess::kill() {
    if (isRunning()) {
        m_process.terminate();
        if (!m_process.waitForFinished(2000)) m_process.kill();
    }
}

bool GameProcess::isRunning() const {
    return m_process.state() != QProcess::NotRunning;
}

qint64 GameProcess::processId() const { return m_process.processId(); }

void GameProcess::onReadyReadStdout() {
    QByteArray data = m_process.readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    for (const QString& line : text.split('\n')) {
        if (!line.trimmed().isEmpty()) emit logReceived(line);
    }
}

void GameProcess::onReadyReadStderr() {
    QByteArray data = m_process.readAllStandardError();
    QString text = QString::fromUtf8(data);
    for (const QString& line : text.split('\n')) {
        if (!line.trimmed().isEmpty()) emit logReceived("[ERR] " + line);
    }
}

void GameProcess::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    QString reason = (status == QProcess::NormalExit && exitCode == 0)
                         ? "Игра завершена штатно"
                         : QString("Игра аварийно завершилась с кодом %1").arg(exitCode);
    emit logReceived("[Launcher] " + reason);
    emit gameExited(exitCode, reason);
}

void GameProcess::onProcessError(QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) {
        emit launchError("Ошибка старта: процесс не смог запуститься");
    } else if (error == QProcess::Crashed) {
        emit logReceived("[Launcher] Процесс игры неожиданно упал (Crashed)");
    }
}

} // namespace LCA::Launch