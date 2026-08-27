#include <graph/graph_arch.h>
#include <g2d_arch.h>
#include <stdint.h>
#include <string.h>
#include <ewokos_config.h>

#ifdef ARCH_BOOST
#include <emmintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void graph_fill_cpu(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void graph_blt_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void graph_blt_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);
void graph_blt_mask_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale);
void graph_rotate_to_cpu(graph_t* g, graph_t* dst, int rot);
void graph_gaussian_cpu(graph_t* g, int x, int y, int w, int h, int r);
void graph_glass_cpu(graph_t* g, int x, int y, int w, int h, int r);

#ifdef ARCH_BOOST
static inline void x86_stream_copy_row(uint32_t* dst, const uint32_t* src, int32_t pixels) {
    int32_t i = 0;

    if (pixels <= 0) {
        return;
    }

    /* Streaming stores require aligned destinations; handle the prefix first. */
    while (i < pixels && (((ewokos_addr_t)(dst + i)) & 0x0F) != 0) {
        dst[i] = src[i];
        ++i;
    }

    for (; i + 16 <= pixels; i += 16) {
        _mm_stream_si128((__m128i*)(dst + i + 0), _mm_loadu_si128((const __m128i*)(src + i + 0)));
        _mm_stream_si128((__m128i*)(dst + i + 4), _mm_loadu_si128((const __m128i*)(src + i + 4)));
        _mm_stream_si128((__m128i*)(dst + i + 8), _mm_loadu_si128((const __m128i*)(src + i + 8)));
        _mm_stream_si128((__m128i*)(dst + i + 12), _mm_loadu_si128((const __m128i*)(src + i + 12)));
    }

    for (; i + 4 <= pixels; i += 4) {
        _mm_stream_si128((__m128i*)(dst + i), _mm_loadu_si128((const __m128i*)(src + i)));
    }

    for (; i < pixels; ++i) {
        dst[i] = src[i];
    }
}

static inline int x86_can_stream_blt(graph_t* src, const grect_t* sr,
        graph_t* dst, const grect_t* dr) {
    const uint32_t* src_begin;
    const uint32_t* src_end;
    uint32_t* dst_begin;
    uint32_t* dst_end;

    if (src == NULL || dst == NULL) {
        return 0;
    }
    if (src == dst) {
        return 0;
    }
    if (sr->w != dr->w || sr->h != dr->h) {
        return 0;
    }
    if (sr->w < 64) {
        return 0;
    }

    src_begin = src->buffer + sr->y * src->w + sr->x;
    src_end = src->buffer + (sr->y + sr->h - 1) * src->w + sr->x + sr->w;
    dst_begin = dst->buffer + dr->y * dst->w + dr->x;
    dst_end = dst->buffer + (dr->y + dr->h - 1) * dst->w + dr->x + dr->w;

    if (!(dst_end <= src_begin || src_end <= dst_begin)) {
        return 0;
    }
    return 1;
}

/* Fill a run of pixels with a constant color. A scalar head of at most
   3 pixels brings the destination to a 16-byte boundary so the streaming
   stores stay aligned; the caller issues one _mm_sfence after all runs. */
static inline void x86_fill_run(uint32_t* dst, int32_t pixels, uint32_t color, __m128i vc) {
    int32_t i = 0;

    if (pixels <= 0) {
        return;
    }

    while (i < pixels && (((ewokos_addr_t)(dst + i)) & 0x0F) != 0) {
        dst[i] = color;
        ++i;
    }

    for (; i + 16 <= pixels; i += 16) {
        _mm_stream_si128((__m128i*)(dst + i + 0), vc);
        _mm_stream_si128((__m128i*)(dst + i + 4), vc);
        _mm_stream_si128((__m128i*)(dst + i + 8), vc);
        _mm_stream_si128((__m128i*)(dst + i + 12), vc);
    }

    for (; i + 4 <= pixels; i += 4) {
        _mm_stream_si128((__m128i*)(dst + i), vc);
    }

    for (; i < pixels; ++i) {
        dst[i] = color;
    }
}

static inline void x86_fill_opaque(graph_t* g, const grect_t* r, uint32_t color) {
    uint8_t cb = (uint8_t)color;
    int same_bytes = (((color >> 8) & 0xff) == cb) &&
            (((color >> 16) & 0xff) == cb) &&
            (((color >> 24) & 0xff) == cb);

    /* whole-width rectangles are one contiguous block in memory */
    int full_rows = (r->x == 0 && r->w == g->w);

    if (same_bytes) {
        /* all four channel bytes equal: a byte memset fills whole pixels */
        if (full_rows) {
            memset(g->buffer + r->y * g->w, cb, (size_t)r->w * r->h * 4);
        } else {
            for (int32_t row = 0; row < r->h; ++row) {
                memset(g->buffer + (r->y + row) * g->w + r->x, cb, (size_t)r->w * 4);
            }
        }
        return;
    }

    __m128i vc = _mm_set1_epi32((int)color);
    if (full_rows) {
        x86_fill_run(g->buffer + r->y * g->w, r->w * r->h, color, vc);
    } else {
        for (int32_t row = 0; row < r->h; ++row) {
            x86_fill_run(g->buffer + (r->y + row) * g->w + r->x, r->w, color, vc);
        }
    }
    _mm_sfence();
}
#endif

void graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
#ifdef ARCH_BOOST
    if(g == NULL || w <= 0 || h <= 0)
        return;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);
    if(r.w <= 0 || r.h <= 0)
        return;

    if(color_a(color) == 0xff) {
        x86_fill_opaque(g, &r, color);
        return;
    }

    /* translucent fill: handled by the g2d engine alpha fill (simd rows
       with scalar head/tail alignment, same blend math as blit alpha) */
    arch_g2d_fill_alpha(g->buffer, g->w, g->h, r.x, r.y, r.w, r.h, color);
#else
    graph_fill_cpu(g, x, y, w, h, color);
#endif
}

void graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
#ifdef ARCH_BOOST
    if(sw > 0 && sh > 0 && dw > 0 && dh > 0) {
        grect_t sr = {sx, sy, sw, sh};
        grect_t dr = {dx, dy, dw, dh};
        graph_insect(dst, &dr);
        if(dst->clip.w > 0 && dst->clip.h > 0)
            grect_insect(&dst->clip, &dr);

        if(graph_insect_with(src, &sr, dst, &dr)) {
            if(dx < 0)
                sr.x -= dx;
            if(dy < 0)
                sr.y -= dy;

            if(x86_can_stream_blt(src, &sr, dst, &dr)) {
                for(int32_t row = 0; row < sr.h; ++row) {
                    uint32_t* dst_row = dst->buffer + (dr.y + row) * dst->w + dr.x;
                    const uint32_t* src_row = src->buffer + (sr.y + row) * src->w + sr.x;
                    x86_stream_copy_row(dst_row, src_row, sr.w);
                }
                _mm_sfence();
                return;
            }
        }
    }
#endif
    graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

void graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    graph_blt_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
}

void graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    graph_blt_mask_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

void graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale) {
    graph_scale_tof_cpu(g, dst, (float)scale);
}

void graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    graph_scale_tof_cpu(g, dst, (float)scale);
}

void graph_rotate_to_arch(graph_t* g, graph_t* dst, int rot) {
    graph_rotate_to_cpu(g, dst, rot);
}

void graph_glass_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_glass_cpu(g, x, y, w, h, r);
}

void graph_gaussian_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_gaussian_cpu(g, x, y, w, h, r);
}

#ifdef __cplusplus
}
#endif
