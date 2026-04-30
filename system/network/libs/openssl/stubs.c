/*
 * Complete stubs for OpenSSL functions not available in embedded builds
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* DSO stubs */
typedef void DSO;
typedef void DSO_METHOD;

DSO *DSO_new(void) { return NULL; }
int DSO_free(DSO *dso) { (void)dso; return 0; }
int DSO_load(DSO *dso, const char *filename, DSO_METHOD *meth, int flags) {
    (void)dso; (void)filename; (void)meth; (void)flags;
    return 0;
}
void *DSO_bind_func(DSO *dso, const char *symname) {
    (void)dso; (void)symname;
    return NULL;
}
int DSO_ctrl(DSO *dso, int cmd, long larg, void *parg) {
    (void)dso; (void)cmd; (void)larg; (void)parg;
    return 0;
}
char *DSO_convert_filename(DSO *dso, const char *filename) {
    (void)dso; (void)filename;
    return NULL;
}
char *DSO_merge(DSO *dso, const char *filespec1, const char *filespec2) {
    (void)dso; (void)filespec1; (void)filespec2;
    return NULL;
}
char *DSO_get_filename(DSO *dso) {
    (void)dso;
    return NULL;
}
DSO *DSO_dsobyaddr(void *addr, int flags) {
    (void)addr; (void)flags;
    return NULL;
}

/* Store stubs */
typedef void OSSL_STORE_LOADER;

int ossl_store_loader_store_cache_flush(void *ctx) {
    (void)ctx;
    return 0;
}
int ossl_store_loader_store_remove_all_provided(void *ctx) {
    (void)ctx;
    return 0;
}

/* Rand pool stubs */
int ossl_rand_pool_init(void) { return 1; }
void ossl_rand_pool_cleanup(void) { }
int ossl_rand_pool_keep_random_devices_open(void) { return 0; }
int ossl_pool_acquire_entropy(void *pool) { (void)pool; return 0; }
int ossl_pool_add_nonce_data(void *pool) { (void)pool; return 0; }

/* Conf stubs */
typedef void CONF;
typedef void CONF_METHOD;

CONF_METHOD *NCONF_default(void) { return NULL; }

/* Error stubs */
int ossl_err_load_DSO_strings(void) { return 1; }
int ossl_err_load_OSSL_STORE_strings(void) { return 1; }
int ossl_err_load_PROV_strings(void) { return 1; }

/* Blake2 stubs */
int ossl_blake2s256_init(void *ctx) { (void)ctx; return 0; }
int ossl_blake2s_update(void *ctx, const void *in, size_t inlen) {
    (void)ctx; (void)in; (void)inlen;
    return 0;
}
int ossl_blake2s_final(void *ctx, void *out) {
    (void)ctx; (void)out;
    return 0;
}
int ossl_blake2b512_init(void *ctx) { (void)ctx; return 0; }
int ossl_blake2b_update(void *ctx, const void *in, size_t inlen) {
    (void)ctx; (void)in; (void)inlen;
    return 0;
}
int ossl_blake2b_final(void *ctx, void *out) {
    (void)ctx; (void)out;
    return 0;
}

/* UI stubs */
typedef void UI;
typedef void UI_METHOD;

UI_METHOD *UI_get_default_method(void) { return NULL; }

/* Error state stub */
void ossl_set_error_state(const char *type) { (void)type; }

/* Provider init stubs */
int ossl_default_provider_init(void *prov, void *handle, void *cb) {
    (void)prov; (void)handle; (void)cb;
    return 0;
}
int ossl_base_provider_init(void *prov, void *handle, void *cb) {
    (void)prov; (void)handle; (void)cb;
    return 0;
}
int ossl_null_provider_init(void *prov, void *handle, void *cb) {
    (void)prov; (void)handle; (void)cb;
    return 0;
}

/* UID stubs */
int geteuid(void) { return 0; }
int getegid(void) { return 0; }

/* RAND stubs - simple pseudo-random for embedded */
static unsigned int rand_seed = 1;

int RAND_bytes(unsigned char *buf, int num) {
    for (int i = 0; i < num; i++) {
        rand_seed = rand_seed * 1103515245 + 12345;
        buf[i] = (unsigned char)(rand_seed >> 16);
    }
    return 1;
}

