/* user_settings.h
 * wolfSSL user settings for EwokOS
 */

#ifndef WOLFSSL_USER_SETTINGS_H
#define WOLFSSL_USER_SETTINGS_H

/* EwokOS platform */
#define WOLFSSL_EWOKOS

/* Platform settings */
#define WOLFSSL_GENERAL_ALIGNMENT 4
#define SIZEOF_LONG_LONG 8

/* Math library selection - use TFM (Tiny Fast Math) instead of SP math */
#define USE_FAST_MATH
#define FP_MAX_BITS 4096
#define TFM_TIMING_RESISTANT

/* Disable single precision math (sp_int) - has issues with 64-bit */
/* #define WOLFSSL_SP_MATH_ALL */
/* #define WOLFSSL_SP_SMALL */

/* Disable ARM assembly optimizations - use pure C implementation
 * TFM_ARM assembly is for 32-bit ARM only, not compatible with AArch64
 */
/* #define TFM_ARM */

/* TLS/SSL features */
#define OPENSSL_EXTRA
#define OPENSSL_ALL
#define HAVE_OPENSSL

/* Enable all OpenSSL compatibility functions */
#define HAVE_ECC
#define HAVE_CURVE25519
#define WOLFSSL_HAVE_MLKEM
#define WOLFSSL_WC_MLKEM
/* #define HAVE_ED25519 */ /* Requires WOLFSSL_SHA512 */
#define HAVE_RSA
#define HAVE_DH

/* ECC settings - required for EC_KEY_* functions */
#define ECC_SHAMIR
#define ECC_TIMING_RESISTANT
#define HAVE_COMP_KEY

/* RSA settings */
#define WOLFSSL_KEY_GEN
#define WOLFSSL_KEY_TO_DER
#define WC_RSA_BLINDING
#define WC_RSA_PSS

/* Random number generator */
#define HAVE_HASHDRBG
int ewokos_generate_seed(unsigned char* output, unsigned int sz);
#define CUSTOM_RAND_GENERATE_SEED ewokos_generate_seed

/* Certificate handling */
#define WOLFSSL_CERT_GEN
#define WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_EXT
#define HAVE_PKCS7
#define HAVE_PKCS12

/* ASN.1 handling */
#define WOLFSSL_ASN_TEMPLATE

/* Encoding */
#define WOLFSSL_BASE64_ENCODE

/* Error strings */
#define WOLFSSL_ERROR_CODES

/* I/O */
#define WOLFSSL_USER_IO

/* Time */
#define WOLFSSL_USER_CURRTIME

/* File system - disable for embedded */
#define NO_FILESYSTEM

/* Operating system */
#define SINGLE_THREADED

/* Disable features not needed for embedded */
#define NO_WRITEV
#define NO_DEV_RANDOM
#define NO_WOLFSSL_DIR

/* Define AF_INET6 if not available */
#ifndef AF_INET6
    #define AF_INET6 10
#endif

/* Enable debugging */
#ifdef DEBUG
    #define DEBUG_WOLFSSL
#endif

/* Additional features */
#define HAVE_HKDF
#define HAVE_PKCS8
#define HAVE_X963_KDF
#define WOLFSSL_SHA3
#define WOLFSSL_SHAKE128
#define WOLFSSL_SHAKE256

/* SNI (Server Name Indication) */
#define HAVE_TLS_EXTENSIONS
#define HAVE_SNI

/* ALPN */
#define HAVE_ALPN

/* Session tickets - requires ChaCha20-Poly1305 or AES-GCM for encryption */
#define HAVE_SESSION_TICKET
#define HAVE_CHACHA
#define HAVE_POLY1305
#define HAVE_CHACHA_POLY_AUTHS

/* Certificate status request - disable because it requires OCSP */
/* #define HAVE_CERTIFICATE_STATUS_REQUEST */

/* OCSP - disable to avoid wolfIO_HttpProcessResponseOcspGenericIO undefined reference */
/* #define HAVE_OCSP */

/* CRL */
#define HAVE_CRL

/* Disable deprecated algorithms */
#define NO_MD4
#define NO_RC4
#define NO_DES3

/* wolfSSL version compatibility */
#define WOLFSSL_ALLOW_SSLV3
#define WOLFSSL_ALLOW_TLSV10
#define WOLFSSL_ALLOW_TLSV11

#endif /* WOLFSSL_USER_SETTINGS_H */
