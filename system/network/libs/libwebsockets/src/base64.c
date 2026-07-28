/**
 * Base64 encode/decode for libwebsockets
 */

#include "private.h"

static const char b64_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t lws_b64_encode_string(const char *in, int in_len, char *out, int out_size)
{
	int i, j = 0;
	unsigned char triple[3];
	int remaining;

	for (i = 0; i < in_len; ) {
		remaining = in_len - i;
		int take = remaining < 3 ? remaining : 3;

		memset(triple, 0, 3);
		memcpy(triple, &in[i], take);

		if (j + 4 >= out_size)
			break;

		out[j++] = b64_table[(triple[0] >> 2) & 0x3F];
		out[j++] = b64_table[((triple[0] & 0x03) << 4) | ((triple[1] >> 4) & 0x0F)];
		out[j++] = (take > 1) ? b64_table[((triple[1] & 0x0F) << 2) | ((triple[2] >> 6) & 0x03)] : '=';
		out[j++] = (take > 2) ? b64_table[triple[2] & 0x3F] : '=';

		i += take;
	}

	if (j < out_size)
		out[j] = '\0';

	return (size_t)j;
}

static int b64_decode_char(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

int lws_b64_decode_string(const char *in, char *out, int out_size)
{
	int len = 0;
	int i = 0;
	int in_len = (int)strlen(in);

	while (i < in_len && len < out_size - 1) {
		int a = -1, b = -1, c = -1, d = -1;

		while (i < in_len && (a = b64_decode_char(in[i])) < 0) i++;
		i++;
		while (i < in_len && (b = b64_decode_char(in[i])) < 0) i++;
		i++;
		while (i < in_len && in[i] != '=' && (c = b64_decode_char(in[i])) < 0) i++;
		if (in[i] != '=') i++;
		while (i < in_len && in[i] != '=' && (d = b64_decode_char(in[i])) < 0) i++;
		if (i < in_len && in[i] != '=') i++;

		if (a < 0 || b < 0)
			break;

		if (len < out_size)
			out[len++] = (char)((a << 2) | (b >> 4));
		if (c >= 0 && len < out_size)
			out[len++] = (char)((b << 4) | (c >> 2));
		if (d >= 0 && len < out_size)
			out[len++] = (char)((c << 6) | d);
	}

	if (len < out_size)
		out[len] = '\0';

	return len;
}
