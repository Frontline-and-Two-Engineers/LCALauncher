#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace LCA::Java {

struct JavaInfo {
    QString path;
    int majorVersion{0};
    QString versionString;
    bool isValid{false};
};

class JavaDetector {
public:
    static JavaInfo detectBestJava(const QString& customPath = {});
    static QList<JavaInfo> findAllJavaInstallations();
    static JavaInfo inspectJava(const QString& javaExecutablePath);

private:
    static QStringList getCandidatePaths();
    static int parseMajorVersion(const QString& versionOutput);
};

} // namespace LCA::Java
