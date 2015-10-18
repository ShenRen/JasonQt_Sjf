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

#include "JasonQt_SjfDevice.h"

using namespace JasonQt_Sjf;

// Device
Device::Device(QIODevice *device, const DeviceSettings *deviceSettings):
    m_device(device),
    m_writer(new JasonQt_JsonStream::Writer),
    m_reader(new JasonQt_JsonStream::Reader),
    m_deviceSettings(deviceSettings)
{
    if(m_deviceSettings->useJasonQtEncrypt())
    {
        m_writer->setTranslator(new JasonQt_JsonStream::JasonQtEncryptForJsonStream(
                                    m_deviceSettings->jasonQtEncryptPublicKey(),
                                    m_deviceSettings->jasonQtEncryptPrivateKey()));
        m_reader->setTranslator(new JasonQt_JsonStream::JasonQtEncryptForJsonStream(
                                    m_deviceSettings->jasonQtEncryptPublicKey(),
                                    m_deviceSettings->jasonQtEncryptPrivateKey()));
    }
    else if(m_deviceSettings->translatorMaker())
    {
        m_writer->setTranslator(m_deviceSettings->translatorMaker()());
        m_reader->setTranslator(m_deviceSettings->translatorMaker()());
    }
}

Device::~Device()
{
    if(m_receivedIntervalTimer)
    {
        delete m_receivedIntervalTimer;
    }
    if(m_deviceLifetimeTimer)
    {
        delete m_deviceLifetimeTimer;
    }
    delete m_reader;
    delete m_writer;
}

bool Device::waitForOpen()
{
    if(!this->onOpening()) { return false; }

    if(m_deviceSettings->maxReceivedIntervalMSecs() != -1)
    {
        m_receivedIntervalTimer = new QTimer;
        m_receivedIntervalTimer->setSingleShot(true);

        connect(m_receivedIntervalTimer, &QTimer::timeout, [=]()
        {
            this->waitForCloseAndDelete(MaxReceivedIntervalTimeroutError);
        });

        m_receivedIntervalTimer->start(m_deviceSettings->maxReceivedIntervalMSecs());
    }
    if(m_deviceSettings->maxDeviceLifetimeMSecs() != -1)
    {
        m_deviceLifetimeTimer = new QTimer;
        m_deviceLifetimeTimer->setSingleShot(true);

        connect(m_deviceLifetimeTimer, &QTimer::timeout, [=]()
        {
            this->waitForCloseAndDelete(MaxDeviceLifetimeTimerout);
        });

        m_deviceLifetimeTimer->start(m_deviceSettings->maxDeviceLifetimeMSecs());
    }

    m_writer->setDevice(m_device);
    connect(m_device, &QIODevice::readyRead, this, &Device::onReadyRead, Qt::DirectConnection);

    return true;
}

void Device::waitForCloseAndDelete(const Error &reason)
{
    if(m_inCloseing) { return; }
    m_inCloseing = true;

    emit readyToClose(reason);
    this->onCloseing();
    this->deleteLater();
}

void Device::writeRequestPackage(const QString &action, const QJsonObject &data, const int &flag)
{
    auto buf(data);

    buf["__SjfType"] = "request";
    buf["__SjfAction"] = action;
    buf["__SjfFlag"] = flag;

    m_writer->write(buf);
}

void Device::writeReturnPackage(const QString &action, const QJsonObject &data, const int &flag)
{
    auto buf(data);

    buf["__SjfType"] = "return";
    buf["__SjfAction"] = action;
    buf["__SjfFlag"] = flag;

    m_writer->write(buf);
}

void Device::writePostPackage(const QString &action, const QJsonObject &data)
{
    auto buf(data);

    buf["__SjfType"] = "post";
    buf["__SjfAction"] = action;
    buf["__SjfFlag"] = -1;

    m_writer->write(buf);
}

void Device::writePostPackageForOtherThread(const QString &action, const QJsonObject &data)
{
    JasonQt_InvokeFromThread::waitForInvoke(this->thread(), [&]()
    {
        this->writePostPackage(action, data);
    });
}

