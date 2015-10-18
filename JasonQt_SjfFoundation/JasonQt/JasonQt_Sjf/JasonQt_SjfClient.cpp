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

#include "JasonQt_SjfClient.h"

using namespace JasonQt_Sjf;

// ClientManage
ClientManage::ClientManage(const ClientSettings *clientSettings, const bool &inspectionOnThread):
    m_clientSettings(clientSettings)
{
    this->onReturnPackageReceived();

    if(inspectionOnThread)
    {
        m_inspectionThread = new ThreadNode;
    }
}

ClientManage::~ClientManage()
{
    if(m_device)
    {
        m_device->waitForCloseAndDelete(ThreadCloseError);
    }
    if(m_inspectionThread)
    {
        delete m_inspectionThread;
    }
}

int ClientManage::waitReceivedCallback()
{
    return m_receivedCallbacks.size();
}

void ClientManage::sendRequestPackage(const QString &action, const QJsonObject &data,
                                      const std::function< void(const QJsonObject &) > &succeedCallback,
                                      const std::function< void() > &failCallback)
{
    if(this->waitReceivedCallback() > m_clientSettings->maxReceivedCallbackCount())
    {
        qDebug("ClientManage::send: Can't send, Over max received callback count");
        return;
    }

    auto thread = QThread::currentThread();

    if(m_clientSettings->inspectionOnThread())
    {
        m_inspectionThread->start([=]()
        {
            auto device = this->m_inspectionThread->availableDevice();
            if(!device)
            {
                this->m_inspectionThread->waitForRunning();
                this->m_inspectionThread->insertDevice([&]()->Device *
                {
                    auto device = this->newDevice();
                    device->setReceivedManage(this);
                    if(!device->waitForOpen())
                    {
                        device->waitForCloseAndDelete(OpenError);
                        return NULL;
                    }
                    return device;
                });
            }

            device = this->m_inspectionThread->availableDevice();
            if(!device)
            {
                qDebug("ClientManage::send: Can't send, Device open is fail");
                if(failCallback)
                {
                    if(this->m_clientSettings->callbackOnOriginalThread())
                    {
                        JasonQt_InvokeFromThread::invoke(thread, failCallback);
                    }
                    else
                    {
                        failCallback();
                    }
                }
                return;
            }

            this->sendRequestPackage(thread, device, this->m_inspectionThread->makeFlag(),
                                     action, data,
                                     succeedCallback,
                                     failCallback);
        });
    }
    else
    {
        if(!m_device)
        {
            m_device = this->newDevice();
            m_device->setReceivedManage(this);
            if(!m_device->waitForOpen())
            {
                m_device->waitForCloseAndDelete(OpenError);
                m_device = NULL;

                qDebug("ClientManage::send: Can't send, Device open is fail");
                if(failCallback)
                {
                    failCallback();
                }
                return;
            }

            QObject::connect(m_device, &Device::readyToClose, [=]()
            {
                m_device = NULL;
            });
        }

        this->sendRequestPackage(thread, m_device, ++m_flag,
                                 action, data,
                                 succeedCallback,
                                 failCallback);
    }
}

void ClientManage::sendPostPackage(const QString &action, const QJsonObject &data)
{
    if(this->waitReceivedCallback() > m_clientSettings->maxReceivedCallbackCount())
    {
        qDebug("ClientManage::send: Can't send, Over max received callback count");
        return;
    }

    if(m_clientSettings->inspectionOnThread())
    {
        m_inspectionThread->start([=]()
        {
            auto device = this->m_inspectionThread->availableDevice();
            if(!device)
            {
                this->m_inspectionThread->waitForRunning();
                this->m_inspectionThread->insertDevice([&]()->Device *
                {
                    auto device = this->newDevice();
                    device->setReceivedManage(this);
                    if(!device->waitForOpen())
                    {
                        device->waitForCloseAndDelete(OpenError);
                        return NULL;
                    }
                    return device;
                });
            }

            device = this->m_inspectionThread->availableDevice();
            if(!device)
            {
                qDebug("ClientManage::send: Can't send, Device open is fail");
                return;
            }

            device->writePostPackage(action, data);
        });
    }
    else
    {
        if(!m_device)
        {
            m_device = this->newDevice();
            m_device->setReceivedManage(this);
            if(!m_device->waitForOpen())
            {
                m_device->waitForCloseAndDelete(OpenError);
                m_device = NULL;

                qDebug("ClientManage::send: Can't send, Device open is fail");
                return;
            }

            QObject::connect(m_device, &Device::readyToClose, [=]()
            {
                m_device = NULL;
            });
        }

        m_device->writePostPackage(action, data);
    }
}

