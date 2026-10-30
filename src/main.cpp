#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QFontDatabase>
#include <QFont>

#include "core/auth/AuthService.h"
#include "bridge/AuthBridge.h"
#include "bridge/SettingsBridge.h"
#include "bridge/LauncherBridge.h"
#include "bridge/ServerStatusBridge.h"
#include "bridge/ChannelPostsBridge.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    app.setOrganizationName("LCAProject");
    app.setOrganizationDomain("lcaproject.org");
    app.setApplicationName("LCALauncher");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":/qt/qml/LCALauncher/assets/logo.png"));

    QQuickStyle::setStyle("Basic");

    // Load custom application fonts from assets/fonts
    const QStringList fontFiles = {
        QStringLiteral(":/qt/qml/LCALauncher/assets/fonts/dehinted-RazerF5-Regular.ttf"),
        QStringLiteral(":/qt/qml/LCALauncher/assets/fonts/dehinted-RazerF5-Regitalic.ttf"),
        QStringLiteral(":/qt/qml/LCALauncher/assets/fonts/dehinted-RazerF5-Bold.ttf"),
        QStringLiteral(":/qt/qml/LCALauncher/assets/fonts/dehinted-RazerF5-Bolditalic.ttf")
    };
    for (const auto &file : fontFiles) {
        int id = QFontDatabase::addApplicationFont(file);
        if (id < 0) {
            QString localPath = file;
            localPath.replace(QStringLiteral(":/qt/qml/LCALauncher/"), QString());
            QFontDatabase::addApplicationFont(localPath);
        }
    }

    // Set base font for entire application
    QFont baseFont(QStringLiteral("RazerF5"));
    baseFont.setStyleHint(QFont::SansSerif);
    app.setFont(baseFont);

    // Initialize Core Services
    auto authService = new LCA::Auth::AuthService(&app);
    authService->restoreSavedSession();

    // Initialize Bridge ViewModels
    auto settingsBridge = new LCA::Bridge::SettingsBridge(authService, &app);
    auto authBridge = new LCA::Bridge::AuthBridge(authService, &app);
    auto launcherBridge = new LCA::Bridge::LauncherBridge(authService, settingsBridge, &app);
    auto serverStatusBridge = new LCA::Bridge::ServerStatusBridge(settingsBridge, &app);
    auto channelPostsBridge = new LCA::Bridge::ChannelPostsBridge(settingsBridge, &app);

    QQmlApplicationEngine engine;

    // Expose Bridges to QML
    engine.rootContext()->setContextProperty("appFontFamily", QStringLiteral("RazerF5"));
    engine.rootContext()->setContextProperty("authBridge", authBridge);
    engine.rootContext()->setContextProperty("settingsBridge", settingsBridge);
    engine.rootContext()->setContextProperty("launcherBridge", launcherBridge);
    engine.rootContext()->setContextProperty("serverStatusBridge", serverStatusBridge);
    engine.rootContext()->setContextProperty("channelPostsBridge", channelPostsBridge);

    // Load Main QML from module
    const QUrl url(QStringLiteral("qrc:/qt/qml/LCALauncher/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