bool Device::waitForBytesWritten()
{
    return m_device->waitForBytesWritten(m_deviceSettings->waitForBytesWrittenMSecs());
}

void Device::onReadyRead()
{
    const auto buf = m_device->readAll();
//    qDebug() << buf;

    if(m_deviceSettings->maxSingleReceivedBytesCount() != -1)
    {
        m_singleReceivedBytesCount += buf.size();
        if(m_singleReceivedBytesCount > m_deviceSettings->maxSingleReceivedBytesCount())
        {
            this->waitForCloseAndDelete(OverMaxSingleReceivedBytesCountError);
            return;
        }
    }
    if(m_deviceSettings->maxAltogetherReceivedBytesCount() != -1)
    {
        m_altogetherReceivedBytesCount += buf.size();
        if(m_altogetherReceivedBytesCount > m_deviceSettings->maxAltogetherReceivedBytesCount())
        {
            this->waitForCloseAndDelete(OverMaxAltogetherReceivedBytesCount);
            return;
        }
    }

    m_reader->addData(buf);

    while(m_reader->readyRead())
    {
        if(m_receivedIntervalTimer)
        {
            m_receivedIntervalTimer->stop();
            m_receivedIntervalTimer->start();
        }

        if(m_deviceSettings->maxSingleReceivedBytesCount() != -1)
        {
            m_singleReceivedBytesCount = 0;
        }

        QJsonObject data;
        if(!m_reader->read(data)) { continue; }

        if(!data.contains("__SjfType")
        || !data.contains("__SjfAction")
        || !data.contains("__SjfFlag")) { continue; }

        auto packageType = data["__SjfType"].toString();
        auto packageAction = data["__SjfAction"].toString();
        auto packageFlag = data["__SjfFlag"].toInt();

        if(m_deviceSettings->removeSjfFlag())
        {
            data.remove("__SjfType");
            data.remove("__SjfAction");
            data.remove("__SjfFlag");
        }

        if(m_receivedManage->setCurrentThreadDeviceCallback())
        {
            m_receivedManage->setCurrentThreadDeviceCallback()(this);
        }

        if(packageType == "request")
        {
            if(!m_receivedManage->requestPackageReceivedCallback())
            {
                qDebug("Device::onReadyRead: Request package is received, buf callback is NULL");
                return;
            }

            QJsonObject buf;
            m_receivedManage->requestPackageReceivedCallback()(packageAction, data, buf);

            if(!buf.isEmpty())
            {
                this->writeReturnPackage(packageAction, buf, packageFlag);
            }
        }
        else if(packageType == "return")
        {
            if(!m_receivedManage->returnPackageReceivedCallback())
            {
                qDebug("Device::onReadyRead: Return package is received, buf callback is NULL");
                return;
            }

            m_receivedManage->returnPackageReceivedCallback()(packageAction, data, packageFlag);
        }
        else if(packageType == "post")
        {
            if(!m_receivedManage->postPackageReceivedCallback())
            {
                qDebug("Device::onReadyRead: Post package is received, buf callback is NULL");
                return;
            }

            m_receivedManage->postPackageReceivedCallback()(packageAction, data);
        }
        else
        {
            qDebug() << "Device::onReadyRead: Unknow package type:" << packageType;
        }
    }
}

// ThreadNode
ThreadNode::~ThreadNode()
{
    this->waitForStart([=]()
    {
        auto buf = this->m_devices;
        for(auto device: buf)
        {
            device->waitForCloseAndDelete(ThreadCloseError);
        }
    });
}

Device *ThreadNode::availableDevice()
{
    if(m_devices.empty())
    {
        return NULL;
    }
    return *m_devices.begin();
}

int ThreadNode::makeFlag()
{
    return ++m_flag;
}

int ThreadNode::deviceCount()
{
    return m_devices.size();
}

