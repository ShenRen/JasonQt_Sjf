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

#include "JasonQt_SjfServer.h"

using namespace JasonQt_Sjf;

// ServerManage
ServerManage::ServerManage(const ServerSettings *serverSettings):
    m_serverSettings(serverSettings)
{
    this->setSetCurrentThreadDeviceCallback([=](Device *device)
    {
        this->m_currentThreadDevice[QThread::currentThread()] = device;
    });
}

ServerManage::~ServerManage()
{
    if(m_listenThread)
    {
        qDebug("ServerManage: Destroy with started listen thread");
        delete m_listenThread;
    }
    if(m_inspectionThreadsManage.availableThreadCount())
    {
        qDebug("ServerManage: Destory with start inspection thread");
        m_inspectionThreadsManage.stopThread();
    }
}

bool ServerManage::waitForStarted()
{
    m_currentThreadDevice.clear();

    if(m_serverSettings->listenOnThread())
    {
        if(m_listenThread)
        {
            qDebug("ServerManage::waitForStarted is started");
            return false;
        }

        // Start inspection thread
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.startThread(m_serverSettings->inspectionThreadCount());
        }

        // Start listen thread
        JasonQt_WaitForOtherThread wait;

        m_listenThread = new ThreadNode;
        m_listenThread->start([&]()
        {
            if(!this->onStarting())
            {
                this->onCloseing();
                wait.finish(false);
                return;
            }
            wait.finish(true);
        });

        if(!wait.wait())
        {
            if(m_serverSettings->inspectionDeviceOnThread())
            {
                m_inspectionThreadsManage.stopThread();
            }

            delete m_listenThread;
            m_listenThread = NULL;

            return false;
        }

        // Wait inspection thread started
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.runOnAllThread([=]()
            {
                this->m_currentThreadDevice[QThread::currentThread()];
            });
        }
        else
        {
            m_listenThread->waitForStart([=]()
            {
                this->m_currentThreadDevice[QThread::currentThread()];
            });
        }

        return true;
    }
    else
    {
        // Start inspection thread
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.startThread(m_serverSettings->inspectionThreadCount());
        }
        else
        {
            m_devices = new std::unordered_set<Device *>;
        }

        if(!this->onStarting())
        {
            if(m_serverSettings->inspectionDeviceOnThread())
            {
                m_inspectionThreadsManage.stopThread();
            }
            else
            {
                delete m_devices;
                m_devices = NULL;
            }

            return false;
        }

        // Wait inspection thread started
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.runOnAllThread([=]()
            {
                this->m_currentThreadDevice[QThread::currentThread()];
            });
        }
        else
        {
            m_currentThreadDevice[QThread::currentThread()];
        }

        return true;
    }
}

void ServerManage::waitForClosed()
{
    if(m_serverSettings->listenOnThread())
    {
        if(!m_listenThread)
        {
            qDebug("ServerManage::waitForClosed: Listen thread is NULL");
            return;
        }

        // Stop listen thread
        JasonQt_WaitForOtherThread wait;

        m_listenThread->start([&]()
        {
            this->onCloseing();
            wait.finish();
        });

        wait.wait();

        delete m_listenThread;
        m_listenThread = NULL;

        // Stop inspection thread
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.stopThread();
        }
    }
    else
    {
        this->onCloseing();

        // Stop inspection thread
        if(m_serverSettings->inspectionDeviceOnThread())
        {
            m_inspectionThreadsManage.stopThread();
        }
        else
        {
            delete m_devices;
        }
    }
}

void ServerManage::newDevice(const std::function<Device *()> &deviceMaker)
{
    if(m_serverSettings->inspectionDeviceOnThread())
    {
        auto thread = m_inspectionThreadsManage.nextThread(m_serverSettings->singleThreadDeviceCount());

        if(!thread)
        {
            qDebug("ServerManage::newDevice: thread is NULL");
            return;
        }

        thread->start([=]()
        {
            thread->insertDeviceOnThread([=]()->Device *
            {
                return this->openNewDevice(deviceMaker());
            });
        });
    }
    else
    {
        if(m_serverSettings->listenOnThread())
        {
            m_listenThread->start([=]()
            {
                m_listenThread->insertDeviceOnThread([=]()->Device *
                {
                    return this->openNewDevice(deviceMaker());
                });
            });
        }
        else
        {
            auto device = this->openNewDevice(deviceMaker());
            if(!device) { return; }

            m_devices->insert(device);

            QObject::connect(device, &Device::readyToClose, [=]()
            {
                auto it = this->m_devices->find(device);
                if(it == m_devices->end())
                {
                    qDebug("ThreadNode::insertDevice: error!");
                    return;
                }
                this->m_devices->erase(it);
            });
        }
    }
}

