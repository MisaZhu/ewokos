#ifndef OPENSSL_DH_H
#define OPENSSL_DH_H

#include <stdint.h>
#include <stddef.h>
#include "openssl/bn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DH structure */
typedef struct dh_st DH;

/* DH functions */
DH *DH_new(void);
void DH_free(DH *dh);
int DH_generate_key(DH *dh);
int DH_compute_key(unsigned char *key, const BIGNUM *pub_key, DH *dh);
int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g);
void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_DH_H */
