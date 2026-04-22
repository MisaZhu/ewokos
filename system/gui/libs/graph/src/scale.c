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

    int src_w = g->w;
    int src_h = g->h;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int is_downscale = (scale < 1.0f);

    if (!is_downscale) {
        int hmax = g->h - 1;
        int wmax = g->w - 1;
        uint32_t inv_scale = (uint32_t)((1.0f / scale) * FIXED_SCALE);

        for(int i = 0; i < dst_h; i++) {
            uint32_t gi = (i * inv_scale) >> FIXED_SHIFT;
            int gi0 = (int)gi;
            uint32_t gi_frac = (i * inv_scale) & FIXED_MASK;
            int gi1 = gi0 + 1;

            if(gi0 < 0) { gi0 = 0; gi1 = 0; gi_frac = 0; }
            else if(gi0 >= hmax) { gi0 = hmax; gi1 = hmax; gi_frac = 0; }
            if(gi1 > hmax) gi1 = hmax;

            int gi0w = gi0 * g->w;
            int gi1w = gi1 * g->w;

            for(int j = 0; j < dst_w; j++) {
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
                    dst->buffer[i * dst_w + j] = p00;
                    continue;
                }

                dst->buffer[i * dst_w + j] = bilinear_interp_u8(p00, p01, p10, p11, gj_frac, gi_frac);
            }
        }
    } else {
        float src_scale = 1.0f / scale;
        for(int i = 0; i < dst_h; i++) {
            float src_start_y = i * src_scale;
            float src_end_y = (i + 1) * src_scale;
            int src_y_start = (int)src_start_y;
            int src_y_end = (int)src_end_y;
            if (src_y_start < 0) src_y_start = 0;
            if (src_y_end >= src_h) src_y_end = src_h - 1;

            for(int j = 0; j < dst_w; j++) {
                float src_start_x = j * src_scale;
                float src_end_x = (j + 1) * src_scale;
                int src_x_start = (int)src_start_x;
                int src_x_end = (int)src_end_x;
                if (src_x_start < 0) src_x_start = 0;
                if (src_x_end >= src_w) src_x_end = src_w - 1;

                uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                uint32_t count = 0;

                for (int sy = src_y_start; sy <= src_y_end; sy++) {
                    int row_offset = sy * src_w;
                    for (int sx = src_x_start; sx <= src_x_end; sx++) {
                        uint32_t p = g->buffer[row_offset + sx];
                        sum_r += (p >> 16) & 0xFF;
                        sum_g += (p >> 8) & 0xFF;
                        sum_b += p & 0xFF;
                        sum_a += (p >> 24) & 0xFF;
                        count++;
                    }
                }

                if (count > 0) {
                    uint32_t r = sum_r / count;
                    uint32_t g = sum_g / count;
                    uint32_t b = sum_b / count;
                    uint32_t a = sum_a / count;
                    dst->buffer[i * dst_w + j] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
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

void graph_scale_fit_tof_cpu(graph_t* src, graph_t* dst) {
    if (src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return;

    int src_w = src->w;
    int src_h = src->h;
    int dst_w = dst->w;
    int dst_h = dst->h;

    if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
        return;

    float scale_x = (float)src_w / (float)dst_w;
    float scale_y = (float)src_h / (float)dst_h;
    int is_downscale = (scale_x > 1.0f || scale_y > 1.0f);

    if (!is_downscale) {
        uint32_t scale_x_f = ((uint64_t)src_w * FIXED_SCALE) / dst_w;
        uint32_t scale_y_f = ((uint64_t)src_h * FIXED_SCALE) / dst_h;

        int src_hmax = src_h - 1;
        int src_wmax = src_w - 1;

        for (int y = 0; y < dst_h; y++) {
            uint32_t src_y = (y * scale_y_f);
            int src_y0 = src_y >> FIXED_SHIFT;
            uint32_t src_y_frac = src_y & FIXED_MASK;
            int src_y1 = src_y0 + 1;

            if (src_y0 < 0) { src_y0 = 0; src_y1 = 0; src_y_frac = 0; }
            else if (src_y0 >= src_hmax) { src_y0 = src_hmax; src_y1 = src_hmax; src_y_frac = 0; }
            if (src_y1 > src_hmax) src_y1 = src_hmax;

            int src_y0_offset = src_y0 * src_w;
            int src_y1_offset = src_y1 * src_w;

            for (int x = 0; x < dst_w; x++) {
                uint32_t src_x = (x * scale_x_f);
                int src_x0 = src_x >> FIXED_SHIFT;
                uint32_t src_x_frac = src_x & FIXED_MASK;
                int src_x1 = src_x0 + 1;

                if (src_x0 < 0) { src_x0 = 0; src_x1 = 0; src_x_frac = 0; }
                else if (src_x0 >= src_wmax) { src_x0 = src_wmax; src_x1 = src_wmax; src_x_frac = 0; }
                if (src_x1 > src_wmax) src_x1 = src_wmax;

                uint32_t p00 = src->buffer[src_y0_offset + src_x0];
                uint32_t p01 = src->buffer[src_y0_offset + src_x1];
                uint32_t p10 = src->buffer[src_y1_offset + src_x0];
                uint32_t p11 = src->buffer[src_y1_offset + src_x1];

                if (p00 == p01 && p00 == p10 && p00 == p11) {
                    dst->buffer[y * dst_w + x] = p00;
                    continue;
                }

                dst->buffer[y * dst_w + x] = bilinear_interp_u8(p00, p01, p10, p11, src_x_frac, src_y_frac);
            }
        }
    } else {
        for (int y = 0; y < dst_h; y++) {
            float src_start_y = y * scale_y;
            float src_end_y = (y + 1) * scale_y;
            int src_y_start = (int)src_start_y;
            int src_y_end = (int)src_end_y;
            if (src_y_start < 0) src_y_start = 0;
            if (src_y_end >= src_h) src_y_end = src_h - 1;

            for (int x = 0; x < dst_w; x++) {
                float src_start_x = x * scale_x;
                float src_end_x = (x + 1) * scale_x;
                int src_x_start = (int)src_start_x;
                int src_x_end = (int)src_end_x;
                if (src_x_start < 0) src_x_start = 0;
                if (src_x_end >= src_w) src_x_end = src_w - 1;

                uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                uint32_t count = 0;

                for (int sy = src_y_start; sy <= src_y_end; sy++) {
                    int row_offset = sy * src_w;
                    for (int sx = src_x_start; sx <= src_x_end; sx++) {
                        uint32_t p = src->buffer[row_offset + sx];
                        sum_r += (p >> 16) & 0xFF;
                        sum_g += (p >> 8) & 0xFF;
                        sum_b += p & 0xFF;
                        sum_a += (p >> 24) & 0xFF;
                        count++;
                    }
                }

                if (count > 0) {
                    uint32_t r = sum_r / count;
                    uint32_t g = sum_g / count;
                    uint32_t b = sum_b / count;
                    uint32_t a = sum_a / count;
                    dst->buffer[y * dst_w + x] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
        }
    }
}

void graph_scale_fit_tof(graph_t* src, graph_t* dst) {
#if BSP_BOOST
    graph_scale_fit_tof_cpu(src, dst);
#else
    graph_scale_fit_tof_cpu(src, dst);
#endif
}

graph_t* graph_scale_fitf(graph_t* g, int32_t w, int32_t h) {
	graph_t* ret = NULL;
    if(w <= 0 || h <= 0)
        return NULL;

	ret = graph_new(NULL, w, h);
	if(ret == NULL)
		return NULL;
	graph_scale_fit_tof(g, ret);
	return ret;
}

#ifdef __cplusplus
}
#endif
