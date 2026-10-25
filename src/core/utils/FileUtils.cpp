#include "FileUtils.h"
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>

namespace LCA::Utils {

bool FileUtils::ensureDir(const QString& path) {
    QDir dir(path);
    if (!dir.exists()) {
        return dir.mkpath(".");
    }
    return true;
}

bool FileUtils::removeDir(const QString& path) {
    QDir dir(path);
    return dir.removeRecursively();
}

bool FileUtils::removeFile(const QString& path) {
    return QFile::remove(path);
}

QStringList FileUtils::listFilesRelative(const QString& baseDir) {
    QStringList result;
    QDir dir(baseDir);
    if (!dir.exists()) return result;

    QDirIterator it(baseDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        result.append(dir.relativeFilePath(it.filePath()));
    }
    return result;
}

int FileUtils::cleanExtraFiles(const QString& baseDir, const QSet<QString>& allowedRelativePaths) {
    QDir dir(baseDir);
    if (!dir.exists()) return 0;

    int deletedCount = 0;
    QDirIterator it(baseDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString relPath = dir.relativeFilePath(it.filePath());
        if (!allowedRelativePaths.contains(relPath)) {
            if (QFile::remove(it.filePath())) {
                deletedCount++;
            }
        }
    }
    return deletedCount;
}

QString FileUtils::defaultLauncherDataDir() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) {
        appData = QDir::homePath() + "/.lcalauncher";
    }
    return appData;
}

QString FileUtils::defaultGameDir() {
    return defaultLauncherDataDir() + "/game";
}

} // namespace LCA::Utils
