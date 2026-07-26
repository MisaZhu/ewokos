/**
 * HTTP/WebSocket handshake implementation
 */

#include "private.h"
#include <stdio.h>

/* ---------- HTTP header parsing ---------- */

static int hdr_token_match(const char *line, const char *name)
{
	size_t nlen = strlen(name);
	if (strncasecmp(line, name, nlen) == 0 && line[nlen] == ':')
		return 1;
	return 0;
}

static void hdr_store(struct lws *wsi, int token, const char *value, int len)
{
	if (wsi->hdr[token].data)
		free(wsi->hdr[token].data);
	wsi->hdr[token].data = (char *)malloc((size_t)len + 1);
	if (wsi->hdr[token].data) {
		memcpy(wsi->hdr[token].data, value, (size_t)len);
		wsi->hdr[token].data[len] = '\0';
		wsi->hdr[token].len = len;
	}
}

int lws_parse_http_headers(struct lws *wsi, const char *buf, int len)
{
	int i;

	/* accumulate into header buffer */
	if (wsi->rx_hdr_len + len >= LWS_MAX_HANDSHAKE_LEN)
		return -1;

	memcpy(wsi->rx_hdr_buf + wsi->rx_hdr_len, buf, (size_t)len);
	wsi->rx_hdr_len += len;
	wsi->rx_hdr_buf[wsi->rx_hdr_len] = '\0';

	/* check for end of headers */
	if (strstr(wsi->rx_hdr_buf, "\r\n\r\n") ||
	    strstr(wsi->rx_hdr_buf, "\n\n"))
		wsi->hdr_complete = 1;

	if (!wsi->hdr_complete)
		return 0;

	/* parse the complete header block */
	char *hdr = wsi->rx_hdr_buf;
	char *line_end;
	int first_line = 1;

	while (hdr && *hdr) {
		line_end = strstr(hdr, "\r\n");
		if (!line_end)
			line_end = strstr(hdr, "\n");
		if (!line_end)
			break;

		int line_len = (int)(line_end - hdr);
		if (line_len == 0)
			break; /* empty line = end of headers */

		if (first_line) {
			/* request line: GET /path HTTP/1.1 */
			/* or response line: HTTP/1.1 101 ... */
			if (strncmp(hdr, "HTTP/", 5) == 0) {
				/* response: extract status code */
				char *sp = strchr(hdr, ' ');
				if (sp)
					wsi->http_response_code = atoi(sp + 1);
			} else if (strncmp(hdr, "GET ", 4) == 0) {
				char *path_end = strchr(hdr + 4, ' ');
				if (path_end) {
					int plen = (int)(path_end - (hdr + 4));
					hdr_store(wsi, WSI_TOKEN_GET_URI, hdr + 4, plen);
				}
			} else if (strncmp(hdr, "POST ", 5) == 0) {
				char *path_end = strchr(hdr + 5, ' ');
				if (path_end) {
					int plen = (int)(path_end - (hdr + 5));
					hdr_store(wsi, WSI_TOKEN_POST_URI, hdr + 5, plen);
				}
			}
			first_line = 0;
		} else {
			/* header line: Name: value */
			char *colon = strchr(hdr, ':');
			if (colon && (colon - hdr) < line_len) {
				char *val = colon + 1;
				while (*val == ' ') val++;
				int vlen = line_len - (int)(val - hdr);

				if (hdr_token_match(hdr, "Host"))
					hdr_store(wsi, WSI_TOKEN_HOST, val, vlen);
				else if (hdr_token_match(hdr, "Connection"))
					hdr_store(wsi, WSI_TOKEN_CONNECTION, val, vlen);
				else if (hdr_token_match(hdr, "Upgrade"))
					hdr_store(wsi, WSI_TOKEN_UPGRADE, val, vlen);
				else if (hdr_token_match(hdr, "Sec-WebSocket-Key"))
					hdr_store(wsi, WSI_TOKEN_KEY, val, vlen);
				else if (hdr_token_match(hdr, "Sec-WebSocket-Version"))
					hdr_store(wsi, WSI_TOKEN_VERSION, val, vlen);
				else if (hdr_token_match(hdr, "Sec-WebSocket-Protocol"))
					hdr_store(wsi, WSI_TOKEN_PROTOCOL, val, vlen);
				else if (hdr_token_match(hdr, "Sec-WebSocket-Accept"))
					hdr_store(wsi, WSI_TOKEN_ACCEPT, val, vlen);
				else if (hdr_token_match(hdr, "Sec-WebSocket-Extensions"))
					hdr_store(wsi, WSI_TOKEN_EXTENSIONS, val, vlen);
				else if (hdr_token_match(hdr, "Content-Type"))
					hdr_store(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE, val, vlen);
				else if (hdr_token_match(hdr, "Content-Length"))
					hdr_store(wsi, WSI_TOKEN_HTTP_CONTENT_LENGTH, val, vlen);
				else if (hdr_token_match(hdr, "Origin"))
					hdr_store(wsi, WSI_TOKEN_ORIGIN, val, vlen);
				else if (hdr_token_match(hdr, "User-Agent"))
					hdr_store(wsi, WSI_TOKEN_HTTP_USER_AGENT, val, vlen);
				else if (hdr_token_match(hdr, "Cookie"))
					hdr_store(wsi, WSI_TOKEN_HTTP_COOKIE, val, vlen);
				else if (hdr_token_match(hdr, "Authorization"))
					hdr_store(wsi, WSI_TOKEN_HTTP_AUTHORIZATION, val, vlen);
			}
		}

		hdr = line_end;
		if (*hdr == '\r') hdr++;
		if (*hdr == '\n') hdr++;
	}

	(void)i;
	return 0;
}

