/**
 * WebSocket frame parsing and writing (RFC 6455)
 */

#include "private.h"
#include <stdio.h>

/* ---------- frame parsing (server and client) ---------- */

int lws_ws_frame_parse(struct lws *wsi, const unsigned char *buf, int len)
{
	int i = 0;

	while (i < len) {
		switch (wsi->rx_state) {
		case LWS_RXPS_NEW:
		{
			unsigned char b = buf[i++];
			wsi->rx_final = (b >> 7) & 1;
			wsi->rx_opcode = b & 0x0F;
			wsi->rx_is_binary = (wsi->rx_opcode == WS_OP_BINARY);
			wsi->rx_state = LWS_RXPS_04_FRAME_HDR_1;
			break;
		}

		case LWS_RXPS_04_FRAME_HDR_1:
		{
			unsigned char b = buf[i++];
			wsi->rx_masked = (b >> 7) & 1;
			uint64_t payload_len = b & 0x7F;

			if (payload_len == 126) {
				wsi->rx_state = LWS_RXPS_04_FRAME_HDR_LEN16_2;
			} else if (payload_len == 127) {
				wsi->rx_state = LWS_RXPS_04_FRAME_HDR_LEN64_8;
				wsi->rx_payload_len = 0;
				wsi->rx_payload_remaining = 8; /* bytes to read for len */
			} else {
				wsi->rx_payload_len = payload_len;
				if (wsi->rx_masked)
					wsi->rx_state = LWS_RXPS_04_MASK_NONCE_1;
				else
					wsi->rx_state = LWS_RXPS_PAYLOAD;
				wsi->rx_payload_remaining = wsi->rx_payload_len;
			}
			break;
		}

		case LWS_RXPS_04_FRAME_HDR_LEN16_2:
			wsi->rx_payload_len = (uint64_t)buf[i++] << 8;
			wsi->rx_state = LWS_RXPS_04_FRAME_HDR_LEN16_1;
			break;

		case LWS_RXPS_04_FRAME_HDR_LEN16_1:
			wsi->rx_payload_len |= buf[i++];
			wsi->rx_payload_remaining = wsi->rx_payload_len;
			if (wsi->rx_masked)
				wsi->rx_state = LWS_RXPS_04_MASK_NONCE_1;
			else
				wsi->rx_state = LWS_RXPS_PAYLOAD;
			break;

		case LWS_RXPS_04_FRAME_HDR_LEN64_8:
		{
			/* read 8 bytes of length (big-endian) */
			wsi->rx_payload_len = (wsi->rx_payload_len << 8) | buf[i++];
			wsi->rx_payload_remaining--;
			if (wsi->rx_payload_remaining == 0) {
				wsi->rx_payload_remaining = wsi->rx_payload_len;
				if (wsi->rx_masked)
					wsi->rx_state = LWS_RXPS_04_MASK_NONCE_1;
				else
					wsi->rx_state = LWS_RXPS_PAYLOAD;
			}
			break;
		}

		case LWS_RXPS_04_MASK_NONCE_1:
			wsi->rx_mask[0] = buf[i++];
			wsi->rx_state = LWS_RXPS_04_MASK_NONCE_2;
			break;

		case LWS_RXPS_04_MASK_NONCE_2:
			wsi->rx_mask[1] = buf[i++];
			wsi->rx_state = LWS_RXPS_04_MASK_NONCE_3;
			break;

		case LWS_RXPS_04_MASK_NONCE_3:
			wsi->rx_mask[2] = buf[i++];
			/* one more byte for mask[3] */
			if (i < len) {
				wsi->rx_mask[3] = buf[i++];
				wsi->rx_state = LWS_RXPS_PAYLOAD;
			}
			break;

		case LWS_RXPS_PAYLOAD:
		{
			if (wsi->rx_payload_remaining == 0) {
				/* zero-length payload, frame complete */
				goto frame_complete;
			}

			/* ensure payload buffer */
			uint32_t needed = (uint32_t)wsi->rx_payload_len + 1;
			if (needed > wsi->rx_payload_alloc) {
				unsigned char *nb = (unsigned char *)realloc(
					wsi->rx_payload_buf, needed);
				if (!nb) {
					wsi->state = LWS_CONN_STATE_DEAD;
					return -1;
				}
				wsi->rx_payload_buf = nb;
				wsi->rx_payload_alloc = needed;
			}

			/* consume payload bytes */
			uint32_t consumed = 0;
			uint32_t offset = (uint32_t)(wsi->rx_payload_len -
					wsi->rx_payload_remaining);
			while (i < len && wsi->rx_payload_remaining > 0) {
				unsigned char c = buf[i++];
				if (wsi->rx_masked)
					c ^= wsi->rx_mask[(offset + consumed) & 3];
				wsi->rx_payload_buf[offset + consumed] = c;
				consumed++;
				wsi->rx_payload_remaining--;
			}

			if (wsi->rx_payload_remaining == 0) {
				goto frame_complete;
			}
			break;
		}

		default:
			wsi->rx_state = LWS_RXPS_NEW;
			break;
		}
		continue;

frame_complete:
	{
		wsi->rx_payload_buf[wsi->rx_payload_len] = '\0';

		switch (wsi->rx_opcode) {
		case WS_OP_CLOSE:
		{
			wsi->close_received = 1;
			if (!wsi->close_sent) {
				/* echo close frame back */
				unsigned char close_buf[4];
				close_buf[0] = 0x88; /* FIN + CLOSE */
				close_buf[1] = (unsigned char)wsi->rx_payload_len;
				if (wsi->rx_payload_len >= 2 && !wsi->is_server_side) {
					/* client must mask */
					close_buf[1] |= 0x80;
					close_buf[2] = 0; close_buf[3] = 0;
					/* simplified: zero mask */
				}
				lws_ssl_write(wsi, close_buf, 2);
				wsi->close_sent = 1;
			}
			if (wsi->protocol && wsi->protocol->callback) {
				wsi->protocol->callback(wsi, LWS_CALLBACK_CLOSED,
					wsi->user_space, NULL, 0);
			}
			wsi->state = LWS_CONN_STATE_DEAD;
			return 0;
		}

		case WS_OP_PING:
		{
			/* respond with pong */
			unsigned char pong[2] = { 0x8A, 0x00 };
			pong[1] = (unsigned char)(wsi->rx_payload_len & 0x7F);
			if (!wsi->is_server_side)
				pong[1] |= 0x80; /* client frames must be masked */
			lws_ssl_write(wsi, pong, 2);
			/* also send payload if any */
			if (wsi->rx_payload_len > 0)
				lws_ssl_write(wsi, wsi->rx_payload_buf,
					(int)wsi->rx_payload_len);
			break;
		}

		case WS_OP_PONG:
			if (wsi->protocol && wsi->protocol->callback) {
				enum lws_callback_reasons r = wsi->is_server_side ?
					LWS_CALLBACK_RECEIVE_PONG :
					LWS_CALLBACK_CLIENT_RECEIVE_PONG;
				wsi->protocol->callback(wsi, r,
					wsi->user_space,
					wsi->rx_payload_buf,
					(size_t)wsi->rx_payload_len);
			}
			break;

		case WS_OP_TEXT:
		case WS_OP_BINARY:
		case WS_OP_CONTINUATION:
		{
			enum lws_callback_reasons r = wsi->is_server_side ?
				LWS_CALLBACK_RECEIVE :
				LWS_CALLBACK_CLIENT_RECEIVE;
			if (wsi->protocol && wsi->protocol->callback) {
				wsi->protocol->callback(wsi, r,
					wsi->user_space,
					wsi->rx_payload_buf,
					(size_t)wsi->rx_payload_len);
			}
			break;
		}

		default:
			break;
		}

		/* reset for next frame */
		wsi->rx_state = LWS_RXPS_NEW;
		wsi->rx_payload_len = 0;
		wsi->rx_payload_remaining = 0;
		break;
	}
	}

	return 0;
}

