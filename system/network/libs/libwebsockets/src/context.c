/**
 * libwebsockets context lifecycle management
 */

#include "private.h"
#include <stdio.h>
#include <stdarg.h>

/* ---------- logging ---------- */

static int log_level = LLL_ERR | LLL_WARN | LLL_NOTICE;
static void (*user_log_emit)(int level, const char *line) = NULL;

void lws_set_log_level(int level, void (*log_emit)(int level, const char *line))
{
	log_level = level;
	user_log_emit = log_emit;
}

void _lws_log(int filter, const char *format, ...)
{
	char buf[512];
	va_list ap;

	if (!(filter & log_level))
		return;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	if (user_log_emit)
		user_log_emit(filter, buf);
	else
		fprintf(stderr, "[lws:%d] %s\n", filter, buf);
}

/* ---------- wsi helpers ---------- */

struct lws *lws_wsi_create(struct lws_context *ctx)
{
	struct lws *wsi = (struct lws *)calloc(1, sizeof(struct lws));
	if (!wsi)
		return NULL;

	wsi->fd = -1;
	wsi->context = ctx;
	wsi->state = LWS_CONN_STATE_NONE;
	wsi->rx_state = LWS_RXPS_NEW;

	/* add to context list */
	wsi->next = ctx->wsi_list;
	ctx->wsi_list = wsi;

	return wsi;
}

void lws_hdr_free_all(struct lws *wsi)
{
	int i;
	for (i = 0; i < WSI_TOKEN_COUNT; i++) {
		if (wsi->hdr[i].data) {
			free(wsi->hdr[i].data);
			wsi->hdr[i].data = NULL;
			wsi->hdr[i].len = 0;
		}
	}
	if (wsi->rx_hdr_buf) {
		free(wsi->rx_hdr_buf);
		wsi->rx_hdr_buf = NULL;
		wsi->rx_hdr_len = 0;
	}
}

void lws_wsi_destroy(struct lws *wsi)
{
	struct lws_context *ctx = wsi->context;
	struct lws **pp;

	/* remove from context list */
	for (pp = &ctx->wsi_list; *pp; pp = &(*pp)->next) {
		if (*pp == wsi) {
			*pp = wsi->next;
			break;
		}
	}

	if (wsi->fd >= 0) {
		lws_tls_close(wsi);
		close(wsi->fd);
	}

	lws_hdr_free_all(wsi);

	if (wsi->user_space) {
		free(wsi->user_space);
		wsi->user_space = NULL;
	}
	if (wsi->rx_payload_buf) {
		free(wsi->rx_payload_buf);
		wsi->rx_payload_buf = NULL;
	}
	if (wsi->tx_buf) {
		free(wsi->tx_buf);
		wsi->tx_buf = NULL;
	}
	if (wsi->client_address) free(wsi->client_address);
	if (wsi->client_path) free(wsi->client_path);
	if (wsi->client_host) free(wsi->client_host);
	if (wsi->client_origin) free(wsi->client_origin);
	if (wsi->client_protocol) free(wsi->client_protocol);

	free(wsi);
}

/* ---------- context creation ---------- */

struct lws_context *lws_create_context(
		const struct lws_context_creation_info *info)
{
	struct lws_context *ctx;
	struct lws_vhost *vh;

	ctx = (struct lws_context *)calloc(1, sizeof(struct lws_context));
	if (!ctx)
		return NULL;

	ctx->user = info->user;
	ctx->options = info->options;
	ctx->timeout_secs = info->timeout_secs ? info->timeout_secs : 30;

	/* create default vhost */
	vh = (struct lws_vhost *)calloc(1, sizeof(struct lws_vhost));
	if (!vh) {
		free(ctx);
		return NULL;
	}

	vh->context = ctx;
	vh->listen_port = info->port;
	vh->protocols = info->protocols;
	vh->name = strdup(info->vhost_name ? info->vhost_name : "default");
	ctx->vhost_list = vh;

	/* if server mode, create listening socket */
	if (info->port >= 0) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			lwsl_err("failed to create listen socket\n");
			free(vh->name);
			free(vh);
			free(ctx);
			return NULL;
		}

		int opt = 1;
		setsockopt(fd, SOL_SOCKET, 0x0004 /*SO_REUSEADDR*/, &opt, sizeof(opt));

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_un.s_addr = INADDR_ANY;
		addr.sin_port = htons((uint16_t)info->port);

		if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			lwsl_err("bind failed on port %d\n", info->port);
			close(fd);
			free(vh->name);
			free(vh);
			free(ctx);
			return NULL;
		}

		if (listen(fd, 16) < 0) {
			lwsl_err("listen failed\n");
			close(fd);
			free(vh->name);
			free(vh);
			free(ctx);
			return NULL;
		}

		ctx->listen_fd = fd;
		vh->listen_wsi = NULL;

		/* create a wsi to represent the listener */
		struct lws *lwsi = lws_wsi_create(ctx);
		if (lwsi) {
			lwsi->fd = fd;
			lwsi->state = LWS_CONN_STATE_LISTENING;
			lwsi->vhost = vh;
			lwsi->is_server_side = 1;
			vh->listen_wsi = lwsi;
		}

		lwsl_notice("listening on port %d\n", info->port);
	} else {
		ctx->listen_fd = -1;
	}

	/* call PROTOCOL_INIT for each protocol */
	if (info->protocols) {
		const struct lws_protocols *p = info->protocols;
		int idx = 0;
		while (p->callback && idx < LWS_MAX_PROTOCOLS) {
			if (p->callback(NULL, LWS_CALLBACK_PROTOCOL_INIT, NULL,
					(void *)p->name, 0) < 0) {
				lwsl_err("protocol init failed: %s\n", p->name);
			}
			p++;
			idx++;
		}
	}

	return ctx;
}

