/**
 * libwebsockets utility functions
 */

#include "private.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/* ---------- time ---------- */

int lws_now_secs(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int)tv.tv_sec;
}

uint64_t lws_now_usecs(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

/* ---------- WebSocket key generation ---------- */

void lws_generate_key(struct lws *wsi, char *buf, int len)
{
	/* generate 16 random bytes, base64 encode them */
	unsigned char raw[16];
	int i;

	(void)wsi;

	/* simple pseudo-random from time + address */
	uint32_t seed = (uint32_t)lws_now_usecs() ^ (uint32_t)(uintptr_t)buf;
	for (i = 0; i < 16; i++) {
		seed = seed * 1103515245 + 12345;
		raw[i] = (unsigned char)((seed >> 16) & 0xFF);
	}

	lws_b64_encode_string((const char *)raw, 16, buf, len);
}

/* ---------- Sec-WebSocket-Accept computation ---------- */

int lws_compute_accept(const char *key, char *out, int out_len)
{
	char concat[256];
	unsigned char sha1_digest[20];
	lws_SHA1_CTX ctx;

	if (!key || !out)
		return -1;

	/* concatenate key + GUID */
	int klen = (int)strlen(key);
	int glen = (int)strlen(WS_GUID);
	if (klen + glen >= (int)sizeof(concat))
		return -1;

	memcpy(concat, key, (size_t)klen);
	memcpy(concat + klen, WS_GUID, (size_t)glen + 1);

	/* SHA-1 */
	lws_SHA1_Init(&ctx);
	lws_SHA1_Update(&ctx, (const unsigned char *)concat, (size_t)(klen + glen));
	lws_SHA1_Final(sha1_digest, &ctx);

	/* base64 encode the 20-byte digest */
	lws_b64_encode_string((const char *)sha1_digest, 20, out, out_len);

	return 0;
}

/* ---------- misc stubs ---------- */

int lws_daemonize(const char *_lock_path)
{
	(void)_lock_path;
	return 0;
}

void lws_latency(struct lws_context *context, struct lws *wsi,
		const char *action, int ret, int done)
{
	(void)context;
	(void)wsi;
	(void)action;
	(void)ret;
	(void)done;
}

/* ---------- ringbuffer ---------- */

struct lws_ring *lws_ring_create(size_t element_len, size_t count,
		void (*free_cb)(void *))
{
	struct lws_ring *ring;

	ring = (struct lws_ring *)calloc(1, sizeof(struct lws_ring));
	if (!ring)
		return NULL;

	ring->buflen = (uint32_t)(element_len * count);
	ring->buf = calloc(1, ring->buflen);
	if (!ring->buf) {
		free(ring);
		return NULL;
	}

	ring->element_len = (uint32_t)element_len;
	ring->head = 0;
	ring->tail = 0;
	ring->free_cb = free_cb;

	return ring;
}

void lws_ring_destroy(struct lws_ring *ring)
{
	if (!ring)
		return;

	if (ring->free_cb) {
		/* free remaining elements */
		uint32_t count = lws_ring_get_count_waiting_elements(ring, NULL);
		uint32_t pos = ring->tail;
		uint32_t i;
		for (i = 0; i < count; i++) {
			ring->free_cb((char *)ring->buf +
				(pos % (ring->buflen / ring->element_len)) *
				ring->element_len);
			pos++;
		}
	}

	if (ring->buf)
		free(ring->buf);
	free(ring);
}

uint32_t lws_ring_get_count_free_elements(struct lws_ring *ring)
{
	uint32_t total = ring->buflen / ring->element_len;
	uint32_t used = ring->head - ring->tail;
	return total - used;
}

uint32_t lws_ring_get_count_waiting_elements(struct lws_ring *ring,
		uint32_t *tail)
{
	uint32_t t = tail ? *tail : ring->tail;
	return ring->head - t;
}

uint32_t lws_ring_insert(struct lws_ring *ring, const void *src,
		uint32_t count)
{
	uint32_t total = ring->buflen / ring->element_len;
	uint32_t free_count = lws_ring_get_count_free_elements(ring);
	uint32_t i;

	if (count > free_count)
		count = free_count;

	for (i = 0; i < count; i++) {
		uint32_t pos = (ring->head + i) % total;
		memcpy((char *)ring->buf + pos * ring->element_len,
			(const char *)src + i * ring->element_len,
			ring->element_len);
	}

	ring->head += count;
	return count;
}

uint32_t lws_ring_consume(struct lws_ring *ring, uint32_t *tail,
		void *dest, uint32_t count)
{
	uint32_t total = ring->buflen / ring->element_len;
	uint32_t t = tail ? *tail : ring->tail;
	uint32_t waiting = ring->head - t;
	uint32_t i;

	if (count > waiting)
		count = waiting;

	for (i = 0; i < count; i++) {
		uint32_t pos = (t + i) % total;
		if (dest) {
			memcpy((char *)dest + i * ring->element_len,
				(char *)ring->buf + pos * ring->element_len,
				ring->element_len);
		}
	}

	if (tail)
		*tail = t + count;
	else
		ring->tail = t + count;

	return count;
}
