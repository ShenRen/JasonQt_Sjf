/*
    This file is part of JasonQt

    Copyright: Jason

    Contact email: 188080501@qq.com

    GNU Lesser General Public License Usage
    Alternatively, this file may be used under the terms of the GNU Lesser
    General Public License version 2.1 or version 3 as published by the Free
    Software Foundation and appearing in the file LICENSE.LGPLv21 and
    LICENSE.LGPLv3 included in the packaging of this file. Please review the
    following information to ensure the GNU Lesser General Public License
    requirements will be met: https://www.gnu.org/licenses/lgpl.html and
    http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*/

#ifndef __JasonQt_SjfForTcpServer_h__
#define __JasonQt_SjfForTcpServer_h__

// Qt lib import
#include <QTcpSocket>
#include <QTcpServer>

// JasonQt lib import
#include "JasonQt_SjfForTcpDevice.h"
#include "JasonQt_SjfServer.h"

namespace JasonQt_Sjf
{

class TcpServerSettings: public JasonQt_Sjf::ServerSettings
{
public:
    PropertyDeclare(int, listenPort, setListenPort, = 0)
    PropertyDeclare(QHostAddress, listenAddress, setListenAddress, = QHostAddress::Any)
};

class TcpServerDevice: public JasonQt_Sjf::Device
{
    Q_OBJECT

private:
    qintptr m_socketDescriptor;
    QTcpSocket m_socket;

    const TcpDeviceSettings *m_tcpDeviceSettings;

public:
    TcpServerDevice(qintptr socketDescriptor, const TcpDeviceSettings *tcpDeviceSettings);

    ~TcpServerDevice() = default;

    QTcpSocket *socket();

private:
    bool onOpening();

    void onCloseing();

private slots:
    void onError(const QAbstractSocket::SocketError &socketError);
};

class TcpServerListener: public QTcpServer
{
private:
    std::function<void(qintptr socketDescriptor)> m_onNewConnectionCallback;

public:
    TcpServerListener(const std::function<void(qintptr socketDescriptor)> &callback);

    void incomingConnection(qintptr socketDescriptor);
};

class TcpServeManage: public JasonQt_Sjf::ServerManage
{
private:
    const TcpDeviceSettings m_tcpDeviceSettings;
    const TcpServerSettings m_tcpServerSettings;

    TcpServerListener *m_listener = NULL;

public:
    TcpServeManage(const TcpDeviceSettings &tcpDeviceSettings, const TcpServerSettings &tcpServerSettings);

    ~TcpServeManage();

private:
    bool onStarting();

    void onCloseing();
};

class TcpServerManageExtended: public ServerManageExtendedBase
{
    Q_OBJECT

private:
    TcpServeManage *m_servserManage;

public:
    virtual ~TcpServerManageExtended();

    inline TcpServeManage *tcpServerManage() { return m_servserManage; }

    inline TcpServerDevice *currentThreadDevice() { return (TcpServerDevice *)m_servserManage->currentThreadDeivce(); }

public slots:
    bool begin();

protected:
    virtual void onBeging() { }

    virtual void onReadySettings(JasonQt_Sjf::TcpDeviceSettings &tcpDeviceSettings, JasonQt_Sjf::TcpServerSettings &tcpServerSettings) = 0;
};

}

#endif//__JasonQt_SjfForTcpServer_h__
