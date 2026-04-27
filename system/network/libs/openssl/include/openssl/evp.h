#ifndef OPENSSL_EVP_H
#define OPENSSL_EVP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EVP Cipher modes */
#define EVP_CIPH_STREAM_CIPHER   0x0
#define EVP_CIPH_ECB_MODE        0x1
#define EVP_CIPH_CBC_MODE        0x2
#define EVP_CIPH_CFB_MODE        0x3
#define EVP_CIPH_OFB_MODE        0x4
#define EVP_CIPH_CTR_MODE        0x5
#define EVP_CIPH_GCM_MODE        0x6
#define EVP_CIPH_CCM_MODE        0x7
#define EVP_CIPH_XTS_MODE        0x10001
#define EVP_CIPH_WRAP_MODE       0x10002
#define EVP_CIPH_OCB_MODE        0x10003
#define EVP_CIPH_MODE            0xF0007
#define EVP_CIPH_VARIABLE_LENGTH 0x8
#define EVP_CIPH_CUSTOM_IV       0x10
#define EVP_CIPH_ALWAYS_CALL_INIT 0x20
#define EVP_CIPH_CTRL_INIT       0x40
#define EVP_CIPH_CUSTOM_KEY_LENGTH 0x80
#define EVP_CIPH_NO_PADDING      0x100
#define EVP_CIPH_RAND_KEY        0x200
#define EVP_CIPH_CUSTOM_COPY     0x400
#define EVP_CIPH_FLAG_DEFAULT_ASN1 0x1000
#define EVP_CIPH_FLAG_LENGTH_BITS 0x2000
#define EVP_CIPH_FLAG_FIPS       0x4000
#define EVP_CIPH_FLAG_NON_FIPS_ALLOW 0x8000
#define EVP_CIPH_FLAG_CUSTOM_CIPHER 0x100000
#define EVP_CIPH_FLAG_AEAD_CIPHER 0x200000
#define EVP_CIPH_FLAG_TLS1_1_MULTIBLOCK 0x400000
#define EVP_CIPH_FLAG_PIPELINE 0x800000

/* EVP PKEY types */
#define EVP_PKEY_RSA    6
#define EVP_PKEY_DSA    116
#define EVP_PKEY_DH     28
#define EVP_PKEY_EC     408
#define EVP_PKEY_HMAC   855
#define EVP_PKEY_CMAC   892
#define EVP_PKEY_TLS1_PRF 1021
#define EVP_PKEY_HKDF   1036
#define EVP_PKEY_POLY1305 1161
#define EVP_PKEY_SIPHASH 1162
#define EVP_PKEY_X25519 1034
#define EVP_PKEY_ED25519 1087
#define EVP_PKEY_X448   1035
#define EVP_PKEY_ED448  1088

/* Forward declarations */
typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_md_st EVP_MD;
typedef struct evp_md_ctx_st EVP_MD_CTX;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct evp_pkey_ctx_st EVP_PKEY_CTX;
typedef struct env_md_ctx_st ENV_MD_CTX;

/* EVP_CIPHER functions */
const EVP_CIPHER *EVP_aes_128_ecb(void);
const EVP_CIPHER *EVP_aes_128_cbc(void);
const EVP_CIPHER *EVP_aes_128_cfb1(void);
const EVP_CIPHER *EVP_aes_128_cfb8(void);
const EVP_CIPHER *EVP_aes_128_cfb128(void);
const EVP_CIPHER *EVP_aes_128_ofb(void);
const EVP_CIPHER *EVP_aes_128_ctr(void);
const EVP_CIPHER *EVP_aes_128_gcm(void);
const EVP_CIPHER *EVP_aes_128_ccm(void);
const EVP_CIPHER *EVP_aes_128_xts(void);
const EVP_CIPHER *EVP_aes_128_wrap(void);
const EVP_CIPHER *EVP_aes_128_wrap_pad(void);
const EVP_CIPHER *EVP_aes_128_ocb(void);

const EVP_CIPHER *EVP_aes_192_ecb(void);
const EVP_CIPHER *EVP_aes_192_cbc(void);
const EVP_CIPHER *EVP_aes_192_cfb1(void);
const EVP_CIPHER *EVP_aes_192_cfb8(void);
const EVP_CIPHER *EVP_aes_192_cfb128(void);
const EVP_CIPHER *EVP_aes_192_ofb(void);
const EVP_CIPHER *EVP_aes_192_ctr(void);
const EVP_CIPHER *EVP_aes_192_gcm(void);
const EVP_CIPHER *EVP_aes_192_ccm(void);
const EVP_CIPHER *EVP_aes_192_wrap(void);
const EVP_CIPHER *EVP_aes_192_wrap_pad(void);
const EVP_CIPHER *EVP_aes_192_ocb(void);

