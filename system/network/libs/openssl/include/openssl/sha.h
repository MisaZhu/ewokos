#ifndef OPENSSL_SHA_H
#define OPENSSL_SHA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SHA constants */
#define SHA256_DIGEST_LENGTH 32
#define SHA512_DIGEST_LENGTH 64

/* SHA-256 context */
typedef struct SHA256state_st {
    uint32_t h[8];
    uint32_t Nl, Nh;
    uint8_t data[64];
    unsigned int num, md_len;
} SHA256_CTX;

/* SHA-512 context */
typedef struct SHA512state_st {
    uint64_t h[8];
    uint64_t Nl, Nh;
    uint8_t data[128];
    unsigned int num, md_len;
} SHA512_CTX;

/* SHA-1 context */
typedef struct SHAstate_st {
    uint32_t h[5];
    uint32_t Nl, Nh;
    uint8_t data[64];
    unsigned int num;
} SHA_CTX;

/* SHA functions */
int SHA256_Init(SHA256_CTX *c);
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA256_Final(unsigned char *md, SHA256_CTX *c);
unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md);

int SHA512_Init(SHA512_CTX *c);
int SHA512_Update(SHA512_CTX *c, const void *data, size_t len);
int SHA512_Final(unsigned char *md, SHA512_CTX *c);
unsigned char *SHA512(const unsigned char *d, size_t n, unsigned char *md);

int SHA1_Init(SHA_CTX *c);
int SHA1_Update(SHA_CTX *c, const void *data, size_t len);
int SHA1_Final(unsigned char *md, SHA_CTX *c);
unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_SHA_H */
