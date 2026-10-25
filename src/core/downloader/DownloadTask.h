#pragma once

#include <QUrl>
#include <QString>

namespace LCA::Downloader {

struct DownloadTask {
    QUrl url;
    QString destinationPath;
    QString expectedSha1;
    QString expectedSha256;
    qint64 size{0};
    QString description;
};

} // namespace LCA::Downloader
