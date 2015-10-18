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

#include "JasonQt_JsonStream.h"

using namespace JasonQt_JsonStream;

// Base
void Base::setDevice(QIODevice *device)
{
    m_device = device;
}

void Base::setTranslator(Translator *translator)
{
    if(m_translator)
    {
        delete m_buf;

        delete m_translator;
        m_translator = NULL;
    }
    if(translator)
    {
        m_buf = new QByteArray;
        m_translator = translator;
    }
}

// JasonQtEncryptForJsonStream
JasonQtEncryptForJsonStream::JasonQtEncryptForJsonStream(const QString &publicKey, const QString &privateKey):
    m_key(publicKey, privateKey)
{ }

bool JasonQtEncryptForJsonStream::read(const QByteArray &source, QByteArray &target)
{
    return JasonQt_Encrypt::decrypt(source, target, m_key);
}

void JasonQtEncryptForJsonStream::write(const QByteArray &source, QByteArray &target)
{
    JasonQt_Encrypt::encryption(source, target, m_key);
}

// Writer
void Writer::write(const QJsonObject &data)
{
    auto buf = QJsonDocument(data).toJson(QJsonDocument::Compact);

    if(m_translator)
    {
        m_buf->resize(0);
        m_translator->write(buf, *m_buf);

        quint32 size = m_buf->size();
        m_device->write(QByteArray((const char *)&size, 4));
        m_device->write(*m_buf);
    }
    else
    {
        quint32 size = buf.size();
        m_device->write(QByteArray((const char *)&size, 4));
        m_device->write(buf);
    }
}

// Reader
void Reader::addData(const QByteArray &data)
{
    m_buffer.append(data);
}

int Reader::bufAvailableSize()
{
    return m_buffer.size();
}

bool Reader::readyRead()
{
    if(m_buffer.size() < 6) { return false; } // 4 byte size flag and "{}" 2 byte

    return quint32(m_buffer.size() - 4) >= *((quint32 *)m_buffer.data());
}

bool Reader::read(QJsonObject &target)
{
    if(m_buffer.size() < 6) { return false; }

    auto targetSize = *((quint32 *)m_buffer.data());
    if(quint32(m_buffer.size() - 4) < targetSize) { return false; }

    auto data = QByteArray(m_buffer.data() + 4, targetSize);
    m_buffer.remove(0, 4 + targetSize);

    QJsonParseError error;

    if(m_translator)
    {
        m_buf->resize(0);
        m_translator->read(data, *m_buf);

        target = QJsonDocument::fromJson(*m_buf, &error).object();
    }
    else
    {
        target = QJsonDocument::fromJson(data, &error).object();
    }

    if(error.error != QJsonParseError::NoError)
    {
        target = QJsonObject();
        return false;
    }

    return true;
}

void Reader::clear()
{
    m_buffer.clear();
}
