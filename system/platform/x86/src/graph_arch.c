#include <graph/graph_arch.h>
#include <g2d_arch.h>
#include <stdint.h>
#include <string.h>
#include <ewokos_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARCH_BOOST
#include <emmintrin.h>

/* x86 has no g2dd daemon: every draw goes through these in-process hooks.
   Mirroring aarch64's graph_arch.c, this file is a thin delegation layer -
   each graph_*_arch clips exactly like its *_cpu reference and then hands a
   clean, fully in-bounds region to the matching arch_g2d_* SSE2 engine (or,
   for the alpha-mask blit, an inline SSE2 kernel just as aarch64 inlines its
   NEON mask). There are no cpu fallbacks: ARCH_BOOST is unconditional on x86
   and the *_cpu references are reached only through the non-BOOST dispatch in
   libgraph, never from here. The engines ignore phy/contig (x86 works on the
   virtual buffer), so 0,0 is passed for those. */

int graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if(g == NULL || g->buffer == NULL || w <= 0 || h <= 0)
        return -1;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return 0;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);
    if(r.w <= 0 || r.h <= 0)
        return 0;

    /* opaque fill: memset / SSE row stores; translucent: g2d alpha fill
       (simd rows with scalar head/tail alignment, same blend math as blt) */
    if(color_a(color) == 0xff)
        arch_g2d_fill(g->buffer, 0, 0, g->w, g->h, r.x, r.y, r.w, r.h, color);
    else
        arch_g2d_fill_alpha(g->buffer, g->w, g->h, r.x, r.y, r.w, r.h, color);
    return 0;
}

int graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return -1;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return -1;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return 0;

    if(dx < 0)
        sr.x -= dx;
    if(dy < 0)
        sr.y -= dy;

    /* 1:1 copy of the clipped region (like aarch64, the destination size is
       sr.w/sr.h); the engine keeps overlapping copies within one buffer safe */
    arch_g2d_blt(src->buffer, 0, 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, 0, 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h);
    return 0;
}

int graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return -1;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return -1;
    /* global alpha 0: nothing visible, skip entirely */
    if(alpha == 0)
        return 0;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return 0;

    if(dx < 0)
        sr.x -= dx;
    if(dy < 0)
        sr.y -= dy;

    /* 1:1 blend of the clipped region; the engine applies the same per-block
       transparent/opaque fast paths and div255 blend math as the cpu */
    arch_g2d_blt_alpha(src->buffer, 0, 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, 0, 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h, alpha);
    return 0;
}

/* Per-4-pixel alpha-mask combine, bit-identical to graph_blt_mask_cpu's inner
   op: where src alpha == 0 the destination pixel is cleared to 0; otherwise
   the destination RGB is kept and its alpha becomes min(dst_a, src_a).
     sa = src alpha (bits 0..7 after >>24); da = dst alpha
     mina = min(sa, da): SSE2 has no unsigned 16-bit min (_mm_min_epu16 is
       SSE4.1), but both operands hold an alpha in 0..255 in the low 16-bit
       lane (high lane zero), so the signed _mm_min_epi16 is identical here.
     nz = 0xFFFFFFFF where sa > 0, else 0  -> gates the whole pixel to 0 */
static inline __m128i x86_mask_alpha_4(__m128i S, __m128i D) {
    __m128i sa = _mm_srli_epi32(S, 24);
    __m128i da = _mm_srli_epi32(D, 24);
    __m128i mina = _mm_min_epi16(sa, da);
    __m128i anew = _mm_slli_epi32(mina, 24);
    __m128i rgb = _mm_and_si128(D, _mm_set1_epi32(0x00ffffff));
    __m128i nz = _mm_cmpgt_epi32(sa, _mm_setzero_si128());
    return _mm_and_si128(_mm_or_si128(rgb, anew), nz);
}

