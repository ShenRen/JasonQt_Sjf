#ifndef MYSERVER
#define MYSERVER

// JasonQt lib import
#include "JasonQt_SjfForTcpServer.h"

class MyServer: public JasonQt_Sjf::TcpServerManageExtended
{
    Q_OBJECT

public slots:
    void testAction(const QJsonObject &received, QJsonObject &send)
    {
        qDebug() << "testAction" << received;
        send["return"] = "value";

//        static int count = 0;
//        if(!(++count % 10000))
//        {
//            qDebug() << count;
//        }
    }

private:
    void newDeviceOpened(JasonQt_Sjf::Device *device)
    {
        auto socket = ((QTcpSocket *)device->device());

        qDebug() << "New device opened, From"
                 << QString("%1:%2").arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString(), QString::number(socket->peerPort()));
    }

    void oldDeviceClosed(JasonQt_Sjf::Device *device, const qint64 &timeFlag)
    {
        auto socket = ((QTcpSocket *)device->device());

        qDebug() << "Old device closed, From"
                 << QString("%1:%2").arg(QHostAddress(socket->peerAddress().toIPv4Address()).toString(), QString::number(socket->peerPort()))
                 << ", Time:"
                 << timeflagToString(timeFlag);
    }

    void onReadySettings(JasonQt_Sjf::TcpDeviceSettings &tcpDeviceSettings, JasonQt_Sjf::TcpServerSettings &tcpServerSettings)
    {
        tcpDeviceSettings.setJasonQtEncryptPrivateKey("PrivateKey");
        tcpServerSettings.setListenPort(23410);
    }
};

#endif // MYSERVER