int ServerManage::deviceCount()
{
    if(m_serverSettings->inspectionDeviceOnThread())
    {
        if(!m_inspectionThreadsManage.availableThreadCount()) { return -1; }
        return m_inspectionThreadsManage.deviceCount();
    }
    else if(m_serverSettings->listenOnThread())
    {
        if(!m_listenThread) { return -1; }
        return m_listenThread->deviceCount();
    }
    else
    {
        if(!m_devices) { return -1; }
        return m_devices->size();
    }
}

Device *ServerManage::currentThreadDeivce()
{
    auto it = m_currentThreadDevice.find(QThread::currentThread());
    if(it == m_currentThreadDevice.end()) { return NULL; }

    return it.value();
}

Device *ServerManage::openNewDevice(Device *newDevice)
{
    newDevice->setReceivedManage(this);
    if(!newDevice->waitForOpen())
    {
        newDevice->waitForCloseAndDelete(OpenError);
        return NULL;
    }

    emit newDeviceOpened(newDevice);
    connect(newDevice, &Device::readyToClose, [=, timeFlag = QDateTime::currentMSecsSinceEpoch()]()
    {
        emit oldDeviceClosed(newDevice, QDateTime::currentMSecsSinceEpoch() - timeFlag);
    });

    return newDevice;
}

// ServerManageExtendedBase
void ServerManageExtendedBase::registerToServer()
{
    m_availableReceivedSlots.clear();

    for(auto index = 0; index < this->metaObject()->methodCount(); index++)
    {
        auto method = this->metaObject()->method(index);
        switch(method.methodType())
        {
            case QMetaMethod::Slot:
            {
                if(g_exceptionSlots.contains(method.name())) { continue; }

                static QByteArray receivedSlotFlag("(QJsonObject,QJsonObject&)");

                QString buf = ExtendedBase::fromMethodToCallbackFlag(method);

                if(buf.endsWith(receivedSlotFlag))
                {
                    m_availableReceivedSlots.insert(method.name());

                    qDebug() << "Find received slot:" << method.name();
                }

                break;
            }
            default: { break; }
        }
    }
}

void ServerManageExtendedBase::setServerManage(ServerManage *serverManage)
{
    serverManage->setRequestPackageReceivedCallback([=](const QString &action, const QJsonObject &received, QJsonObject &send)
    {
        if(action.isEmpty())
        {
            qDebug("Request package is received, But action is empty");
            return;
        }

        if(!this->m_availableReceivedSlots.contains(action))
        {
            qDebug() << "Request package is received, Action:" << action << ", but slots is NULL";
            return;
        }

        // Invoke
        QMetaObject::invokeMethod(this, action.toLatin1().data(), Qt::DirectConnection, Q_ARG(QJsonObject, received), Q_ARG(QJsonObject&, send));
    });
    connect(serverManage, &ServerManage::newDeviceOpened, [=](Device *device)
    {
        this->newDeviceOpened(device);
    });
    connect(serverManage, &ServerManage::oldDeviceClosed, [=](Device *device, const qint64 &timeFlag)
    {
        this->oldDeviceClosed(device, timeFlag);
    });
}

QString ServerManageExtendedBase::timeflagToString(const qint64 &time)
{
    if(time < 1000)
    {
        return "0s";
    }
    else if(time < (60 * 1000))
    {
        return QString("%1s").arg(time / 1000);
    }
    else if(time < (3600 * 1000))
    {
        return QString("%1m:%2s").arg(time / 60 / 1000).arg((time % (60 * 1000)) / 1000);
    }
    else
    {
        return QString("%1h:%2m").arg(time / 3600 / 1000).arg((time % (3600 * 1000)) / 1000);
    }
}