void ClientManage::onReturnPackageReceived()
{
    this->setReturnPackageReceivedCallback([=](const QString &, const QJsonObject &received, const int &flag)
    {
        auto it = m_receivedCallbacks.find(flag);
        if(it == m_receivedCallbacks.end())
        {
            qDebug() << "ClientManage::onReceived: Return package is received, flag:" << flag << ", but callback is NULL";
            return;
        }

        if(it.value().timer)
        {
            it.value().timer->stop();
            delete it.value().timer;
        }

        if(this->m_clientSettings->callbackOnOriginalThread())
        {
            if(QThread::currentThread() == it.value().thread)
            {
                auto &&callback = it.value().succeedCallback;
                if(!callback)
                {
                    qDebug("ClientManage::onReceived: Return package is received, But callback is NULL");
                    m_receivedCallbacks.erase(it);
                    return;
                }
                callback(received);
            }
            else
            {
                JasonQt_InvokeFromThread::invoke(it.value().thread, [ callback = it.value().succeedCallback, data = received ]()
                {
                    if(!callback)
                    {
                        qDebug("ClientManage::onReceived: Return package is received, But callback is NULL");
                        return;
                    }
                    callback(data);
                });
            }
        }
        else
        {
            auto &&callback = it.value().succeedCallback;
            if(!callback)
            {
                qDebug("ClientManage::onReceived: Return package is received, But callback is NULL");
                m_receivedCallbacks.erase(it);
                return;
            }
            callback(received);
        }

        m_receivedCallbacks.erase(it);
    });
}

void ClientManage::sendRequestPackage(QThread *thread, Device *device, const int &flag,
                                      const QString &action, const QJsonObject &data,
                                      const std::function<void (const QJsonObject &)> &succeedCallback,
                                      const std::function<void ()> &failCallback)
{
    QTimer *timer = NULL;
    if(this->m_clientSettings->maxRequestWaitTimeMSecs() != -1)
    {
        timer = new QTimer;
        timer->setSingleShot(false);

        QObject::connect(timer, &QTimer::timeout, [=]()
        {
            delete timer;

            auto it = m_receivedCallbacks.find(flag);
            if(it == m_receivedCallbacks.end()) { return; }

            if(it.value().failCallback)
            {
                if(this->m_clientSettings->callbackOnOriginalThread())
                {
                    if(QThread::currentThread() == it.value().thread)
                    {
                        it.value().failCallback();
                    }
                    else
                    {
                        JasonQt_InvokeFromThread::invoke(it.value().thread, [ callback = it.value().failCallback ]()
                        {
                            callback();
                        });
                    }
                }
                else
                {
                    it.value().failCallback();
                }
            }

            m_receivedCallbacks.erase(it);
        });

        timer->start(this->m_clientSettings->maxRequestWaitTimeMSecs());
    }

    m_receivedCallbacks[flag] = { thread, timer, succeedCallback, failCallback };
    device->writeRequestPackage(action, data, flag);
}

