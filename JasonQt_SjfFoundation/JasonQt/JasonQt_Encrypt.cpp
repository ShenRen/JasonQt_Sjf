#include "JasonQt_Encrypt.h"

using namespace JasonQt_Encrypt;

// Rotate x bits to the left
#define JEROL32(_val32, _nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))

#define JEBLK0(i) (m_block->l[i] = (JEROL32(m_block->l[i],24) & 0xFF00FF00) | (JEROL32(m_block->l[i],8) & 0x00FF00FF))

#define JEBLK(i) (m_block->l[i&15] = JEROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] ^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// JE rounds
#define JER0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+JEBLK0(i)+0x5A827999+JEROL32(v,5); w=JEROL32(w,30); }

#define JER1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+JEBLK(i)+0x5A827999+JEROL32(v,5); w=JEROL32(w,30); }

#define JER2(v,w,x,y,z,i) { z+=(w^x^y)+JEBLK(i)+0x6ED9EBA1+JEROL32(v,5); w=JEROL32(w,30); }

#define JER3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+JEBLK(i)+0x8F1BBCDC+JEROL32(v,5); w=JEROL32(w,30); }

#define JER4(v,w,x,y,z,i) { z+=(w^x^y)+JEBLK(i)+0xCA62C1D6+JEROL32(v,5); w=JEROL32(w,30); }

// Key
Key::Key(const QString &publicKey, const QString &privateKey)
{
    memset(allData, '\0', 42);
    this->updatePublicKey((publicKey.isEmpty()) ? (NULL) : (publicKey.toLatin1().data()));
    this->updatePrivateKey((privateKey.isEmpty()) ? (NULL) : (privateKey.toLatin1().data()));
}

void Key::resetSeed()
{
    seed1 = publicKeyDataB;
    seed2 = privateKeyDataB;
}

void Key::randSeed()
{
    seed1 += ((seed2 & (seed1 ^ seed2)) ^ seed1) + 0x5A827999 + JEROL32(seed2, 5);
    seed2 += ((seed1 & (seed2 ^ seed1)) ^ seed2) + 0x5A827999 + JEROL32(seed1, 5);
}

uint8_t Key::nextValue()
{
    this->randSeed();

    uint32_t &&v1 = (seed1 ^ seed2) + 0x6ED9EBA1 + JEROL32(seed1, 5);
    uint32_t *&&v2 = (uint32_t *)((allData + ((v1 * seed1) % ((m_usePrivateKey) ? (38) : (16)))));

    return allData[((seed1 ^ seed2) + 0x6ED9EBA1) % ((m_usePrivateKey) ? (42) : (21))]
               ^ (((v1 ^ *v2) + 0xCA62C1D6 + JEROL32(*v2, 9)) & 0xff);
}

void Key::updatePublicKey(const char *publicKey)
{
    if(publicKey)
    {
        KeyCompute::compute(publicKey, (char *)publicKeyMain);
        seed1 = publicKeyDataB;
        publicKeyAppend = strlen(publicKey) % 256;
    }
    else
    {
        seed1 = 0;
    }
}

void Key::updatePrivateKey(const char *privateKey)
{
    if(privateKey)
    {
        KeyCompute::compute(privateKey, (char *)privateKeyMain);
        privateKeyAppend = strlen(privateKey) % 256;
        m_usePrivateKey = true;
        seed2 = privateKeyDataB;
    }
    else
    {
        m_usePrivateKey = false;
        seed2 = 0;
    }
}

// KeyCompute
KeyCompute::KeyCompute()
{
    m_block = (JE_WORKSPACE_BLOCK *)m_workspace;
    reset();
}

void KeyCompute::reset()
{
    m_state[0] = 0x67452301;
    m_state[1] = 0xEFCDAB89;
    m_state[2] = 0x98BADCFE;
    m_state[3] = 0x10325476;
    m_state[4] = 0xC3D2E1F0;

    m_count[0] = 0;
    m_count[1] = 0;
}

void KeyCompute::update(uint8_t *data, uint32_t len)
{
    uint32_t i, j;

    j = (m_count[0] >> 3) & 63;

    if((m_count[0] += len << 3) < (len << 3))
    {
        m_count[1]++;
    }

    m_count[1] += (len >> 29);

    if((j + len) > 63)
    {
        i = 64 - j;
        memcpy(&m_buffer[j], data, i);
        transform(m_state, m_buffer);

        for(; i + 63 < len; i += 64)
        {
            transform(m_state, &data[i]);
        }

        j = 0;
    }
    else
    {
        i = 0;
    }

    memcpy(&m_buffer[j], &data[i], len - i);
}

void KeyCompute::final()
{
    uint32_t i;
    uint8_t finalcount[8];

    for(i = 0; i < 8; i++)
    {
        finalcount[i] = (uint8_t)((m_count[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8) ) & 255);
    }

    update((uint8_t *)"\200", 1);

    while((m_count[0] & 504) != 448)
    {
        update((uint8_t *)"\0", 1);
    }

    update(finalcount, 8);

    for(i = 0; i < 20; i++)
    {
        m_digest[i] = (uint8_t)((m_state[i >> 2] >> ((3 - (i & 3)) * 8) ) & 255);
    }

    i = 0;
    memset(m_buffer, 0, 64);
    memset(m_state, 0, 20);
    memset(m_count, 0, 8);
    memset(finalcount, 0, 8);
    transform(m_state, m_buffer);
}

void KeyCompute::compute(const char *key, char *target)
{
    KeyCompute JE;
    JE.update((uint8_t *)key, strlen(key));
    JE.final();
    memcpy(target, JE.m_digest, 20);
}

