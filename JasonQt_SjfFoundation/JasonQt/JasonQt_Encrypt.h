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

#ifndef __JasonQt_Encrypt_h__
#define __JasonQt_Encrypt_h__

// Qt lib import
#include <QString>

namespace JasonQt_Encrypt
{

#pragma pack(push)
#pragma pack(1)

class Key
{
private:
    union
    {
        uint8_t allData[42];
        struct
        {
            union
            {
                struct
                {
                    uint8_t publicKeyMain[20];
                    uint8_t publicKeyAppend;
                };
                struct
                {
                    uint8_t publicKeyDataA[17];
                    uint32_t publicKeyDataB;
                };
            };
            union
            {
                struct
                {
                    uint8_t privateKeyMain[20];
                    uint8_t privateKeyAppend;
                };
                struct
                {
                    uint8_t privateKeyDataA[17];
                    uint32_t privateKeyDataB;
                };
            };
        };
    };
    uint32_t seed1;
    uint32_t seed2;
    bool m_usePrivateKey;

public:
    Key(const QString &publicKey = QString(), const QString &privateKey = QString());

    inline void setUsePrivateKey(const bool &usePrivateKey) { m_usePrivateKey = usePrivateKey; }

    void resetSeed();

    void randSeed();

    uint8_t nextValue();

    void updatePublicKey(const char *publicKey);

    void updatePrivateKey(const char *privateKey);
};

class KeyCompute
{
public:
    union JE_WORKSPACE_BLOCK
    {
        uint8_t c[64];
        uint32_t l[16];
    };

private:
    uint32_t m_state[5];
    uint32_t m_count[2];
    uint8_t m_buffer[64];
    uint8_t m_digest[20];

    uint8_t m_workspace[64];
    JE_WORKSPACE_BLOCK *m_block; // JE pointer to the byte array above

public:
    KeyCompute();

    void reset();

    void update(uint8_t *data, uint32_t len);

    void final();

    static void compute(const char *key, char *target);

private:
    void transform(uint32_t *state, uint8_t *buffer);
};

#pragma pack(pop)

void encryption(const void *source, const int &size, void *target, Key &key);

void encryption(const QByteArray &source, QByteArray &target, Key &key);

bool decrypt(const void *source, const int &size, void *target, Key &key);

bool decrypt(const QByteArray &source, QByteArray &target, Key &key);

}

#endif//__JasonQt_Encrypt_h__
