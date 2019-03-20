#include "base16.h"

void base16_encode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen) {
	i32_t i;
	for (i = 0; i < srcLen; i++) {
		dst[2 * i] = (src[i] & 0xF) + 'A';
		dst[2 * i + 1] = ((src[i] >> 4) & 0xF) + 'A';
	}

	*dstLen = 2 * srcLen;
}

void base16_decode(const char *src, i32_t srcLen, char *dst, i32_t *dstLen) {
	i32_t i;
	for (i = 0; i < srcLen / 2; i++) {
		i32_t low = src[2 * i] - 'A';
		i32_t high = src[2 * i + 1] - 'A';
		char c = (char)(low | (high << 4));
		dst[i] = c;
	}

	*dstLen = srcLen / 2;
}
