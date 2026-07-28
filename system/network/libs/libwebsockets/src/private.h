/**
 * libwebsockets private header - internal structures
 */

#ifndef __LWS_PRIVATE_H__
#define __LWS_PRIVATE_H__

#include <libwebsockets/libwebsockets.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

/* ---------- connection states ---------- */

enum lws_conn_state {
	LWS_CONN_STATE_NONE = 0,
	LWS_CONN_STATE_LISTENING,
	LWS_CONN_STATE_CONNECTING,
	LWS_CONN_STATE_HANDSHAKE_SENT,    /* client: waiting for 101 */
	LWS_CONN_STATE_HANDSHAKE_RECV,    /* server: parsing request */
	LWS_CONN_STATE_ESTABLISHED,
	LWS_CONN_STATE_CLOSING,
	LWS_CONN_STATE_CLOSED,
	LWS_CONN_STATE_DEAD,
};

/* ---------- HTTP header storage ---------- */

struct lws_hdr_entry {
	char *data;
	int len;
};

/* ---------- per-connection (wsi) ---------- */

struct lws {
	int fd;                            /* socket fd */
	enum lws_conn_state state;
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;
	void *user_space;                  /* per-session data */
	void *opaque_user_data;            /* user-set via lws_set_wsi_user */

	/* HTTP parsing */
	struct lws_hdr_entry hdr[WSI_TOKEN_COUNT];
	char *rx_hdr_buf;                  /* raw header accumulation */
	int rx_hdr_len;
	int hdr_complete;
	int http_response_code;

	/* WebSocket frame parsing */
	enum lws_rx_parse_state rx_state;
	unsigned char rx_mask[4];
	uint64_t rx_payload_len;
	uint64_t rx_payload_remaining;
	int rx_masked;
	int rx_final;
	int rx_opcode;
	int rx_is_binary;
	unsigned char *rx_payload_buf;     /* accumulated payload */
	uint32_t rx_payload_alloc;

	/* write buffer */
	unsigned char *tx_buf;
	uint32_t tx_len;
	uint32_t tx_offset;

	/* flags */
	unsigned int callback_on_writable:1;
	unsigned int is_server_side:1;
	unsigned int close_sent:1;
	unsigned int close_received:1;
	unsigned int http_mode:1;          /* pure HTTP (no upgrade) */

	/* TLS */
	WOLFSSL *ssl;
	WOLFSSL_CTX *ssl_ctx;

	/* client connect info (saved) */
	char *client_address;
	int client_port;
	char *client_path;
	char *client_host;
	char *client_origin;
	char *client_protocol;
	int client_ssl;

	/* linked list */
	struct lws *next;
};

/* ---------- vhost ---------- */

struct lws_vhost {
	struct lws_context *context;
	char *name;
	int listen_port;
	const struct lws_protocols *protocols;
	void *protocol_vh_priv[LWS_MAX_PROTOCOLS];
	struct lws *listen_wsi;
	struct lws_vhost *next;
};

/* ---------- context ---------- */

struct lws_context {
	struct lws_vhost *vhost_list;
	struct lws *wsi_list;              /* all active connections */
	int listen_fd;
	void *user;
	uint32_t options;
	int service_requested;
	uint32_t timeout_secs;
	unsigned int being_destroyed:1;
};

/* ---------- internal helpers ---------- */

/* handshake.c */
int lws_server_handshake(struct lws *wsi);
int lws_client_handshake_send(struct lws *wsi);
int lws_client_handshake_recv(struct lws *wsi);
int lws_parse_http_headers(struct lws *wsi, const char *buf, int len);
void lws_hdr_free_all(struct lws *wsi);

/* websocket.c */
int lws_ws_frame_parse(struct lws *wsi, const unsigned char *buf, int len);
int lws_ws_frame_write(struct lws *wsi, unsigned char *buf, size_t len,
		enum lws_write_protocol wp);

/* service.c */
int lws_service_one(struct lws_context *ctx, int timeout_ms);
void lws_wsi_destroy(struct lws *wsi);
struct lws *lws_wsi_create(struct lws_context *ctx);

/* misc.c */
void lws_generate_key(struct lws *wsi, char *buf, int len);
int lws_compute_accept(const char *key, char *out, int out_len);

/* tls.c */
int lws_tls_client_connect(struct lws *wsi);
void lws_tls_close(struct lws *wsi);
int lws_ssl_read(struct lws *wsi, unsigned char *buf, int len);
int lws_ssl_write(struct lws *wsi, const unsigned char *buf, int len);

/* WebSocket GUID for Sec-WebSocket-Accept */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* opcodes */
#define WS_OP_CONTINUATION 0x0
#define WS_OP_TEXT         0x1
#define WS_OP_BINARY       0x2
#define WS_OP_CLOSE        0x8
#define WS_OP_PING         0x9
#define WS_OP_PONG         0xA

#endif /* __LWS_PRIVATE_H__ */
