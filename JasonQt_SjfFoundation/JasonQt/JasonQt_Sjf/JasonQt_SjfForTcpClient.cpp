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

#include "JasonQt_SjfForTcpClient.h"

using namespace JasonQt_Sjf;

// TcpClientDevice
TcpClientDevice::TcpClientDevice(const TcpDeviceSettings *tcpDeviceSettings, const TcpClientSettings *tcpClientSettings):
    JasonQt_Sjf::Device(&m_socket, tcpDeviceSettings),
    m_tcpDeviceSettings(tcpDeviceSettings),
    m_tcpClientSettings(tcpClientSettings)
{ }

bool TcpClientDevice::onOpening()
{
    m_socket.connectToHost(m_tcpClientSettings->serverAddress(), m_tcpClientSettings->serverPort());
    if(!m_socket.waitForConnected(m_tcpDeviceSettings->connectWaitTimeMSecs())) { return false; }

    connect(&m_socket, (void(QAbstractSocket::*)(QAbstractSocket::SocketError))&QTcpSocket::error, this, &TcpClientDevice::onError, Qt::DirectConnection);

    return true;
}

void TcpClientDevice::onCloseing()
{
    if(m_socket.state() == QAbstractSocket::ConnectedState)
    {
        m_socket.close();
    }
}

void TcpClientDevice::onError(const QAbstractSocket::SocketError &socketError)
{
    switch(socketError)
    {
        case QAbstractSocket::SocketTimeoutError: { return; }
        case QAbstractSocket::RemoteHostClosedError: { this->waitForCloseAndDelete(RemoteClose); break; }
        default: { this->waitForCloseAndDelete(DeviceError); break; }
    }
}

// TcpClientManage
TcpClientManage::TcpClientManage(const TcpDeviceSettings &tcpDeviceSettings, const TcpClientSettings &tcpClientSettings):
    ClientManage(&m_tcpClientSettings, tcpClientSettings.inspectionOnThread()),
    m_tcpDeviceSettings(tcpDeviceSettings),
    m_tcpClientSettings(tcpClientSettings)
{ }

Device *TcpClientManage::newDevice()
{
    return new TcpClientDevice(&m_tcpDeviceSettings, &m_tcpClientSettings);
}

// TcpClientManageExtended
TcpClientManageExtended::~TcpClientManageExtended()
{
    if(m_clientManage)
    {
        delete m_clientManage;
    }
}

#ifdef QT_QML_LIB
void TcpClientManageExtended::setToContextProperty(QQmlApplicationEngine &engine, const QString &name)
{
    engine.rootContext()->setContextProperty(name, this);
}
#endif

void TcpClientManageExtended::begin()
{
    JasonQt_Sjf::TcpDeviceSettings tcpDeviceSettings;
    JasonQt_Sjf::TcpClientSettings tcpClientSettings;

    this->onReadySettings(tcpDeviceSettings, tcpClientSettings);

    m_clientManage = new JasonQt_Sjf::TcpClientManage(tcpDeviceSettings, tcpClientSettings);

    this->setClientManage(m_clientManage);
    this->registerToClient();
}

// Qml expand
#ifdef QT_QML_LIB
TcpClientManageExtendedForQuick::TcpClientManageExtendedForQuick()
{
    this->setUseQmlMode(true);
}

void TcpClientManageExtendedForQuick::readySettingsDone(const QJsonObject &tcpDeviceSettings, const QJsonObject &tcpClientSettings)
{
    *m_tcpDeviceSettingsBuf = tcpDeviceSettings;
    *m_tcpClientSettingsBuf = tcpClientSettings;
}

