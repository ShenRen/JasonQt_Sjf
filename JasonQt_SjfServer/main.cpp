// Qt lib import
#include <QCoreApplication>

// Project lib import
#include "MyServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MyServer myServer;
    qDebug() << "Begin return:" << myServer.begin();

    return a.exec();
}

//    JasonQt_Sjf::TcpDeviceSettings tcpDeviceSettings;
//    JasonQt_Sjf::TcpServerSettings tcpServerSettings;

//    tcpServerSettings.setListenPort(23410);

//    JasonQt_Sjf::TcpServeManage tcpServerManage(tcpDeviceSettings, tcpServerSettings);

//    int count = 0;
//    qint64 timeFlag = 0;

//    tcpServerManage.setRequestPackageReceivedCallback([&](const QString &action, const QJsonObject &received, QJsonObject &send)
//    {
//        count++;
//        if(!(count % 100000))
//        {
//            if(timeFlag)
//            {
//                qDebug() << count << QDateTime::currentMSecsSinceEpoch() - timeFlag;
//            }
//            timeFlag = QDateTime::currentMSecsSinceEpoch();
//        }

////        qDebug() << action << received;
//        send["return"] = "value";
//    });

//    qDebug() << "waitForStarted:" << tcpServerManage.waitForStarted();
