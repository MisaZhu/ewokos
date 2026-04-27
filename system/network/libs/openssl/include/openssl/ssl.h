#ifndef OPENSSL_SSL_H
#define OPENSSL_SSL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SSL version constants */
#define OPENSSL_VERSION_NUMBER 0x10100000L
#define OPENSSL_VERSION_TEXT   "OpenSSL 1.1.0 for EwokOS"

/* SSL modes */
#define SSL_MODE_ENABLE_PARTIAL_WRITE       0x00000001L
#define SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER 0x00000002L
#define SSL_MODE_AUTO_RETRY                 0x00000004L
#define SSL_MODE_NO_AUTO_CHAIN              0x00000008L
#define SSL_MODE_RELEASE_BUFFERS            0x00000010L
#define SSL_MODE_SEND_CLIENTHELLO_TIME      0x00000020L
#define SSL_MODE_SEND_SERVERHELLO_TIME      0x00000040L

/* SSL options */
#define SSL_OP_NO_SSLv2                     0x01000000L
#define SSL_OP_NO_SSLv3                     0x02000000L
#define SSL_OP_NO_TLSv1                     0x04000000L
#define SSL_OP_NO_TLSv1_2                   0x08000000L
#define SSL_OP_NO_TLSv1_1                   0x10000000L
#define SSL_OP_NO_DTLSv1                    0x04000000L
#define SSL_OP_NO_DTLSv1_2                  0x08000000L

/* SSL verify modes */
#define SSL_VERIFY_NONE                     0x00
#define SSL_VERIFY_PEER                     0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT     0x02
#define SSL_VERIFY_CLIENT_ONCE              0x04

/* SSL error codes */
#define SSL_ERROR_NONE                      0
#define SSL_ERROR_SSL                       1
#define SSL_ERROR_WANT_READ                 2
#define SSL_ERROR_WANT_WRITE                3
#define SSL_ERROR_WANT_X509_LOOKUP          4
#define SSL_ERROR_SYSCALL                   5
#define SSL_ERROR_ZERO_RETURN               6
#define SSL_ERROR_WANT_CONNECT              7
#define SSL_ERROR_WANT_ACCEPT               8

/* SSL states */
#define SSL_ST_CONNECT                      0x1000
#define SSL_ST_ACCEPT                       0x2000
#define SSL_ST_MASK                         0x0FFF
#define SSL_ST_INIT                         (SSL_ST_CONNECT|SSL_ST_ACCEPT)
#define SSL_ST_BEFORE                       0x4000
#define SSL_ST_OK                           0x03
#define SSL_ST_RENEGOTIATE                  (0x04|SSL_ST_INIT)

/* File types for certificates */
#define SSL_FILETYPE_PEM                    1
#define SSL_FILETYPE_ASN1                   2

/* Forward declarations */
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct x509_st X509;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct bio_st BIO;

/* SSL_METHOD functions */
const SSL_METHOD *TLS_method(void);
const SSL_METHOD *TLS_client_method(void);
const SSL_METHOD *TLS_server_method(void);
const SSL_METHOD *SSLv23_method(void);
const SSL_METHOD *SSLv23_client_method(void);
const SSL_METHOD *SSLv23_server_method(void);

/* SSL_CTX functions */
SSL_CTX *SSL_CTX_new(const SSL_METHOD *meth);
void SSL_CTX_free(SSL_CTX *ctx);
int SSL_CTX_set_cipher_list(SSL_CTX *ctx, const char *str);
long SSL_CTX_set_options(SSL_CTX *ctx, long options);
long SSL_CTX_get_options(SSL_CTX *ctx);
int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile, const char *CApath);
int SSL_CTX_use_certificate_file(SSL_CTX *ctx, const char *file, int type);
int SSL_CTX_use_PrivateKey_file(SSL_CTX *ctx, const char *file, int type);
int SSL_CTX_check_private_key(const SSL_CTX *ctx);
void SSL_CTX_set_verify(SSL_CTX *ctx, int mode, int (*verify_callback)(int, X509 *));
void SSL_CTX_set_default_passwd_cb(SSL_CTX *ctx, int (*cb)(char *, int, int, void *));
long SSL_CTX_set_mode(SSL_CTX *ctx, long mode);
long SSL_CTX_get_mode(SSL_CTX *ctx);
int SSL_CTX_set_session_cache_mode(SSL_CTX *ctx, int mode);

/* SSL functions */
SSL *SSL_new(SSL_CTX *ctx);
void SSL_free(SSL *ssl);
int SSL_set_fd(SSL *ssl, int fd);
int SSL_get_fd(const SSL *ssl);
int SSL_connect(SSL *ssl);
int SSL_accept(SSL *ssl);
int SSL_shutdown(SSL *ssl);
int SSL_read(SSL *ssl, void *buf, int num);
int SSL_write(SSL *ssl, const void *buf, int num);
int SSL_get_error(const SSL *ssl, int ret);
int SSL_want(const SSL *ssl);
int SSL_pending(const SSL *ssl);
const char *SSL_get_version(const SSL *ssl);
const char *SSL_get_cipher(const SSL *ssl);
X509 *SSL_get_peer_certificate(const SSL *ssl);
long SSL_get_verify_result(const SSL *ssl);
int SSL_do_handshake(SSL *ssl);
int SSL_renegotiate(SSL *ssl);
int SSL_state(const SSL *ssl);
void SSL_set_connect_state(SSL *ssl);
void SSL_set_accept_state(SSL *ssl);
int SSL_is_init_finished(const SSL *ssl);

/* Utility functions */
int SSL_library_init(void);
void SSL_load_error_strings(void);
const char *ERR_error_string(unsigned long e, char *buf);
unsigned long ERR_get_error(void);
void ERR_clear_error(void);
void ERR_free_strings(void);

/* BIO functions */
BIO *BIO_new_socket(int sock, int close_flag);
void BIO_free(BIO *bio);
int BIO_read(BIO *bio, void *buf, int len);
int BIO_write(BIO *bio, const void *buf, int len);
int BIO_puts(BIO *bio, const char *buf);
int BIO_gets(BIO *bio, char *buf, int size);

/* X509 functions */
void X509_free(X509 *x);
int X509_verify_cert_error_string(long n);

/* Simplified crypto functions */
int RAND_bytes(unsigned char *buf, int num);
void RAND_seed(const void *buf, int num);
int RAND_status(void);

/* Hash functions */
unsigned char *MD5(const unsigned char *d, size_t n, unsigned char *md);
unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);
unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md);

/* Base64 functions */
int EVP_EncodeBlock(unsigned char *t, const unsigned char *f, int n);
int EVP_DecodeBlock(unsigned char *t, const unsigned char *f, int n);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_SSL_H */
