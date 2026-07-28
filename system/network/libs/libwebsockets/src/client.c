/**
 * libwebsockets client connection
 */

#include "private.h"
#include <stdio.h>
#include <arpa/inet.h>

struct lws *lws_client_connect_via_info(
		const struct lws_client_connect_info *ccinfo)
{
	struct lws_context *ctx;
	struct lws *wsi;
	int fd;
	struct sockaddr_in addr;

	if (!ccinfo || !ccinfo->context)
		return NULL;

	ctx = ccinfo->context;

	/* create socket */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		lwsl_err("client: socket() failed\n");
		return NULL;
	}

	/* resolve address (simple: only IP addresses supported) */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)ccinfo->port);

	/* try as IP address first */
	if (inet_pton(AF_INET, ccinfo->address, &addr.sin_addr.s_un.s_addr) <= 0) {
		/* try DNS resolution via gethostbyname */
		struct hostent *he = gethostbyname(ccinfo->address);
		if (!he) {
			lwsl_err("client: cannot resolve %s\n", ccinfo->address);
			close(fd);
			return NULL;
		}
		memcpy(&addr.sin_addr.s_un.s_addr, he->h_addr_list[0], (size_t)he->h_length);
	}

	/* connect */
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		lwsl_err("client: connect to %s:%d failed\n",
			ccinfo->address, ccinfo->port);
		close(fd);
		return NULL;
	}

	/* create wsi */
	wsi = lws_wsi_create(ctx);
	if (!wsi) {
		close(fd);
		return NULL;
	}

	wsi->fd = fd;
	wsi->is_server_side = 0;
	wsi->rx_state = LWS_RXPS_NEW;
	wsi->client_port = ccinfo->port;
	wsi->client_ssl = ccinfo->ssl_connection;

	/* save connection parameters */
	if (ccinfo->address)
		wsi->client_address = strdup(ccinfo->address);
	if (ccinfo->path)
		wsi->client_path = strdup(ccinfo->path);
	else
		wsi->client_path = strdup("/");
	if (ccinfo->host)
		wsi->client_host = strdup(ccinfo->host);
	else
		wsi->client_host = strdup(ccinfo->address);
	if (ccinfo->origin)
		wsi->client_origin = strdup(ccinfo->origin);
	if (ccinfo->protocol)
		wsi->client_protocol = strdup(ccinfo->protocol);

	/* assign protocol from vhost */
	if (ctx->vhost_list && ctx->vhost_list->protocols) {
		const struct lws_protocols *p = ctx->vhost_list->protocols;

		if (ccinfo->protocol) {
			/* find matching protocol by name */
			int idx = 0;
			while (p->callback && idx < LWS_MAX_PROTOCOLS) {
				if (p->name && strcmp(p->name, ccinfo->protocol) == 0) {
					wsi->protocol = p;
					break;
				}
				p++;
				idx++;
			}
		}
		if (!wsi->protocol)
			wsi->protocol = &ctx->vhost_list->protocols[0];

		/* allocate per-session data */
		if (wsi->protocol->per_session_data_size > 0) {
			wsi->user_space = calloc(1,
				wsi->protocol->per_session_data_size);
		}
	}

	wsi->vhost = ctx->vhost_list;
	wsi->opaque_user_data = ccinfo->userdata;

	/* TLS handshake if requested */
	if (ccinfo->ssl_connection) {
		if (lws_tls_client_connect(wsi) != 0) {
			lwsl_err("client: TLS handshake failed\n");
			lws_wsi_destroy(wsi);
			return NULL;
		}
	}

	/* send handshake immediately (blocking connect already done) */
	if (lws_client_handshake_send(wsi) == 0) {
		wsi->state = LWS_CONN_STATE_HANDSHAKE_SENT;
	} else {
		lws_wsi_destroy(wsi);
		return NULL;
	}

	lwsl_info("client connecting to %s:%d%s\n",
		ccinfo->address, ccinfo->port,
		ccinfo->path ? ccinfo->path : "/");

	return wsi;
}

struct lws *lws_client_connect_extended(struct lws_context *context,
		const char *address, int port, int ssl_connection,
		const char *path, const char *host, const char *origin,
		const char *protocol, int ietf_version_or_minus_one)
{
	struct lws_client_connect_info ccinfo;

	(void)ietf_version_or_minus_one;

	memset(&ccinfo, 0, sizeof(ccinfo));
	ccinfo.context = context;
	ccinfo.address = address;
	ccinfo.port = port;
	ccinfo.ssl_connection = ssl_connection;
	ccinfo.path = path;
	ccinfo.host = host;
	ccinfo.origin = origin;
	ccinfo.protocol = protocol;

	return lws_client_connect_via_info(&ccinfo);
}
