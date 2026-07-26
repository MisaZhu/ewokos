/**
 * libwebsockets - minimal port for EwokOS
 *
 * Provides WebSocket client/server functionality following the
 * libwebsockets API conventions (RFC 6455).
 */

#ifndef __LIBWEBSOCKETS_H__
#define __LIBWEBSOCKETS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ---------- version ---------- */
#define LWS_LIBRARY_VERSION_MAJOR 4
#define LWS_LIBRARY_VERSION_MINOR 3
#define LWS_LIBRARY_VERSION_PATCH 0
#define LWS_LIBRARY_VERSION "4.3.0-ewokos"

/* ---------- limits ---------- */
#define LWS_MAX_PROTOCOLS      16
#define LWS_MAX_EXTENSIONS     4
#define LWS_MAX_SMP            1
#define LWS_PRE                16   /* space before payload for frame hdr */
#define LWS_MAX_SOCKETS        64
#define LWS_DEFAULT_RX_BUF     4096
#define LWS_MAX_HANDSHAKE_LEN  2048

/* ---------- enums ---------- */

enum lws_log_levels {
	LLL_ERR   = 1 << 0,
	LLL_WARN  = 1 << 1,
	LLL_NOTICE= 1 << 2,
	LLL_INFO  = 1 << 3,
	LLL_DEBUG = 1 << 4,
	LLL_PARSER= 1 << 5,
	LLL_HEADER= 1 << 6,
	LLL_CLIENT= 1 << 10,
};

enum lws_callback_reasons {
	LWS_CALLBACK_ESTABLISHED,
	LWS_CALLBACK_CLIENT_CONNECTION_ERROR,
	LWS_CALLBACK_CLIENT_ESTABLISHED,
	LWS_CALLBACK_CLOSED,
	LWS_CALLBACK_CLOSED_HTTP,
	LWS_CALLBACK_RECEIVE,
	LWS_CALLBACK_RECEIVE_PONG,
	LWS_CALLBACK_CLIENT_RECEIVE,
	LWS_CALLBACK_CLIENT_RECEIVE_PONG,
	LWS_CALLBACK_CLIENT_WRITEABLE,
	LWS_CALLBACK_SERVER_WRITEABLE,
	LWS_CALLBACK_HTTP,
	LWS_CALLBACK_HTTP_BODY,
	LWS_CALLBACK_HTTP_BODY_COMPLETION,
	LWS_CALLBACK_HTTP_DROP_PROTOCOL,
	LWS_CALLBACK_HTTP_WRITEABLE,
	LWS_CALLBACK_FILTER_NETWORK_CONNECTION,
	LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION,
	LWS_CALLBACK_PROTOCOL_INIT,
	LWS_CALLBACK_PROTOCOL_DESTROY,
	LWS_CALLBACK_WSI_CREATE,
	LWS_CALLBACK_WSI_DESTROY,
	LWS_CALLBACK_GET_THREAD_ID,
	LWS_CALLBACK_EVENT_WAIT_CANCELLED,
	LWS_CALLBACK_TIMER,
};

enum lws_write_protocol {
	LWS_WRITE_TEXT,
	LWS_WRITE_BINARY,
	LWS_WRITE_CONTINUATION,
	LWS_WRITE_CLOSE,
	LWS_WRITE_PING,
	LWS_WRITE_PONG,
	LWS_WRITE_HTTP,
	LWS_WRITE_HTTP_FINAL,
	LWS_WRITE_HTTP_HEADERS,
	LWS_WRITE_RAW,
};

enum lws_rx_parse_state {
	LWS_RXPS_NEW,
	LWS_RXPS_04_MASK_NONCE_1,
	LWS_RXPS_04_MASK_NONCE_2,
	LWS_RXPS_04_MASK_NONCE_3,
	LWS_RXPS_04_FRAME_HDR_1,
	LWS_RXPS_04_FRAME_HDR_LEN,
	LWS_RXPS_04_FRAME_HDR_LEN16_2,
	LWS_RXPS_04_FRAME_HDR_LEN16_1,
	LWS_RXPS_04_FRAME_HDR_LEN64_8,
	LWS_RXPS_PAYLOAD,
};

enum lws_close_status {
	LWS_CLOSE_STATUS_NOSTATUS          = 0,
	LWS_CLOSE_STATUS_NORMAL            = 1000,
	LWS_CLOSE_STATUS_GOINGAWAY         = 1001,
	LWS_CLOSE_STATUS_PROTOCOL_ERR      = 1002,
	LWS_CLOSE_STATUS_UNACCEPTABLE      = 1003,
	LWS_CLOSE_STATUS_NO_STATUS         = 1005,
	LWS_CLOSE_STATUS_ABNORMAL_CLOSE    = 1006,
	LWS_CLOSE_STATUS_INVALID_PAYLOAD   = 1007,
	LWS_CLOSE_STATUS_POLICY_VIOLATION  = 1008,
	LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE = 1009,
	LWS_CLOSE_STATUS_EXTENSION_REQUIRED= 1010,
	LWS_CLOSE_STATUS_UNEXPECTED_CONDITION = 1011,
	LWS_CLOSE_STATUS_TLS_FAILURE       = 1015,
};