static inline uint32_t x86_mask_alpha_px(uint32_t s, uint32_t d) {
    uint8_t sa = (uint8_t)((s >> 24) & 0xff);
    uint8_t da = (uint8_t)((d >> 24) & 0xff);
    if(sa == 0)
        return 0;
    if(da > sa)
        return ((uint32_t)sa << 24) | (d & 0x00ffffff);
    return d;
}

int graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return -1;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return -1;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return 0;

    if(dx < 0)
        sr.x -= dx;
    if(dy < 0)
        sr.y -= dy;

    int32_t ex = sr.x + sr.w;
    int32_t ey = sr.y + sr.h;
    for(int32_t syi = sr.y, dyi = dr.y; syi < ey; ++syi, ++dyi) {
        const uint32_t* srow = src->buffer + (size_t)syi * src->w;
        uint32_t* drow = dst->buffer + (size_t)dyi * dst->w;
        int32_t sxi = sr.x;
        int32_t dxi = dr.x;

        /* scalar head brings the destination row start to a 16-byte boundary
           so the aligned temporal stores stay legal */
        for(; sxi < ex && (((ewokos_addr_t)(drow + dxi)) & 0x0F) != 0; ++sxi, ++dxi)
            drow[dxi] = x86_mask_alpha_px(srow[sxi], drow[dxi]);

        for(; sxi + 4 <= ex; sxi += 4, dxi += 4) {
            __m128i S = _mm_loadu_si128((const __m128i*)(srow + sxi));
            __m128i D = _mm_load_si128((const __m128i*)(drow + dxi));
            _mm_store_si128((__m128i*)(drow + dxi), x86_mask_alpha_4(S, D));
        }

        for(; sxi < ex; ++sxi, ++dxi)
            drow[dxi] = x86_mask_alpha_px(srow[sxi], drow[dxi]);
    }
    _mm_sfence();
    return 0;
}

int graph_gaussian_blur_arch(graph_t* g, int x, int y, int w, int h, int r) {
    if(g == NULL || g->buffer == NULL)
        return -1;
    /* replicate graph_gaussian_blur_cpu's clip so the engine sees the exact
       same intersected rect (the engine reproduces its box semantics) */
    grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return 0;
    arch_g2d_gaussian(g->buffer, 0, 0, g->w, g->h, ir.x, ir.y, ir.w, ir.h, r);
    return 0;
}

int graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL)
        return -1;
    if(scale <= 0.0 || g->w <= 0 || g->h <= 0 || dst->w <= 0 || dst->h <= 0)
        return -1;
    /* derive the 16.16 source step from the scale factor exactly the way
       graph_scale_tof_cpu does ((uint32_t)(FIXED_SCALE/scale)), NOT from the
       src/dst dimension ratio, so the SSE2 separable bilinear stays bit-exact */
    float fscale = (float)scale;
    uint32_t inv = (uint32_t)(65536.0f / fscale);
    arch_g2d_scale_inv(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, inv, inv);
    return 0;
}

int graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL)
        return -1;
    if(scale <= 0.0 || g->w <= 0 || g->h <= 0 || dst->w <= 0 || dst->h <= 0)
        return -1;
    float fscale = (float)scale;
    uint32_t inv = (uint32_t)(65536.0f / fscale);
    arch_g2d_scale_inv(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, inv, inv);
    return 0;
}

int graph_rotate_to_arch(graph_t* g, graph_t* dst, int rot) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL ||
            g->w <= 0 || g->h <= 0)
        return -1;

    /* the g2d engine writes the rotated surface into dst; make sure dst is
       large enough for the quadrant (90/270 swap the dimensions) */
    if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
        if(dst->w < g->h || dst->h < g->w)
            return -1;
    }
    else if(rot == G_ROTATE_180) {
        if(dst->w < g->w || dst->h < g->h)
            return -1;
    }
    else
        return -1;

    /* quadrant rot codes map 1:1 to clockwise degrees */
    arch_g2d_rotate(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, rot * 90);
    return 0;
}

