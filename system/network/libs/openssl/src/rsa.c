#include "openssl/rsa.h"
#include <stdlib.h>

struct rsa_st {
    BIGNUM *n;
    BIGNUM *e;
    BIGNUM *d;
};

RSA *RSA_new(void) {
    RSA *rsa = (RSA *)calloc(1, sizeof(RSA));
    return rsa;
}

void RSA_free(RSA *rsa) {
    if (rsa) {
        BN_free(rsa->n);
        BN_free(rsa->e);
        BN_clear_free(rsa->d);
        free(rsa);
    }
}

int RSA_size(const RSA *rsa) {
    if (!rsa || !rsa->n) return 0;
    return BN_num_bytes(rsa->n);
}