/* ---------- frame writing ---------- */

int lws_ws_frame_write(struct lws *wsi, unsigned char *buf, size_t len,
		enum lws_write_protocol wp)
{
	unsigned char hdr[14];
	int hdr_len = 0;
	unsigned char opcode;
	int mask = !wsi->is_server_side; /* clients must mask */

	switch (wp) {
	case LWS_WRITE_TEXT:
		opcode = WS_OP_TEXT;
		break;
	case LWS_WRITE_BINARY:
		opcode = WS_OP_BINARY;
		break;
	case LWS_WRITE_CONTINUATION:
		opcode = WS_OP_CONTINUATION;
		break;
	case LWS_WRITE_CLOSE:
		opcode = WS_OP_CLOSE;
		break;
	case LWS_WRITE_PING:
		opcode = WS_OP_PING;
		break;
	case LWS_WRITE_PONG:
		opcode = WS_OP_PONG;
		break;
	case LWS_WRITE_HTTP:
	case LWS_WRITE_HTTP_FINAL:
	case LWS_WRITE_HTTP_HEADERS:
	case LWS_WRITE_RAW:
		/* raw write without framing */
		return lws_ssl_write(wsi, buf, (int)len);
	default:
		opcode = WS_OP_TEXT;
		break;
	}

	/* byte 0: FIN + opcode */
	hdr[hdr_len++] = (unsigned char)(0x80 | opcode);

