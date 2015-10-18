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

#include "JasonQt_SjfForTcpServer.h"

using namespace JasonQt_Sjf;

// TcpServerDevice
TcpServerDevice::TcpServerDevice(qintptr socketDescriptor, const TcpDeviceSettings *tcpDeviceSettings):
    JasonQt_Sjf::Device(&m_socket, tcpDeviceSettings),
    m_socketDescriptor(socketDescriptor),
    m_tcpDeviceSettings(tcpDeviceSettings)
{ }

QTcpSocket *TcpServerDevice::socket()
{
    return &m_socket;
}

bool TcpServerDevice::onOpening()
{
    if(!m_socket.setSocketDescriptor(m_socketDescriptor)) { return false; }
    if(!m_socket.waitForConnected(m_tcpDeviceSettings->connectWaitTimeMSecs())) { return false; }

    connect(&m_socket, (void(QAbstractSocket::*)(QAbstractSocket::SocketError))&QTcpSocket::error, this, &TcpServerDevice::onError, Qt::DirectConnection);

    return true;
}

void TcpServerDevice::onCloseing()
{
    if(m_socket.state() == QAbstractSocket::ConnectedState)
    {
        m_socket.close();
    }
}

void TcpServerDevice::onError(const QAbstractSocket::SocketError &socketError)
{
    switch(socketError)
    {
        case QAbstractSocket::SocketTimeoutError: { return; }
        case QAbstractSocket::RemoteHostClosedError: { this->waitForCloseAndDelete(RemoteClose); break; }
        default: { this->waitForCloseAndDelete(DeviceError); break; }
    }
}

// TcpServerListener
TcpServerListener::TcpServerListener(const std::function<void (qintptr)> &callback):
    m_onNewConnectionCallback(callback)
{ }

void TcpServerListener::incomingConnection(qintptr socketDescriptor)
{
    m_onNewConnectionCallback(socketDescriptor);
}

// TcpServeManage
TcpServeManage::TcpServeManage(const TcpDeviceSettings &tcpDeviceSettings, const TcpServerSettings &tcpServerSettings):
    ServerManage(&m_tcpServerSettings),
    m_tcpDeviceSettings(tcpDeviceSettings),
    m_tcpServerSettings(tcpServerSettings)
{ }

TcpServeManage::~TcpServeManage()
{
    if(m_listener)
    {
        qDebug("TcpServeManage: Destroy with started listener");
        delete m_listener;
    }
}

bool TcpServeManage::onStarting()
{
    if(m_listener)
    {
        qDebug("TcpServeManage::onStarting: Listener is available");
        return false;
    }

    m_listener = new TcpServerListener([=](qintptr socketDescriptor)
    {
        this->newDevice([=]()
        {
            return new TcpServerDevice(socketDescriptor, &this->m_tcpDeviceSettings);
        });
    });

    return m_listener->listen(m_tcpServerSettings.listenAddress(), m_tcpServerSettings.listenPort());
}

void TcpServeManage::onCloseing()
{
    if(!m_listener)
    {
        qDebug("TcpServeManage::onCloseing: Listener is NULL");
        return;
    }

    delete m_listener;
    m_listener = NULL;
}

// TcpServerManageExtended
TcpServerManageExtended::~TcpServerManageExtended()
{
    if(m_servserManage)
    {
        delete m_servserManage;
    }
}

bool TcpServerManageExtended::begin()
{
    TcpDeviceSettings tcpDeviceSettings;
    TcpServerSettings tcpServerSettings;

    this->onReadySettings(tcpDeviceSettings, tcpServerSettings);

    m_servserManage = new TcpServeManage(tcpDeviceSettings, tcpServerSettings);

    this->registerToServer();
    this->setServerManage(m_servserManage);

    if(!m_servserManage->waitForStarted())
    {
        delete m_servserManage;
        m_servserManage = NULL;
        return false;
    }

    onBeging();
    return true;
}
