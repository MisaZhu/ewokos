#include <graph/graph.h>
#include <graph/graph_arch.h>
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

/* Interpolate two packed channels (bytes 0 and 2 of a 32-bit word) with an
   8-bit weight: w0 + w1 = 256 keeps each 16-bit lane carry-free, since the
   per-lane sum is at most 255*256 = 0xFF00 */
static inline uint32_t scale_lerp_rb(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1) >> 8) & 0x00FF00FF);
}

static inline uint32_t scale_lerp_ga(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1) >> 8)
            & 0x00FF00FF) << 8);
}

/* Bilinear with pre-quantized 8-bit fractional weights (0..256):
   two-pass lerp over channel pairs, 8x32-bit multiplies total */
static inline uint32_t bilinear_interp_w8(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                                          uint32_t fx8, uint32_t fy8) {
    uint32_t top_rb = scale_lerp_rb(p00, p01, fx8);
    uint32_t top_ga = scale_lerp_ga(p00, p01, fx8);
    uint32_t bot_rb = scale_lerp_rb(p10, p11, fx8);
    uint32_t bot_ga = scale_lerp_ga(p10, p11, fx8);
    return scale_lerp_rb(top_rb, bot_rb, fy8) | scale_lerp_ga(top_ga, bot_ga, fy8);
}

static inline uint32_t bilinear_interp_u8(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
                                          uint32_t fx, uint32_t fy) {
    /* Quantize 16-bit fractions to rounded 8-bit weights (max +/-1 LSB),
       replacing 16x64-bit multiplies and per-channel unpacking */
    return bilinear_interp_w8(p00, p01, p10, p11, (fx + 128) >> 8, (fy + 128) >> 8);
}

/**
 * @brief Uniform image scaling, fixed-point bilinear. scale is converted to
 *        16.16 fixed point once at entry; no float in the loops.
 *        Column mapping is row-independent and precomputed once, with 8-bit
 *        quantized weights, so the inner loop is pure loads + packed lerps.
 * @param g source image
 * @param dst output image, w/h must be pre-set to round(g->w*scale), round(g->h*scale)
 * @param scale scaling factor
 */
