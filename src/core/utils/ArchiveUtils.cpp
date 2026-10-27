#include "ArchiveUtils.h"
#include "FileUtils.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>

namespace LCA::Utils {

bool ArchiveUtils::extractZip(const QString& zipPath, const QString& destinationDir) {
    if (!QFileInfo::exists(zipPath)) return false;
    FileUtils::ensureDir(destinationDir);

    QProcess process;

#if defined(Q_OS_WIN)
    // 1. Try built-in tar on Windows 10/11
    QString tarPath = QStandardPaths::findExecutable("tar.exe");
    if (!tarPath.isEmpty()) {
        process.start(tarPath, {"-xf", zipPath, "-C", destinationDir});
        if (process.waitForFinished(180000) && process.exitCode() == 0) {
            return true;
        }
    }

    // 2. Fallback: PowerShell Expand-Archive
    QString powershell = QStandardPaths::findExecutable("powershell.exe");
    if (!powershell.isEmpty()) {
        QString cmd = QString("Expand-Archive -Force -Path '%1' -DestinationPath '%2'")
                          .arg(zipPath, destinationDir);
        process.start(powershell, {"-NoProfile", "-NonInteractive", "-Command", cmd});
        if (process.waitForFinished(240000) && process.exitCode() == 0) {
            return true;
        }
    }
    return false;
#elif defined(Q_OS_MACOS) || defined(Q_OS_APPLE)
    // On macOS, try unzip -o -q, fallback to bsdtar -xf
    process.start("unzip", {"-o", "-q", zipPath, "-d", destinationDir});
    if (process.waitForFinished(180000) && process.exitCode() == 0) {
        return true;
    }

    process.start("tar", {"-xf", zipPath, "-C", destinationDir});
    return process.waitForFinished(180000) && process.exitCode() == 0;
#else
    // Linux
    QString unzipPath = QStandardPaths::findExecutable("unzip");
    if (!unzipPath.isEmpty()) {
        process.start(unzipPath, {"-o", "-q", zipPath, "-d", destinationDir});
        if (process.waitForFinished(180000) && process.exitCode() == 0) {
            return true;
        }
    }

    process.start("tar", {"-xf", zipPath, "-C", destinationDir});
    return process.waitForFinished(180000) && process.exitCode() == 0;
#endif
}

bool ArchiveUtils::extractTarGz(const QString& tarGzPath, const QString& destinationDir) {
    if (!QFileInfo::exists(tarGzPath)) return false;
    FileUtils::ensureDir(destinationDir);

    QProcess process;
#if defined(Q_OS_WIN)
    QString tarPath = QStandardPaths::findExecutable("tar.exe");
    if (!tarPath.isEmpty()) {
        process.start(tarPath, {"-xzf", tarGzPath, "-C", destinationDir});
        return process.waitForFinished(180000) && process.exitCode() == 0;
    }
    return false;
#else
    process.start("tar", {"-xzf", tarGzPath, "-C", destinationDir});
    return process.waitForFinished(180000) && process.exitCode() == 0;
#endif
}

} // namespace LCA::Utils
