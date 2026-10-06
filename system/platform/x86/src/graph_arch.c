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

void graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if(g == NULL || g->buffer == NULL || w <= 0 || h <= 0)
        return;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);
    if(r.w <= 0 || r.h <= 0)
        return;

    /* opaque fill: memset / SSE row stores; translucent: g2d alpha fill
       (simd rows with scalar head/tail alignment, same blend math as blt) */
    if(color_a(color) == 0xff)
        arch_g2d_fill(g->buffer, 0, 0, g->w, g->h, r.x, r.y, r.w, r.h, color);
    else
        arch_g2d_fill_alpha(g->buffer, g->w, g->h, r.x, r.y, r.w, r.h, color);
}

void graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return;

    if(dx < 0)
        sr.x -= dx;
    if(dy < 0)
        sr.y -= dy;

    /* 1:1 copy of the clipped region (like aarch64, the destination size is
       sr.w/sr.h); the engine keeps overlapping copies within one buffer safe */
    arch_g2d_blt(src->buffer, 0, 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, 0, 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h);
}

void graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;
    /* global alpha 0: nothing visible, skip entirely */
    if(alpha == 0)
        return;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return;

    if(dx < 0)
        sr.x -= dx;
    if(dy < 0)
        sr.y -= dy;

    /* 1:1 blend of the clipped region; the engine applies the same per-block
       transparent/opaque fast paths and div255 blend math as the cpu */
    arch_g2d_blt_alpha(src->buffer, 0, 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, 0, 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h, alpha);
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

void graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    if(src == NULL || dst == NULL || src->buffer == NULL || dst->buffer == NULL)
        return;
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
    if(dst->clip.w > 0 && dst->clip.h > 0)
        grect_insect(&dst->clip, &dr);
    if(!graph_insect_with(src, &sr, dst, &dr))
        return;

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
}

void graph_gaussian_blur_arch(graph_t* g, int x, int y, int w, int h, int r) {
    if(g == NULL || g->buffer == NULL)
        return;
    /* replicate graph_gaussian_blur_cpu's clip so the engine sees the exact
       same intersected rect (the engine reproduces its box semantics) */
    grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return;
    arch_g2d_gaussian(g->buffer, 0, 0, g->w, g->h, ir.x, ir.y, ir.w, ir.h, r);
}

void graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL)
        return;
    if(scale <= 0.0 || g->w <= 0 || g->h <= 0 || dst->w <= 0 || dst->h <= 0)
        return;
    /* derive the 16.16 source step from the scale factor exactly the way
       graph_scale_tof_cpu does ((uint32_t)(FIXED_SCALE/scale)), NOT from the
       src/dst dimension ratio, so the SSE2 separable bilinear stays bit-exact */
    float fscale = (float)scale;
    uint32_t inv = (uint32_t)(65536.0f / fscale);
    arch_g2d_scale_inv(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, inv, inv);
}

void graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL)
        return;
    if(scale <= 0.0 || g->w <= 0 || g->h <= 0 || dst->w <= 0 || dst->h <= 0)
        return;
    float fscale = (float)scale;
    uint32_t inv = (uint32_t)(65536.0f / fscale);
    arch_g2d_scale_inv(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, inv, inv);
}

void graph_rotate_to_arch(graph_t* g, graph_t* dst, int rot) {
    if(g == NULL || dst == NULL || g->buffer == NULL || dst->buffer == NULL ||
            g->w <= 0 || g->h <= 0)
        return;

    /* the g2d engine writes the rotated surface into dst; make sure dst is
       large enough for the quadrant (90/270 swap the dimensions) */
    if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
        if(dst->w < g->h || dst->h < g->w)
            return;
    }
    else if(rot == G_ROTATE_180) {
        if(dst->w < g->w || dst->h < g->h)
            return;
    }
    else
        return;

    /* quadrant rot codes map 1:1 to clockwise degrees */
    arch_g2d_rotate(g->buffer, 0, 0, g->w, g->h,
            dst->buffer, 0, 0, dst->w, dst->h, rot * 90);
}
#endif /* ARCH_BOOST */

#ifdef __cplusplus
}
#endif
