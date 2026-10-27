#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>

#include "core/auth/AuthService.h"
#include "bridge/AuthBridge.h"
#include "bridge/SettingsBridge.h"
#include "bridge/LauncherBridge.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    app.setOrganizationName("LCAProject");
    app.setOrganizationDomain("lcaproject.org");
    app.setApplicationName("LCALauncher");
    app.setApplicationVersion("1.0.0");

    QQuickStyle::setStyle("Basic");

    // Initialize Core Services
    auto authService = new LCA::Auth::AuthService(&app);
    authService->restoreSavedSession();

    // Initialize Bridge ViewModels
    auto settingsBridge = new LCA::Bridge::SettingsBridge(authService, &app);
    auto authBridge = new LCA::Bridge::AuthBridge(authService, &app);
    auto launcherBridge = new LCA::Bridge::LauncherBridge(authService, settingsBridge, &app);

    QQmlApplicationEngine engine;

    // Expose Bridges to QML
    engine.rootContext()->setContextProperty("authBridge", authBridge);
    engine.rootContext()->setContextProperty("settingsBridge", settingsBridge);
    engine.rootContext()->setContextProperty("launcherBridge", launcherBridge);

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
