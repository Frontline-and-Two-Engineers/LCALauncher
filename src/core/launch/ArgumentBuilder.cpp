#include "ArgumentBuilder.h"
#include <QDir>
#include <QFile>

namespace LCA::Launch {

QStringList ArgumentBuilder::buildJvmArguments(const LaunchConfig& config) {
    QStringList args;

    // 1. Memory allocation
    args << QString("-Xms1024M");
    args << QString("-Xmx%1M").arg(config.ramMb);

    // 2. Modern GC options optimized for Java 21 & Minecraft
    args << "-XX:+UseG1GC"
         << "-XX:+ParallelRefProcEnabled"
         << "-XX:MaxGCPauseMillis=200"
         << "-XX:+UnlockExperimentalVMOptions"
         << "-XX:+DisableExplicitGC"
         << "-XX:+AlwaysPreTouch";

    // 3. System and brand properties
    args << "-Dminecraft.launcher.brand=LCALauncher";
    args << "-Dminecraft.launcher.version=1.0.0";
    if (!config.nativesDir.isEmpty()) {
        args << QString("-Djava.library.path=%1").arg(config.nativesDir);
    }

    // 4. Custom Auth: authlib-injector (if enabled & exists)
    if (!config.authlibInjectorPath.isEmpty() && QFile::exists(config.authlibInjectorPath) &&
        !config.authServerYggdrasilUrl.isEmpty() && config.session.userType == "custom") {
        args << QString("-javaagent:%1=%2")
                    .arg(config.authlibInjectorPath, config.authServerYggdrasilUrl);
    }

    // 5. NeoForge 1.21.1 module-path & classloader options
    args << "-Djava.system.class.loader=cpw.mods.modlauncher.TransformingClassLoader";

    if (!config.modulePath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString sep = ";";
#else
        const QString sep = ":";
#endif
        args << "-p" << config.modulePath.join(sep);
        args << "--add-modules" << "ALL-MODULE-PATH";
    }

    // 6. Extra custom JVM arguments from settings
    args.append(config.extraJvmArgs);

    // 7. Classpath
    if (!config.classpath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString sep = ";";
#else
        const QString sep = ":";
#endif
        args << "-cp" << config.classpath.join(sep);
    }

    return args;
}

QStringList ArgumentBuilder::buildGameArguments(const LaunchConfig& config) {
    QStringList args;

    args << "--username" << (config.session.username.isEmpty() ? "Player" : config.session.username);
    args << "--version" << config.versionId;
    args << "--gameDir" << config.gameDir;
    args << "--assetsDir" << config.assetsDir;
    args << "--assetIndex" << config.assetIndex;
    args << "--uuid" << config.session.uuid;
    args << "--accessToken" << (config.session.accessToken.isEmpty() ? "0" : config.session.accessToken);
    args << "--userType" << "mojang"; // Standard client flag
    args << "--versionType" << "NeoForge";

    // Fast direct connection to server
    if (config.autoConnect && !config.serverAddress.isEmpty()) {
        args << "--quickPlayMultiplayer" << QString("%1:%2").arg(config.serverAddress).arg(config.serverPort);
    }

    return args;
}

QStringList ArgumentBuilder::buildAllArguments(const LaunchConfig& config) {
    QStringList allArgs = buildJvmArguments(config);

    // Main class
    allArgs << config.mainClass;

    // Game arguments
    allArgs.append(buildGameArguments(config));

    return allArgs;
}

} // namespace LCA::Launch