/* ---------- server handshake (respond to client upgrade request) ---------- */

int lws_server_handshake(struct lws *wsi)
{
	char accept_key[64];
	char response[512];
	int rlen;

	/* verify it's a WebSocket upgrade request */
	if (!wsi->hdr[WSI_TOKEN_KEY].data) {
		/* not a websocket request - handle as plain HTTP */
		wsi->http_mode = 1;
		if (wsi->protocol && wsi->protocol->callback) {
			const char *uri = wsi->hdr[WSI_TOKEN_GET_URI].data ?
				wsi->hdr[WSI_TOKEN_GET_URI].data : "/";
			wsi->protocol->callback(wsi, LWS_CALLBACK_HTTP,
					wsi->user_space, (void *)uri,
					strlen(uri));
		}
		return 0;
	}

	/* compute Sec-WebSocket-Accept */
	if (lws_compute_accept(wsi->hdr[WSI_TOKEN_KEY].data,
			accept_key, sizeof(accept_key)) < 0) {
		wsi->state = LWS_CONN_STATE_DEAD;
		return -1;
	}

	/* send 101 response */
	rlen = snprintf(response, sizeof(response),
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n",
		accept_key);

	/* add protocol if requested */
	if (wsi->hdr[WSI_TOKEN_PROTOCOL].data) {
		rlen += snprintf(response + rlen, sizeof(response) - (size_t)rlen,
			"Sec-WebSocket-Protocol: %s\r\n",
			wsi->hdr[WSI_TOKEN_PROTOCOL].data);
	}

	rlen += snprintf(response + rlen, sizeof(response) - (size_t)rlen,
		"\r\n");

	lws_ssl_write(wsi, (const unsigned char *)response, rlen);

	/* transition to established */
	wsi->state = LWS_CONN_STATE_ESTABLISHED;
	wsi->rx_state = LWS_RXPS_NEW;

	/* notify protocol */
	if (wsi->protocol && wsi->protocol->callback) {
		wsi->protocol->callback(wsi, LWS_CALLBACK_ESTABLISHED,
				wsi->user_space, NULL, 0);
	}

	lwsl_info("server handshake complete, ws established\n");
	return 0;
}

/* ---------- client handshake send ---------- */

int lws_client_handshake_send(struct lws *wsi)
{
	char key_b64[32];
	char request[1024];
	int rlen;

	/* generate random key */
	lws_generate_key(wsi, key_b64, sizeof(key_b64));

	/* store our key for verification of server response */
	hdr_store(wsi, WSI_TOKEN_KEY, key_b64, (int)strlen(key_b64));

	rlen = snprintf(request, sizeof(request),
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Version: 13\r\n",
		wsi->client_path ? wsi->client_path : "/",
		wsi->client_host ? wsi->client_host : "localhost",
		key_b64);

	if (wsi->client_protocol) {
		rlen += snprintf(request + rlen, sizeof(request) - (size_t)rlen,
			"Sec-WebSocket-Protocol: %s\r\n",
			wsi->client_protocol);
	}

	if (wsi->client_origin) {
		rlen += snprintf(request + rlen, sizeof(request) - (size_t)rlen,
			"Origin: %s\r\n", wsi->client_origin);
	}

	rlen += snprintf(request + rlen, sizeof(request) - (size_t)rlen,
		"\r\n");

	if (lws_ssl_write(wsi, (const unsigned char *)request, rlen) < 0)
		return -1;

	lwsl_info("client handshake sent\n");
	return 0;
}

/* ---------- client handshake receive ---------- */