int RAND_priv_bytes(unsigned char *buf, int num) {
    return RAND_bytes(buf, num);
}

void RAND_seed(const void *buf, int num) {
    (void)buf;
    (void)num;
}

int RAND_status(void) {
    return 1;
}

/* Extended RAND stubs */
int RAND_bytes_ex(void *ctx, unsigned char *buf, int num, int strength) {
    (void)ctx; (void)strength;
    return RAND_bytes(buf, num);
}

int RAND_priv_bytes_ex(void *ctx, unsigned char *buf, int num, int strength) {
    (void)ctx; (void)strength;
    return RAND_priv_bytes(buf, num);
}

void *RAND_get0_private(void *ctx) {
    (void)ctx;
    return NULL;
}

void ossl_rand_cleanup_int(void) { }

/* Provider stubs */
void *ossl_provider_ctx(void *prov) {
    (void)prov;
    return NULL;
}

int ossl_provider_up_ref(void *prov) {
    (void)prov;
    return 1;
}

void ossl_provider_free(void *prov) {
    (void)prov;
}

void *ossl_provider_libctx(void *prov) {
    (void)prov;
    return NULL;
}

int ossl_provider_init_as_child(void *ctx, void *handle, void *in) {
    (void)ctx; (void)handle; (void)in;
    return 0;
}

void ossl_provider_deinit_child(void *ctx) {
    (void)ctx;
}

void ossl_rand_ctx_free(void *ctx) {
    (void)ctx;
}

int ossl_provider_query_operation(void *prov, int operation_id, void **out) {
    (void)prov; (void)operation_id; (void)out;
    return 0;
}

void ossl_provider_unquery_operation(void *prov, int operation_id, void *out) {
    (void)prov; (void)operation_id; (void)out;
}

int ossl_provider_doall_activated(void *libctx, void (*fn)(void *, void *), void *arg) {
    (void)libctx; (void)fn; (void)arg;
    return 1;
}

int ossl_provider_test_operation_bit(void *prov, int bitnum) {
    (void)prov; (void)bitnum;
    return 0;
}

void ossl_provider_set_operation_bit(void *prov, int bitnum) {
    (void)prov; (void)bitnum;
}

void ossl_provider_default_props_update(void *libctx, void *props) {
    (void)libctx; (void)props;
}

/* Config stubs */
void ossl_config_modules_free(void) { }
int CONF_modules_load_file_ex(void *libctx, const char *filename, const char *appname, unsigned long flags) {
    (void)libctx; (void)filename; (void)appname; (void)flags;
    return 1;
}

int CONF_parse_list(const char *list, char sep, int nospc, int (*list_cb)(const char *elem, int len, void *usr), void *arg) {
    (void)list; (void)sep; (void)nospc; (void)list_cb; (void)arg;
    return 1;
}

/* Encoder stubs */
void *OSSL_PROVIDER_get0_provider_ctx(void *prov) {
    (void)prov;
    return NULL;
}

/* ChaCha20 stubs - PowerPC specific */
unsigned int OPENSSL_ppccap_P = 0;

void ChaCha20_ctr32_vsx(unsigned char *out, const unsigned char *inp, size_t len, const unsigned int key[8], const unsigned int ctr[4]) {
    (void)out; (void)inp; (void)len; (void)key; (void)ctr;
}

void ChaCha20_ctr32_vmx(unsigned char *out, const unsigned char *inp, size_t len, const unsigned int key[8], const unsigned int ctr[4]) {
    (void)out; (void)inp; (void)len; (void)key; (void)ctr;
}

void ChaCha20_ctr32_int(unsigned char *out, const unsigned char *inp, size_t len, const unsigned int key[8], const unsigned int ctr[4]) {
    (void)out; (void)inp; (void)len; (void)key; (void)ctr;
}

/* OpenSSL Initialization stubs */
int OPENSSL_init_crypto(uint64_t opts, const void *settings) {
    (void)opts; (void)settings;
    return 1;
}

int OPENSSL_init_ssl(uint64_t opts, const void *settings) {
    (void)opts; (void)settings;
    return 1;
}
