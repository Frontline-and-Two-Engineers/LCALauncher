#pragma once

#include <QString>

namespace LCA::Utils {

class ArchiveUtils {
public:
    /**
     * @brief Extracts a .zip file into target destination directory across macOS, Windows, and Linux.
     */
    static bool extractZip(const QString& zipPath, const QString& destinationDir);

    /**
     * @brief Extracts a .tar.gz archive.
     */
    static bool extractTarGz(const QString& tarGzPath, const QString& destinationDir);
};

} // namespace LCA::Utils
