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

#ifndef __JasonQt_SjfServer_h__
#define __JasonQt_SjfServer_h__

// JasonQt_Sjf lib import
#include "JasonQt_SjfDevice.h"

namespace JasonQt_Sjf
{

#define SjfServerFail(ErrorMessage)                                                 \
    send["OK"] = false;                                                             \
    send["error"] = ErrorMessage;                                                   \
    return;

#define SjfServerSucceed                                                            \
    send["OK"] = true;                                                              \
    return;

class ServerSettings
{
public:
    PropertyDeclare(bool, listenOnThread, setListenOnThread, = true)
    PropertyDeclare(bool, inspectionDeviceOnThread, setInspectionDeviceOnThread, = true)

    PropertyDeclare(int, inspectionThreadCount, setInspectionThreadCount, = 2)
    PropertyDeclare(int, singleThreadDeviceCount, setSingleThreadDeviceCount, = 2)
};

class ServerManage: public QObject, public ReceivedManage
{
    Q_OBJECT
    DeleteClassCopy(ServerManage)

private:
    const ServerSettings *m_serverSettings;

    QHash< const QThread *, Device * > m_currentThreadDevice;

    std::unordered_set<Device *> *m_devices = NULL;
    ThreadNode *m_listenThread = NULL;
    ThreadsManage m_inspectionThreadsManage;

public:
    ServerManage(const ServerSettings *serverSettings);

    virtual ~ServerManage();

    bool waitForStarted();

    void waitForClosed();

    void newDevice(const std::function<Device *()> &deviceMaker);

    int deviceCount();

    Device *currentThreadDeivce();

protected:
    virtual bool onStarting() = 0;

    virtual void onCloseing() = 0;

private:
    Device *openNewDevice(Device *newDevice);

signals:
    void newDeviceOpened(Device *device);

    void oldDeviceClosed(Device *device, const qint64 connectTime);
};

class ServerManageExtendedBase: public ExtendedBase
{
    Q_OBJECT

private:
    QSet<QString> m_availableReceivedSlots;

public:
    void registerToServer();

    void setServerManage(ServerManage *serverManage);

    QString timeflagToString(const qint64 &time);

protected:
    virtual void newDeviceOpened(Device *) { }

    virtual void oldDeviceClosed(Device *, const qint64 &) { }
};

}

#endif//__JasonQt_SjfServer_h__
