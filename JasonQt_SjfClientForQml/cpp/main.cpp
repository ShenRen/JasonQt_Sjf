// Qt lib import
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

// JasonQt lib import
#include "JasonQt_SjfForTcpClient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    JasonQt_Sjf::registerTcpClientManageExtendedForQuick();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
