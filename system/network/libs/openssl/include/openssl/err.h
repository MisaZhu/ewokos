#ifndef OPENSSL_ERR_H
#define OPENSSL_ERR_H

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error library codes */
#define ERR_LIB_NONE        1
#define ERR_LIB_SYS         2
#define ERR_LIB_BN          3
#define ERR_LIB_RSA         4
#define ERR_LIB_DH          5
#define ERR_LIB_EVP         6
#define ERR_LIB_BUF         7
#define ERR_LIB_OBJ         8
#define ERR_LIB_PEM         9
#define ERR_LIB_DSA         10
#define ERR_LIB_X509        11
#define ERR_LIB_ASN1        12
#define ERR_LIB_CONF        13
#define ERR_LIB_CRYPTO      14
#define ERR_LIB_EC          15
#define ERR_LIB_SSL         20
#define ERR_LIB_BIO         32
#define ERR_LIB_PKCS7       33
#define ERR_LIB_PKCS12      35
#define ERR_LIB_RAND        36
#define ERR_LIB_ENGINE      38
#define ERR_LIB_OCSP        39
#define ERR_LIB_UI          40
#define ERR_LIB_COMP        41
#define ERR_LIB_ECDSA       42
#define ERR_LIB_ECDH        43
#define ERR_LIB_OSSL_STORE  44
#define ERR_LIB_FIPS        45
#define ERR_LIB_CMS         46
#define ERR_LIB_TS          47
#define ERR_LIB_HMAC        48
#define ERR_LIB_CT          50
#define ERR_LIB_ASYNC       51
#define ERR_LIB_KDF         52
#define ERR_LIB_SM2         53
#define ERR_LIB_USER        128

/* Error function codes - simplified */
#define ERR_F_MALLOC        1
#define ERR_F_FREE          2
#define ERR_F_READ          3
#define ERR_F_WRITE         4
#define ERR_F_SOCKET        5
#define ERR_F_CONNECT       6
#define ERR_F_ACCEPT        7
#define ERR_F_BIND          8
#define ERR_F_LISTEN        9

/* Error reason codes - simplified */
#define ERR_R_SYS_LIB       1
#define ERR_R_BN_LIB        2
#define ERR_R_RSA_LIB       3
#define ERR_R_DH_LIB        4
#define ERR_R_EVP_LIB       5
#define ERR_R_BUF_LIB       6
#define ERR_R_OBJ_LIB       7
#define ERR_R_PEM_LIB       8
#define ERR_R_X509_LIB      9
#define ERR_R_ASN1_LIB      10
#define ERR_R_CONF_LIB      11
#define ERR_R_CRYPTO_LIB    12
#define ERR_R_EC_LIB        13
#define ERR_R_SSL_LIB       14
#define ERR_R_BIO_LIB       15
#define ERR_R_PKCS7_LIB     16
#define ERR_R_PKCS12_LIB    17
#define ERR_R_RAND_LIB      18
#define ERR_R_DSA_LIB       19
#define ERR_R_ENGINE_LIB    20
#define ERR_R_OCSP_LIB      21
#define ERR_R_UI_LIB        22
#define ERR_R_COMP_LIB      23
#define ERR_R_ECDSA_LIB     24
#define ERR_R_ECDH_LIB      25
#define ERR_R_OSSL_STORE_LIB 26
#define ERR_R_FIPS_LIB      27
#define ERR_R_CMS_LIB       28
#define ERR_R_TS_LIB        29
#define ERR_R_HMAC_LIB      30
#define ERR_R_CT_LIB        31
#define ERR_R_ASYNC_LIB     32
#define ERR_R_KDF_LIB       33
#define ERR_R_SM2_LIB       34

#define ERR_R_NESTED_ASN1_ERROR         58
#define ERR_R_MISSING_ASN1_EOS          63

/* Function prototypes */
void ERR_put_error(int lib, int func, int reason, const char *file, int line);
void ERR_clear_error(void);
unsigned long ERR_get_error(void);
unsigned long ERR_get_error_line(const char **file, int *line);
unsigned long ERR_get_error_line_data(const char **file, int *line,
                                       const char **data, int *flags);
unsigned long ERR_peek_error(void);
unsigned long ERR_peek_error_line(const char **file, int *line);
unsigned long ERR_peek_error_line_data(const char **file, int *line,
                                        const char **data, int *flags);
unsigned long ERR_peek_last_error(void);
unsigned long ERR_peek_last_error_line(const char **file, int *line);
unsigned long ERR_peek_last_error_line_data(const char **file, int *line,
                                             const char **data, int *flags);
void ERR_add_error_data(int num, ...);
void ERR_add_error_vdata(int num, va_list args);

const char *ERR_error_string(unsigned long e, char *buf);
const char *ERR_lib_error_string(unsigned long e);
const char *ERR_func_error_string(unsigned long e);
const char *ERR_reason_error_string(unsigned long e);

void ERR_print_errors_cb(int (*cb)(const char *str, size_t len, void *u),
                          void *u);
void ERR_print_errors_fp(FILE *fp);

/* Error string loading */
void ERR_load_crypto_strings(void);
void ERR_free_strings(void);
void ERR_load_ERR_strings(void);
void ERR_load_BN_strings(void);
void ERR_load_RSA_strings(void);
void ERR_load_DH_strings(void);
void ERR_load_EVP_strings(void);
void ERR_load_BUF_strings(void);
void ERR_load_OBJ_strings(void);
void ERR_load_PEM_strings(void);
void ERR_load_DSA_strings(void);
void ERR_load_X509_strings(void);
void ERR_load_ASN1_strings(void);
void ERR_load_CONF_strings(void);
void ERR_load_CRYPTO_strings(void);
void ERR_load_EC_strings(void);
void ERR_load_SSL_strings(void);
void ERR_load_BIO_strings(void);
void ERR_load_PKCS7_strings(void);
void ERR_load_PKCS12_strings(void);
void ERR_load_RAND_strings(void);
void ERR_load_ENGINE_strings(void);
void ERR_load_OCSP_strings(void);
void ERR_load_UI_strings(void);
void ERR_load_COMP_strings(void);
void ERR_load_ECDSA_strings(void);
void ERR_load_ECDH_strings(void);
void ERR_load_OSSL_STORE_strings(void);
void ERR_load_FIPS_strings(void);
void ERR_load_CMS_strings(void);
void ERR_load_TS_strings(void);
void ERR_load_HMAC_strings(void);
void ERR_load_CT_strings(void);
void ERR_load_ASYNC_strings(void);
void ERR_load_KDF_strings(void);
void ERR_load_SM2_strings(void);

/* Error code building macros */
#define ERR_PACK(l,f,r)         (((((unsigned long)l)&0xFFL)*0x1000000)| \
                                 ((((unsigned long)f)&0xFFF)*0x1000)| \
                                 ((((unsigned long)r)&0xFFFL)))
#define ERR_GET_LIB(l)          (int)((((unsigned long)l)>>24L)&0xFFL)
#define ERR_GET_FUNC(l)         (int)((((unsigned long)l)>>12L)&0FFFL)
#define ERR_GET_REASON(l)       (int)((l)&0FFFL)
#define ERR_FATAL_ERROR(l)      (int)((((unsigned long)l)>>31L)&0x1L)

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_ERR_H */
