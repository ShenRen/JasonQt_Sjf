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

#ifndef __JasonQt_SjfClient_h__
#define __JasonQt_SjfClient_h__

// JasonQt_Sjf lib import
#include "JasonQt_SjfDevice.h"

namespace JasonQt_Sjf
{

#define SjfWaitStyleReturnPair(signalName, succeedSlotName, failSlotName, waitFunName)  \
                                                                                        \
private:                                                                                \
    QEventLoop *m_ ## signalName ## EventLoop = NULL;                                   \
    QJsonObject m_ ## signalName ## JsonBuf;                                            \
                                                                                        \
public Q_SLOTS:                                                                         \
    void succeedSlotName(const QJsonObject &received)                                   \
    {                                                                                   \
        this->m_ ## signalName ## JsonBuf = received;                                   \
        this->m_ ## signalName ## EventLoop->exit(true);                                \
    }                                                                                   \
                                                                                        \
    void failSlotName()                                                                 \
    {                                                                                   \
        this->m_ ## signalName ## EventLoop->exit(false);                               \
    }                                                                                   \
                                                                                        \
public:                                                                                 \
    std::pair< bool, QJsonObject > waitFunName(const QJsonObject &send)                 \
    {                                                                                   \
        if(this->m_ ## signalName ## EventLoop) { return { false, { } }; }              \
                                                                                        \
        this->m_ ## signalName ## EventLoop = new QEventLoop;                           \
                                                                                        \
        this->signalName(send);                                                         \
                                                                                        \
        if(!this->m_ ## signalName ## EventLoop->exec())                                \
        {                                                                               \
            delete this->m_ ## signalName ## EventLoop;                                 \
            this->m_ ## signalName ## EventLoop = NULL;                                 \
            return { false, { } };                                                      \
        }                                                                               \
                                                                                        \
        delete this->m_ ## signalName ## EventLoop;                                     \
        this->m_ ## signalName ## EventLoop = NULL;                                     \
        return { true, this->m_ ## signalName ## JsonBuf };                             \
    }                                                                                   \
                                                                                        \
Q_SIGNALS:                                                                              \
    void signalName(const QJsonObject &);                                               \
                                                                                        \
private:

#define SjfWaitStyleReturnJson(signalName, succeedSlotName, failSlotName, waitFunName)  \
                                                                                        \
private:                                                                                \
    QEventLoop *m_ ## signalName ## EventLoop = NULL;                                   \
    QJsonObject m_ ## signalName ## JsonBuf;                                            \
                                                                                        \
public Q_SLOTS:                                                                         \
    void succeedSlotName(const QJsonObject &received)                                   \
    {                                                                                   \
        this->m_ ## signalName ## JsonBuf = received;                                   \
        this->m_ ## signalName ## EventLoop->exit(true);                                \
    }                                                                                   \
                                                                                        \
    void failSlotName()                                                                 \
    {                                                                                   \
        this->m_ ## signalName ## EventLoop->exit(false);                               \
    }                                                                                   \
                                                                                        \
public Q_SLOTS:                                                                         \
    QJsonObject waitFunName(const QJsonObject &send)                                    \
    {                                                                                   \
        if(this->m_ ## signalName ## EventLoop) { return { { { "OK", false } } }; }     \
                                                                                        \
        this->m_ ## signalName ## EventLoop = new QEventLoop;                           \
                                                                                        \
        this->signalName(send);                                                         \
                                                                                        \
        if(!this->m_ ## signalName ## EventLoop->exec())                                \
        {                                                                               \
            delete this->m_ ## signalName ## EventLoop;                                 \
            this->m_ ## signalName ## EventLoop = NULL;                                 \
            return { { { "OK", false } } };                                             \
        }                                                                               \
                                                                                        \
        delete this->m_ ## signalName ## EventLoop;                                     \
        this->m_ ## signalName ## EventLoop = NULL;                                     \
        if(!this->m_ ## signalName ## JsonBuf.contains("OK"))                           \
        {                                                                               \
            this->m_ ## signalName ## JsonBuf["OK"] = false;                            \
        }                                                                               \
        return this->m_ ## signalName ## JsonBuf;                                       \
    }                                                                                   \
                                                                                        \
Q_SIGNALS:                                                                              \
    void signalName(const QJsonObject &);                                               \
                                                                                        \
private:


class ClientSettings
{
public:
    PropertyDeclare(int, maxRequestWaitTimeMSecs, setMaxRequestWaitTimeMSecs, = 30 * 1000)

    PropertyDeclare(bool, inspectionOnThread, setInspectionOnThread, = true)
    PropertyDeclare(bool, callbackOnOriginalThread, setCallbackOnOriginalThread, = true)

    PropertyDeclare(int, maxReceivedCallbackCount, setMaxReceivedCallbackCount, = 500)
};

class ClientManage: public QObject, public ReceivedManage
{
    Q_OBJECT
    DeleteClassCopy(ClientManage)

private:
    struct RecrivedCallbackNode
    {
        QThread *thread;
        QTimer *timer;
        std::function< void(const QJsonObject &) > succeedCallback;
        std::function< void() > failCallback;
    };

private:
    const ClientSettings *m_clientSettings;

    ThreadNode *m_inspectionThread = NULL;

    Device *m_device = NULL;
    int m_flag = 0;

    QHash< int, RecrivedCallbackNode > m_receivedCallbacks;

public:
    ClientManage(const ClientSettings *clientSettings, const bool &inspectionOnThread);

    virtual ~ClientManage();

    int waitReceivedCallback();

    void sendRequestPackage(const QString &action, const QJsonObject &data,
                            const std::function< void(const QJsonObject &) > &succeedCallback,
                            const std::function< void() > &failCallback = NULL);

    void sendPostPackage(const QString &action, const QJsonObject &data);

protected:
    virtual Device *newDevice() = 0;

    private:
    void onReturnPackageReceived();

    void sendRequestPackage(QThread *thread, Device *device, const int &flag,
                            const QString &action, const QJsonObject &data,
                            const std::function< void(const QJsonObject &) > &succeedCallback,
                            const std::function< void() > &failCallback);
};

class ClientManageExtendedBase: public ExtendedBase
{
    Q_OBJECT

private:
    QSet<QString> m_availableSucceedSlots;
    QSet<QString> m_availableFailSlots;
    QSet<QString> m_availablePostSlots;
    QMap<int,std::pair<QString,QMetaObject::Connection>> m_availableSignals;

    ClientManage *m_clientManage = NULL;

#ifdef QT_QML_LIB
    PropertyDeclare(bool, useQmlMode, setUseQmlMode, = false)
#endif

public:
    void registerToClient();

    void setClientManage(ClientManage *clientManage);

private:
    void onPortReceived(const QString &action, const QJsonObject &received);

private slots:
    void onSend(const QJsonObject &data);

    void onSend(const QVariant &data);
};

}

#endif//__JasonQt_SjfClient_h__