void KeyCompute::transform(uint32_t *state, uint8_t *buffer)
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

    memcpy(m_block, buffer, 64);

    // 4 rounds of 20 operations each. Loop unrolled.
    JER0(a,b,c,d,e, 0); JER0(e,a,b,c,d, 1); JER0(d,e,a,b,c, 2); JER0(c,d,e,a,b, 3);
    JER0(b,c,d,e,a, 4); JER0(a,b,c,d,e, 5); JER0(e,a,b,c,d, 6); JER0(d,e,a,b,c, 7);
    JER0(c,d,e,a,b, 8); JER0(b,c,d,e,a, 9); JER0(a,b,c,d,e,10); JER0(e,a,b,c,d,11);
    JER0(d,e,a,b,c,12); JER0(c,d,e,a,b,13); JER0(b,c,d,e,a,14); JER0(a,b,c,d,e,15);
    JER1(e,a,b,c,d,16); JER1(d,e,a,b,c,17); JER1(c,d,e,a,b,18); JER1(b,c,d,e,a,19);
    JER2(a,b,c,d,e,20); JER2(e,a,b,c,d,21); JER2(d,e,a,b,c,22); JER2(c,d,e,a,b,23);
    JER2(b,c,d,e,a,24); JER2(a,b,c,d,e,25); JER2(e,a,b,c,d,26); JER2(d,e,a,b,c,27);
    JER2(c,d,e,a,b,28); JER2(b,c,d,e,a,29); JER2(a,b,c,d,e,30); JER2(e,a,b,c,d,31);
    JER2(d,e,a,b,c,32); JER2(c,d,e,a,b,33); JER2(b,c,d,e,a,34); JER2(a,b,c,d,e,35);
    JER2(e,a,b,c,d,36); JER2(d,e,a,b,c,37); JER2(c,d,e,a,b,38); JER2(b,c,d,e,a,39);
    JER3(a,b,c,d,e,40); JER3(e,a,b,c,d,41); JER3(d,e,a,b,c,42); JER3(c,d,e,a,b,43);
    JER3(b,c,d,e,a,44); JER3(a,b,c,d,e,45); JER3(e,a,b,c,d,46); JER3(d,e,a,b,c,47);
    JER3(c,d,e,a,b,48); JER3(b,c,d,e,a,49); JER3(a,b,c,d,e,50); JER3(e,a,b,c,d,51);
    JER3(d,e,a,b,c,52); JER3(c,d,e,a,b,53); JER3(b,c,d,e,a,54); JER3(a,b,c,d,e,55);
    JER3(e,a,b,c,d,56); JER3(d,e,a,b,c,57); JER3(c,d,e,a,b,58); JER3(b,c,d,e,a,59);
    JER4(a,b,c,d,e,60); JER4(e,a,b,c,d,61); JER4(d,e,a,b,c,62); JER4(c,d,e,a,b,63);
    JER4(b,c,d,e,a,64); JER4(a,b,c,d,e,65); JER4(e,a,b,c,d,66); JER4(d,e,a,b,c,67);
    JER4(c,d,e,a,b,68); JER4(b,c,d,e,a,69); JER4(a,b,c,d,e,70); JER4(e,a,b,c,d,71);
    JER4(d,e,a,b,c,72); JER4(c,d,e,a,b,73); JER4(b,c,d,e,a,74); JER4(a,b,c,d,e,75);
    JER4(e,a,b,c,d,76); JER4(d,e,a,b,c,77); JER4(c,d,e,a,b,78); JER4(b,c,d,e,a,79);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;

    a = b = c = d = e = 0;
}

void JasonQt_Encrypt::encryption(const void *source, const int &size, void *target, Key &key)
{
    if(!size) { return; }

    uint8_t const *from = (uint8_t const *)source;
    uint8_t *to = (uint8_t *)target;
    int n = (size + 7) / 8;

    switch(size % 8)
    {
        case 0:	do { *to++ = (*from++) + key.nextValue();
        case 7:      *to++ = (*from++) + key.nextValue();
        case 6:      *to++ = (*from++) + key.nextValue();
        case 5:      *to++ = (*from++) + key.nextValue();
        case 4:      *to++ = (*from++) + key.nextValue();
        case 3:      *to++ = (*from++) + key.nextValue();
        case 2:      *to++ = (*from++) + key.nextValue();
        case 1:      *to++ = (*from++) + key.nextValue();
            } while (--n > 0);
    }
}

void JasonQt_Encrypt::encryption(const QByteArray &source, QByteArray &target, Key &key)
{
    QByteArray buf;
    buf.resize(source.size());

    encryption(source.data(), source.size(), buf.data(), key);

    target = buf;
}

bool JasonQt_Encrypt::decrypt(const void *source, const int &size, void *target, Key &key)
{
    if(!size) { return true; }

    uint8_t const *from = (uint8_t const *)source;
    uint8_t *to = (uint8_t *)target;
    int n = (size + 7) / 8;

    switch(size % 8)
    {
        case 0:	do { *to++ = (*from++) - key.nextValue();
        case 7:      *to++ = (*from++) - key.nextValue();
        case 6:      *to++ = (*from++) - key.nextValue();
        case 5:      *to++ = (*from++) - key.nextValue();
        case 4:      *to++ = (*from++) - key.nextValue();
        case 3:      *to++ = (*from++) - key.nextValue();
        case 2:      *to++ = (*from++) - key.nextValue();
        case 1:      *to++ = (*from++) - key.nextValue();
            } while (--n > 0);
    }

    return true;
}

bool JasonQt_Encrypt::decrypt(const QByteArray &source, QByteArray &target, Key &key)
{
    QByteArray buf;
    buf.resize(source.size());

    decrypt(source.data(), source.size(), buf.data(), key);

    target = buf;

    return true;
}
