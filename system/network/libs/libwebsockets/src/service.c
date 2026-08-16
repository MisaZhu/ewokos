/**
 * libwebsockets service loop - poll-based event processing
 */

#include "private.h"
#include <stdio.h>

/* forward declarations */
static void lws_service_wsi(struct lws *wsi);
static int lws_accept_new_connection(struct lws *ctx_wsi);
static void lws_do_client_connect(struct lws *wsi);

int lws_service(struct lws_context *context, int timeout_ms)
{
    if (!context || context->being_destroyed)
        return -1;

    return lws_service_one(context, timeout_ms);
}

int lws_service_one(struct lws_context *ctx, int timeout_ms)
{
    struct pollfd fds[LWS_MAX_SOCKETS + 1];
    struct lws *wsi_map[LWS_MAX_SOCKETS + 1];
    int nfds = 0;
    struct lws *wsi;
    int n, i;

    /* build poll set */
    for (wsi = ctx->wsi_list; wsi && nfds < LWS_MAX_SOCKETS; wsi = wsi->next) {
        if (wsi->fd < 0)
            continue;
        if (wsi->state == LWS_CONN_STATE_DEAD)
            continue;

        fds[nfds].fd = wsi->fd;
        fds[nfds].events = POLLIN;
        if (wsi->callback_on_writable || wsi->tx_len > wsi->tx_offset)
            fds[nfds].events |= POLLOUT;
        fds[nfds].revents = 0;
        wsi_map[nfds] = wsi;
        nfds++;
    }

    if (nfds == 0) {
        /* nothing to poll, just sleep */
        if (timeout_ms > 0)
            usleep((uint32_t)timeout_ms * 1000);
        return 0;
    }

    n = poll(fds, (nfds_t)nfds, timeout_ms);
    if (n < 0)
        return -1;

    if (n == 0)
        return 0; /* timeout */

    /* process events */
    for (i = 0; i < nfds; i++) {
        if (fds[i].revents == 0)
            continue;

        wsi = wsi_map[i];

        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            if (wsi->state == LWS_CONN_STATE_LISTENING)
                continue; /* listener shouldn't get these normally */

            /* connection error/hangup */
            if (wsi->protocol && wsi->protocol->callback &&
                wsi->state == LWS_CONN_STATE_ESTABLISHED) {
                wsi->protocol->callback(wsi, LWS_CALLBACK_CLOSED,
                        wsi->user_space, NULL, 0);
            }
            wsi->state = LWS_CONN_STATE_DEAD;
            continue;
        }

        if (fds[i].revents & POLLIN)
            lws_service_wsi(wsi);

        if (fds[i].revents & POLLOUT) {
            /* flush pending tx data */
            if (wsi->tx_len > wsi->tx_offset) {
                int sent = lws_ssl_write(wsi,
                        wsi->tx_buf + wsi->tx_offset,
                        (int)(wsi->tx_len - wsi->tx_offset));
                if (sent > 0)
                    wsi->tx_offset += (uint32_t)sent;
                if (wsi->tx_offset >= wsi->tx_len) {
                    wsi->tx_len = 0;
                    wsi->tx_offset = 0;
                }
            }

            /* writable callback */
            if (wsi->callback_on_writable &&
                wsi->state == LWS_CONN_STATE_ESTABLISHED) {
                wsi->callback_on_writable = 0;
                if (wsi->protocol && wsi->protocol->callback) {
                    enum lws_callback_reasons r =
                        wsi->is_server_side ?
                        LWS_CALLBACK_SERVER_WRITEABLE :
                        LWS_CALLBACK_CLIENT_WRITEABLE;
                    wsi->protocol->callback(wsi, r,
                            wsi->user_space, NULL, 0);
                }
            }
        }
    }

    /* cleanup dead connections */
    wsi = ctx->wsi_list;
    while (wsi) {
        struct lws *next = wsi->next;
        if (wsi->state == LWS_CONN_STATE_DEAD)
            lws_wsi_destroy(wsi);
        wsi = next;
    }

    return n;
}

