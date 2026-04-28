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

/* SHA-512 implementation */

static const uint64_t sha512_k[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
    0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
    0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
    0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
    0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
    0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
    0x748f82ee5deb97fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
    0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
    0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

#define ROTR64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))
#define CH64(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ64(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP064(x) (ROTR64(x, 28) ^ ROTR64(x, 34) ^ ROTR64(x, 39))
#define EP164(x) (ROTR64(x, 14) ^ ROTR64(x, 18) ^ ROTR64(x, 41))
#define SIG064(x) (ROTR64(x, 1) ^ ROTR64(x, 8) ^ ((x) >> 7))
#define SIG164(x) (ROTR64(x, 19) ^ ROTR64(x, 61) ^ ((x) >> 6))

int SHA512_Init(SHA512_CTX *c) {
    if (!c) return 0;
    
    c->h[0] = 0x6a09e667f3bcc908;
    c->h[1] = 0xbb67ae8584caa73b;
    c->h[2] = 0x3c6ef372fe94f82b;
    c->h[3] = 0xa54ff53a5f1d36f1;
    c->h[4] = 0x510e527fade682d1;
    c->h[5] = 0x9b05688c2b3e6c1f;
    c->h[6] = 0x1f83d9abfb41bd6b;
    c->h[7] = 0x5be0cd19137e2179;
    
    c->Nl = 0;
    c->Nh = 0;
    c->num = 0;
    c->md_len = SHA512_DIGEST_LENGTH;
    
    return 1;
}

static void sha512_transform(SHA512_CTX *c, const uint8_t *data) {
    uint64_t a, b, c_, d, e, f, g, h, t1, t2, w[80];
    int i;
    
    for (i = 0; i < 16; i++) {
        w[i] = ((uint64_t)data[i * 8] << 56) |
               ((uint64_t)data[i * 8 + 1] << 48) |
               ((uint64_t)data[i * 8 + 2] << 40) |
               ((uint64_t)data[i * 8 + 3] << 32) |
               ((uint64_t)data[i * 8 + 4] << 24) |
               ((uint64_t)data[i * 8 + 5] << 16) |
               ((uint64_t)data[i * 8 + 6] << 8) |
               ((uint64_t)data[i * 8 + 7]);
    }
    
    for (i = 16; i < 80; i++) {
        w[i] = SIG164(w[i - 2]) + w[i - 7] + SIG064(w[i - 15]) + w[i - 16];
    }
    
    a = c->h[0];
    b = c->h[1];
    c_ = c->h[2];
    d = c->h[3];
    e = c->h[4];
    f = c->h[5];
    g = c->h[6];
    h = c->h[7];
    
    for (i = 0; i < 80; i++) {
        t1 = h + EP164(e) + CH64(e, f, g) + sha512_k[i] + w[i];
        t2 = EP064(a) + MAJ64(a, b, c_);
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

int SHA512_Update(SHA512_CTX *c, const void *data, size_t len) {
    if (!c || !data) return 0;
    
    const uint8_t *ptr = data;
    size_t n;
    
    if (c->num != 0) {
        n = 128 - c->num;
        if (len < n) {
            memcpy(c->data + c->num, ptr, len);
            c->num += len;
            return 1;
        }
        memcpy(c->data + c->num, ptr, n);
        sha512_transform(c, c->data);
        c->Nl += 1024;
        if (c->Nl < 1024) c->Nh++;
        ptr += n;
        len -= n;
        c->num = 0;
    }
    
    while (len >= 128) {
        sha512_transform(c, ptr);
        c->Nl += 1024;
        if (c->Nl < 1024) c->Nh++;
        ptr += 128;
        len -= 128;
    }
    
    if (len > 0) {
        memcpy(c->data, ptr, len);
        c->num = len;
    }
    
    return 1;
}

int SHA512_Final(unsigned char *md, SHA512_CTX *c) {
    if (!c || !md) return 0;
    
    size_t n = c->num;
    
    c->Nl += n * 8;
    if (c->Nl < n * 8) c->Nh++;
    
    c->data[n] = 0x80;
    n++;
    
    if (n > 112) {
        memset(c->data + n, 0, 128 - n);
        sha512_transform(c, c->data);
        n = 0;
    }
    
    memset(c->data + n, 0, 112 - n);
    
    c->data[112] = (c->Nh >> 56) & 0xFF;
    c->data[113] = (c->Nh >> 48) & 0xFF;
    c->data[114] = (c->Nh >> 40) & 0xFF;
    c->data[115] = (c->Nh >> 32) & 0xFF;
    c->data[116] = (c->Nh >> 24) & 0xFF;
    c->data[117] = (c->Nh >> 16) & 0xFF;
    c->data[118] = (c->Nh >> 8) & 0xFF;
    c->data[119] = c->Nh & 0xFF;
    c->data[120] = (c->Nl >> 56) & 0xFF;
    c->data[121] = (c->Nl >> 48) & 0xFF;
    c->data[122] = (c->Nl >> 40) & 0xFF;
    c->data[123] = (c->Nl >> 32) & 0xFF;
    c->data[124] = (c->Nl >> 24) & 0xFF;
    c->data[125] = (c->Nl >> 16) & 0xFF;
    c->data[126] = (c->Nl >> 8) & 0xFF;
    c->data[127] = c->Nl & 0xFF;
    
    sha512_transform(c, c->data);
    
    for (int i = 0; i < 8; i++) {
        md[i * 8] = (c->h[i] >> 56) & 0xFF;
        md[i * 8 + 1] = (c->h[i] >> 48) & 0xFF;
        md[i * 8 + 2] = (c->h[i] >> 40) & 0xFF;
        md[i * 8 + 3] = (c->h[i] >> 32) & 0xFF;
        md[i * 8 + 4] = (c->h[i] >> 24) & 0xFF;
        md[i * 8 + 5] = (c->h[i] >> 16) & 0xFF;
        md[i * 8 + 6] = (c->h[i] >> 8) & 0xFF;
        md[i * 8 + 7] = c->h[i] & 0xFF;
    }
    
    return 1;
}

unsigned char *SHA512(const unsigned char *d, size_t n, unsigned char *md) {
    static unsigned char static_md[SHA512_DIGEST_LENGTH];
    SHA512_CTX c;
    
    if (!md) md = static_md;
    
    SHA512_Init(&c);
    SHA512_Update(&c, d, n);
    SHA512_Final(md, &c);
    
    return md;
}
