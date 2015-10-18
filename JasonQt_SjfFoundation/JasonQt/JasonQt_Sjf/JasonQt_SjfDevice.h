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

#ifndef __JasonQt_SjfDevice_h__
#define __JasonQt_SjfDevice_h__

#ifndef Q_OS_LINUX
#   if (__cplusplus < 201402)
//#       error("Plwase add c++14 config on pro file")
#   endif
#endif

// Qt lib import
#include <QIODevice>
#include <QThreadPool>

// JasonQt lib import
#include "JasonQt_Foundation.h"
#include "JasonQt_JsonStream.h"

namespace JasonQt_Sjf
{

// Enum defined
enum Error
{
    NoError,
    RemoteClose,
    OpenError,
    DeviceError,
    WaitForBytesWrittenTimeoutError,
    MaxReceivedIntervalTimeroutError,
    MaxDeviceLifetimeTimerout,
    OverMaxSingleReceivedBytesCountError,
    OverMaxAltogetherReceivedBytesCount,
    JsonStreamReadError,
    ThreadCloseError
};

// Class defined
typedef JasonQt_JsonStream::Translator Translator;

class DeviceSettings
{
public:
    PropertyDeclare(int, waitForBytesWrittenMSecs, setWaitForBytesWrittenMSecs, = 30 * 1000)
    PropertyDeclare(int, maxReceivedIntervalMSecs, setMaxReceivedIntervalMSecs, = 60 * 1000)
    PropertyDeclare(int, maxDeviceLifetimeMSecs, setMaxDeviceLifetimeMSecs, = -1)

    PropertyDeclare(bool, useJasonQtEncrypt, setUseJasonQtEncrypt, = true)
    PropertyDeclare(QString, jasonQtEncryptPublicKey, setJasonQtEncryptPublicKey, = QString("SjfKey"))
    PropertyDeclare(QString, jasonQtEncryptPrivateKey, setJasonQtEncryptPrivateKey, = QString())
    PropertyDeclare(std::function< Translator *() >, translatorMaker, setTranslatorMaker, = NULL)

    PropertyDeclare(bool, removeSjfFlag, setRemoveSjfFlag, = true)

    PropertyDeclare(int, maxSingleReceivedBytesCount, setMaxSingleReceivedBytesCount, = 8 * 1024 * 1024)
    PropertyDeclare(int, maxAltogetherReceivedBytesCount, setMaxAltogetherReceivedBytesCount, = -1)
};

class Device;

class ReceivedManage
{
    DeleteClassCopy(ReceivedManage)

protected:
    std::function< void(const QString &action, const QJsonObject &received, QJsonObject &send) > m_requestPackageReceivedCallback = NULL;
    std::function< void(const QString &action, const QJsonObject &received, const int &flag) > m_returnPackageReceivedCallback = NULL;
    std::function< void(const QString &action, const QJsonObject &received) > m_postPackageReceivedCallback = NULL;
    std::function< void(Device *device) > m_setCurrentThreadDeviceCallback = NULL;

public:
    ReceivedManage() = default;

    inline const std::function< void(const QString &action, const QJsonObject &received, QJsonObject &send) > &requestPackageReceivedCallback() const
    { return m_requestPackageReceivedCallback; }

    inline const std::function< void(const QString &action, const QJsonObject &received, const int &flag) > &returnPackageReceivedCallback() const
    { return m_returnPackageReceivedCallback; }

    inline const std::function< void(const QString &action, const QJsonObject &received) > &postPackageReceivedCallback() const
    { return m_postPackageReceivedCallback; }

    inline const std::function< void(Device *device) > &setCurrentThreadDeviceCallback() const
    { return m_setCurrentThreadDeviceCallback; }

    void setRequestPackageReceivedCallback(const std::function<void(const QString &action, const QJsonObject &received, QJsonObject &send)> &callback)
    { m_requestPackageReceivedCallback = callback; }

    void setReturnPackageReceivedCallback(const std::function<void(const QString &action, const QJsonObject &received, const int &flag)> &callback)
    { m_returnPackageReceivedCallback = callback; }

    void setPostPackageReceivedCallback(const std::function<void(const QString &action, const QJsonObject &received)> &callback)
    { m_postPackageReceivedCallback = callback; }

    void setSetCurrentThreadDeviceCallback(const std::function< void(Device *device) > &callback)
    { m_setCurrentThreadDeviceCallback = callback; }
};

class Device: public QObject
{
    Q_OBJECT
    DeleteClassCopy(Device)

private:
    QIODevice *m_device;
    JasonQt_JsonStream::Writer *m_writer;
    JasonQt_JsonStream::Reader *m_reader;

    QTimer *m_receivedIntervalTimer = NULL;
    QTimer *m_deviceLifetimeTimer = NULL;

    const DeviceSettings *m_deviceSettings = NULL;
    ReceivedManage const *m_receivedManage = NULL;

    int m_singleReceivedBytesCount = 0;
    int m_altogetherReceivedBytesCount = 0;

    bool m_inCloseing = false;

public:
    Device(QIODevice *device, const DeviceSettings *deviceSettings);

    virtual ~Device();

    inline QIODevice *device() { return m_device; }

    inline Translator *writerTranslator() { return m_writer->translator(); }

    inline Translator *readerTranslator() { return m_reader->translator(); }

    inline void setReceivedManage(const ReceivedManage *receivedManage) { m_receivedManage = receivedManage; }

public slots:
    bool waitForOpen();

    void waitForCloseAndDelete(const Error &reason);

    void writeRequestPackage(const QString &action, const QJsonObject &data, const int &flag);

    void writeReturnPackage(const QString &action, const QJsonObject &data, const int &flag);

    void writePostPackage(const QString &action, const QJsonObject &data);

    void writePostPackageForOtherThread(const QString &action, const QJsonObject &data);

    bool waitForBytesWritten();

signals:
    void readyToClose(const Error reason);

protected:
    virtual bool onOpening() = 0;

    virtual void onCloseing() = 0;

private slots:
    void onReadyRead();
};

class ThreadNode: public JasonQt_Thread
{
    DeleteClassCopy(ThreadNode)

private:
    std::unordered_set<Device *> m_devices;
    int m_flag = 0;

public:
    ThreadNode() = default;

    ~ThreadNode();

    Device *availableDevice();

    int makeFlag();

    int deviceCount();

    void insertDevice(const std::function<Device *()> &insertCallback);

    void insertDeviceOnThread(const std::function<Device *()> &insertCallback);
};

class ThreadsManage
{
    DeleteClassCopy(ThreadsManage)

private:
    QVector<ThreadNode *> m_clientThreads;
    int m_currentInspectionThreadIndex = -1;

public:
    ThreadsManage() = default;

    ~ThreadsManage();

    void runOnAllThread(const std::function< void(void) > &callback);

    ThreadNode *nextThread(const int &singleThreadDeviceCount);

    int availableThreadCount();

    void startThread(const int &count);

    void stopThread();

    int deviceCount();
};

class ExtendedBase: public QObject
{
    Q_OBJECT

protected:
    static QSet<QString> g_exceptionSlots;
    static QSet<QString> g_exceptionSignals;

protected:
    static QString fromMethodToCallbackFlag(const QMetaMethod &method);
};

}

QDebug &operator <<(QDebug &d, const JasonQt_Sjf::Error &flag);

#endif//__JasonQt_SjfDevice_h__