	/* byte 1: MASK + payload length */
	if (len < 126) {
		hdr[hdr_len++] = (unsigned char)((mask ? 0x80 : 0) | (len & 0x7F));
	} else if (len < 65536) {
		hdr[hdr_len++] = (unsigned char)((mask ? 0x80 : 0) | 126);
		hdr[hdr_len++] = (unsigned char)((len >> 8) & 0xFF);
		hdr[hdr_len++] = (unsigned char)(len & 0xFF);
	} else {
		hdr[hdr_len++] = (unsigned char)((mask ? 0x80 : 0) | 127);
		/* 64-bit length (we only use lower 32 bits) */
		hdr[hdr_len++] = 0;
		hdr[hdr_len++] = 0;
		hdr[hdr_len++] = 0;
		hdr[hdr_len++] = 0;
		hdr[hdr_len++] = (unsigned char)((len >> 24) & 0xFF);
		hdr[hdr_len++] = (unsigned char)((len >> 16) & 0xFF);
		hdr[hdr_len++] = (unsigned char)((len >> 8) & 0xFF);
		hdr[hdr_len++] = (unsigned char)(len & 0xFF);
	}

	/* masking key (clients only) */
	unsigned char mask_key[4] = {0, 0, 0, 0};
	if (mask) {
		/* simple pseudo-random mask */
		uint32_t seed = (uint32_t)(len ^ (uint32_t)wsi->fd ^ 0xDEAD);
		mask_key[0] = (unsigned char)(seed & 0xFF);
		mask_key[1] = (unsigned char)((seed >> 8) & 0xFF);
		mask_key[2] = (unsigned char)((seed >> 16) & 0xFF);
		mask_key[3] = (unsigned char)((seed >> 24) & 0xFF);
		memcpy(&hdr[hdr_len], mask_key, 4);
		hdr_len += 4;
	}

	/* send header */
	lws_ssl_write(wsi, hdr, hdr_len);

	/* send payload (masked if client) */
	if (len > 0) {
		if (mask) {
			/* mask in-place would corrupt user buffer, send in chunks */
			unsigned char chunk[512];
			size_t remaining = len;
			size_t offset = 0;

			while (remaining > 0) {
				size_t take = remaining < sizeof(chunk) ?
					remaining : sizeof(chunk);
				size_t j;
				for (j = 0; j < take; j++)
					chunk[j] = buf[offset + j] ^
						mask_key[(offset + j) & 3];
				lws_ssl_write(wsi, chunk, (int)take);
				offset += take;
				remaining -= take;
			}
		} else {
			lws_ssl_write(wsi, buf, (int)len);
		}
	}

	return (int)len;
}

/* ---------- public write API ---------- */

int lws_write(struct lws *wsi, unsigned char *buf, size_t len,
		enum lws_write_protocol wp)
{
	if (!wsi || wsi->fd < 0)
		return -1;

	if (wsi->state != LWS_CONN_STATE_ESTABLISHED &&
	    wp != LWS_WRITE_HTTP && wp != LWS_WRITE_HTTP_FINAL &&
	    wp != LWS_WRITE_HTTP_HEADERS && wp != LWS_WRITE_RAW)
		return -1;

	return lws_ws_frame_write(wsi, buf, len, wp);
}

int lws_read(struct lws *wsi, unsigned char *buf, size_t len)
{
	if (!wsi || wsi->fd < 0)
		return -1;

	int n = lws_ssl_read(wsi, buf, (int)len);
	if (n > 0 && wsi->state == LWS_CONN_STATE_ESTABLISHED)
		lws_ws_frame_parse(wsi, buf, n);

	return n;
}

int lws_close_reason(struct lws *wsi, enum lws_close_status status,
		unsigned char *buf, size_t len)
{
	unsigned char close_buf[128];
	int close_len = 0;

	if (!wsi)
		return -1;

	/* 2-byte status code */
	close_buf[close_len++] = (unsigned char)((status >> 8) & 0xFF);
	close_buf[close_len++] = (unsigned char)(status & 0xFF);

	/* optional reason text */
	if (buf && len > 0 && len < sizeof(close_buf) - 2) {
		memcpy(&close_buf[close_len], buf, len);
		close_len += (int)len;
	}

	lws_ws_frame_write(wsi, close_buf, (size_t)close_len, LWS_WRITE_CLOSE);
	wsi->close_sent = 1;

	return 0;
}

/* ---------- frame info ---------- */

int lws_frame_is_binary(struct lws *wsi)
{
	return wsi ? wsi->rx_is_binary : 0;
}

size_t lws_remaining_packet_payload(struct lws *wsi)
{
	return wsi ? (size_t)wsi->rx_payload_remaining : 0;
}

int lws_is_final_fragment(struct lws *wsi)
{
	return wsi ? wsi->rx_final : 1;
}

unsigned char lws_get_reserved_bits(struct lws *wsi)
{
	(void)wsi;
	return 0;
}