void TcpClientManageExtendedForQuick::onReadySettings(TcpDeviceSettings &tcpDeviceSettings, TcpClientSettings &tcpClientSettings)
{
    QJsonObject tcpDeviceSettingsBuf, tcpClientSettingsBuf;

    // tcpDeviceSettings
    tcpDeviceSettingsBuf["connectWaitTimeMSecs"] = tcpDeviceSettings.connectWaitTimeMSecs();
    tcpDeviceSettingsBuf["disconnectWaitTimeMSecs"] = tcpDeviceSettings.disconnectWaitTimeMSecs();

    tcpDeviceSettingsBuf["waitForBytesWrittenMSecs"] = tcpDeviceSettings.waitForBytesWrittenMSecs();
    tcpDeviceSettingsBuf["maxReceivedIntervalMSecs"] = tcpDeviceSettings.maxReceivedIntervalMSecs();
    tcpDeviceSettingsBuf["maxDeviceLifetimeMSecs"] = tcpDeviceSettings.maxDeviceLifetimeMSecs();

    tcpDeviceSettingsBuf["useJasonQtEncrypt"] = tcpDeviceSettings.useJasonQtEncrypt();
    tcpDeviceSettingsBuf["jasonQtEncryptPublicKey"] = tcpDeviceSettings.jasonQtEncryptPublicKey();
    tcpDeviceSettingsBuf["jasonQtEncryptPrivateKey"] = tcpDeviceSettings.jasonQtEncryptPrivateKey();

    tcpDeviceSettingsBuf["removeSjfFlag"] = tcpDeviceSettings.removeSjfFlag();

    tcpDeviceSettingsBuf["maxSingleReceivedBytesCount"] = tcpDeviceSettings.maxSingleReceivedBytesCount();
    tcpDeviceSettingsBuf["maxAltogetherReceivedBytesCount"] = tcpDeviceSettings.maxAltogetherReceivedBytesCount();

    // tcpClientSettings
    tcpClientSettingsBuf["serverPort"] = tcpClientSettings.serverPort();
    tcpClientSettingsBuf["serverAddress"] = tcpClientSettings.serverAddress();

    tcpClientSettingsBuf["maxRequestWaitTimeMSecs"] = tcpClientSettings.maxRequestWaitTimeMSecs();

    tcpClientSettingsBuf["inspectionOnThread"] = tcpClientSettings.inspectionOnThread();
    tcpClientSettingsBuf["callbackOnOriginalThread"] = tcpClientSettings.callbackOnOriginalThread();

    tcpClientSettingsBuf["maxReceivedCallbackCount"] = tcpClientSettings.maxReceivedCallbackCount();

    // onReadySettings
    m_tcpDeviceSettingsBuf = &tcpDeviceSettingsBuf;
    m_tcpClientSettingsBuf = &tcpClientSettingsBuf;
    QMetaObject::invokeMethod(this, "onReadySettings", Qt::DirectConnection, Q_ARG(QVariant, tcpDeviceSettingsBuf), Q_ARG(QVariant, tcpClientSettingsBuf));
    m_tcpDeviceSettingsBuf = NULL;
    m_tcpClientSettingsBuf = NULL;

    // tcpDeviceSettings
    tcpDeviceSettings.setConnectWaitTimeMSecs(tcpDeviceSettingsBuf["connectWaitTimeMSecs"].toInt());
    tcpDeviceSettings.setDisconnectWaitTimeMSecs(tcpDeviceSettingsBuf["disconnectWaitTimeMSecs"].toInt());

    tcpDeviceSettings.setWaitForBytesWrittenMSecs(tcpDeviceSettingsBuf["waitForBytesWrittenMSecs"].toInt());
    tcpDeviceSettings.setMaxReceivedIntervalMSecs(tcpDeviceSettingsBuf["maxReceivedIntervalMSecs"].toInt());
    tcpDeviceSettings.setMaxDeviceLifetimeMSecs(tcpDeviceSettingsBuf["maxDeviceLifetimeMSecs"].toInt());

    tcpDeviceSettings.setUseJasonQtEncrypt(tcpDeviceSettingsBuf["useJasonQtEncrypt"].toBool());
    tcpDeviceSettings.setJasonQtEncryptPublicKey(tcpDeviceSettingsBuf["jasonQtEncryptPublicKey"].toString());
    tcpDeviceSettings.setJasonQtEncryptPrivateKey(tcpDeviceSettingsBuf["jasonQtEncryptPrivateKey"].toString());

    tcpDeviceSettings.setRemoveSjfFlag(tcpDeviceSettingsBuf["removeSjfFlag"].toBool());


    tcpDeviceSettings.setMaxSingleReceivedBytesCount(tcpDeviceSettingsBuf["maxSingleReceivedBytesCount"].toInt());
    tcpDeviceSettings.setMaxAltogetherReceivedBytesCount(tcpDeviceSettingsBuf["maxAltogetherReceivedBytesCount"].toInt());

    // tcpClientSettings
    tcpClientSettings.setServerPort(tcpClientSettingsBuf["serverPort"].toInt());
    tcpClientSettings.setServerAddress(tcpClientSettingsBuf["serverAddress"].toString());

    tcpClientSettings.setMaxRequestWaitTimeMSecs(tcpClientSettingsBuf["maxRequestWaitTimeMSecs"].toInt());

    tcpClientSettings.setInspectionOnThread(tcpClientSettingsBuf["inspectionOnThread"].toBool());
    tcpClientSettings.setCallbackOnOriginalThread(tcpClientSettingsBuf["callbackOnOriginalThread"].toBool());

    tcpClientSettings.setMaxReceivedCallbackCount(tcpClientSettingsBuf["maxReceivedCallbackCount"].toInt());
}

void JasonQt_Sjf::registerTcpClientManageExtendedForQuick()
{
    qmlRegisterType<TcpClientManageExtendedForQuick>("JasonQtSjf", 1, 0, "JsfForTcpClientManageExtended");
}
#endif