void lws_context_destroy(struct lws_context *context)
{
	struct lws *wsi, *next;
	struct lws_vhost *vh, *vh_next;

	if (!context)
		return;

	context->being_destroyed = 1;

	/* destroy all connections */
	wsi = context->wsi_list;
	while (wsi) {
		next = wsi->next;

		if (wsi->protocol && wsi->protocol->callback) {
			wsi->protocol->callback(wsi, LWS_CALLBACK_CLOSED,
					wsi->user_space, NULL, 0);
		}
		lws_wsi_destroy(wsi);
		wsi = next;
	}

	/* destroy vhosts */
	vh = context->vhost_list;
	while (vh) {
		vh_next = vh->next;

		/* call PROTOCOL_DESTROY */
		if (vh->protocols) {
			const struct lws_protocols *p = vh->protocols;
			int idx = 0;
			while (p->callback && idx < LWS_MAX_PROTOCOLS) {
				p->callback(NULL, LWS_CALLBACK_PROTOCOL_DESTROY,
						NULL, NULL, 0);
				p++;
				idx++;
			}
		}

		if (vh->name) free(vh->name);
		free(vh);
		vh = vh_next;
	}

	free(context);
}

void lws_cancel_service(struct lws_context *context)
{
	if (context)
		context->service_requested = 1;
}

void lws_cancel_service_pt(struct lws *wsi)
{
	if (wsi && wsi->context)
		wsi->context->service_requested = 1;
}

/* ---------- accessors ---------- */

void *lws_wsi_user(struct lws *wsi)
{
	return wsi ? wsi->opaque_user_data : NULL;
}

void lws_set_wsi_user(struct lws *wsi, void *user)
{
	if (wsi)
		wsi->opaque_user_data = user;
}

int lws_get_socket_fd(struct lws *wsi)
{
	return wsi ? wsi->fd : -1;
}

struct lws_context *lws_get_context(const struct lws *wsi)
{
	return wsi ? wsi->context : NULL;
}

struct lws_vhost *lws_get_vhost(struct lws *wsi)
{
	return wsi ? wsi->vhost : NULL;
}

const struct lws_protocols *lws_get_protocol(struct lws *wsi)
{
	return wsi ? wsi->protocol : NULL;
}

void *lws_protocol_vh_priv_zalloc(struct lws_vhost *vhost,
		const struct lws_protocols *prot, int size)
{
	int idx;
	const struct lws_protocols *p;

	if (!vhost || !prot || !vhost->protocols)
		return NULL;

	p = vhost->protocols;
	for (idx = 0; idx < LWS_MAX_PROTOCOLS && p->callback; idx++, p++) {
		if (p == prot) {
			if (vhost->protocol_vh_priv[idx])
				return vhost->protocol_vh_priv[idx];
			vhost->protocol_vh_priv[idx] = calloc(1, (size_t)size);
			return vhost->protocol_vh_priv[idx];
		}
	}
	return NULL;
}

void *lws_protocol_vh_priv_get(struct lws_vhost *vhost,
		const struct lws_protocols *prot)
{
	int idx;
	const struct lws_protocols *p;

	if (!vhost || !prot || !vhost->protocols)
		return NULL;

	p = vhost->protocols;
	for (idx = 0; idx < LWS_MAX_PROTOCOLS && p->callback; idx++, p++) {
		if (p == prot)
			return vhost->protocol_vh_priv[idx];
	}
	return NULL;
}

void *lws_get_protocol_vh_user(struct lws_context *context,
		const struct lws_protocols *prot)
{
	if (!context || !context->vhost_list)
		return NULL;
	return lws_protocol_vh_priv_get(context->vhost_list, prot);
}

int lws_callback_on_writable(struct lws *wsi)
{
	if (!wsi)
		return -1;
	wsi->callback_on_writable = 1;
	return 0;
}

int lws_callback_on_writable_all_protocol(
		const struct lws_context *context,
		const struct lws_protocols *protocol)
{
	struct lws *wsi;

	if (!context)
		return -1;

	for (wsi = context->wsi_list; wsi; wsi = wsi->next) {
		if (wsi->protocol == protocol &&
		    wsi->state == LWS_CONN_STATE_ESTABLISHED)
			wsi->callback_on_writable = 1;
	}
	return 0;
}

const char *lws_get_library_version(void)
{
	return LWS_LIBRARY_VERSION;
}
