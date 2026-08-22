#include "graph/rgb15.h"
#include "graph/graph_arch.h"
/*
*	ARGB8888 -> XRGB1555: keep the top 5 bits of each channel,
*	packed as 0RRRRRGGGGGBBBBB (bit 15 unused).
*	Walks the source backwards like argb_2_nv12 does, so the output
*	is rotated 180 degrees for the same panel scan order.
*/
static inline uint16_t rgb2rgb15_pixel(uint8_t *rgb){
    register uint32_t b = rgb[0];
    register uint32_t g = rgb[1];
    register uint32_t r = rgb[2];
    return (uint16_t)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

void rgb2rgb15_cpu(uint16_t  *out,  uint32_t *in , int w, int h)
{
    uint16_t *p = out;
    uint32_t *rgb = (in + w * h);

    for(int i = 0; i < h; i++){
        for( int j = 0; j < w; j++){
            rgb--;
            *p = rgb2rgb15_pixel((uint8_t*)rgb);
            p++;
        }
    }

    return;
}

inline void argb_2_rgb15(uint16_t  *out,  uint32_t *in , int w, int h) {
#ifdef ARCH_BOOST
    argb_2_rgb15_arch(out,  in , w, h);
#else
    rgb2rgb15_cpu(out,  in , w, h);
#endif
}

/*
*	XRGB1555 -> ARGB8888: expand each 5-bit channel to 8 bits
*	via (x << 3) | (x >> 2), alpha fixed at 0xFF.
*	Straight linear scan, no rotation.
*/
static inline uint32_t rgb15_pixel_to_argb(uint16_t v) {
	uint32_t r = (v >> 10) & 0x1f;
	uint32_t g = (v >>  5) & 0x1f;
	uint32_t b =  v        & 0x1f;
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	return 0xff000000u | (r << 16) | (g << 8) | b;
}

static void rgb15_2_argb_cpu(uint32_t *out, uint16_t *in, int w, int h)
{
	for (int i = 0; i < w * h; i++)
		out[i] = rgb15_pixel_to_argb(in[i]);
}

void rgb15_2_argb(uint32_t *out, uint16_t *in, int w, int h) {
#ifdef ARCH_BOOST
	rgb15_2_argb_arch(out, in, w, h);
#else
	rgb15_2_argb_cpu(out, in, w, h);
#endif
}

/*
 *	Big-endian XRGB1555 byte stream -> ARGB8888.
 *	Source is raw bytes: [hi][lo] per pixel, big-endian order.
 *	No intermediate buffer or byte-swap needed.
 */
static void rgb15be_2_argb_cpu(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
	for (int y = 0; y < h; y++) {
		const uint8_t *src = in + y * bpr;
		uint32_t *dst = out + y * w;
		for (int x = 0; x < w; x++) {
			uint32_t v = ((uint32_t)src[x * 2] << 8) | src[x * 2 + 1];
			uint32_t r = (v >> 10) & 0x1f;
			uint32_t g = (v >>  5) & 0x1f;
			uint32_t b =  v        & 0x1f;
			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);
			dst[x] = 0xff000000u | (r << 16) | (g << 8) | b;
		}
	}
}

__attribute__((weak))
void rgb15be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
	rgb15be_2_argb_cpu(out, in, bpr, w, h);
}

void rgb15be_2_argb(uint32_t *out, const uint8_t *in, int bpr, int w, int h) {
#ifdef ARCH_BOOST
	rgb15be_2_argb_arch(out, in, bpr, w, h);
#else
	rgb15be_2_argb_cpu(out, in, bpr, w, h);
#endif
}
