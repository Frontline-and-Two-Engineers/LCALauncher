#pragma once

#include <QString>
#include <QStringList>
#include <QSet>

namespace LCA::Utils {

class FileUtils {
public:
    static bool ensureDir(const QString& path);
    static bool removeDir(const QString& path);
    static bool removeFile(const QString& path);

    static QStringList listFilesRelative(const QString& baseDir);
    static int cleanExtraFiles(const QString& baseDir, const QSet<QString>& allowedRelativePaths);

    static QString defaultLauncherDataDir();
    static QString defaultGameDir();
};

} // namespace LCA::Utils