/* ========================================================================
 * Pixel-format conversions (strong SSE2 overrides of the __attribute__((weak))
 * CPU defaults in libgraph uv12.c/rgb15.c/rgb24.c). SSE2 baseline only:
 * no SSSE3/SSE4.1 (no pshufb, no mullo_epi32, no blendv).
 *
 * argb_2_nv12 / argb_2_rgb15 walk the source 180-degrees rotated exactly like
 * the CPU backwards scan: output (y,x) reads in[(h-1-y)*w + (w-1-x)].
 * ====================================================================== */
static inline __m128i x86_rev_u32x4(__m128i v) {
    return _mm_shuffle_epi32(v, _MM_SHUFFLE(0, 1, 2, 3));
}

/* row_pixels[k] = base[-k] for k=0..15 (reversed 16-pixel load) */
static inline void x86_load16_rev(const uint32_t* base, uint32_t* row_pixels) {
    _mm_storeu_si128((__m128i*)(row_pixels + 0),  x86_rev_u32x4(_mm_loadu_si128((const __m128i*)(base - 3))));
    _mm_storeu_si128((__m128i*)(row_pixels + 4),  x86_rev_u32x4(_mm_loadu_si128((const __m128i*)(base - 7))));
    _mm_storeu_si128((__m128i*)(row_pixels + 8),  x86_rev_u32x4(_mm_loadu_si128((const __m128i*)(base - 11))));
    _mm_storeu_si128((__m128i*)(row_pixels + 12), x86_rev_u32x4(_mm_loadu_si128((const __m128i*)(base - 15))));
}

static inline uint32_t conv_src_rot(const uint32_t* in, int w, int h, int y, int x) {
    return in[(h - 1 - y) * w + (w - 1 - x)];
}

static inline uint8_t conv_rgb_to_y(uint32_t px) {
    uint32_t b = px & 0xff, g = (px >> 8) & 0xff, r = (px >> 16) & 0xff;
    return (uint8_t)((306 * r + 601 * g + 117 * b) >> 10);
}

