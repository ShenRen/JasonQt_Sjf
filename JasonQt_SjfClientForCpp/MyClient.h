#ifndef MYCLIENT
#define MYCLIENT

// JasonQt lib import
#include "JasonQt_SjfForTcpClient.h"

class MyClient: public JasonQt_Sjf::TcpClientManageExtended
{
    Q_OBJECT

public slots:
    void onTestActionSucceed(const QJsonObject &received)
    {
        qDebug() << "onTestActionSucceed" << received;
    }

    void onTestActionFail()
    {
        qDebug() << "onTestActionFail";
    }

signals:
    void testAction(const QJsonObject &data);

private:
    void onReadySettings(JasonQt_Sjf::TcpDeviceSettings &tcpDeviceSettings, JasonQt_Sjf::TcpClientSettings &tcpClientSettings)
    {
        tcpDeviceSettings.setJasonQtEncryptPrivateKey("PrivateKey");
        tcpClientSettings.setServerPort(23410);
    }
};

#endif // MYCLIENT