/* ---------- opaque types ---------- */

struct lws_context;
struct lws;
struct lws_vhost;

/* ---------- callback ---------- */

typedef int (*lws_callback_function)(struct lws *wsi,
		enum lws_callback_reasons reason,
		void *user, void *in, size_t len);

/* ---------- protocol ---------- */

struct lws_protocols {
	const char *name;
	lws_callback_function callback;
	size_t per_session_data_size;
	size_t rx_buffer_size;
	unsigned int id;
	void *user;
};

/* ---------- context creation info ---------- */

struct lws_context_creation_info {
	int port;                          /* -1 = client only, 0 = random */
	const char *iface;                 /* bind interface (NULL = any) */
	const struct lws_protocols *protocols;
	const char *const *plugin_dirs;
	int gid;
	int uid;
	unsigned int options;
	void *user;
	int ka_time;                       /* tcp keepalive time (0=off) */
	int ka_probes;
	int ka_interval;
	uint32_t timeout_secs;
	const char *vhost_name;
	const char *ssl_cert_filepath;
	const char *ssl_private_key_filepath;
	uint32_t max_http_header_data;
	uint32_t max_http_header_pool;
	unsigned int count_threads;
	int fd_limit_per_thread;
	uint32_t timeout_secs_ah_idle;
	unsigned int pt_serv_buf_size;
};

/* ---------- client connect info ---------- */

struct lws_client_connect_info {
	struct lws_context *context;
	const char *address;
	int port;
	int ssl_connection;               /* 0 = plain, nonzero = TLS */
	const char *path;
	const char *host;
	const char *origin;
	const char *protocol;
	const char *ietf_version_or_minus_one;
	void *userdata;
	const char *method;               /* NULL = GET (ws), "RAW" etc */
	const char *iface;
	unsigned int retry_and_idle_policy;
	int keepalive_secs;
};

/* ---------- extension (stub) ---------- */

struct lws_extension {
	const char *name;
	void *callback;
	const char *per_session_data_size;
};

/* ---------- context options ---------- */

#define LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT   (1 << 0)
#define LWS_SERVER_OPTION_VALIDATE_UTF8        (1 << 3)
#define LWS_SERVER_OPTION_EXPLICIT_VHOSTS      (1 << 5)
#define LWS_SERVER_OPTION_LIBUV                (1 << 1)

/* ---------- public API ---------- */

/* context lifecycle */
struct lws_context *lws_create_context(
		const struct lws_context_creation_info *info);
void lws_context_destroy(struct lws_context *context);
int lws_service(struct lws_context *context, int timeout_ms);
int lws_service_fd(struct lws_context *context, void *pollfd);
void lws_cancel_service(struct lws_context *context);
void lws_cancel_service_pt(struct lws *wsi);

/* connection */
struct lws *lws_client_connect_via_info(
		const struct lws_client_connect_info *ccinfo);
struct lws *lws_client_connect_extended(struct lws_context *context,
		const char *address, int port, int ssl_connection,
		const char *path, const char *host, const char *origin,
		const char *protocol, int ietf_version_or_minus_one);

/* I/O */
int lws_write(struct lws *wsi, unsigned char *buf, size_t len,
		enum lws_write_protocol wp);
int lws_read(struct lws *wsi, unsigned char *buf, size_t len);

/* close */
int lws_close_reason(struct lws *wsi, enum lws_close_status status,
		unsigned char *buf, size_t len);

/* protocol helpers */
int lws_callback_on_writable(struct lws *wsi);
int lws_callback_on_writable_all_protocol(
		const struct lws_context *context,
		const struct lws_protocols *protocol);

/* wsi accessors */
void *lws_wsi_user(struct lws *wsi);
void lws_set_wsi_user(struct lws *wsi, void *user);
int lws_get_socket_fd(struct lws *wsi);
struct lws_context *lws_get_context(const struct lws *wsi);
struct lws_vhost *lws_get_vhost(struct lws *wsi);
const struct lws_protocols *lws_get_protocol(struct lws *wsi);
void *lws_protocol_vh_priv_zalloc(struct lws_vhost *vhost,
		const struct lws_protocols *prot, int size);
void *lws_protocol_vh_priv_get(struct lws_vhost *vhost,
		const struct lws_protocols *prot);
void *lws_get_protocol_vh_user(struct lws_context *context,
		const struct lws_protocols *prot);

/* HTTP helpers */
int lws_return_http_status(struct lws *wsi, unsigned int code,
		const char *html_body);