int lws_service_fd(struct lws_context *context, void *pollfd)
{
    (void)context;
    (void)pollfd;
    /* simplified: not implemented for single-fd mode */
    return 0;
}

/* ---------- internal: handle readable wsi ---------- */

static void lws_service_wsi(struct lws *wsi)
{
    switch (wsi->state) {
    case LWS_CONN_STATE_LISTENING:
        lws_accept_new_connection(wsi);
        break;

    case LWS_CONN_STATE_CONNECTING:
        lws_do_client_connect(wsi);
        break;

    case LWS_CONN_STATE_HANDSHAKE_SENT:
        lws_client_handshake_recv(wsi);
        break;

    case LWS_CONN_STATE_HANDSHAKE_RECV:
    case LWS_CONN_STATE_ESTABLISHED:
    {
        unsigned char buf[LWS_DEFAULT_RX_BUF];
        int n = lws_ssl_read(wsi, buf, sizeof(buf));

        if (n <= 0) {
            /* connection closed */
            if (wsi->protocol && wsi->protocol->callback &&
                wsi->state == LWS_CONN_STATE_ESTABLISHED) {
                wsi->protocol->callback(wsi, LWS_CALLBACK_CLOSED,
                        wsi->user_space, NULL, 0);
            }
            wsi->state = LWS_CONN_STATE_DEAD;
            return;
        }

        if (wsi->state == LWS_CONN_STATE_HANDSHAKE_RECV) {
            /* accumulate HTTP request headers */
            lws_parse_http_headers(wsi, (const char *)buf, n);
            if (wsi->hdr_complete)
                lws_server_handshake(wsi);
        } else {
            /* established: parse WebSocket frames */
            lws_ws_frame_parse(wsi, buf, n);
        }
        break;
    }

    default:
        break;
    }
}

/* ---------- accept new connection ---------- */

static int lws_accept_new_connection(struct lws *listen_wsi)
{
    struct sockaddr_in client_addr;
    uint32_t addr_len = sizeof(client_addr);
    int fd;
    struct lws *new_wsi;
    struct lws_context *ctx = listen_wsi->context;

    fd = accept(listen_wsi->fd, (struct sockaddr *)&client_addr, &addr_len);
    if (fd < 0)
        return -1;

    new_wsi = lws_wsi_create(ctx);
    if (!new_wsi) {
        close(fd);
        return -1;
    }

    new_wsi->fd = fd;
    new_wsi->state = LWS_CONN_STATE_HANDSHAKE_RECV;
    new_wsi->vhost = listen_wsi->vhost;
    new_wsi->is_server_side = 1;
    new_wsi->rx_state = LWS_RXPS_NEW;

    /* allocate header accumulation buffer */
    new_wsi->rx_hdr_buf = (char *)malloc(LWS_MAX_HANDSHAKE_LEN);
    if (!new_wsi->rx_hdr_buf) {
        lws_wsi_destroy(new_wsi);
        return -1;
    }
    new_wsi->rx_hdr_len = 0;

    /* allocate per-session data for first protocol */
    if (listen_wsi->vhost->protocols &&
        listen_wsi->vhost->protocols[0].per_session_data_size > 0) {
        new_wsi->user_space = calloc(1,
            listen_wsi->vhost->protocols[0].per_session_data_size);
    }
    new_wsi->protocol = &listen_wsi->vhost->protocols[0];

    lwsl_info("accepted new connection fd=%d\n", fd);
    return 0;
}

/* ---------- client connect completion ---------- */

static void lws_do_client_connect(struct lws *wsi)
{
    /* connection completed (non-blocking connect or just connected) */
    /* send the WebSocket handshake request */
    if (lws_client_handshake_send(wsi) == 0)
        wsi->state = LWS_CONN_STATE_HANDSHAKE_SENT;
    else
        wsi->state = LWS_CONN_STATE_DEAD;
}
