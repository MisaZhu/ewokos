#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define FIXED_SHIFT 16
#define FIXED_SCALE (1 << FIXED_SHIFT)
#define FIXED_MASK (FIXED_SCALE - 1)

static inline uint32_t bilinear_interp_u8(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                                          uint32_t fx, uint32_t fy) {
    uint32_t one_minus_fx = FIXED_SCALE - fx;
    uint32_t one_minus_fy = FIXED_SCALE - fy;

    uint32_t r00 = (p00 >> 16) & 0xFF;
    uint32_t g00 = (p00 >> 8) & 0xFF;
    uint32_t b00 = p00 & 0xFF;
    uint32_t a00 = (p00 >> 24) & 0xFF;

    uint32_t r01 = (p01 >> 16) & 0xFF;
    uint32_t g01 = (p01 >> 8) & 0xFF;
    uint32_t b01 = p01 & 0xFF;
    uint32_t a01 = (p01 >> 24) & 0xFF;

    uint32_t r10 = (p10 >> 16) & 0xFF;
    uint32_t g10 = (p10 >> 8) & 0xFF;
    uint32_t b10 = p10 & 0xFF;
    uint32_t a10 = (p10 >> 24) & 0xFF;

    uint32_t r11 = (p11 >> 16) & 0xFF;
    uint32_t g11 = (p11 >> 8) & 0xFF;
    uint32_t b11 = p11 & 0xFF;
    uint32_t a11 = (p11 >> 24) & 0xFF;

    uint64_t tmp_r = (uint64_t)one_minus_fx * one_minus_fy * r00 +
                     (uint64_t)fx * one_minus_fy * r01 +
                     (uint64_t)one_minus_fx * fy * r10 +
                     (uint64_t)fx * fy * r11;
    uint64_t tmp_g = (uint64_t)one_minus_fx * one_minus_fy * g00 +
                     (uint64_t)fx * one_minus_fy * g01 +
                     (uint64_t)one_minus_fx * fy * g10 +
                     (uint64_t)fx * fy * g11;
    uint64_t tmp_b = (uint64_t)one_minus_fx * one_minus_fy * b00 +
                     (uint64_t)fx * one_minus_fy * b01 +
                     (uint64_t)one_minus_fx * fy * b10 +
                     (uint64_t)fx * fy * b11;
    uint64_t tmp_a = (uint64_t)one_minus_fx * one_minus_fy * a00 +
                     (uint64_t)fx * one_minus_fy * a01 +
                     (uint64_t)one_minus_fx * fy * a10 +
                     (uint64_t)fx * fy * a11;

    uint32_t r = tmp_r >> (2 * FIXED_SHIFT);
    uint32_t g = tmp_g >> (2 * FIXED_SHIFT);
    uint32_t b = tmp_b >> (2 * FIXED_SHIFT);
    uint32_t a = tmp_a >> (2 * FIXED_SHIFT);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    int hmax = g->h - 1;
    int wmax = g->w - 1;
    uint32_t inv_scale = (uint32_t)((1.0f / scale) * FIXED_SCALE);

    for(int i = 0; i < dst->h; i++) {
        uint32_t gi = (i * inv_scale) >> FIXED_SHIFT;
        int gi0 = (int)gi;
        uint32_t gi_frac = (i * inv_scale) & FIXED_MASK;
        int gi1 = gi0 + 1;

        if(gi0 < 0) { gi0 = 0; gi1 = 0; gi_frac = 0; }
        else if(gi0 >= hmax) { gi0 = hmax; gi1 = hmax; gi_frac = 0; }
        if(gi1 > hmax) gi1 = hmax;

        int gi0w = gi0 * g->w;
        int gi1w = gi1 * g->w;

        for(int j = 0; j < dst->w; j++) {
            uint32_t gj = (j * inv_scale) >> FIXED_SHIFT;
            int gj0 = (int)gj;
            uint32_t gj_frac = (j * inv_scale) & FIXED_MASK;
            int gj1 = gj0 + 1;

            if(gj0 < 0) { gj0 = 0; gj1 = 0; gj_frac = 0; }
            else if(gj0 >= wmax) { gj0 = wmax; gj1 = wmax; gj_frac = 0; }
            if(gj1 > wmax) gj1 = wmax;

            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + gj1];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + gj1];

            if(p00 == p01 && p00 == p10 && p00 == p11) {
                dst->buffer[i * dst->w + j] = p00;
                continue;
            }

            dst->buffer[i * dst->w + j] = bilinear_interp_u8(p00, p01, p10, p11, gj_frac, gi_frac);
        }
    }
}

inline void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
#if BSP_BOOST
    graph_scale_tof_bsp(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

inline void graph_scale_tof_fast(graph_t* g, graph_t* dst, float scale) {
#if BSP_BOOST
    graph_scale_tof_fast_bsp(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

graph_t* graph_scalef_fast(graph_t* g, float scale) {
	graph_t* ret = NULL;
	if(scale <= 0.0)
		return NULL;
	ret = graph_new(NULL, g->w*scale, g->h*scale);
	if(ret == NULL)
		return NULL;
	graph_scale_tof_fast(g, ret, scale);
	return ret;
}

graph_t* graph_scalef(graph_t* g, float scale) {
	graph_t* ret = NULL;
	if(scale <= 0.0)
		return NULL;
	
	ret = graph_new(NULL, g->w*scale, g->h*scale);
	if(ret == NULL)
		return NULL;
	graph_scale_tof(g, ret, scale);
	return ret;
}

#ifdef __cplusplus
}
#endif
