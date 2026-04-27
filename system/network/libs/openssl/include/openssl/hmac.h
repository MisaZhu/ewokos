#ifndef OPENSSL_HMAC_H
#define OPENSSL_HMAC_H

#include <stdint.h>
#include <stddef.h>
#include "openssl/evp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* HMAC functions */
unsigned char *HMAC(const EVP_MD *evp_md, const void *key, int key_len,
                    const unsigned char *d, size_t n, unsigned char *md,
                    unsigned int *md_len);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_HMAC_H */
