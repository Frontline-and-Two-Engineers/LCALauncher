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
        QString nativeDir = QDir::toNativeSeparators(config.nativesDir);
        args << QString("-Djava.library.path=%1").arg(nativeDir);
        args << QString("-Djna.tmpdir=%1").arg(nativeDir);
        args << QString("-Dorg.lwjgl.system.SharedLibraryExtractPath=%1").arg(nativeDir);
        args << QString("-Dio.netty.native.workdir=%1").arg(nativeDir);
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_APPLE)
    args << "-XstartOnFirstThread";
#endif

    // 4. Custom Auth: authlib-injector (if enabled & exists)
    if (!config.authlibInjectorPath.isEmpty() && QFile::exists(config.authlibInjectorPath) &&
        !config.authServerYggdrasilUrl.isEmpty() && config.session.userType == "custom") {
        args << QString("-javaagent:%1=%2")
                    .arg(QDir::toNativeSeparators(config.authlibInjectorPath), config.authServerYggdrasilUrl);
    }

    // 5. NeoForge 1.21.1 module-path & JVM reflection arguments
    if (!config.librariesDir.isEmpty()) {
        args << QString("-DlibraryDirectory=%1").arg(QDir::toNativeSeparators(config.librariesDir));
    }
    args << QString("-DignoreList=client-extra,%1.jar").arg(config.versionId);
    args << "-Djava.net.preferIPv6Addresses=system";

    if (!config.modulePath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString sep = ";";
        QStringList nativeMp;
        for (const QString& p : config.modulePath) nativeMp.append(QDir::toNativeSeparators(p));
        args << "-p" << nativeMp.join(sep);
#else
        const QString sep = ":";
        args << "-p" << config.modulePath.join(sep);
#endif
        args << "--add-modules" << "ALL-MODULE-PATH";
        args << "--add-opens" << "java.base/java.util.jar=cpw.mods.securejarhandler";
        args << "--add-opens" << "java.base/java.lang.invoke=cpw.mods.securejarhandler";
        args << "--add-exports" << "java.base/sun.security.util=cpw.mods.securejarhandler";
        args << "--add-exports" << "jdk.naming.dns/com.sun.jndi.dns=java.naming";
    }

    // 6. Extra custom JVM arguments from settings
    args.append(config.extraJvmArgs);

    // 7. Classpath
    if (!config.classpath.isEmpty()) {
#ifdef Q_OS_WIN
        const QString sep = ";";
        QStringList nativeCp;
        for (const QString& p : config.classpath) nativeCp.append(QDir::toNativeSeparators(p));
        args << "-cp" << nativeCp.join(sep);
#else
        const QString sep = ":";
        args << "-cp" << config.classpath.join(sep);
#endif
    }

    return args;
}

QStringList ArgumentBuilder::buildGameArguments(const LaunchConfig& config) {
    QStringList args;

    args << "--username" << (config.session.username.isEmpty() ? "Player" : config.session.username);
    args << "--version" << config.versionId;
    args << "--gameDir" << QDir::toNativeSeparators(config.gameDir);
    args << "--assetsDir" << QDir::toNativeSeparators(config.assetsDir);
    args << "--assetIndex" << config.assetIndex;
    args << "--uuid" << config.session.uuid;
    args << "--accessToken" << (config.session.accessToken.isEmpty() ? "0" : config.session.accessToken);
    args << "--userType" << "mojang";
    args << "--versionType" << "NeoForge";

    // Extra game arguments from version JSON
    args.append(config.extraGameArgs);

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
