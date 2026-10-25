#pragma once

#include <QString>
#include <QStringList>
#include "core/auth/UserSession.h"

namespace LCA::Launch {

struct LaunchConfig {
    QString javaPath;
    int ramMb{4096};
    QString gameDir;
    QString assetsDir;
    QString nativesDir;
    QString assetIndex{"17"};
    QString versionId{"1.21.1-neoforge"};
    QString mainClass{"cpw.mods.bootstraplauncher.BootstrapLauncher"};
    
    // Auth
    Auth::UserSession session;
    QString authlibInjectorPath;
    QString authServerYggdrasilUrl;

    // Server auto-connect
    QString serverAddress;
    int serverPort{25565};
    bool autoConnect{false};

    // Classpath and module paths
    QStringList classpath;
    QStringList modulePath;
    QStringList extraJvmArgs;
};

class ArgumentBuilder {
public:
    static QStringList buildJvmArguments(const LaunchConfig& config);
    static QStringList buildGameArguments(const LaunchConfig& config);
    static QStringList buildAllArguments(const LaunchConfig& config);
};

} // namespace LCA::Launch
