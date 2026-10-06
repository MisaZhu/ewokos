#include "graph/rgb24.h"
#include "graph/graph_arch.h"

/*
 *  ARGB8888 -> RGB24: strip the alpha byte, keep 0x00RRGGBB.
 *  Straight linear scan, no rotation.
 */
static void argb_2_rgb24_cpu(uint32_t *out, uint32_t *in, int w, int h)
{
	for (int i = 0; i < w * h; i++)
		out[i] = in[i] & 0x00ffffffu;
}

/*
 *  RGB24 -> ARGB8888: set alpha to 0xFF, keep the lower 24 bits.
 *  Straight linear scan, no rotation.
 */
static void rgb24_2_argb_cpu(uint32_t *out, uint32_t *in, int w, int h)
{
	for (int i = 0; i < w * h; i++)
		out[i] = 0xff000000u | (in[i] & 0x00ffffffu);
}

void argb_2_rgb24(uint32_t *out, uint32_t *in, int w, int h) {
#ifdef ARCH_BOOST
	if(argb_2_rgb24_arch(out, in, w, h) == 0)
		return;
#endif
	argb_2_rgb24_cpu(out, in, w, h);
}

void rgb24_2_argb(uint32_t *out, uint32_t *in, int w, int h) {
#ifdef ARCH_BOOST
	if(rgb24_2_argb_arch(out, in, w, h) == 0)
		return;
#endif
	rgb24_2_argb_cpu(out, in, w, h);
}

/*
 *  Big-endian [00][RR][GG][BB] byte stream -> ARGB8888.
 *  Source is raw bytes in big-endian order.
 *  No intermediate buffer or byte-swap needed.
 */
static void rgb24be_2_argb_cpu(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
	for (int y = 0; y < h; y++) {
		const uint8_t *src = in + y * bpr;
		uint32_t *dst = out + y * w;
		for (int x = 0; x < w; x++) {
			dst[x] = 0xff000000u |
			         ((uint32_t)src[x * 4 + 1] << 16) |
			         ((uint32_t)src[x * 4 + 2] <<  8) |
			          (uint32_t)src[x * 4 + 3];
		}
	}
}

void rgb24be_2_argb(uint32_t *out, const uint8_t *in, int bpr, int w, int h) {
#ifdef ARCH_BOOST
	if(rgb24be_2_argb_arch(out, in, bpr, w, h) == 0)
		return;
#endif
	rgb24be_2_argb_cpu(out, in, bpr, w, h);
}
