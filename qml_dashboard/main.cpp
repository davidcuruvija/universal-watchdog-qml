#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <iostream>
#include <memory>
#include <unistd.h>

#include "Config.h"
#include "ProcessMonitor.h"
#include "WatchdogModel.h"
#include "logger.h"
#include "watchdog.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    Config config;
    Logger logger(std::cout);
    Watchdog watchdog(logger, config);

    watchdog.registerWatchable(std::make_shared<ProcessMonitor>(getpid()));
    watchdog.start();

    WatchdogModel watchdogModel(watchdog);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("watchdogModel", &watchdogModel);
    engine.loadFromModule("WatchdogDashboard", "Dashboard");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&watchdog]() {
        watchdog.stop();
    });

    return app.exec();
}