const EVP_CIPHER *EVP_aes_256_ecb(void);
const EVP_CIPHER *EVP_aes_256_cbc(void);
const EVP_CIPHER *EVP_aes_256_cfb1(void);
const EVP_CIPHER *EVP_aes_256_cfb8(void);
const EVP_CIPHER *EVP_aes_256_cfb128(void);
const EVP_CIPHER *EVP_aes_256_ofb(void);
const EVP_CIPHER *EVP_aes_256_ctr(void);
const EVP_CIPHER *EVP_aes_256_gcm(void);
const EVP_CIPHER *EVP_aes_256_ccm(void);
const EVP_CIPHER *EVP_aes_256_xts(void);
const EVP_CIPHER *EVP_aes_256_wrap(void);
const EVP_CIPHER *EVP_aes_256_wrap_pad(void);
const EVP_CIPHER *EVP_aes_256_ocb(void);

const EVP_CIPHER *EVP_des_ecb(void);
const EVP_CIPHER *EVP_des_cbc(void);
const EVP_CIPHER *EVP_des_cfb1(void);
const EVP_CIPHER *EVP_des_cfb8(void);
const EVP_CIPHER *EVP_des_cfb64(void);
const EVP_CIPHER *EVP_des_ofb(void);

const EVP_CIPHER *EVP_des_ede_ecb(void);
const EVP_CIPHER *EVP_des_ede_cbc(void);
const EVP_CIPHER *EVP_des_ede_cfb64(void);
const EVP_CIPHER *EVP_des_ede_ofb(void);

const EVP_CIPHER *EVP_des_ede3_ecb(void);
const EVP_CIPHER *EVP_des_ede3_cbc(void);
const EVP_CIPHER *EVP_des_ede3_cfb1(void);
const EVP_CIPHER *EVP_des_ede3_cfb8(void);
const EVP_CIPHER *EVP_des_ede3_cfb64(void);
const EVP_CIPHER *EVP_des_ede3_ofb(void);

const EVP_CIPHER *EVP_rc4(void);
const EVP_CIPHER *EVP_rc4_40(void);
const EVP_CIPHER *EVP_rc4_hmac_md5(void);

const EVP_CIPHER *EVP_chacha20(void);
const EVP_CIPHER *EVP_chacha20_poly1305(void);

const EVP_CIPHER *EVP_enc_null(void);

/* EVP_CIPHER_CTX functions */
EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void);
void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_cleanup(EVP_CIPHER_CTX *ctx);

int EVP_CipherInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                       void *impl, const unsigned char *key,
                       const unsigned char *iv, int enc);
int EVP_CipherUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                      int *outl, const unsigned char *in, int inl);
int EVP_CipherFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
int EVP_CipherInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                    const unsigned char *key, const unsigned char *iv,
                    int enc);
int EVP_CipherFinal(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);

int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                        void *impl, const unsigned char *key,
                        const unsigned char *iv);
int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                       int *outl, const unsigned char *in, int inl);
int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
int EVP_EncryptInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                     const unsigned char *key, const unsigned char *iv);
int EVP_EncryptFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);

int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                        void *impl, const unsigned char *key,
                        const unsigned char *iv);
int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                       int *outl, const unsigned char *in, int inl);
int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
int EVP_DecryptInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                     const unsigned char *key, const unsigned char *iv);
int EVP_DecryptFinal(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);

int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *ctx, int pad);
int EVP_CIPHER_CTX_set_key_length(EVP_CIPHER_CTX *ctx, int keylen);
int EVP_CIPHER_CTX_ctrl(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr);
int EVP_CIPHER_CTX_rand_key(EVP_CIPHER_CTX *ctx, unsigned char *key);

