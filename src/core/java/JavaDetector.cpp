#include "JavaDetector.h"
#include "core/utils/FileUtils.h"
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>

namespace LCA::Java {

JavaInfo JavaDetector::inspectJava(const QString& javaExecutablePath) {
    JavaInfo info;
    info.path = javaExecutablePath;

    if (javaExecutablePath.isEmpty() || !QFile::exists(javaExecutablePath)) {
        return info;
    }

    // Try fast inspection first: check adjacent 'release' file in parent directory
    QFileInfo fileInfo(javaExecutablePath);
    QDir binDir = fileInfo.dir();
    QFile releaseFile(binDir.filePath("../release"));
    if (releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = releaseFile.readAll();
        static QRegularExpression reRelease(R"(JAVA_VERSION="([0-9]+)(\.[0-9]+)?.*")");
        auto match = reRelease.match(content);
        if (match.hasMatch()) {
            info.majorVersion = match.captured(1).toInt();
            info.versionString = match.captured(0);
            info.isValid = (info.majorVersion >= 21);
            if (info.isValid) return info;
        }
    }

    // Fallback: run `java -version`
    QProcess process;
    process.start(javaExecutablePath, {"-version"});
    if (!process.waitForFinished(3000)) {
        process.kill();
        return info;
    }

    // `java -version` prints to stderr
    QString output = QString::fromUtf8(process.readAllStandardError()) + 
                     QString::fromUtf8(process.readAllStandardOutput());

    info.majorVersion = parseMajorVersion(output);
    info.versionString = output.split('\n').value(0).trimmed();
    info.isValid = (info.majorVersion >= 21);

    return info;
}

int JavaDetector::parseMajorVersion(const QString& versionOutput) {
    // Examples:
    // openjdk version "21.0.2" 2024-01-16
    // java version "1.8.0_381"
    static QRegularExpression re(R"(version "(?:1\.)?([0-9]+))");
    auto match = re.match(versionOutput);
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    return 0;
}

QStringList JavaDetector::getCandidatePaths() {
    QStringList candidates;

    // 1. Local launcher runtime directory
    QString localRuntime = Utils::FileUtils::defaultLauncherDataDir() + "/runtime";
#ifdef Q_OS_WIN
    candidates << localRuntime + "/java-21/bin/java.exe";
#else
    candidates << localRuntime + "/java-21/bin/java";
#endif

    // 2. JAVA_HOME
    QString javaHome = qEnvironmentVariable("JAVA_HOME");
    if (!javaHome.isEmpty()) {
#ifdef Q_OS_WIN
        candidates << javaHome + "/bin/java.exe";
#else
        candidates << javaHome + "/bin/java";
#endif
    }

    // 3. System PATH
#ifdef Q_OS_WIN
    QString sysJava = QStandardPaths::findExecutable("java.exe");
#else
    QString sysJava = QStandardPaths::findExecutable("java");
#endif
    if (!sysJava.isEmpty()) {
        candidates << sysJava;
    }

    // 4. OS-specific common JVM paths
#ifdef Q_OS_MACOS
    QDir jvmDir("/Library/Java/JavaVirtualMachines");
    if (jvmDir.exists()) {
        for (const QString& entry : jvmDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            candidates << QString("/Library/Java/JavaVirtualMachines/%1/Contents/Home/bin/java").arg(entry);
        }
    }
    // Also check Homebrew openjdk
    candidates << "/opt/homebrew/opt/openjdk@21/bin/java";
    candidates << "/opt/homebrew/opt/openjdk/bin/java";
    candidates << "/usr/local/opt/openjdk@21/bin/java";
    // Check user sdkman / asdf
    candidates << QDir::homePath() + "/.sdkman/candidates/java/current/bin/java";

#elif defined(Q_OS_LINUX)
    // Common Linux distribution locations
    QStringList linuxDirs = {"/usr/lib/jvm", "/usr/java", "/opt/java", "/opt/jdk"};
    for (const QString& base : linuxDirs) {
        QDir dir(base);
        if (dir.exists()) {
            for (const QString& entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                candidates << QString("%1/%2/bin/java").arg(base, entry);
            }
        }
    }
    candidates << "/etc/alternatives/java";
    candidates << QDir::homePath() + "/.sdkman/candidates/java/current/bin/java";

#elif defined(Q_OS_WIN)
    QString progFiles = qEnvironmentVariable("ProgramFiles");
    QString progFilesX86 = qEnvironmentVariable("ProgramFiles(x86)");
    QString localAppData = qEnvironmentVariable("LOCALAPPDATA");

    QStringList winVendors = {
        "Eclipse Adoptium",
        "Java",
        "BellSoft",
        "Microsoft",
        "Zulu",
        "Amazon Corretto",
        "Semeru"
    };

    auto scanWinDir = [&](const QString& rootDir) {
        if (rootDir.isEmpty()) return;
        for (const QString& vendor : winVendors) {
            QDir vendorDir(rootDir + "/" + vendor);
            if (vendorDir.exists()) {
                for (const QString& jdk : vendorDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    candidates << QString("%1/%2/%3/bin/java.exe").arg(rootDir, vendor, jdk);
                }
            }
        }
    };

    scanWinDir(progFiles);
    scanWinDir(progFilesX86);
    scanWinDir(localAppData + "/Programs");

    // Check Windows Registry for JavaSoft JDK entries
    QSettings registryJdk("HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\JDK", QSettings::NativeFormat);
    for (const QString& versionKey : registryJdk.childGroups()) {
        QString javaHome = registryJdk.value(versionKey + "/JavaHome").toString();
        if (!javaHome.isEmpty()) {
            candidates << javaHome + "/bin/java.exe";
        }
    }
#endif

    candidates.removeDuplicates();
    return candidates;
}

QList<JavaInfo> JavaDetector::findAllJavaInstallations() {
    QList<JavaInfo> result;
    for (const QString& candidate : getCandidatePaths()) {
        JavaInfo info = inspectJava(candidate);
        if (info.isValid) {
            result.append(info);
        }
    }
    return result;
}

JavaInfo JavaDetector::detectBestJava(const QString& customPath) {
    if (!customPath.isEmpty()) {
        JavaInfo customInfo = inspectJava(customPath);
        if (customInfo.isValid) {
            return customInfo;
        }
    }

    for (const QString& candidate : getCandidatePaths()) {
        JavaInfo info = inspectJava(candidate);
        if (info.isValid && info.majorVersion >= 21) {
            return info;
        }
    }

    return {};
}

} // namespace LCA::Java
