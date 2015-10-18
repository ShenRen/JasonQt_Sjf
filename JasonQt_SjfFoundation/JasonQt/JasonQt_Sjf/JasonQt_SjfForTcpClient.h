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

#ifndef __JasonQt_SjfForTcpClient_h__
#define __JasonQt_SjfForTcpClient_h__

// Qt lib import
#include <QTcpSocket>

#ifdef QT_QML_LIB
#   include <QtQml>
#   include <QQmlApplicationEngine>
#endif

// JasonQt lib import
#include "JasonQt_SjfForTcpDevice.h"
#include "JasonQt_SjfClient.h"

namespace JasonQt_Sjf
{

class TcpClientSettings: public JasonQt_Sjf::ClientSettings
{
public:
    PropertyDeclare(quint16, serverPort, setServerPort, = 0)
    PropertyDeclare(QString, serverAddress, setServerAddress, = "127.0.0.1")
};

class TcpClientDevice: public JasonQt_Sjf::Device
{
    Q_OBJECT

private:
    QTcpSocket m_socket;

    const TcpDeviceSettings *m_tcpDeviceSettings;
    const TcpClientSettings *m_tcpClientSettings;

public:
    TcpClientDevice(const TcpDeviceSettings *tcpDeviceSettings, const TcpClientSettings *tcpClientSettings);

    ~TcpClientDevice() = default;

private:
    bool onOpening();

    void onCloseing();

private slots:
    void onError(const QAbstractSocket::SocketError &socketError);
};

class TcpClientManage: public JasonQt_Sjf::ClientManage
{
private:
    const TcpDeviceSettings m_tcpDeviceSettings;
    const TcpClientSettings m_tcpClientSettings;

public:
    TcpClientManage(const TcpDeviceSettings &tcpDeviceSettings, const TcpClientSettings &tcpClientSettings);

    ~TcpClientManage() = default;

private:
    Device *newDevice();
};

class TcpClientManageExtended: public ClientManageExtendedBase
{
    Q_OBJECT

private:
    TcpClientManage *m_clientManage;

public:
    virtual ~TcpClientManageExtended();

    inline TcpClientManage *clientManage() { return m_clientManage; }

#ifdef QT_QML_LIB
    void setToContextProperty(QQmlApplicationEngine &engine, const QString &name);
#endif

public slots:
    void begin();

protected:
    virtual void onReadySettings(JasonQt_Sjf::TcpDeviceSettings &tcpDeviceSettings, JasonQt_Sjf::TcpClientSettings &tcpClientSettings) = 0;
};

// Qml expand
#ifdef QT_QML_LIB

class TcpClientManageExtendedForQuick: public JasonQt_Sjf::TcpClientManageExtended
{
    Q_OBJECT

private:
    QJsonObject *m_tcpDeviceSettingsBuf = NULL;
    QJsonObject *m_tcpClientSettingsBuf = NULL;

public:
    TcpClientManageExtendedForQuick();

public slots:
    void readySettingsDone(const QJsonObject &tcpDeviceSettings, const QJsonObject &tcpClientSettings);

private:
    void onReadySettings(JasonQt_Sjf::TcpDeviceSettings &tcpDeviceSettings, JasonQt_Sjf::TcpClientSettings &tcpClientSettings);
};

void registerTcpClientManageExtendedForQuick();

#endif

}

#endif//__JasonQt_SjfForTcpClient_h__
