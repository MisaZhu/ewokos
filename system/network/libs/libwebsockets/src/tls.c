/**
 * TLS support via wolfSSL for libwebsockets (EwokOS)
 */

#include "private.h"

/* ---------- custom I/O callbacks for wolfSSL (WOLFSSL_USER_IO) ---------- */

static int ewokos_ssl_recv_cb(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int fd = *(int *)ctx;
    int ret;

    (void)ssl;

    if (sz <= 0)
        return WOLFSSL_CBIO_ERR_WANT_READ;

    ret = (int)recv(fd, buf, (uint32_t)sz, 0);

    if (ret == 0)
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    if (ret < 0)
        return WOLFSSL_CBIO_ERR_GENERAL;
    return ret;
}

static int ewokos_ssl_send_cb(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    int fd = *(int *)ctx;
    int ret;

    (void)ssl;

    if (sz <= 0)
        return 0;

    ret = (int)send(fd, buf, (uint32_t)sz, 0);

    if (ret < 0)
        return WOLFSSL_CBIO_ERR_GENERAL;
    if (ret == 0)
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    return ret;
}

/* ---------- TLS client connect ---------- */

int lws_tls_client_connect(struct lws *wsi)
{
    WOLFSSL_CTX *ctx;
    WOLFSSL *ssl;
    int ret;

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        lwsl_err("tls: wolfSSL_Init failed\n");
        return -1;
    }

    /* use v23 method: negotiates highest mutually-supported TLS version */
    ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (!ctx) {
        lwsl_err("tls: wolfSSL_CTX_new failed\n");
        return -1;
    }

    /* set custom I/O at CTX level */
    wolfSSL_CTX_SetIORecv(ctx, ewokos_ssl_recv_cb);
    wolfSSL_CTX_SetIOSend(ctx, ewokos_ssl_send_cb);

    /* skip cert verification for embedded (no CA store) */
    wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);

    ssl = wolfSSL_new(ctx);
    if (!ssl) {
        lwsl_err("tls: wolfSSL_new failed\n");
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    /* explicitly set I/O callbacks and context at SSL level
     * (OPENSSL_EXTRA + BIO can interfere with CTX inheritance) */
    wolfSSL_SSLSetIORecv(ssl, ewokos_ssl_recv_cb);
    wolfSSL_SSLSetIOSend(ssl, ewokos_ssl_send_cb);
    wolfSSL_SetIOReadCtx(ssl, &wsi->fd);
    wolfSSL_SetIOWriteCtx(ssl, &wsi->fd);

    /* SNI */
    if (wsi->client_host)
        wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME,
            wsi->client_host, (unsigned short)strlen(wsi->client_host));

    /* perform TLS handshake */
    ret = wolfSSL_connect(ssl);
    if (ret != WOLFSSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, ret);
        lwsl_err("tls: wolfSSL_connect failed (ret=%d err=%d)\n", ret, err);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    wsi->ssl = ssl;
    wsi->ssl_ctx = ctx;

    lwsl_info("tls: handshake complete\n");
    return 0;
}

/* ---------- TLS close ---------- */

void lws_tls_close(struct lws *wsi)
{
    if (wsi->ssl) {
        wolfSSL_shutdown(wsi->ssl);
        wolfSSL_free(wsi->ssl);
        wsi->ssl = NULL;
    }
    if (wsi->ssl_ctx) {
        wolfSSL_CTX_free(wsi->ssl_ctx);
        wsi->ssl_ctx = NULL;
    }
}

/* ---------- unified read/write (TLS or plain) ---------- */

int lws_ssl_read(struct lws *wsi, unsigned char *buf, int len)
{
    if (wsi->ssl) {
        int ret = wolfSSL_read(wsi->ssl, buf, len);
        if (ret <= 0) {
            int err = wolfSSL_get_error(wsi->ssl, ret);
            if (err == WOLFSSL_ERROR_WANT_READ ||
                err == WOLFSSL_ERROR_WANT_WRITE)
                return 0; /* would block, try later */
            return -1;
        }
        return ret;
    }
    return (int)recv(wsi->fd, buf, (uint32_t)len, 0);
}

int lws_ssl_write(struct lws *wsi, const unsigned char *buf, int len)
{
    if (wsi->ssl) {
        int ret = wolfSSL_write(wsi->ssl, buf, len);
        if (ret <= 0) {
            int err = wolfSSL_get_error(wsi->ssl, ret);
            if (err == WOLFSSL_ERROR_WANT_READ ||
                err == WOLFSSL_ERROR_WANT_WRITE)
                return 0; /* would block */
            return -1;
        }
        return ret;
    }
    return (int)send(wsi->fd, buf, (uint32_t)len, 0);
}