int lws_client_handshake_recv(struct lws *wsi)
{
	unsigned char buf[LWS_DEFAULT_RX_BUF];
	int n;

	n = lws_ssl_read(wsi, buf, sizeof(buf));
	if (n <= 0) {
		if (wsi->protocol && wsi->protocol->callback) {
			wsi->protocol->callback(wsi,
				LWS_CALLBACK_CLIENT_CONNECTION_ERROR,
				wsi->user_space, (void *)"connection closed", 17);
		}
		wsi->state = LWS_CONN_STATE_DEAD;
		return -1;
	}

	/* accumulate headers */
	if (!wsi->rx_hdr_buf) {
		wsi->rx_hdr_buf = (char *)malloc(LWS_MAX_HANDSHAKE_LEN);
		if (!wsi->rx_hdr_buf) {
			wsi->state = LWS_CONN_STATE_DEAD;
			return -1;
		}
		wsi->rx_hdr_len = 0;
	}

	lws_parse_http_headers(wsi, (const char *)buf, n);

	if (!wsi->hdr_complete)
		return 0; /* need more data */

	/* verify 101 response */
	if (wsi->http_response_code != 101) {
		char err[64];
		snprintf(err, sizeof(err), "HTTP %d", wsi->http_response_code);
		if (wsi->protocol && wsi->protocol->callback) {
			wsi->protocol->callback(wsi,
				LWS_CALLBACK_CLIENT_CONNECTION_ERROR,
				wsi->user_space, (void *)err, strlen(err));
		}
		wsi->state = LWS_CONN_STATE_DEAD;
		return -1;
	}

	/* verify Sec-WebSocket-Accept */
	if (wsi->hdr[WSI_TOKEN_ACCEPT].data && wsi->hdr[WSI_TOKEN_KEY].data) {
		char expected[64];
		lws_compute_accept(wsi->hdr[WSI_TOKEN_KEY].data,
				expected, sizeof(expected));
		if (strcmp(wsi->hdr[WSI_TOKEN_ACCEPT].data, expected) != 0) {
			lwsl_err("Sec-WebSocket-Accept mismatch\n");
			wsi->state = LWS_CONN_STATE_DEAD;
			return -1;
		}
	}

	/* established! */
	wsi->state = LWS_CONN_STATE_ESTABLISHED;
	wsi->rx_state = LWS_RXPS_NEW;

	if (wsi->protocol && wsi->protocol->callback) {
		wsi->protocol->callback(wsi, LWS_CALLBACK_CLIENT_ESTABLISHED,
				wsi->user_space, NULL, 0);
	}

	lwsl_info("client connection established\n");
	return 0;
}

/* ---------- header accessors ---------- */

const char *lws_hdr_simple_ptr(struct lws *wsi, int h)
{
	if (!wsi || h < 0 || h >= WSI_TOKEN_COUNT)
		return NULL;
	return wsi->hdr[h].data;
}

int lws_hdr_total_length(struct lws *wsi, int h)
{
	if (!wsi || h < 0 || h >= WSI_TOKEN_COUNT)
		return 0;
	return wsi->hdr[h].len;
}

int lws_hdr_copy(struct lws *wsi, char *dest, int len, int h)
{
	if (!wsi || h < 0 || h >= WSI_TOKEN_COUNT || !wsi->hdr[h].data)
		return -1;

	int copy_len = wsi->hdr[h].len;
	if (copy_len >= len)
		copy_len = len - 1;

	memcpy(dest, wsi->hdr[h].data, (size_t)copy_len);
	dest[copy_len] = '\0';
	return copy_len;
}

/* ---------- HTTP response helpers ---------- */

int lws_return_http_status(struct lws *wsi, unsigned int code,
		const char *html_body)
{
	char buf[1024];
	const char *status_text;
	int body_len = html_body ? (int)strlen(html_body) : 0;
	int n;

	switch (code) {
	case 200: status_text = "OK"; break;
	case 400: status_text = "Bad Request"; break;
	case 404: status_text = "Not Found"; break;
	case 500: status_text = "Internal Server Error"; break;
	default:  status_text = "Unknown"; break;
	}

	n = snprintf(buf, sizeof(buf),
		"HTTP/1.1 %u %s\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		code, status_text, body_len,
		html_body ? html_body : "");

	lws_ssl_write(wsi, (const unsigned char *)buf, n);
	wsi->state = LWS_CONN_STATE_DEAD;
	return 0;
}

int lws_callback_http_dummy(struct lws *wsi,
		enum lws_callback_reasons reason,
		void *user, void *in, size_t len)
{
	(void)user;
	(void)in;
	(void)len;

	switch (reason) {
	case LWS_CALLBACK_HTTP:
		lws_return_http_status(wsi, 404, "<html><body>404</body></html>");
		break;
	default:
		break;
	}
	return 0;
}

int lws_add_http_common_headers(struct lws *wsi, unsigned int code,
		const char *content_type, uint64_t content_len,
		unsigned char **p, unsigned char *end)
{
	(void)wsi;
	(void)code;
	(void)content_type;
	(void)content_len;
	(void)p;
	(void)end;
	return 0;
}

int lws_finalize_write_http_header(struct lws *wsi,
		unsigned char *start, unsigned char **p, unsigned char *end)
{
	(void)wsi;
	(void)start;
	(void)p;
	(void)end;
	return 0;
}

unsigned char *lws_token_to_string(int token)
{
	(void)token;
	return (unsigned char *)"";
}
