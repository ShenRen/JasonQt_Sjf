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

#ifndef __JasonQt_JsonStream_h__
#define __JasonQt_JsonStream_h__

#if (__cplusplus < 201103)
#   error("Plwase add c++11 config on pro file")
#endif

#ifndef QT_CORE_LIB
#   error("Plwaer add core in pro file")
#endif

// C++ lib import
#include <functional>

// Qt lib import
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QBuffer>
#include <QDebug>

// JasonQt lib import
#include "JasonQt_Encrypt.h"

namespace JasonQt_JsonStream
{

class Translator
{
public:
    virtual ~Translator() = default;

    virtual bool read(const QByteArray &source, QByteArray &target) = 0;

    virtual void write(const QByteArray &source, QByteArray &target) = 0;
};

class JasonQtEncryptForJsonStream: public Translator
{
private:
    JasonQt_Encrypt::Key m_key;

public:
    JasonQtEncryptForJsonStream(const QString &publicKey, const QString &privateKey);

    inline JasonQt_Encrypt::Key &key() { return m_key; }

    bool read(const QByteArray &source, QByteArray &target);

    void write(const QByteArray &source, QByteArray &target);
};

class Base
{
protected:
    QIODevice *m_device = NULL;
    Translator *m_translator = NULL;

    QByteArray *m_buf;

public:
    inline QIODevice *device() { return m_device; }

    inline Translator *translator() { return m_translator; }

    void setDevice(QIODevice *device);

    void setTranslator(Translator *translator);
};

class Writer: public Base
{
public:
    void write(const QJsonObject &data);
};

class Reader: public Base
{
private:
    QByteArray m_buffer;

public:
    void addData(const QByteArray &data);

    int bufAvailableSize();

    bool readyRead();

    bool read(QJsonObject &target);

    void clear();
};

}

#endif//__JasonQt_JsonStream_h__
