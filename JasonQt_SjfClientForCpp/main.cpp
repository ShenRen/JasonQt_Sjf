// Qt lib import
#include <QCoreApplication>

// Project lib import
#include "MyClient.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MyClient myClient;
    myClient.begin();

    QJsonObject data;
    data["key"] = "value";

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]()
    {
        if(myClient.clientManage()->waitReceivedCallback() > 450) { return; }

        myClient.testAction(data);
    });
    timer.start(1000);

    return a.exec();
}

//    JasonQt_Sjf::TcpDeviceSettings tcpDeviceSettings;
//    JasonQt_Sjf::TcpClientSettings tcpClientSettings;

//    tcpClientSettings.setServerPort(23410);
//    tcpClientSettings.setInspectionOnThread(false);

//    JasonQt_Sjf::TcpClientManage tcpClientManage(tcpDeviceSettings, tcpClientSettings);

//    QTimer timer;
//    QObject::connect(&timer, &QTimer::timeout, [&]()
//    {
//        if(tcpClientManage.waitReceivedCallback() > tcpClientSettings.maxReceivedCallbackCount() - 50) { return; }

//        QJsonObject buf;
//        buf["key"] = "value";
//        tcpClientManage.sendRequestPackage("testAction", buf, [&](const QJsonObject &)
//        {
////            qDebug() << received;
//        });
//    });
//    timer.start(0);