void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale)
{
    if(!g || !dst || !g->buffer || !dst->buffer) return;
    if(scale <= 0.0f) return;

    int src_w = g->w;
    int src_h = g->h;
    int dst_w = dst->w;
    int dst_h = dst->h;

    if(src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0) return;

    uint32_t inv_scale_f = (uint32_t)(FIXED_SCALE / scale);

    uint32_t *src_buf = g->buffer;
    uint32_t *out_buf = dst->buffer;

    int src_wmax = src_w - 1;
    int src_hmax = src_h - 1;

    /* 1:1 fast path: exact copy */
    if(inv_scale_f == FIXED_SCALE && dst_w == src_w && dst_h == src_h) {
        memcpy(out_buf, src_buf, (size_t)src_w * (size_t)src_h * sizeof(uint32_t));
        return;
    }

    /* Column mapping depends only on ox: precompute once for all rows */
    int *x0 = (int*)malloc((size_t)dst_w * sizeof(int));
    int *x1 = (int*)malloc((size_t)dst_w * sizeof(int));
    uint16_t *fx8_arr = (uint16_t*)malloc((size_t)dst_w * sizeof(uint16_t));

    if(x0 != NULL && x1 != NULL && fx8_arr != NULL) {
        uint32_t sx_f = 0;
        for(int ox = 0; ox < dst_w; ox++) {
            int cx0 = (int)(sx_f >> FIXED_SHIFT);
            uint32_t fx = sx_f & FIXED_MASK;

            if(cx0 >= src_wmax) { cx0 = src_wmax; fx = 0; }
            x0[ox] = cx0;
            x1[ox] = (cx0 < src_wmax) ? cx0 + 1 : src_wmax;
            fx8_arr[ox] = (uint16_t)((fx + 128) >> 8); /* 0..256, rounded */
            sx_f += inv_scale_f;
        }

        uint32_t sy_f = 0;
        for(int oy = 0; oy < dst_h; oy++) {
            int y0 = (int)(sy_f >> FIXED_SHIFT);
            uint32_t fy = sy_f & FIXED_MASK;

            if(y0 >= src_hmax) { y0 = src_hmax; fy = 0; }
            int y1 = (y0 < src_hmax) ? y0 + 1 : src_hmax;
            uint32_t fy8 = (fy + 128) >> 8;

            const uint32_t *row0 = src_buf + y0 * src_w;
            const uint32_t *row1 = src_buf + y1 * src_w;
            uint32_t *drow = out_buf + oy * dst_w;

            for(int ox = 0; ox < dst_w; ox++) {
                int cx0 = x0[ox];
                int cx1 = x1[ox];

                uint32_t p00 = row0[cx0];
                uint32_t p01 = row0[cx1];
                uint32_t p10 = row1[cx0];
                uint32_t p11 = row1[cx1];

                if(p00 == p01 && p00 == p10 && p00 == p11) {
                    drow[ox] = p00;
                    continue;
                }

                drow[ox] = bilinear_interp_w8(p00, p01, p10, p11, fx8_arr[ox], fy8);
            }

            sy_f += inv_scale_f;
        }

        free(x0);
        free(x1);
        free(fx8_arr);
        return;
    }

    free(x0);
    free(x1);
    free(fx8_arr);

    /* Fallback: incremental fixed-point stepping, no per-pixel multiply */
    for(int oy = 0; oy < dst_h; oy++)
    {
        // sy_f = oy * (1/scale), source Y coordinate in fixed point
        uint32_t sy_f = (uint32_t)oy * inv_scale_f;
        int y0 = (int)(sy_f >> FIXED_SHIFT);
        uint32_t fy = sy_f & FIXED_MASK;

        if(y0 >= src_hmax) { y0 = src_hmax; fy = 0; }
        int y1 = (y0 < src_hmax) ? y0 + 1 : src_hmax;

        const uint32_t *row0 = src_buf + y0 * src_w;
        const uint32_t *row1 = src_buf + y1 * src_w;
        uint32_t *drow = out_buf + oy * dst_w;

        uint32_t sx_f = 0;
        for(int ox = 0; ox < dst_w; ox++)
        {
            int x0 = (int)(sx_f >> FIXED_SHIFT);
            uint32_t fx = sx_f & FIXED_MASK;

            if(x0 >= src_wmax) { x0 = src_wmax; fx = 0; }
            int x1 = (x0 < src_wmax) ? x0 + 1 : src_wmax;

            uint32_t p00 = row0[x0];
            uint32_t p01 = row0[x1];
            uint32_t p10 = row1[x0];
            uint32_t p11 = row1[x1];

            if(p00 == p01 && p00 == p10 && p00 == p11) {
                drow[ox] = p00;
            } else {
                drow[ox] = bilinear_interp_u8(p00, p01, p10, p11, fx, fy);
            }

            sx_f += inv_scale_f;
        }
    }
}

inline void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
    if(graph_g2d_avaliable(g) == 0 && graph_g2d_avaliable(dst) == 0) {
        graph_scale_tof_g2d(g, dst, scale);
        return;
    }

#if ARCH_BOOST
    //graph_scale_tof_cpu(g, dst, scale);
    graph_scale_tof_arch(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

inline void graph_scale_tof_fast(graph_t* g, graph_t* dst, float scale) {
    if(graph_g2d_avaliable(g) == 0 && graph_g2d_avaliable(dst) == 0) {
        graph_scale_tof_g2d(g, dst, scale);
        return;
    }
#if ARCH_BOOST
    //graph_scale_tof_cpu(g, dst, scale);
    graph_scale_tof_fast_arch(g, dst, scale);
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
#if ARCH_BOOST
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