const EVP_CIPHER *EVP_CIPHER_CTX_cipher(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_nid(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_block_size(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_key_length(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_iv_length(const EVP_CIPHER_CTX *ctx);
void *EVP_CIPHER_CTX_get_app_data(const EVP_CIPHER_CTX *ctx);
void EVP_CIPHER_CTX_set_app_data(EVP_CIPHER_CTX *ctx, void *data);
int EVP_CIPHER_CTX_type(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_mode(const EVP_CIPHER_CTX *ctx);
int EVP_CIPHER_CTX_flags(const EVP_CIPHER_CTX *ctx);

int EVP_CIPHER_nid(const EVP_CIPHER *cipher);
int EVP_CIPHER_block_size(const EVP_CIPHER *cipher);
int EVP_CIPHER_key_length(const EVP_CIPHER *cipher);
int EVP_CIPHER_iv_length(const EVP_CIPHER *cipher);
unsigned long EVP_CIPHER_flags(const EVP_CIPHER *cipher);
int EVP_CIPHER_mode(const EVP_CIPHER *cipher);

/* EVP_MD (Message Digest) functions */
const EVP_MD *EVP_md_null(void);
const EVP_MD *EVP_md2(void);
const EVP_MD *EVP_md4(void);
const EVP_MD *EVP_md5(void);
const EVP_MD *EVP_sha1(void);
const EVP_MD *EVP_sha224(void);
const EVP_MD *EVP_sha256(void);
const EVP_MD *EVP_sha384(void);
const EVP_MD *EVP_sha512(void);
const EVP_MD *EVP_sha512_224(void);
const EVP_MD *EVP_sha512_256(void);
const EVP_MD *EVP_sha3_224(void);
const EVP_MD *EVP_sha3_256(void);
const EVP_MD *EVP_sha3_384(void);
const EVP_MD *EVP_sha3_512(void);
const EVP_MD *EVP_shake128(void);
const EVP_MD *EVP_shake256(void);
const EVP_MD *EVP_mdc2(void);
const EVP_MD *EVP_ripemd160(void);
const EVP_MD *EVP_whirlpool(void);
const EVP_MD *EVP_blake2b512(void);
const EVP_MD *EVP_blake2s256(void);
const EVP_MD *EVP_sm3(void);

const EVP_MD *EVP_md5_sha1(void);

/* EVP_MD_CTX functions */
EVP_MD_CTX *EVP_MD_CTX_new(void);
void EVP_MD_CTX_free(EVP_MD_CTX *ctx);
int EVP_MD_CTX_reset(EVP_MD_CTX *ctx);
int EVP_MD_CTX_cleanup(EVP_MD_CTX *ctx);

int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl);
int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt);
int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);
int EVP_DigestInit(EVP_MD_CTX *ctx, const EVP_MD *type);
int EVP_DigestFinal(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);

int EVP_MD_CTX_copy_ex(EVP_MD_CTX *out, const EVP_MD_CTX *in);
int EVP_MD_CTX_copy(EVP_MD_CTX *out, const EVP_MD_CTX *in);

int EVP_Digest(const void *data, size_t count, unsigned char *md,
                unsigned int *size, const EVP_MD *type, void *impl);

int EVP_MD_size(const EVP_MD *md);
int EVP_MD_block_size(const EVP_MD *md);
unsigned long EVP_MD_flags(const EVP_MD *md);
int EVP_MD_type(const EVP_MD *md);
int EVP_MD_pkey_type(const EVP_MD *md);
int EVP_MD_CTX_size(const EVP_MD_CTX *ctx);
int EVP_MD_CTX_block_size(const EVP_MD_CTX *ctx);
int EVP_MD_CTX_type(const EVP_MD_CTX *ctx);
const EVP_MD *EVP_MD_CTX_md(const EVP_MD_CTX *ctx);
void *EVP_MD_CTX_md_data(const EVP_MD_CTX *ctx);

/* EVP_PKEY functions */
EVP_PKEY *EVP_PKEY_new(void);
void EVP_PKEY_free(EVP_PKEY *pkey);
EVP_PKEY *EVP_PKEY_dup(EVP_PKEY *pkey);
int EVP_PKEY_up_ref(EVP_PKEY *pkey);
int EVP_PKEY_assign(EVP_PKEY *pkey, int type, void *key);
int EVP_PKEY_set_type(EVP_PKEY *pkey, int type);
int EVP_PKEY_set_type_str(EVP_PKEY *pkey, const char *str, int len);
int EVP_PKEY_type(int type);
int EVP_PKEY_base_id(const EVP_PKEY *pkey);
int EVP_PKEY_bits(const EVP_PKEY *pkey);
int EVP_PKEY_security_bits(const EVP_PKEY *pkey);
int EVP_PKEY_size(const EVP_PKEY *pkey);
int EVP_PKEY_id(const EVP_PKEY *pkey);

/* Base64 encoding/decoding */
int EVP_EncodeBlock(unsigned char *t, const unsigned char *f, int n);
int EVP_DecodeBlock(unsigned char *t, const unsigned char *f, int n);

/* Sign/Verify functions */
int EVP_SignInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl);
int EVP_SignInit(EVP_MD_CTX *ctx, const EVP_MD *type);
int EVP_SignUpdate(EVP_MD_CTX *ctx, const void *d, unsigned int cnt);
int EVP_SignFinal(EVP_MD_CTX *ctx, unsigned char *sigret,
                   unsigned int *siglen, EVP_PKEY *pkey);

int EVP_VerifyInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl);
int EVP_VerifyInit(EVP_MD_CTX *ctx, const EVP_MD *type);
int EVP_VerifyUpdate(EVP_MD_CTX *ctx, const void *d, unsigned int cnt);
int EVP_VerifyFinal(EVP_MD_CTX *ctx, const unsigned char *sigbuf,
                     unsigned int siglen, EVP_PKEY *pkey);

/* Seal/Open functions */
int EVP_SealInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                  unsigned char **ek, int *ekl, unsigned char *iv,
                  EVP_PKEY **pubk, int npubk);
int EVP_SealUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                    int *outl, const unsigned char *in, int inl);
int EVP_SealFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);

int EVP_OpenInit(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,
                  const unsigned char *ek, int ekl, const unsigned char *iv,
                  EVP_PKEY *priv);
int EVP_OpenUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out,
                    int *outl, const unsigned char *in, int inl);
int EVP_OpenFinal(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);

/* Utility functions */
void EVP_cleanup(void);
void OpenSSL_add_all_ciphers(void);
void OpenSSL_add_all_digests(void);
void OpenSSL_add_all_algorithms(void);

/* PBE functions */
int EVP_PBE_CipherInit(const char *pass, int passlen,
                        unsigned char *salt, int saltlen, int iter,
                        const EVP_CIPHER *cipher, const EVP_MD *md,
                        int enc, EVP_CIPHER_CTX *ctx);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_EVP_H */