int lws_callback_http_dummy(struct lws *wsi,
		enum lws_callback_reasons reason,
		void *user, void *in, size_t len);
const char *lws_hdr_simple_ptr(struct lws *wsi, int h);
int lws_hdr_total_length(struct lws *wsi, int h);
int lws_hdr_copy(struct lws *wsi, char *dest, int len, int h);
unsigned char *lws_token_to_string(int token);
int lws_add_http_common_headers(struct lws *wsi, unsigned int code,
		const char *content_type, uint64_t content_len,
		unsigned char **p, unsigned char *end);
int lws_finalize_write_http_header(struct lws *wsi,
		unsigned char *start, unsigned char **p, unsigned char *end);

/* header tokens */
enum http_token {
	WSI_TOKEN_GET_URI,
	WSI_TOKEN_POST_URI,
	WSI_TOKEN_HOST,
	WSI_TOKEN_CONNECTION,
	WSI_TOKEN_UPGRADE,
	WSI_TOKEN_KEY,
	WSI_TOKEN_VERSION,
	WSI_TOKEN_PROTOCOL,
	WSI_TOKEN_ACCEPT,
	WSI_TOKEN_NONCE,
	WSI_TOKEN_EXTENSIONS,
	WSI_TOKEN_HTTP_CONTENT_TYPE,
	WSI_TOKEN_HTTP_CONTENT_LENGTH,
	WSI_TOKEN_HTTP_SET_COOKIE,
	WSI_TOKEN_HTTP_COOKIE,
	WSI_TOKEN_HTTP_USER_AGENT,
	WSI_TOKEN_HTTP_ACCEPT,
	WSI_TOKEN_HTTP_ACCEPT_ENCODING,
	WSI_TOKEN_HTTP_ACCEPT_LANGUAGE,
	WSI_TOKEN_HTTP_PRAGMA,
	WSI_TOKEN_HTTP_CACHE_CONTROL,
	WSI_TOKEN_HTTP_AUTHORIZATION,
	WSI_TOKEN_ORIGIN,
	WSI_TOKEN_HTTP,
	WSI_TOKEN_COUNT
};

/* logging */
void lws_set_log_level(int level, void (*log_emit)(int level, const char *line));
void _lws_log(int filter, const char *format, ...);

#define lwsl_err(...)   _lws_log(LLL_ERR, __VA_ARGS__)
#define lwsl_warn(...)  _lws_log(LLL_WARN, __VA_ARGS__)
#define lwsl_notice(...) _lws_log(LLL_NOTICE, __VA_ARGS__)
#define lwsl_info(...)  _lws_log(LLL_INFO, __VA_ARGS__)
#define lwsl_debug(...) _lws_log(LLL_DEBUG, __VA_ARGS__)

/* utility */
int lws_now_secs(void);
uint64_t lws_now_usecs(void);
size_t lws_b64_encode_string(const char *in, int in_len,
		char *out, int out_size);
int lws_b64_decode_string(const char *in, char *out, int out_size);
int lws_daemonize(const char *_lock_path);
const char *lws_get_library_version(void);
void lws_latency(struct lws_context *context, struct lws *wsi,
		const char *action, int ret, int done);
int lws_frame_is_binary(struct lws *wsi);
size_t lws_remaining_packet_payload(struct lws *wsi);
int lws_is_final_fragment(struct lws *wsi);
unsigned char lws_get_reserved_bits(struct lws *wsi);

/* sha1 (internal but exposed for testing) */
typedef struct {
	uint32_t state[5];
	uint32_t count[2];
	unsigned char buffer[64];
} lws_SHA1_CTX;

void lws_SHA1_Init(lws_SHA1_CTX *ctx);
void lws_SHA1_Update(lws_SHA1_CTX *ctx, const unsigned char *data, size_t len);
void lws_SHA1_Final(unsigned char digest[20], lws_SHA1_CTX *ctx);

/* ringbuffer (simplified) */
struct lws_ring {
	void *buf;
	uint32_t buflen;
	uint32_t head;
	uint32_t tail;
	uint32_t element_len;
	void (*free_cb)(void *);
};

struct lws_ring *lws_ring_create(size_t element_len, size_t count,
		void (*free_cb)(void *));
void lws_ring_destroy(struct lws_ring *ring);
uint32_t lws_ring_insert(struct lws_ring *ring, const void *src, uint32_t count);
uint32_t lws_ring_consume(struct lws_ring *ring, uint32_t *tail,
		void *dest, uint32_t count);
uint32_t lws_ring_get_count_free_elements(struct lws_ring *ring);
uint32_t lws_ring_get_count_waiting_elements(struct lws_ring *ring, uint32_t *tail);

#ifdef __cplusplus
}
#endif

#endif /* __LIBWEBSOCKETS_H__ */