void ThreadNode::insertDevice(const std::function<Device *()> &insertCallback)
{
    auto device = insertCallback();
    if(!device) { return; }

    QObject::connect(device, &Device::readyToClose, [=]()
    {
        auto it = this->m_devices.find(device);
        if(it == m_devices.end())
        {
            qDebug("ThreadNode::insertDevice: error!");
            return;
        }
        this->m_devices.erase(it);
    });
    this->m_devices.insert(device);
}

void ThreadNode::insertDeviceOnThread(const std::function<Device *()> &insertCallback)
{
    this->start([=]()
    {
        this->insertDevice(insertCallback);
    });
}

// ThreadManage
ThreadsManage::~ThreadsManage()
{
    this->stopThread();
}

void ThreadsManage::runOnAllThread(const std::function<void ()> &callback)
{
    for(auto thread: m_clientThreads)
    {
        thread->waitForStart(callback);
    }
}

ThreadNode *ThreadsManage::nextThread(const int &singleThreadDeviceCount)
{
    int flag = 0;

    for(auto index = 0; index < m_clientThreads.size(); index++)
    {
        if(++m_currentInspectionThreadIndex >= m_clientThreads.size())
        {
            m_currentInspectionThreadIndex = 0;
        }

        auto thread = m_clientThreads[m_currentInspectionThreadIndex];
        if((thread->deviceCount() <= flag) && (thread->deviceCount() < singleThreadDeviceCount))
        {
            return thread;
        }
        else if(!flag || (thread->deviceCount() < flag))
        {
            flag = thread->deviceCount();
        }
    }

    for(auto thread: m_clientThreads)
    {
        if((thread->deviceCount() <= flag) && (thread->deviceCount() < singleThreadDeviceCount))
        {
            return thread;
        }
    }

    return NULL;
}

int ThreadsManage::availableThreadCount()
{
    return m_clientThreads.size();
}

void ThreadsManage::startThread(const int &count)
{
    for(auto i = 0; i < count; i++)
    {
        auto thread = new ThreadNode;
        m_clientThreads.push_back(thread);
    }
}

void ThreadsManage::stopThread()
{
    for(auto thread: m_clientThreads)
    {
        delete thread;
    }
    m_clientThreads.clear();
}

int ThreadsManage::deviceCount()
{
    auto sum = 0;
    for(auto thread: m_clientThreads)
    {
        sum += thread->deviceCount();
    }
    return sum;
}

// ExtendedBase
QSet<QString> ExtendedBase::g_exceptionSlots( { "deleteLater", "_q_reregisterTimers", "onSend", "begin" } );
QSet<QString> ExtendedBase::g_exceptionSignals( { "destroyed", "objectNameChanged" } );

QString ExtendedBase::fromMethodToCallbackFlag(const QMetaMethod &method)
{
    QString buf;

    buf += method.name();
    buf += "(";
    for(auto index = 0; index < method.parameterCount(); index++)
    {
        if(index)
        {
            buf += ",";
        }
        buf += method.parameterTypes()[index];
    }
    buf += ")";

    return buf;
}

// qDebug expand
QDebug &operator <<(QDebug &d, const JasonQt_Sjf::Error &flag)
{
    switch(flag)
    {
        case NoError:                                   d << "NoError"; break;
        case RemoteClose:                               d << "RemoteClose"; break;
        case OpenError:                                 d << "OpenError"; break;
        case DeviceError:                               d << "DeviceError"; break;
        case MaxReceivedIntervalTimeroutError:          d << "MaxReceivedIntervalTimeroutError"; break;
        case MaxDeviceLifetimeTimerout:                 d << "MaxDeviceLifetimeTimerout"; break;
        case WaitForBytesWrittenTimeoutError:           d << "WaitForBytesWrittenTimeoutError"; break;
        case OverMaxSingleReceivedBytesCountError:      d << "OverMaxSingleReceivedBytesCountError"; break;
        case OverMaxAltogetherReceivedBytesCount:       d << "OverMaxAltogetherReceivedBytesCount"; break;
        case JsonStreamReadError:                       d << "XmlStreamReadError"; break;
        case ThreadCloseError:                          d << "ThreadCloseError"; break;
        default:                                        d << "UnknowDeviceError"; break;
    }
    return d;
}
