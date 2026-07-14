#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "jjycontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    JjyController controller;
    engine.rootContext()->setContextProperty("jjyController", &controller);

    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/JjySimulator/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(mainUrl);

    return app.exec();
}
