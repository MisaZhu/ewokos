#include "openssl/sha.h"
#include <string.h>

/* SHA-256 implementation based on FIPS 180-4 */

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

int SHA256_Init(SHA256_CTX *c) {
    if (!c) return 0;
    
    c->h[0] = 0x6a09e667;
    c->h[1] = 0xbb67ae85;
    c->h[2] = 0x3c6ef372;
    c->h[3] = 0xa54ff53a;
    c->h[4] = 0x510e527f;
    c->h[5] = 0x9b05688c;
    c->h[6] = 0x1f83d9ab;
    c->h[7] = 0x5be0cd19;
    
    c->Nl = 0;
    c->Nh = 0;
    c->num = 0;
    c->md_len = SHA256_DIGEST_LENGTH;
    
    return 1;
}

static void sha256_transform(SHA256_CTX *c, const uint8_t *data) {
    uint32_t a, b, c_, d, e, f, g, h, t1, t2, m[64];
    int i;
    
    for (i = 0; i < 16; i++) {
        m[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    
    for (i = 16; i < 64; i++) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    
    a = c->h[0];
    b = c->h[1];
    c_ = c->h[2];
    d = c->h[3];
    e = c->h[4];
    f = c->h[5];
    g = c->h[6];
    h = c->h[7];
    
    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + sha256_k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c_);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c_;
        c_ = b;
        b = a;
        a = t1 + t2;
    }
    
    c->h[0] += a;
    c->h[1] += b;
    c->h[2] += c_;
    c->h[3] += d;
    c->h[4] += e;
    c->h[5] += f;
    c->h[6] += g;
    c->h[7] += h;
}

int SHA256_Update(SHA256_CTX *c, const void *data, size_t len) {
    if (!c || !data) return 0;
    
    const uint8_t *ptr = data;
    size_t n;
    
    if (c->num != 0) {
        n = 64 - c->num;
        if (len < n) {
            memcpy(c->data + c->num, ptr, len);
            c->num += len;
            return 1;
        }
        memcpy(c->data + c->num, ptr, n);
        sha256_transform(c, c->data);
        c->Nl += 512;
        if (c->Nl < 512) c->Nh++;
        ptr += n;
        len -= n;
        c->num = 0;
    }
    
    while (len >= 64) {
        sha256_transform(c, ptr);
        c->Nl += 512;
        if (c->Nl < 512) c->Nh++;
        ptr += 64;
        len -= 64;
    }
    
    if (len > 0) {
        memcpy(c->data, ptr, len);
        c->num = len;
    }
    
    return 1;
}

int SHA256_Final(unsigned char *md, SHA256_CTX *c) {
    if (!c || !md) return 0;
    
    uint8_t final[64];
    size_t n = c->num;
    
    c->Nl += n * 8;
    if (c->Nl < n * 8) c->Nh++;
    
    c->data[n] = 0x80;
    n++;
    
    if (n > 56) {
        memset(c->data + n, 0, 64 - n);
        sha256_transform(c, c->data);
        n = 0;
    }
    
    memset(c->data + n, 0, 56 - n);
    
    c->data[56] = (c->Nh >> 24) & 0xFF;
    c->data[57] = (c->Nh >> 16) & 0xFF;
    c->data[58] = (c->Nh >> 8) & 0xFF;
    c->data[59] = c->Nh & 0xFF;
    c->data[60] = (c->Nl >> 24) & 0xFF;
    c->data[61] = (c->Nl >> 16) & 0xFF;
    c->data[62] = (c->Nl >> 8) & 0xFF;
    c->data[63] = c->Nl & 0xFF;
    
    sha256_transform(c, c->data);
    
    for (int i = 0; i < 8; i++) {
        md[i * 4] = (c->h[i] >> 24) & 0xFF;
        md[i * 4 + 1] = (c->h[i] >> 16) & 0xFF;
        md[i * 4 + 2] = (c->h[i] >> 8) & 0xFF;
        md[i * 4 + 3] = c->h[i] & 0xFF;
    }
    
    return 1;
}

unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md) {
    static unsigned char static_md[SHA256_DIGEST_LENGTH];
    SHA256_CTX c;
    
    if (!md) md = static_md;
    
    SHA256_Init(&c);
    SHA256_Update(&c, d, n);
    SHA256_Final(md, &c);
    
    return md;
}