static inline void conv_rgb_to_uv(uint32_t px, uint8_t* uv) {
    int32_t b = (int32_t)(px & 0xff), g = (int32_t)((px >> 8) & 0xff), r = (int32_t)((px >> 16) & 0xff);
    uv[0] = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
    uv[1] = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

static inline uint16_t conv_rgb_to_555(uint32_t px) {
    uint32_t b = px & 0xff, g = (px >> 8) & 0xff, r = (px >> 16) & 0xff;
    return (uint16_t)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

/* 4 ARGB pixels -> 4x32 Y=(306r+601g+117b)>>10 via SSE2 madd (no mullo_epi32) */
static inline __m128i x86_rgb_to_y32x4(__m128i px) {
    const __m128i m0ff = _mm_set1_epi32(0xff);
    __m128i b = _mm_and_si128(px, m0ff);
    __m128i g = _mm_and_si128(_mm_srli_epi32(px, 8), m0ff);
    __m128i r = _mm_and_si128(_mm_srli_epi32(px, 16), m0ff);
    __m128i rg = _mm_packs_epi32(r, g);                     /* [r0..r3,g0..g3] */
    rg = _mm_unpacklo_epi16(rg, _mm_srli_si128(rg, 8));     /* [r0,g0,r1,g1,r2,g2,r3,g3] */
    __m128i part = _mm_madd_epi16(rg, _mm_set1_epi32((601 << 16) | 306));
    __m128i bp = _mm_packs_epi32(b, _mm_setzero_si128());   /* [b0..b3,0..0] */
    bp = _mm_unpacklo_epi16(bp, _mm_setzero_si128());       /* [b0,0,b1,0,b2,0,b3,0] */
    part = _mm_add_epi32(part, _mm_madd_epi16(bp, _mm_set1_epi32(117)));
    return _mm_srli_epi32(part, 10);
}

static inline void x86_rgb_to_y16(const uint32_t* px, uint8_t* y) {
    __m128i y0 = x86_rgb_to_y32x4(_mm_loadu_si128((const __m128i*)(px + 0)));
    __m128i y1 = x86_rgb_to_y32x4(_mm_loadu_si128((const __m128i*)(px + 4)));
    __m128i y2 = x86_rgb_to_y32x4(_mm_loadu_si128((const __m128i*)(px + 8)));
    __m128i y3 = x86_rgb_to_y32x4(_mm_loadu_si128((const __m128i*)(px + 12)));
    __m128i p0 = _mm_packs_epi32(y0, y1);                   /* Y0..Y7  (16-bit) */
    __m128i p1 = _mm_packs_epi32(y2, y3);                   /* Y8..Y15 (16-bit) */
    _mm_storeu_si128((__m128i*)y, _mm_packus_epi16(p0, p1));
}

/* 4 ARGB pixels -> 4x32 XRGB1555 */
static inline __m128i x86_argb_to_555_4px(__m128i px) {
    const __m128i m1f = _mm_set1_epi32(0x1f);
    __m128i b5 = _mm_and_si128(_mm_srli_epi32(px, 3), m1f);
    __m128i g5 = _mm_and_si128(_mm_srli_epi32(px, 11), m1f);
    __m128i r5 = _mm_and_si128(_mm_srli_epi32(px, 19), m1f);
    return _mm_or_si128(_mm_or_si128(_mm_slli_epi32(r5, 10), _mm_slli_epi32(g5, 5)), b5);
}

/* 8 XRGB1555 (16-bit lanes) -> 2x(4 ARGB) stored at dst */
static inline void x86_555_to_argb_8px(__m128i px, uint32_t* dst) {
    __m128i r5 = _mm_srli_epi16(_mm_and_si128(px, _mm_set1_epi16(0x7c00)), 10);
    __m128i g5 = _mm_srli_epi16(_mm_and_si128(px, _mm_set1_epi16(0x03e0)), 5);
    __m128i b5 = _mm_and_si128(px, _mm_set1_epi16(0x001f));
    __m128i r8 = _mm_or_si128(_mm_slli_epi16(r5, 3), _mm_srli_epi16(r5, 2));
    __m128i g8 = _mm_or_si128(_mm_slli_epi16(g5, 3), _mm_srli_epi16(g5, 2));
    __m128i b8 = _mm_or_si128(_mm_slli_epi16(b5, 3), _mm_srli_epi16(b5, 2));
    __m128i ar = _mm_or_si128(_mm_set1_epi16((short)0xff00), r8);   /* 0xffRR */
    __m128i gb = _mm_or_si128(_mm_slli_epi16(g8, 8), b8);           /* 0xGGBB */
    _mm_storeu_si128((__m128i*)(dst + 0), _mm_unpacklo_epi16(gb, ar));
    _mm_storeu_si128((__m128i*)(dst + 4), _mm_unpackhi_epi16(gb, ar));
}

static inline __m128i x86_byteswap16(__m128i v) {
    return _mm_or_si128(_mm_slli_epi16(v, 8), _mm_srli_epi16(v, 8));
}

static inline __m128i x86_byteswap32(__m128i v) {
    __m128i s = _mm_or_si128(_mm_slli_epi16(v, 8), _mm_srli_epi16(v, 8));
    return _mm_or_si128(_mm_srli_epi32(s, 16), _mm_slli_epi32(s, 16));
}

int argb_2_nv12_arch(uint8_t* out, uint32_t* in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    uint8_t* y_plane = out;
    uint8_t* uv_plane = out + w * h;

    for(int y = 0; y < h; y += 2) {
        uint8_t* y_row0 = y_plane + y * w;
        uint8_t* y_row1 = (y + 1 < h) ? (y_row0 + w) : NULL;
        uint8_t* uv_row = uv_plane + (y >> 1) * w;
        int x = 0;

        if(y + 1 < h) {
            for(; x + 15 < w; x += 16) {
                const uint32_t* src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                const uint32_t* src_row1 = in + (h - 2 - y) * w + (w - 1 - x);
                uint32_t row0[16], row1[16];
                x86_load16_rev(src_row0, row0);
                x86_load16_rev(src_row1, row1);
                x86_rgb_to_y16(row0, y_row0 + x);
                x86_rgb_to_y16(row1, y_row1 + x);
                conv_rgb_to_uv(row0[0],  uv_row + x);
                conv_rgb_to_uv(row0[2],  uv_row + x + 2);
                conv_rgb_to_uv(row0[4],  uv_row + x + 4);
                conv_rgb_to_uv(row0[6],  uv_row + x + 6);
                conv_rgb_to_uv(row0[8],  uv_row + x + 8);
                conv_rgb_to_uv(row0[10], uv_row + x + 10);
                conv_rgb_to_uv(row0[12], uv_row + x + 12);
                conv_rgb_to_uv(row0[14], uv_row + x + 14);
            }
            for(; x + 1 < w; x += 2) {
                uint32_t p00 = conv_src_rot(in, w, h, y, x);
                uint32_t p01 = conv_src_rot(in, w, h, y, x + 1);
                uint32_t p10 = conv_src_rot(in, w, h, y + 1, x);
                uint32_t p11 = conv_src_rot(in, w, h, y + 1, x + 1);
                y_row0[x]     = conv_rgb_to_y(p00);
                y_row0[x + 1] = conv_rgb_to_y(p01);
                y_row1[x]     = conv_rgb_to_y(p10);
                y_row1[x + 1] = conv_rgb_to_y(p11);
                conv_rgb_to_uv(p00, uv_row + x);
            }
            if(x < w) {
                y_row0[x] = conv_rgb_to_y(conv_src_rot(in, w, h, y, x));
                y_row1[x] = conv_rgb_to_y(conv_src_rot(in, w, h, y + 1, x));
            }
        }
        else {
            for(; x + 15 < w; x += 16) {
                const uint32_t* src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                uint32_t row0[16];
                x86_load16_rev(src_row0, row0);
                x86_rgb_to_y16(row0, y_row0 + x);
            }
            for(; x < w; ++x)
                y_row0[x] = conv_rgb_to_y(conv_src_rot(in, w, h, y, x));
        }
    }
    return 0;
}

int argb_2_rgb15_arch(uint16_t* out, uint32_t* in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    for(int y = 0; y < h; y++) {
        uint16_t* dst_row = out + y * w;
        int x = 0;
        for(; x + 15 < w; x += 16) {
            const uint32_t* src_row = in + (h - 1 - y) * w + (w - 1 - x);
            uint32_t row_pixels[16];
            x86_load16_rev(src_row, row_pixels);
            __m128i v0 = x86_argb_to_555_4px(_mm_loadu_si128((const __m128i*)(row_pixels + 0)));
            __m128i v1 = x86_argb_to_555_4px(_mm_loadu_si128((const __m128i*)(row_pixels + 4)));
            __m128i v2 = x86_argb_to_555_4px(_mm_loadu_si128((const __m128i*)(row_pixels + 8)));
            __m128i v3 = x86_argb_to_555_4px(_mm_loadu_si128((const __m128i*)(row_pixels + 12)));
            _mm_storeu_si128((__m128i*)(dst_row + x),     _mm_packs_epi32(v0, v1));
            _mm_storeu_si128((__m128i*)(dst_row + x + 8), _mm_packs_epi32(v2, v3));
        }
        for(; x < w; ++x)
            dst_row[x] = conv_rgb_to_555(conv_src_rot(in, w, h, y, x));
    }
    return 0;
}

int rgb15_2_argb_arch(uint32_t* out, uint16_t* in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    int n = w * h, i = 0;
    for(; i + 7 < n; i += 8)
        x86_555_to_argb_8px(_mm_loadu_si128((const __m128i*)(in + i)), out + i);
    for(; i < n; ++i) {
        uint32_t v = in[i];
        uint32_t r = (v >> 10) & 0x1f, g = (v >> 5) & 0x1f, b = v & 0x1f;
        r = (r << 3) | (r >> 2); g = (g << 3) | (g >> 2); b = (b << 3) | (b >> 2);
        out[i] = 0xff000000u | (r << 16) | (g << 8) | b;
    }
    return 0;
}

int rgb15be_2_argb_arch(uint32_t* out, const uint8_t* in, int bpr, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    for(int y = 0; y < h; y++) {
        const uint8_t* src_row = in + y * bpr;
        uint32_t* dst_row = out + y * w;
        int x = 0;
        for(; x + 7 < w; x += 8) {
            __m128i px = x86_byteswap16(_mm_loadu_si128((const __m128i*)(src_row + x * 2)));
            x86_555_to_argb_8px(px, dst_row + x);
        }
        for(; x < w; ++x) {
            const uint8_t* p = src_row + x * 2;
            uint32_t v = ((uint32_t)p[0] << 8) | p[1];
            uint32_t r = (v >> 10) & 0x1f, g = (v >> 5) & 0x1f, b = v & 0x1f;
            r = (r << 3) | (r >> 2); g = (g << 3) | (g >> 2); b = (b << 3) | (b >> 2);
            dst_row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
    return 0;
}

int argb_2_rgb24_arch(uint32_t* out, uint32_t* in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    const __m128i mask = _mm_set1_epi32(0x00ffffff);
    int n = w * h, i = 0;
    for(; i + 7 < n; i += 8) {
        _mm_storeu_si128((__m128i*)(out + i),     _mm_and_si128(_mm_loadu_si128((const __m128i*)(in + i)), mask));
        _mm_storeu_si128((__m128i*)(out + i + 4), _mm_and_si128(_mm_loadu_si128((const __m128i*)(in + i + 4)), mask));
    }
    for(; i < n; ++i)
        out[i] = in[i] & 0x00ffffffu;
    return 0;
}

int rgb24_2_argb_arch(uint32_t* out, uint32_t* in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    const __m128i mask = _mm_set1_epi32(0x00ffffff);
    const __m128i alpha = _mm_set1_epi32((int)0xff000000);
    int n = w * h, i = 0;
    for(; i + 7 < n; i += 8) {
        _mm_storeu_si128((__m128i*)(out + i),     _mm_or_si128(_mm_and_si128(_mm_loadu_si128((const __m128i*)(in + i)), mask), alpha));
        _mm_storeu_si128((__m128i*)(out + i + 4), _mm_or_si128(_mm_and_si128(_mm_loadu_si128((const __m128i*)(in + i + 4)), mask), alpha));
    }
    for(; i < n; ++i)
        out[i] = 0xff000000u | (in[i] & 0x00ffffffu);
    return 0;
}

int rgb24be_2_argb_arch(uint32_t* out, const uint8_t* in, int bpr, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return -1;

    const __m128i alpha = _mm_set1_epi32((int)0xff000000);
    for(int y = 0; y < h; y++) {
        const uint8_t* src_row = in + y * bpr;
        uint32_t* dst_row = out + y * w;
        int x = 0;
        for(; x + 3 < w; x += 4) {
            __m128i raw = _mm_loadu_si128((const __m128i*)(src_row + x * 4));
            _mm_storeu_si128((__m128i*)(dst_row + x), _mm_or_si128(x86_byteswap32(raw), alpha));
        }
        for(; x < w; ++x) {
            const uint8_t* p = src_row + x * 4;
            dst_row[x] = 0xff000000u | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
        }
    }
    return 0;
}

#endif /* ARCH_BOOST */

#ifdef __cplusplus
}
#endif