// ClientManageExtendedBase
void ClientManageExtendedBase::registerToClient()
{
    for(auto connection: m_availableSignals)
    {
        disconnect(connection.second);
    }
    m_availableSignals.clear();

    for(auto index = 0; index < this->metaObject()->methodCount(); index++)
    {
        auto method = this->metaObject()->method(index);
        switch(method.methodType())
        {
            case QMetaMethod::Slot:
            {
                if(g_exceptionSlots.contains(method.name())) { continue; }

                static QByteArray succeedSlotFlag("Succeed(QJsonObject)");
                static QByteArray failSlotFlag("Fail()");
#ifdef QT_QML_LIB
                static QByteArray succeedSlotFlagForQml("Succeed(QVariant)");
#endif
                static QByteArray postSlotFlag("(QJsonObject)");

                QString buf = ExtendedBase::fromMethodToCallbackFlag(method);

                if(buf.startsWith("on") && buf.endsWith(succeedSlotFlag))
                {
                    buf.remove(0, 2);
                    buf[0] = QString(buf[0]).toLower().toLatin1()[0];
                    buf.remove(buf.size() - succeedSlotFlag.size(), succeedSlotFlag.size());

                    m_availableSucceedSlots.insert(buf);

                    qDebug() << "Find succeed slots:" << buf << "(" << method.name() << ")";
                }
                else if(buf.startsWith("on") && buf.endsWith(failSlotFlag))
                {
                    buf.remove(0, 2);
                    buf[0] = QString(buf[0]).toLower().toLatin1()[0];
                    buf.remove(buf.size() - failSlotFlag.size(), failSlotFlag.size());

                    m_availableFailSlots.insert(buf);

                    qDebug() << "Find fail slots:" << buf << "(" << method.name() << ")";
                }
#ifdef QT_QML_LIB
                else if(buf.startsWith("on") && buf.endsWith(succeedSlotFlagForQml))
                {
                    buf.remove(0, 2);
                    buf[0] = QString(buf[0]).toLower().toLatin1()[0];
                    buf.remove(buf.size() - succeedSlotFlagForQml.size(), succeedSlotFlagForQml.size());

                    m_availableSucceedSlots.insert(buf);

                    qDebug() << "Find QML succeed slots:" << buf << "(" << method.name() << ")";
                }
#endif
                else if(buf.startsWith("on") && buf.endsWith(postSlotFlag))
                {
                    buf.remove(0, 2);
                    buf[0] = QString(buf[0]).toLower().toLatin1()[0];
                    buf.remove(buf.size() - postSlotFlag.size(), postSlotFlag.size());

                    m_availablePostSlots.insert(buf);

                    qDebug() << "Find post slots:" << buf << "(" << method.name() << ")";
                }

                break;
            }
            case QMetaMethod::Signal:
            {
                if(g_exceptionSignals.contains(method.name())) { continue; }

                QString buf = ExtendedBase::fromMethodToCallbackFlag(method);

                if(buf.endsWith("(QJsonObject)"))
                {
                    m_availableSignals[index] = { method.name(), connect(this, ("2" + buf).toLatin1().data(), this, SLOT(onSend(QJsonObject))) };

                    qDebug() << "Find signal:" << method.name();
                }
#ifdef QT_QML_LIB
                else if(buf.endsWith("(QVariant)"))
                {
                    m_availableSignals[index] = { method.name(), connect(this, ("2" + buf).toLatin1().data(), this, SLOT(onSend(QVariant))) };

                    qDebug() << "Find signal:" << method.name();
                }
#endif

                break;
            }
            default: { break; }
        }
    }

    m_clientManage->setPostPackageReceivedCallback([=](const QString &action, const QJsonObject &received)
    {
        this->onPortReceived(action, received);
    });
}

void ClientManageExtendedBase::setClientManage(ClientManage *clientManage)
{
    m_clientManage = clientManage;
}

void ClientManageExtendedBase::onPortReceived(const QString &action, const QJsonObject &received)
{
    if(m_availablePostSlots.contains(action))
    {
        auto buf = action;
        buf[0] = QString(action[0]).toUpper().toLatin1()[0];
        buf = "on" + buf;

#ifdef QT_QML_LIB
        if(this->useQmlMode())
        {
            QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::QueuedConnection, Q_ARG(QVariant, received));
        }
        else
#endif
        {
            QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::DirectConnection, Q_ARG(QJsonObject, received));
        }
    }
}

void ClientManageExtendedBase::onSend(const QJsonObject &data)
{
    auto action = m_availableSignals[QObject::senderSignalIndex()].first;

    if(m_availableSucceedSlots.contains(action))
    {
        if(m_availableFailSlots.contains(action))
        {
            m_clientManage->sendRequestPackage(action, data, [=](const QJsonObject &received)
            {
                auto buf = action;
                buf[0] = QString(action[0]).toUpper().toLatin1()[0];
                buf = "on" + buf + "Succeed";

#ifdef QT_QML_LIB
                if(this->useQmlMode())
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::QueuedConnection, Q_ARG(QVariant, received));
                }
                else
#endif
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::DirectConnection, Q_ARG(QJsonObject, received));
                }
            },
            [=]()
            {
                auto buf = action;
                buf[0] = QString(action[0]).toUpper().toLatin1()[0];
                buf = "on" + buf + "Fail";

#ifdef QT_QML_LIB
                if(this->useQmlMode())
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::QueuedConnection);
                }
                else
#endif
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::DirectConnection);
                }
            });
        }
        else
        {
            m_clientManage->sendRequestPackage(action, data, [=](const QJsonObject &received)
            {
                auto buf = action;
                buf[0] = QString(action[0]).toUpper().toLatin1()[0];
                buf = "on" + buf + "Succeed";

#ifdef QT_QML_LIB
                if(this->useQmlMode())
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::QueuedConnection, Q_ARG(QVariant, received));
                }
                else
#endif
                {
                    QMetaObject::invokeMethod(this, buf.toLatin1().data(), Qt::DirectConnection, Q_ARG(QJsonObject, received));
                }
            });
        }
    }
    else
    {
        m_clientManage->sendPostPackage(action, data);
    }
}

void ClientManageExtendedBase::onSend(const QVariant &data)
{
    this->onSend(QJsonObject::fromVariantMap(data.toMap()));
}
