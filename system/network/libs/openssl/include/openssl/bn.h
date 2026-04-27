#ifndef OPENSSL_BN_H
#define OPENSSL_BN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BN structure */
struct bignum_st {
    unsigned long *d;
    int top;
    int dmax;
    int neg;
};
typedef struct bignum_st BIGNUM;

/* BN functions */
BIGNUM *BN_new(void);
void BN_free(BIGNUM *a);
void BN_clear_free(BIGNUM *a);
int BN_set_word(BIGNUM *a, unsigned long w);
unsigned long BN_get_word(const BIGNUM *a);
int BN_hex2bn(BIGNUM **a, const char *str);
char *BN_bn2hex(const BIGNUM *a);
int BN_bn2bin(const BIGNUM *a, unsigned char *to);
BIGNUM *BN_bin2bn(const unsigned char *s, int len, BIGNUM *ret);
int BN_num_bytes(const BIGNUM *a);
int BN_num_bits(const BIGNUM *a);
BIGNUM *BN_copy(BIGNUM *a, const BIGNUM *b);
int BN_cmp(const BIGNUM *a, const BIGNUM *b);

/* BN_CTX for temporary variables */
typedef struct bignum_ctx BN_CTX;
BN_CTX *BN_CTX_new(void);
void BN_CTX_free(BN_CTX *ctx);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_BN_H */
