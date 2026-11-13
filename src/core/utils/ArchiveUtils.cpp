#include "ArchiveUtils.h"
#include "FileUtils.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace LCA::Utils {

bool ArchiveUtils::extractZip(const QString& zipPath, const QString& destinationDir) {
    if (!QFileInfo::exists(zipPath)) return false;
    FileUtils::ensureDir(destinationDir);

    QProcess process;

#if defined(Q_OS_WIN)
    const QString nativeZip = QDir::toNativeSeparators(zipPath);
    const QString nativeDest = QDir::toNativeSeparators(destinationDir);

    // 1. Try built-in tar on Windows 10/11
    QString tarPath = QStandardPaths::findExecutable("tar.exe");
    if (tarPath.isEmpty() && QFileInfo::exists("C:/Windows/System32/tar.exe")) {
        tarPath = "C:/Windows/System32/tar.exe";
    }
    if (!tarPath.isEmpty()) {
        process.start(tarPath, {"-xf", nativeZip, "-C", nativeDest});
        if (process.waitForFinished(180000) && process.exitCode() == 0) {
            return true;
        }
        qWarning() << "[ArchiveUtils] tar.exe failed:" << process.exitCode() << process.readAllStandardError();
    }

    // 2. Fallback: PowerShell (.NET ZipFile first, then Expand-Archive)
    QString powershell = QStandardPaths::findExecutable("powershell.exe");
    if (powershell.isEmpty() && QFileInfo::exists("C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe")) {
        powershell = "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe";
    }
    if (!powershell.isEmpty()) {
        QString psZip = nativeZip;
        QString psDest = nativeDest;
        psZip.replace("'", "''");
        psDest.replace("'", "''");

        const QString dotnetCmd = QString(
            "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
            "[System.IO.Compression.ZipFile]::ExtractToDirectory('%1', '%2')"
        ).arg(psZip, psDest);
        process.start(powershell, {"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", dotnetCmd});
        if (process.waitForFinished(240000) && process.exitCode() == 0) {
            return true;
        }
        qWarning() << "[ArchiveUtils] PowerShell ZipFile failed:" << process.exitCode() << process.readAllStandardError();

        const QString expandCmd = QString("Expand-Archive -Force -LiteralPath '%1' -DestinationPath '%2'")
                                      .arg(psZip, psDest);
        process.start(powershell, {"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", expandCmd});
        if (process.waitForFinished(240000) && process.exitCode() == 0) {
            return true;
        }
        qWarning() << "[ArchiveUtils] Expand-Archive failed:" << process.exitCode() << process.readAllStandardError();
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
