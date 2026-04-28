#ifndef OPENSSL_ED25519_H
#define OPENSSL_ED25519_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ED25519 key size definitions */
#define ED25519_PUBLIC_KEY_LEN 32
#define ED25519_PRIVATE_KEY_LEN 64  /* seed (32 bytes) + public key (32 bytes) */
#define ED25519_SEED_LEN 32
#define ED25519_SIGNATURE_LEN 64

/* ED25519 key pair structure */
typedef struct ed25519_keypair_st {
    uint8_t public_key[ED25519_PUBLIC_KEY_LEN];
    uint8_t private_key[ED25519_PRIVATE_KEY_LEN];
} ED25519_KEYPAIR;

/* ED25519 signature structure */
typedef struct ed25519_sig_st {
    uint8_t signature[ED25519_SIGNATURE_LEN];
} ED25519_SIG;

/* Curve25519 functions */

/**
 * Curve25519 scalar multiplication: result = scalar*point
 */
int crypto_scalarmult_curve25519(uint8_t *result, const uint8_t *scalar, const uint8_t *point);

/**
 * Curve25519 base point scalar multiplication: result = scalar*B
 */
int crypto_scalarmult_curve25519_base(uint8_t *result, const uint8_t *scalar);

/* ED25519 functions */

/**
 * Generate ED25519 key pair from random seed
 */
int ED25519_generate_keypair(ED25519_KEYPAIR *keypair, const uint8_t seed[ED25519_SEED_LEN]);

/**
 * Sign a message with ED25519
 */
int ED25519_sign(ED25519_SIG *signature, const uint8_t *message, size_t message_len,
                  const ED25519_KEYPAIR *keypair);

/**
 * Verify an ED25519 signature
 */
int ED25519_verify(const ED25519_SIG *signature, const uint8_t *message, size_t message_len,
                    const uint8_t public_key[ED25519_PUBLIC_KEY_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_ED25519_H */