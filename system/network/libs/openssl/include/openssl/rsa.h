#ifndef OPENSSL_RSA_H
#define OPENSSL_RSA_H

#include <stdint.h>
#include <stddef.h>
#include "openssl/bn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* RSA structure */
typedef struct rsa_st RSA;

/* RSA functions */
RSA *RSA_new(void);
void RSA_free(RSA *rsa);
int RSA_size(const RSA *rsa);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_RSA_H */