/* SHA-1 implementation */
static const uint32_t sha1_k[4] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

int SHA1_Init(SHA_CTX *c) {
    if (!c) return 0;
    
    c->h[0] = 0x67452301;
    c->h[1] = 0xefcdab89;
    c->h[2] = 0x98badcfe;
    c->h[3] = 0x10325476;
    c->h[4] = 0xc3d2e1f0;
    
    c->Nl = 0;
    c->Nh = 0;
    c->num = 0;
    
    return 1;
}

static void sha1_transform(SHA_CTX *c, const uint8_t *data) {
    uint32_t a, b, c_, d, e, t, w[80];
    int i;
    
    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    
    for (i = 16; i < 80; i++) {
        w[i] = ROTR(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    
    a = c->h[0];
    b = c->h[1];
    c_ = c->h[2];
    d = c->h[3];
    e = c->h[4];
    
    for (i = 0; i < 80; i++) {
        if (i < 20) {
            t = ((b & c_) | (~b & d)) + sha1_k[0];
        } else if (i < 40) {
            t = (b ^ c_ ^ d) + sha1_k[1];
        } else if (i < 60) {
            t = ((b & c_) | (b & d) | (c_ & d)) + sha1_k[2];
        } else {
            t = (b ^ c_ ^ d) + sha1_k[3];
        }
        t += ROTR(a, 5) + e + w[i];
        e = d;
        d = c_;
        c_ = ROTR(b, 30);
        b = a;
        a = t;
    }
    
    c->h[0] += a;
    c->h[1] += b;
    c->h[2] += c_;
    c->h[3] += d;
    c->h[4] += e;
}

int SHA1_Update(SHA_CTX *c, const void *data, size_t len) {
    if (!c || !data) return 0;
    
    const uint8_t *ptr = data;
    size_t n;
    
    if (c->num != 0) {
        n = 64 - c->num;
        if (len < n) {
            memcpy(c->data + c->num, ptr, len);
            c->num += len;
            return 1;
        }
        memcpy(c->data + c->num, ptr, n);
        sha1_transform(c, c->data);
        c->Nl += 512;
        if (c->Nl < 512) c->Nh++;
        ptr += n;
        len -= n;
        c->num = 0;
    }
    
    while (len >= 64) {
        sha1_transform(c, ptr);
        c->Nl += 512;
        if (c->Nl < 512) c->Nh++;
        ptr += 64;
        len -= 64;
    }
    
    if (len > 0) {
        memcpy(c->data, ptr, len);
        c->num = len;
    }
    
    return 1;
}

int SHA1_Final(unsigned char *md, SHA_CTX *c) {
    if (!c || !md) return 0;
    
    size_t n = c->num;
    
    c->Nl += n * 8;
    if (c->Nl < n * 8) c->Nh++;
    
    c->data[n] = 0x80;
    n++;
    
    if (n > 56) {
        memset(c->data + n, 0, 64 - n);
        sha1_transform(c, c->data);
        n = 0;
    }
    
    memset(c->data + n, 0, 56 - n);
    
    c->data[56] = (c->Nh >> 24) & 0xFF;
    c->data[57] = (c->Nh >> 16) & 0xFF;
    c->data[58] = (c->Nh >> 8) & 0xFF;
    c->data[59] = c->Nh & 0xFF;
    c->data[60] = (c->Nl >> 24) & 0xFF;
    c->data[61] = (c->Nl >> 16) & 0xFF;
    c->data[62] = (c->Nl >> 8) & 0xFF;
    c->data[63] = c->Nl & 0xFF;
    
    sha1_transform(c, c->data);
    
    for (int i = 0; i < 5; i++) {
        md[i * 4] = (c->h[i] >> 24) & 0xFF;
        md[i * 4 + 1] = (c->h[i] >> 16) & 0xFF;
        md[i * 4 + 2] = (c->h[i] >> 8) & 0xFF;
        md[i * 4 + 3] = c->h[i] & 0xFF;
    }
    
    return 1;
}

unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md) {
    static unsigned char static_md[20];
    SHA_CTX c;
    
    if (!md) md = static_md;
    
    SHA1_Init(&c);
    SHA1_Update(&c, d, n);
    SHA1_Final(md, &c);
    
    return md;
}
