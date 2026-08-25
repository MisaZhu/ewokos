#include <graph/graph_arch.h>
#include <ewoksys/core.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openlibm.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef ARCH_BOOST
#include <arm_neon.h>

#define MIN(a, b) (((a) > (b))?(b):(a))

static inline uint16x8_t neon_div255_u16(uint16x8_t v)
{
    uint16x8_t t = vaddq_u16(v, vdupq_n_u16(1));
    t = vaddq_u16(t, vshrq_n_u16(v, 8));
    return vshrq_n_u16(t, 8);
}

static inline void neon_16(uint32_t *s, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.16b-v23.16b}, [%0]\n\t"    // Load source
        "st4 {v20.16b-v23.16b}, [%1]\n\t"    // Store to destination
        :
        : "r"(s), "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_alpha_16(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)f);
    uint8x16x4_t bg = vld4q_u8((const uint8_t*)b);
    uint8x16x4_t out;
    uint8x16_t full = vdupq_n_u8(0xff);
    uint8x16_t scaled = vdupq_n_u8(alpha_more);

    uint8x8_t a_lo = vmovn_u16(neon_div255_u16(vmull_u8(vget_low_u8(fg.val[3]), vget_low_u8(scaled))));
    uint8x8_t a_hi = vmovn_u16(neon_div255_u16(vmull_u8(vget_high_u8(fg.val[3]), vget_high_u8(scaled))));
    uint8x16_t a = vcombine_u8(a_lo, a_hi);
    uint8x16_t inv_a = vsubq_u8(full, a);

    uint16x8_t oa_lo = neon_div255_u16(vmull_u8(vsub_u8(vget_low_u8(full), vget_low_u8(bg.val[3])), a_lo));
    uint16x8_t oa_hi = neon_div255_u16(vmull_u8(vsub_u8(vget_high_u8(full), vget_high_u8(bg.val[3])), a_hi));

    /* out = div255(fg*a + bg*(255-a)) per channel, low/high halves widened */
    for(int c = 0; c < 3; c++) {
        uint16x8_t lo = vaddq_u16(vmull_u8(vget_low_u8(fg.val[c]), a_lo),
                                  vmull_u8(vget_low_u8(bg.val[c]), vget_low_u8(inv_a)));
        uint16x8_t hi = vaddq_u16(vmull_u8(vget_high_u8(fg.val[c]), a_hi),
                                  vmull_u8(vget_high_u8(bg.val[c]), vget_high_u8(inv_a)));
        out.val[c] = vcombine_u8(vmovn_u16(neon_div255_u16(lo)), vmovn_u16(neon_div255_u16(hi)));
    }
    /* out_a = bg_a + div255((255-bg_a)*a) */
    out.val[3] = vcombine_u8(
        vmovn_u16(vaddq_u16(vmovl_u8(vget_low_u8(bg.val[3])), oa_lo)),
        vmovn_u16(vaddq_u16(vmovl_u8(vget_high_u8(bg.val[3])), oa_hi)));

    vst4q_u8((uint8_t*)d, out);
}

static inline void neon_fill_load_16(uint32_t *s)
{
    __asm volatile(
        "ld4 {v20.16b-v23.16b}, [%0]\n\t"    // Load source
        :
        : "r"(s)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_store_16(uint32_t *d)
{
    __asm volatile(
        "st4 {v20.16b-v23.16b}, [%0]\n\t"    // Store to destination
        :
        : "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size, uint8_t alpha_more)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 16)
    {
        neon_alpha_16(dst, src, dst, alpha_more);
    }
    else
    {
        // For size < 16, use memcpy to handle the edge case safely.
        uint32_t fg[16] = {0};
        uint32_t bg[16] = {0};
        memcpy(fg, src, 4*size);
        memcpy(bg, dst, 4*size);
        neon_alpha_16(bg, fg, bg, alpha_more);
        memcpy(dst, bg, 4*size);
    }
}

static inline void graph_pixel_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 16)
    {
        neon_16(src, dst);
    }
    else
    {
        memcpy(dst, src, 4 * size);
    }
}

void graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    uint32_t buf[16] = {0};

    if(g == NULL || w <= 0 || h <= 0)
        return;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);

    register int32_t ex, ey;
    y = r.y;
    ex = r.x + r.w;
    ey = r.y + r.h;

    for(int i = 0; i < 16; i++)
        buf[i] = color;
    
    if(color_a(color) == 0xff) {
        neon_fill_load_16(buf);
        for(; y < ey; y++) {
            x = r.x;
            for(; x < ex; x+=16) {
                uint32_t *dst = &g->buffer[y * g->w + x];
                int pixels = ex - x;
                if(pixels > 16)
                    pixels = 16;
                /* 16px SIMD store needs a 16-byte aligned row start: the
                   graph buffer may be Device-mapped framebuffer memory
                   where any unaligned access faults */
                if(pixels == 16 && (((uintptr_t)dst & 0xF) == 0))
                    neon_fill_store_16(dst);
                else
                    memcpy(dst, buf, pixels * 4);
            }
        }
    }
    else {
        for(; y < ey; y++) {
            x = r.x;
            for(; x < ex; x+=16) {
                graph_pixel_argb_neon(g, x, y, buf, MIN(ex-x, 16), 0xFF);
            }
        }
    }
}

#if defined(__GNUC__) && !defined(__clang__)
/* GCC with -mstrict-align lowers vld1q_u32/vst1q_u32 on pointers without
   proven 16-byte alignment to scalar ldp w/orr/stp w sequences. LD1/ST1
   tolerate unaligned addresses in Normal memory (the kernel boots with
   SCTLR_EL1.A=0), so emit them directly. NOTE: do NOT use LDP/STP q-form
   here — unlike LD1/ST1 they require 16-byte alignment even with A=0.
   IMPORTANT: unaligned accesses to Device memory fault unconditionally
   (SCTLR.A only relaxes Normal memory). The VC framebuffer sits in a
   reserved RAM carve-out that SYS_MEM_MAP maps as Device-nGnRnE, so a
   dirty-rect row whose x*4 offset is not 16-byte aligned crashes on the
   first 16-byte SIMD store. Callers must only reach these helpers with
   16-byte aligned pointers (see blt_copy_row_neon). */
static inline uint32x4_t blt_ld1q_u32_bsp(const uint32_t *p) {
    uint32x4_t v;
    __asm__("ld1 {%0.4s}, [%1]" : "=w"(v) : "r"(p));
    return v;
}
static inline void blt_st1q_u32_bsp(uint32_t *p, uint32x4_t v) {
    __asm__("st1 {%1.4s}, [%0]" :: "r"(p), "w"(v) : "memory");
}
static inline void blt_ld1q_x4_u32_bsp(const uint32_t *p, uint32x4_t *a, uint32x4_t *b, uint32x4_t *c, uint32x4_t *d) {
    /* 4 independent single-register LD1s instead of the ld1 {vA.4s-vD.4s}
       range form: GCC "=w" constraints do NOT guarantee consecutive
       register allocation, so the range syntax breaks as soon as register
       pressure rises. Throughput is equivalent (the 4-reg form is 4 uops
       anyway) and independent loads pipeline at least as well. */
    *a = blt_ld1q_u32_bsp(p);
    *b = blt_ld1q_u32_bsp(p + 4);
    *c = blt_ld1q_u32_bsp(p + 8);
    *d = blt_ld1q_u32_bsp(p + 12);
}
static inline void blt_st1q_x4_u32_bsp(uint32_t *p, uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d) {
    blt_st1q_u32_bsp(p, a);
    blt_st1q_u32_bsp(p + 4, b);
    blt_st1q_u32_bsp(p + 8, c);
    blt_st1q_u32_bsp(p + 12, d);
}
#else
static inline uint32x4_t blt_ld1q_u32_bsp(const uint32_t *p) {
    return vld1q_u32(p);
}
static inline void blt_st1q_u32_bsp(uint32_t *p, uint32x4_t v) {
    vst1q_u32(p, v);
}
static inline void blt_ld1q_x4_u32_bsp(const uint32_t *p, uint32x4_t *a, uint32x4_t *b, uint32x4_t *c, uint32x4_t *d) {
    *a = vld1q_u32(p);
    *b = vld1q_u32(p + 4);
    *c = vld1q_u32(p + 8);
    *d = vld1q_u32(p + 12);
}
static inline void blt_st1q_x4_u32_bsp(uint32_t *p, uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d) {
    vst1q_u32(p, a);
    vst1q_u32(p + 4, b);
    vst1q_u32(p + 8, c);
    vst1q_u32(p + 12, d);
}
#endif

/* Row copy that never falls back to libc memcpy: the EwokOS libc one is a
   plain byte loop, while 128-bit NEON load/stores move 16 pixels in a few
   instructions and merge cleanly into write-combine bursts on non-cacheable
   scan-out memory. */
static inline void blt_copy_row_neon(uint32_t *dp, const uint32_t *sp, int32_t w) {
    /* The destination graph can be the framebuffer, which SYS_MEM_MAP maps
       as Device memory when its physical base sits in a reserved carve-out
       (VC fb on Raspberry Pi): ANY unaligned access to Device memory is an
       unconditional alignment fault, so the 16-byte SIMD path may only run
       on 16-byte aligned rows. Odd-x dirty rects fall back to a scalar
       word copy instead. */
    if((((uintptr_t)dp | (uintptr_t)sp) & 0xF) != 0) {
        for(int32_t x = 0; x < w; x++)
            dp[x] = sp[x];
        return;
    }
    int32_t x = 0;
    /* 32 pixels (128 bytes) per iteration with a rolling prefetch 4 cache
       lines ahead: keeps the load pipe fed on long rows / full-screen runs
       instead of stalling on every new cache line. */
    for(; x <= w - 32; x += 32) {
        __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(sp + x));
        uint32x4_t v0, v1, v2, v3, v4, v5, v6, v7;
        blt_ld1q_x4_u32_bsp(sp + x, &v0, &v1, &v2, &v3);
        blt_ld1q_x4_u32_bsp(sp + x + 16, &v4, &v5, &v6, &v7);
        blt_st1q_x4_u32_bsp(dp + x, v0, v1, v2, v3);
        blt_st1q_x4_u32_bsp(dp + x + 16, v4, v5, v6, v7);
    }
    /* 16 pixels */
    if(x <= w - 16) {
        uint32x4_t v0, v1, v2, v3;
        blt_ld1q_x4_u32_bsp(sp + x, &v0, &v1, &v2, &v3);
        blt_st1q_x4_u32_bsp(dp + x, v0, v1, v2, v3);
        x += 16;
    }
    /* 8 pixels */
    if(x <= w - 8) {
        uint32x4_t v0 = blt_ld1q_u32_bsp(sp + x);
        uint32x4_t v1 = blt_ld1q_u32_bsp(sp + x + 4);
        blt_st1q_u32_bsp(dp + x, v0);
        blt_st1q_u32_bsp(dp + x + 4, v1);
        x += 8;
    }
    /* 4 pixels */
    if(x <= w - 4) {
        blt_st1q_u32_bsp(dp + x, blt_ld1q_u32_bsp(sp + x));
        x += 4;
    }
    /* Tail */
    for(; x < w; x++)
        dp[x] = sp[x];
}

inline void graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
    
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    /* overlapping copy within one graph needs memmove ordering; the CPU
       path handles it */
    if(src == dst &&
            dx < sx + sw && sx < dx + dw &&
            dy < sy + sh && sy < dy + dh) {
        graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
        return;
    }

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

    register int32_t ex, ey;
    sy = sr.y;
    dy = dr.y;
    ex = sr.x + sr.w;
    ey = sr.y + sr.h;
    int32_t w = ex - sr.x;

    /* Full-width rows on both sides: whole region is contiguous in memory,
       collapse the row loop into one streaming NEON copy (common full-screen
       path) */
    if(w == src->w && w == dst->w) {
        blt_copy_row_neon(&dst->buffer[dy * w], &src->buffer[sy * w],
                (ey - sy) * w);
        return;
    }

    for(; sy < ey; sy++, dy++) {
        const uint32_t *sp = &src->buffer[sy * src->w + sr.x];
        uint32_t *dp = &dst->buffer[dy * dst->w + dr.x];
        blt_copy_row_neon(dp, sp, w);
    }
}

/* Blend 16 pixels whose effective alpha is already known to be neither
   all-zero nor all-opaque. a_lo/a_hi are the per-pixel effective alphas
   (src_a scaled by the global alpha). Channels use vmull+vmlal instead of
   vmull+vmull+vaddq: one fewer instruction per channel half. */
static inline void blt_alpha16_blend_core(uint32_t *dp, uint8x16x4_t fg,
        uint8x8_t a_lo, uint8x8_t a_hi)
{
    uint8x16x4_t bg = vld4q_u8((const uint8_t*)dp);
    uint8x16_t full = vdupq_n_u8(0xff);
    uint8x16_t a = vcombine_u8(a_lo, a_hi);
    uint8x16_t inv_a = vsubq_u8(full, a);

    uint8x16x4_t out;
    /* out = div255(fg*a + bg*(255-a)) per channel, low/high halves widened */
    for(int c = 0; c < 3; c++) {
        uint16x8_t lo = vmull_u8(vget_low_u8(fg.val[c]), a_lo);
        lo = vmlal_u8(lo, vget_low_u8(bg.val[c]), vget_low_u8(inv_a));
        uint16x8_t hi = vmull_u8(vget_high_u8(fg.val[c]), a_hi);
        hi = vmlal_u8(hi, vget_high_u8(bg.val[c]), vget_high_u8(inv_a));
        out.val[c] = vcombine_u8(vmovn_u16(neon_div255_u16(lo)), vmovn_u16(neon_div255_u16(hi)));
    }
    /* out_a = bg_a + div255((255-bg_a)*a) */
    uint16x8_t oa_lo = neon_div255_u16(vmull_u8(vsub_u8(vget_low_u8(full), vget_low_u8(bg.val[3])), a_lo));
    uint16x8_t oa_hi = neon_div255_u16(vmull_u8(vsub_u8(vget_high_u8(full), vget_high_u8(bg.val[3])), a_hi));
    out.val[3] = vcombine_u8(
        vmovn_u16(vaddq_u16(vmovl_u8(vget_low_u8(bg.val[3])), oa_lo)),
        vmovn_u16(vaddq_u16(vmovl_u8(vget_high_u8(bg.val[3])), oa_hi)));

    vst4q_u8((uint8_t*)dp, out);
}

/* 16px blend with global alpha 0xff: effective alpha is the source alpha
   itself, skipping the per-block div255(fg_a*alpha) scaling chain. The
   caller must have ruled out the all-transparent and all-opaque cases. */
static inline void blt_alpha16_blend_a255(uint32_t *dp, uint8x16x4_t fg)
{
    blt_alpha16_blend_core(dp, fg,
            vget_low_u8(fg.val[3]), vget_high_u8(fg.val[3]));
}

/* 16px blend with a global alpha < 0xff */
static inline void blt_alpha16_blend_scaled(uint32_t *dp, uint8x16x4_t fg,
        uint8x16_t alpha_vec)
{
    uint8x8_t a_lo = vmovn_u16(neon_div255_u16(vmull_u8(vget_low_u8(fg.val[3]), vget_low_u8(alpha_vec))));
    uint8x8_t a_hi = vmovn_u16(neon_div255_u16(vmull_u8(vget_high_u8(fg.val[3]), vget_high_u8(alpha_vec))));
    blt_alpha16_blend_core(dp, fg, a_lo, a_hi);
}

/* 16px sub-block already loaded interleaved (cheap ld1). Alphas are the
   top byte of each u32 lane: max-of-OR == 0 means all transparent,
   min-of-AND == 0xff means all opaque. Uniform blocks never touch the
   expensive ld4/st4 de-interleave path: transparent skips all memory
   access, opaque is a plain st1 copy. Only truly mixed blocks reload the
   source de-interleaved for the blend math. */
static inline void blt_alpha16_a255_regs(uint32_t *dp, const uint32_t *sp,
        uint32x4_t s0, uint32x4_t s1, uint32x4_t s2, uint32x4_t s3)
{
    uint32x4_t or_a = vorrq_u32(vorrq_u32(s0, s1), vorrq_u32(s2, s3));
    if(vmaxvq_u32(vshrq_n_u32(or_a, 24)) == 0)
        return;
    uint32x4_t and_a = vandq_u32(vandq_u32(s0, s1), vandq_u32(s2, s3));
    if(vminvq_u32(vshrq_n_u32(and_a, 24)) == 0xff) {
        blt_st1q_x4_u32_bsp(dp, s0, s1, s2, s3);
        return;
    }
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)sp);
    blt_alpha16_blend_a255(dp, fg);
}

/* 32px block, global alpha 0xff: combined transparent/opaque checks over
   all 8 interleaved vectors first (one reduction each), then per-16px
   sub-block checks only when the block is mixed. Blending preserves
   block-level semantics bit-exactly: fg_a==0 is the identity on dst, and
   fg_a==0xff gives div255(x*255) == x per channel with
   out_a = bg_a + (255-bg_a) == 255. */
static inline void blt_alpha32_a255(uint32_t *dp, const uint32_t *sp)
{
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(sp));

    uint32x4_t s0, s1, s2, s3, s4, s5, s6, s7;
    blt_ld1q_x4_u32_bsp(sp, &s0, &s1, &s2, &s3);
    blt_ld1q_x4_u32_bsp(sp + 16, &s4, &s5, &s6, &s7);

    uint32x4_t or_a = vorrq_u32(
            vorrq_u32(vorrq_u32(s0, s1), vorrq_u32(s2, s3)),
            vorrq_u32(vorrq_u32(s4, s5), vorrq_u32(s6, s7)));
    /* All 32 source alphas == 0: dst untouched, no read/write at all */
    if(vmaxvq_u32(vshrq_n_u32(or_a, 24)) == 0)
        return;
    uint32x4_t and_a = vandq_u32(
            vandq_u32(vandq_u32(s0, s1), vandq_u32(s2, s3)),
            vandq_u32(vandq_u32(s4, s5), vandq_u32(s6, s7)));
    /* All 32 source alphas == 0xff: plain interleaved copy, no de/re-
       interleave at all */
    if(vminvq_u32(vshrq_n_u32(and_a, 24)) == 0xff) {
        blt_st1q_x4_u32_bsp(dp, s0, s1, s2, s3);
        blt_st1q_x4_u32_bsp(dp + 16, s4, s5, s6, s7);
        return;
    }

    /* Mixed block: blend reads dst too, prefetch it as well */
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(dp));
    blt_alpha16_a255_regs(dp, sp, s0, s1, s2, s3);
    blt_alpha16_a255_regs(dp + 16, sp + 16, s4, s5, s6, s7);
}

/* 32px block with a global alpha < 0xff */
static inline void blt_alpha32_scaled(uint32_t *dp, const uint32_t *sp,
        uint8x16_t alpha_vec)
{
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(sp));
    /* scaled blend always reads dst unless fully transparent */
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(dp));

    uint8x16x4_t fg0 = vld4q_u8((const uint8_t*)sp);
    uint8x16x4_t fg1 = vld4q_u8((const uint8_t*)(sp + 16));

    uint8x16_t a_or = vorrq_u8(fg0.val[3], fg1.val[3]);
    if(vmaxvq_u8(a_or) == 0)
        return;

    blt_alpha16_blend_scaled(dp, fg0, alpha_vec);
    blt_alpha16_blend_scaled(dp + 16, fg1, alpha_vec);
}

/* Remaining 16px block with per-block checks, global alpha 0xff */
static inline void blt_alpha16_a255_checked(uint32_t *dp, const uint32_t *sp)
{
    uint32x4_t s0, s1, s2, s3;
    blt_ld1q_x4_u32_bsp(sp, &s0, &s1, &s2, &s3);
    blt_alpha16_a255_regs(dp, sp, s0, s1, s2, s3);
}

/* Remaining 16px block with per-block checks, global alpha < 0xff */
static inline void blt_alpha16_scaled_checked(uint32_t *dp, const uint32_t *sp,
        uint8x16_t alpha_vec)
{
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)sp);
    if(vmaxvq_u8(fg.val[3]) == 0)
        return;
    blt_alpha16_blend_scaled(dp, fg, alpha_vec);
}

inline void graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    /* Global alpha 0: nothing visible, skip entirely */
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

    register int32_t ex, ey;
    sy = sr.y;
    dy = dr.y;
    ex = sr.x + sr.w;
    ey = sr.y + sr.h;
    int32_t w = ex - sr.x;

    if(alpha == 0xff) {
        /* most common case: global alpha full, effective alpha = src alpha */
        for(; sy < ey; sy++, dy++) {
            const uint32_t *sp = &src->buffer[sy * src->w + sr.x];
            uint32_t *dp = &dst->buffer[dy * dst->w + dr.x];
            int32_t x = 0;

            for(; x <= w - 32; x += 32)
                blt_alpha32_a255(dp + x, sp + x);
            if(x <= w - 16) {
                blt_alpha16_a255_checked(dp + x, sp + x);
                x += 16;
            }
            /* Tail: zero-padded block; padding lanes blend as identity */
            if(x < w) {
                int remain = w - x;
                uint32_t fg[16] = {0}, bg[16] = {0};
                memcpy(fg, sp + x, 4 * remain);
                memcpy(bg, dp + x, 4 * remain);
                uint8x16x4_t fgv = vld4q_u8((const uint8_t*)fg);
                blt_alpha16_blend_a255(bg, fgv);
                memcpy(dp + x, bg, 4 * remain);
            }
        }
        return;
    }

    uint8x16_t alpha_vec16 = vdupq_n_u8(alpha);
    for(; sy < ey; sy++, dy++) {
        const uint32_t *sp = &src->buffer[sy * src->w + sr.x];
        uint32_t *dp = &dst->buffer[dy * dst->w + dr.x];
        int32_t x = 0;

        for(; x <= w - 32; x += 32)
            blt_alpha32_scaled(dp + x, sp + x, alpha_vec16);
        if(x <= w - 16) {
            blt_alpha16_scaled_checked(dp + x, sp + x, alpha_vec16);
            x += 16;
        }
        /* Tail: zero-padded block; padding lanes blend as identity */
        if(x < w) {
            int remain = w - x;
            uint32_t fg[16] = {0}, bg[16] = {0};
            memcpy(fg, sp + x, 4 * remain);
            memcpy(bg, dp + x, 4 * remain);
            uint8x16x4_t fgv = vld4q_u8((const uint8_t*)fg);
            blt_alpha16_blend_scaled(bg, fgv, alpha_vec16);
            memcpy(dp + x, bg, 4 * remain);
        }
    }
}

static inline void neon_mask_alpha_16(uint32_t *dst, uint32_t *src)
{
    __asm volatile(
        // Load 16 dst pixels (RGBA)
        "ld4 {v20.16b-v23.16b}, [%0]\n\t"
        // Load 16 src pixels (RGBA)
        "ld4 {v24.16b-v27.16b}, [%1]\n\t"
        
        // v23 = dst_a, v27 = src_a
        // Create zero vector
        "movi v28.16b, #0\n\t"
        // Compare src_a > 0 (cmhi returns 0xFF where true, 0x00 where false)
        "cmhi v28.16b, v27.16b, v28.16b\n\t"  // v28: 0xFF where src_a > 0, 0x00 where src_a == 0
        
        // Compare dst_a > src_a
        "cmhi v29.16b, v23.16b, v27.16b\n\t"  // v29: 0xFF where dst_a > src_a
        
        // Create mask for src_a == 0: set all channels to 0
        "and v20.16b, v20.16b, v28.16b\n\t"    // R
        "and v21.16b, v21.16b, v28.16b\n\t"    // G
        "and v22.16b, v22.16b, v28.16b\n\t"    // B
        "and v23.16b, v23.16b, v28.16b\n\t"    // A
        
        // For dst_a > src_a: keep dst RGB, replace alpha with src_a
        // First, mask src_a where condition is true
        "and v30.16b, v27.16b, v29.16b\n\t"
        // Mask dst_a where condition is false
        "mvn v28.16b, v29.16b\n\t"
        "and v23.16b, v23.16b, v28.16b\n\t"
        // Combine
        "orr v23.16b, v23.16b, v30.16b\n\t"
        
        // Store result
        "st4 {v20.16b-v23.16b}, [%0]\n\t"
        :
        : "r"(dst), "r"(src)
        : "memory", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", 
          "v28", "v29", "v30");
}

static inline void graph_pixel_alpha_mask_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 16)
    {
        neon_mask_alpha_16(dst, src);
    }
    else
    {
        // For size < 16, use memcpy to handle boundaries
        uint32_t src_buf[16] = {0};
        uint32_t dst_buf[16] = {0};
        memcpy(src_buf, src, 4 * size);
        memcpy(dst_buf, dst, 4 * size);
        neon_mask_alpha_16(dst_buf, src_buf);
        memcpy(dst, dst_buf, 4 * size);
    }
}

inline void graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
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

    register int32_t ex, ey;
    sy = sr.y;
    dy = dr.y;
    ex = sr.x + sr.w;
    ey = sr.y + sr.h;

    // Loop unrolling, process 2 rows at a time for better instruction-level parallelism
    for(; sy < ey - 1; sy += 2, dy += 2) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset1 = sy * src->w;
        register int32_t offset2 = (sy + 1) * src->w;
        
        // Preload next row data to cache
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&src->buffer[offset1]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[dy * dst->w + dx]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[(dy + 1) * dst->w + dx]));
        
        for(; sx < ex - 15; sx += 16, dx += 16) {
            // Process two rows in parallel
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset1 + sx], 16);
            graph_pixel_alpha_mask_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], 16);
        }
        
        // Process remaining pixels
        if(sx < ex) {
            int remain = ex - sx;
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset1 + sx], remain);
            graph_pixel_alpha_mask_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], remain);
        }
    }
    
    // Process last row (if total rows is odd)
    if(sy < ey) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset = sy * src->w;
        
        for(; sx < ex; sx += 16, dx += 16) {
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex - sx, 16));
        }
    }
}


static void glass_neon(uint32_t* args, int width, int height, 
                int x, int y, int w, int h, int r) {
    // Validate parameters.
    if (!args || r <= 0 || w <= 0 || h <= 0 || width <= 0 || height <= 0) 
        return;
    if (x < 0 || y < 0 || x + w > width || y + h > height)
        return;

    // Use a fixed random seed so the effect stays deterministic.
    srand(0x12345678);  // Reuse the same seed on every call

    // Precompute frequently used values.
    int range = 2*r;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    // Initialize NEON registers.
    int32x4_t vx = vdupq_n_s32(x);
    int32x4_t vy = vdupq_n_s32(y);
    int32x4_t vx_end = vdupq_n_s32(x_end);
    int32x4_t vy_end = vdupq_n_s32(y_end);
    int32x4_t vwidth = vdupq_n_s32(width);

    // Pre-generate all random offsets.
    int total_pixels = w * h;
    int* rand_offsets = malloc(total_pixels * 2 * sizeof(int));

    for (int i = 0; i < total_pixels * 2; i++) {
        rand_offsets[i] = (rand() % range) - r;
    }

    // Process the image region.
    int offset_index = 0;
    for (int j = y; j <= y_end; j++) {
        int32x4_t vj = vdupq_n_s32(j);
        
        // Preload data into the cache.
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&args[j * width + x]));
        
        for (int i = x; i <= x_end; i += 16) {
            // Handle the remaining pixels when fewer than 16 are left.
            int remaining = x_end - i + 1;
            if (remaining < 16) {
                for (int k = 0; k < remaining; k++) {
                    int rx = i + k + rand_offsets[offset_index++];
                    int ry = j + rand_offsets[offset_index++];
                    
                    // Clamp to the valid bounds.
                    rx = (rx < x) ? x : ((rx > x_end) ? x_end : rx);
                    ry = (ry < y) ? y : ((ry > y_end) ? y_end : ry);
                    
                    args[j * width + i + k] = args[ry * width + rx];
                }
                break;
            }
            
            // Generate random offsets for 16 pixels.
            int rand_x[16], rand_y[16];
            for (int k = 0; k < 16; k++) {
                rand_x[k] = rand_offsets[offset_index++];
                rand_y[k] = rand_offsets[offset_index++];
            }
            
            // Process the first 4 pixels.
            int32x4_t vrand_x0 = vld1q_s32(rand_x);
            int32x4_t vrand_y0 = vld1q_s32(rand_y);
            int32x4_t vi0 = {i, i+1, i+2, i+3};
            int32x4_t rx0 = vaddq_s32(vi0, vrand_x0);
            int32x4_t ry0 = vaddq_s32(vj, vrand_y0);
            rx0 = vmaxq_s32(vx, vminq_s32(vx_end, rx0));
            ry0 = vmaxq_s32(vy, vminq_s32(vy_end, ry0));
            int32x4_t rpos0 = vmlaq_s32(rx0, ry0, vwidth);
            
            // Process pixels 4-7.
            int32x4_t vrand_x1 = vld1q_s32(&rand_x[4]);
            int32x4_t vrand_y1 = vld1q_s32(&rand_y[4]);
            int32x4_t vi1 = {i+4, i+5, i+6, i+7};
            int32x4_t rx1 = vaddq_s32(vi1, vrand_x1);
            int32x4_t ry1 = vaddq_s32(vj, vrand_y1);
            rx1 = vmaxq_s32(vx, vminq_s32(vx_end, rx1));
            ry1 = vmaxq_s32(vy, vminq_s32(vy_end, ry1));
            int32x4_t rpos1 = vmlaq_s32(rx1, ry1, vwidth);
            
            // Process pixels 8-11.
            int32x4_t vrand_x2 = vld1q_s32(&rand_x[8]);
            int32x4_t vrand_y2 = vld1q_s32(&rand_y[8]);
            int32x4_t vi2 = {i+8, i+9, i+10, i+11};
            int32x4_t rx2 = vaddq_s32(vi2, vrand_x2);
            int32x4_t ry2 = vaddq_s32(vj, vrand_y2);
            rx2 = vmaxq_s32(vx, vminq_s32(vx_end, rx2));
            ry2 = vmaxq_s32(vy, vminq_s32(vy_end, ry2));
            int32x4_t rpos2 = vmlaq_s32(rx2, ry2, vwidth);
            
            // Process the last 4 pixels.
            int32x4_t vrand_x3 = vld1q_s32(&rand_x[12]);
            int32x4_t vrand_y3 = vld1q_s32(&rand_y[12]);
            int32x4_t vi3 = {i+12, i+13, i+14, i+15};
            int32x4_t rx3 = vaddq_s32(vi3, vrand_x3);
            int32x4_t ry3 = vaddq_s32(vj, vrand_y3);
            rx3 = vmaxq_s32(vx, vminq_s32(vx_end, rx3));
            ry3 = vmaxq_s32(vy, vminq_s32(vy_end, ry3));
            int32x4_t rpos3 = vmlaq_s32(rx3, ry3, vwidth);
            
            // Extract the positions into scalar arrays.
            int rpos_arr0[4], rpos_arr1[4], rpos_arr2[4], rpos_arr3[4];
            vst1q_s32(rpos_arr0, rpos0);
            vst1q_s32(rpos_arr1, rpos1);
            vst1q_s32(rpos_arr2, rpos2);
            vst1q_s32(rpos_arr3, rpos3);
            
            // Gather the pixel values in a batch.
            uint32_t pixels[16];
            for (int k = 0; k < 4; k++) {
                pixels[k]      = args[rpos_arr0[k]];
                pixels[k + 4]  = args[rpos_arr1[k]];
                pixels[k + 8]  = args[rpos_arr2[k]];
                pixels[k + 12] = args[rpos_arr3[k]];
            }
            
            // Store the results in a batch.
            uint32x4_t pixels0 = vld1q_u32(pixels);
            uint32x4_t pixels1 = vld1q_u32(&pixels[4]);
            uint32x4_t pixels2 = vld1q_u32(&pixels[8]);
            uint32x4_t pixels3 = vld1q_u32(&pixels[12]);
            vst1q_u32(&args[j * width + i],      pixels0);
            vst1q_u32(&args[j * width + i + 4],  pixels1);
            vst1q_u32(&args[j * width + i + 8],  pixels2);
            vst1q_u32(&args[j * width + i + 12], pixels3);
        }
    }
    
    free(rand_offsets);
}

static void graph_glass_neon(graph_t* g, int x, int y, int w, int h, int r) {
    if (g == NULL || r == 0) {
        return;
    }

    grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return;
    x = ir.x;
    y = ir.y;
    w = ir.w;
    h = ir.h;

    glass_neon(g->buffer, g->w, g->h, x, y, w, h, 2);
}

static void gaussian_blur_neon(uint32_t* pixels, int width, int height,
                       int x, int y, int w, int h, int radius) {
    if (radius <= 0) return;
    
    // Clamp to valid bounds.
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    if (w <= 0 || h <= 0) return;
    
    // Build the Gaussian kernel.
    int kernel_size = radius * 2 + 1;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    float sigma = radius / 2.0f;
    float sum = 0.0f;
    
    for (int i = -radius; i <= radius; i++) {
        float val = expf(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = val;
        sum += val;
    }
    
    // Normalize the kernel.
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    // Temporary buffer.
    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    
    // NEON-optimized horizontal blur, processing 16 pixels at a time.
    for (int j = 0; j < h; j++) {
        // Preload data into the cache.
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&pixels[(y + j) * width + x]));
        
        for (int i = 0; i < w; i += 16) {
            if (i + 16 > w) {
                // Handle the remaining pixels when fewer than 16 are left.
                for (int k = i; k < w; k++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int px = x + k + m;
                        if (px < x) px = x;
                        if (px >= x + w) px = x + w - 1;
                        
                        uint32_t pixel = pixels[(y + j) * width + px];
                        float weight = kernel[m + radius];
                        
                        // Extract ARGB channels.
                        uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                        uint16x8_t vPixel16 = vmovl_u8(vPixel);
                        uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                        float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                        
                        // Multiply by the weight and accumulate.
                        accum = vmlaq_n_f32(accum, vPixelF, weight);
                    }
                    
                    // Convert back to integers and store the result.
                    uint32x4_t result = vcvtq_u32_f32(accum);
                    uint8x8_t res8 = vmovn_u16(vcombine_u16(
                        vmovn_u32(result),
                        vmovn_u32(result)
                    ));
                    temp[j * w + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
                }
                break;
            }
            
            // Process 16 pixels in parallel.
            float32x4_t accum[16] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // Process 16 pixels in parallel.
                for (int k = 0; k < 16; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    
                    // Extract ARGB channels.
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // Multiply by the weight and accumulate.
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // 转换为整数并存储
            for (int k = 0; k < 16; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                temp[j * w + i + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
        }
    }
    
    // NEON优化垂直模糊，并行处理16个像素
    for (int j = 0; j < h; j += 16) {
        if (j + 16 > h) {
            // 处理剩余不足16个像素的情况
            for (int k = j; k < h; k++) {
                for (int i = 0; i < w; i++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int py = y + k + m;
                        if (py < y) py = y;
                        if (py >= y + h) py = y + h - 1;
                        
                        uint32_t pixel = temp[(py - y) * w + i];
                        float weight = kernel[m + radius];
                        
                        // 提取ARGB通道
                        uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                        uint16x8_t vPixel16 = vmovl_u8(vPixel);
                        uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                        float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                        
                        // 乘以权重并累加
                        accum = vmlaq_n_f32(accum, vPixelF, weight);
                    }
                    
                    // 转换为整数并存储
                    uint32x4_t result = vcvtq_u32_f32(accum);
                    uint8x8_t res8 = vmovn_u16(vcombine_u16(
                        vmovn_u32(result),
                        vmovn_u32(result)
                    ));
                    pixels[(y + k) * width + (x + i)] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
                }
            }
            break;
        }
        
        for (int i = 0; i < w; i++) {
            // 预加载数据到缓存
            __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&temp[i]));
            
            // 并行处理16个像素
            float32x4_t accum[16] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // 处理16个像素
                for (int k = 0; k < 16; k++) {
                    int py = y + j + k + m;
                    if (py < y) py = y;
                    if (py >= y + h) py = y + h - 1;
                    
                    uint32_t pixel = temp[(py - y) * w + i];
                    
                    // 提取ARGB通道
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // 乘以权重并累加
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // 转换为整数并存储
            for (int k = 0; k < 16; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                pixels[(y + j + k) * width + (x + i)] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
        }
    }
    
    free(temp);
    free(kernel);
}

static void graph_gaussian_neon(graph_t* g, int x, int y, int w, int h, int r) {
    if (g == NULL || r == 0) {
        return;
    }

    grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return;
    x = ir.x;
    y = ir.y;
    w = ir.w;
    h = ir.h;

    gaussian_blur_neon(g->buffer, g->w, g->h, x, y, w, h, r);
}

inline void graph_glass_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_glass_neon(g, x, y, w, h, r);
}

inline void graph_gaussian_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_gaussian_neon(g, x, y, w, h, r);
}

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define WEIGHT_TABLE_SIZE 256
static float weight_table[WEIGHT_TABLE_SIZE];
static int weight_table_initialized = 0;

static inline float fast_sinf(float x) {
    const float PI = 3.141592653589793f;
    const float TWO_PI = 6.283185307179586f;
    
    x = fmodf(x, TWO_PI);
    if(x < -PI) x += TWO_PI;
    else if(x > PI) x -= TWO_PI;
    
    const float x2 = x * x;
    const float x4 = x2 * x2;
    return x * (0.99999994f + x2 * (-0.16666665f + x2 * 0.008333329f));
}

static inline float lanczos_sinc(float x) {
    x = fabsf(x);
    if(x < 0.0001f)
        return 1.0f;
    if(x >= 3.0f)
        return 0.0f;
    const float PI = 3.141592653589793f;
    float pi_x = PI * x;
    return fast_sinf(pi_x) / pi_x;
}

static inline float lanczos_kernel(float x, int a) {
    x = x < 0 ? -x : x;
    if(x < (float)a) {
        return lanczos_sinc(x) * lanczos_sinc(x / (float)a);
    }
    return 0.0f;
}

static void init_weight_table(void) {
    if(weight_table_initialized) return;
    
    for(int i = 0; i < WEIGHT_TABLE_SIZE; i++) {
        float x = (float)i / (WEIGHT_TABLE_SIZE - 1) * 6.0f - 3.0f;
        weight_table[i] = lanczos_kernel(x, 3);
    }
    
    weight_table_initialized = 1;
}

static inline float get_weight_fast(float x) {
    float normalized = (x + 3.0f) * (1.0f / 6.0f);
    int index = (int)(normalized * (WEIGHT_TABLE_SIZE - 1));
    if(index < 0) index = 0;
    if(index >= WEIGHT_TABLE_SIZE) index = WEIGHT_TABLE_SIZE - 1;
    return weight_table[index];
}

/* Fast floor: avoids the library call overhead of floorf */
static inline int fast_floorf(float x) {
    int i = (int)x;
    return i - ((float)i > x);
}

/* Max x-positions covered by the precomputed weight table per 16-pixel block.
   For scale >= 0.2 with lanczos_a=2 the span is at most ~100 positions;
   smaller scales take the inline-weight fallback below. */
#define SCALE_XW_MAX 128

void graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale) {
    init_weight_table();

    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    float effective_scale = scale < 1.0f ? (float)scale : 1.0f;
    const int lanczos_a = 2;
    float inv_scale = 1.0f / (float)scale;
    float adjusted_a = (float)lanczos_a / effective_scale;  /* hoisted: constant */
    int src_w = g->w;
    int src_h = g->h;
    const uint32_t* src_buf = g->buffer;
    uint32_t* dst_buf = dst->buffer;
    int dst_w = dst->w;
    int dst_h = dst->h;

    for(int i = 0; i < dst_h; i++) {
        float center_y = (float)i * inv_scale;

        /* --- Precompute y-weights once per output row (shared by all j-blocks) --- */
        int start_y = fast_floorf(center_y - adjusted_a);
        int end_y   = fast_floorf(center_y + adjusted_a) + 1;

        float          ky_arr[16];
        const uint32_t* row_ptrs[16];
        int n_y = 0;
        for(int y = start_y; y <= end_y && n_y < 16; y++) {
            float ky = get_weight_fast((y - center_y) * effective_scale);
            if(ky != 0.0f) {
                ky_arr[n_y]  = ky;
                row_ptrs[n_y] = src_buf + CLAMP(y, 0, src_h - 1) * src_w;
                n_y++;
            }
        }

        int j = 0;
        for(; j <= dst_w - 16; j += 16) {
            float center_x[16];
            int start_x[16], end_x[16];

            for(int k = 0; k < 16; k++) {
                center_x[k] = (float)(j + k) * inv_scale;
                start_x[k]  = fast_floorf(center_x[k] - adjusted_a);
                end_x[k]    = fast_floorf(center_x[k] + adjusted_a) + 1;
            }

            int min_start_x = start_x[0];
            int max_end_x   = end_x[0];
            for(int k = 1; k < 16; k++) {
                if(start_x[k] < min_start_x) min_start_x = start_x[k];
                if(end_x[k]   > max_end_x)   max_end_x   = end_x[k];
            }

            int num_x = max_end_x - min_start_x + 1;

            /* --- Precompute x-weights once per block (reused across all y-rows) --- */
            float xw[SCALE_XW_MAX * 16];
            int use_precomputed = (num_x <= SCALE_XW_MAX) && (n_y >= 2);

            if(use_precomputed) {
                for(int x = min_start_x; x <= max_end_x; x++) {
                    float* xw_row = &xw[(x - min_start_x) * 16];
                    for(int k = 0; k < 16; k++) {
                        xw_row[k] = (x >= start_x[k] && x <= end_x[k])
                            ? get_weight_fast(((float)x - center_x[k]) * effective_scale)
                            : 0.0f;
                    }
                }
            }

            float32x4_t sum_r[4]      = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_g[4]      = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_b[4]      = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_a[4]      = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_weight[4] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};

            for(int yi = 0; yi < n_y; yi++) {
                float32x4_t ky_vec = vdupq_n_f32(ky_arr[yi]);
                const uint32_t* row_ptr = row_ptrs[yi];

                if(use_precomputed) {
                    /* x-weights already include the per-pixel range check */
                    for(int x = min_start_x; x <= max_end_x; x++) {
                        const float* xw_row = &xw[(x - min_start_x) * 16];
                        float32x4_t weight0 = vmulq_f32(vld1q_f32(xw_row),      ky_vec);
                        float32x4_t weight1 = vmulq_f32(vld1q_f32(xw_row + 4),  ky_vec);
                        float32x4_t weight2 = vmulq_f32(vld1q_f32(xw_row + 8),  ky_vec);
                        float32x4_t weight3 = vmulq_f32(vld1q_f32(xw_row + 12), ky_vec);

                        int cx = CLAMP(x, 0, src_w - 1);
                        uint32_t p = row_ptr[cx];

                        float32x4_t r_f = vdupq_n_f32((float)((p >> 16) & 0xFF));
                        float32x4_t g_f = vdupq_n_f32((float)((p >>  8) & 0xFF));
                        float32x4_t b_f = vdupq_n_f32((float)( p        & 0xFF));
                        float32x4_t a_f = vdupq_n_f32((float)((p >> 24) & 0xFF));

                        sum_r[0] = vfmaq_f32(sum_r[0], r_f, weight0);
                        sum_r[1] = vfmaq_f32(sum_r[1], r_f, weight1);
                        sum_r[2] = vfmaq_f32(sum_r[2], r_f, weight2);
                        sum_r[3] = vfmaq_f32(sum_r[3], r_f, weight3);
                        sum_g[0] = vfmaq_f32(sum_g[0], g_f, weight0);
                        sum_g[1] = vfmaq_f32(sum_g[1], g_f, weight1);
                        sum_g[2] = vfmaq_f32(sum_g[2], g_f, weight2);
                        sum_g[3] = vfmaq_f32(sum_g[3], g_f, weight3);
                        sum_b[0] = vfmaq_f32(sum_b[0], b_f, weight0);
                        sum_b[1] = vfmaq_f32(sum_b[1], b_f, weight1);
                        sum_b[2] = vfmaq_f32(sum_b[2], b_f, weight2);
                        sum_b[3] = vfmaq_f32(sum_b[3], b_f, weight3);
                        sum_a[0] = vfmaq_f32(sum_a[0], a_f, weight0);
                        sum_a[1] = vfmaq_f32(sum_a[1], a_f, weight1);
                        sum_a[2] = vfmaq_f32(sum_a[2], a_f, weight2);
                        sum_a[3] = vfmaq_f32(sum_a[3], a_f, weight3);
                        sum_weight[0] = vaddq_f32(sum_weight[0], weight0);
                        sum_weight[1] = vaddq_f32(sum_weight[1], weight1);
                        sum_weight[2] = vaddq_f32(sum_weight[2], weight2);
                        sum_weight[3] = vaddq_f32(sum_weight[3], weight3);
                    }
                } else {
                    /* Fallback for very wide kernels: compute kx inline */
                    for(int x = min_start_x; x <= max_end_x; x++) {
                        float kx[16];
                        for(int k = 0; k < 16; k++)
                            kx[k] = (x >= start_x[k] && x <= end_x[k])
                                ? get_weight_fast(((float)x - center_x[k]) * effective_scale)
                                : 0.0f;

                        float32x4_t weight0 = vmulq_f32(vld1q_f32(kx),      ky_vec);
                        float32x4_t weight1 = vmulq_f32(vld1q_f32(kx + 4),  ky_vec);
                        float32x4_t weight2 = vmulq_f32(vld1q_f32(kx + 8),  ky_vec);
                        float32x4_t weight3 = vmulq_f32(vld1q_f32(kx + 12), ky_vec);

                        int cx = CLAMP(x, 0, src_w - 1);
                        uint32_t p = row_ptr[cx];

                        float32x4_t r_f = vdupq_n_f32((float)((p >> 16) & 0xFF));
                        float32x4_t g_f = vdupq_n_f32((float)((p >>  8) & 0xFF));
                        float32x4_t b_f = vdupq_n_f32((float)( p        & 0xFF));
                        float32x4_t a_f = vdupq_n_f32((float)((p >> 24) & 0xFF));

                        sum_r[0] = vfmaq_f32(sum_r[0], r_f, weight0);
                        sum_r[1] = vfmaq_f32(sum_r[1], r_f, weight1);
                        sum_r[2] = vfmaq_f32(sum_r[2], r_f, weight2);
                        sum_r[3] = vfmaq_f32(sum_r[3], r_f, weight3);
                        sum_g[0] = vfmaq_f32(sum_g[0], g_f, weight0);
                        sum_g[1] = vfmaq_f32(sum_g[1], g_f, weight1);
                        sum_g[2] = vfmaq_f32(sum_g[2], g_f, weight2);
                        sum_g[3] = vfmaq_f32(sum_g[3], g_f, weight3);
                        sum_b[0] = vfmaq_f32(sum_b[0], b_f, weight0);
                        sum_b[1] = vfmaq_f32(sum_b[1], b_f, weight1);
                        sum_b[2] = vfmaq_f32(sum_b[2], b_f, weight2);
                        sum_b[3] = vfmaq_f32(sum_b[3], b_f, weight3);
                        sum_a[0] = vfmaq_f32(sum_a[0], a_f, weight0);
                        sum_a[1] = vfmaq_f32(sum_a[1], a_f, weight1);
                        sum_a[2] = vfmaq_f32(sum_a[2], a_f, weight2);
                        sum_a[3] = vfmaq_f32(sum_a[3], a_f, weight3);
                        sum_weight[0] = vaddq_f32(sum_weight[0], weight0);
                        sum_weight[1] = vaddq_f32(sum_weight[1], weight1);
                        sum_weight[2] = vaddq_f32(sum_weight[2], weight2);
                        sum_weight[3] = vaddq_f32(sum_weight[3], weight3);
                    }
                }
            }

            float32x4_t inv_weight0 = vrecpeq_f32(sum_weight[0]);
            float32x4_t inv_weight1 = vrecpeq_f32(sum_weight[1]);
            float32x4_t inv_weight2 = vrecpeq_f32(sum_weight[2]);
            float32x4_t inv_weight3 = vrecpeq_f32(sum_weight[3]);
            inv_weight0 = vmulq_f32(inv_weight0, vrecpsq_f32(sum_weight[0], inv_weight0));
            inv_weight1 = vmulq_f32(inv_weight1, vrecpsq_f32(sum_weight[1], inv_weight1));
            inv_weight2 = vmulq_f32(inv_weight2, vrecpsq_f32(sum_weight[2], inv_weight2));
            inv_weight3 = vmulq_f32(inv_weight3, vrecpsq_f32(sum_weight[3], inv_weight3));

            sum_r[0] = vmulq_f32(sum_r[0], inv_weight0);
            sum_r[1] = vmulq_f32(sum_r[1], inv_weight1);
            sum_r[2] = vmulq_f32(sum_r[2], inv_weight2);
            sum_r[3] = vmulq_f32(sum_r[3], inv_weight3);
            sum_g[0] = vmulq_f32(sum_g[0], inv_weight0);
            sum_g[1] = vmulq_f32(sum_g[1], inv_weight1);
            sum_g[2] = vmulq_f32(sum_g[2], inv_weight2);
            sum_g[3] = vmulq_f32(sum_g[3], inv_weight3);
            sum_b[0] = vmulq_f32(sum_b[0], inv_weight0);
            sum_b[1] = vmulq_f32(sum_b[1], inv_weight1);
            sum_b[2] = vmulq_f32(sum_b[2], inv_weight2);
            sum_b[3] = vmulq_f32(sum_b[3], inv_weight3);
            sum_a[0] = vmulq_f32(sum_a[0], inv_weight0);
            sum_a[1] = vmulq_f32(sum_a[1], inv_weight1);
            sum_a[2] = vmulq_f32(sum_a[2], inv_weight2);
            sum_a[3] = vmulq_f32(sum_a[3], inv_weight3);

            uint16x8_t result_r01 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_r[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_r[1])));
            uint16x8_t result_r23 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_r[2])),
                vqmovn_u32(vcvtq_u32_f32(sum_r[3])));
            uint16x8_t result_g01 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_g[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_g[1])));
            uint16x8_t result_g23 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_g[2])),
                vqmovn_u32(vcvtq_u32_f32(sum_g[3])));
            uint16x8_t result_b01 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_b[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_b[1])));
            uint16x8_t result_b23 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_b[2])),
                vqmovn_u32(vcvtq_u32_f32(sum_b[3])));
            uint16x8_t result_a01 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_a[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_a[1])));
            uint16x8_t result_a23 = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_a[2])),
                vqmovn_u32(vcvtq_u32_f32(sum_a[3])));

            uint8x16_t r16 = vcombine_u8(vqmovn_u16(result_r01), vqmovn_u16(result_r23));
            uint8x16_t g16 = vcombine_u8(vqmovn_u16(result_g01), vqmovn_u16(result_g23));
            uint8x16_t b16 = vcombine_u8(vqmovn_u16(result_b01), vqmovn_u16(result_b23));
            uint8x16_t a16 = vcombine_u8(vqmovn_u16(result_a01), vqmovn_u16(result_a23));

            /* interleave channels into B,G,R,A byte order (little-endian ARGB) */
            uint8x16x2_t bg = vzipq_u8(b16, g16);
            uint8x16x2_t ra = vzipq_u8(r16, a16);
            uint16x8x2_t plo = vzipq_u16(vreinterpretq_u16_u8(bg.val[0]),
                                         vreinterpretq_u16_u8(ra.val[0]));
            uint16x8x2_t phi = vzipq_u16(vreinterpretq_u16_u8(bg.val[1]),
                                         vreinterpretq_u16_u8(ra.val[1]));

            vst1q_u32(&dst_buf[i * dst_w + j],      vreinterpretq_u32_u16(plo.val[0]));
            vst1q_u32(&dst_buf[i * dst_w + j + 4],  vreinterpretq_u32_u16(plo.val[1]));
            vst1q_u32(&dst_buf[i * dst_w + j + 8],  vreinterpretq_u32_u16(phi.val[0]));
            vst1q_u32(&dst_buf[i * dst_w + j + 12], vreinterpretq_u32_u16(phi.val[1]));
        }

        /* Scalar tail: use get_weight_fast instead of expensive lanczos_kernel */
        for(; j < dst_w; j++) {
            float center_x = (float)j * inv_scale;
            int start_x = fast_floorf(center_x - adjusted_a);
            int end_x   = fast_floorf(center_x + adjusted_a) + 1;

            float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f, sum_a = 0.0f;
            float sum_weight = 0.0f;

            for(int yi = 0; yi < n_y; yi++) {
                float ky = ky_arr[yi];
                const uint32_t* row_ptr = row_ptrs[yi];
                for(int x = start_x; x <= end_x; x++) {
                    float kx = get_weight_fast((x - center_x) * effective_scale);
                    if(kx == 0.0f) continue;
                    float weight = ky * kx;
                    int cx = CLAMP(x, 0, src_w - 1);
                    uint32_t p = row_ptr[cx];
                    sum_r += (float)((p >> 16) & 0xFF) * weight;
                    sum_g += (float)((p >>  8) & 0xFF) * weight;
                    sum_b += (float)( p        & 0xFF) * weight;
                    sum_a += (float)((p >> 24) & 0xFF) * weight;
                    sum_weight += weight;
                }
            }

            if(sum_weight != 0.0f) {
                float inv_w = 1.0f / sum_weight;
                sum_r *= inv_w;
                sum_g *= inv_w;
                sum_b *= inv_w;
                sum_a *= inv_w;
            }

            uint8_t r     = (uint8_t)CLAMP(sum_r, 0.0f, 255.0f);
            uint8_t g_val = (uint8_t)CLAMP(sum_g, 0.0f, 255.0f);
            uint8_t b     = (uint8_t)CLAMP(sum_b, 0.0f, 255.0f);
            uint8_t a_val = (uint8_t)CLAMP(sum_a, 0.0f, 255.0f);
            dst_buf[i * dst_w + j] = ((uint32_t)a_val << 24) | ((uint32_t)r << 16)
                                   | ((uint32_t)g_val <<  8) |  (uint32_t)b;
        }
    }
}

enum {
    GRAPH_SCALE_FIXED_SHIFT = 16,
    GRAPH_SCALE_FIXED_SCALE = 1 << GRAPH_SCALE_FIXED_SHIFT,
    GRAPH_SCALE_FIXED_MASK = GRAPH_SCALE_FIXED_SCALE - 1
};

/* interpolate the packed (byte 0, byte 2) channels with an 8-bit weight;
   w0+w1 = 256 keeps each 16-bit lane carry-free (255*256 = 0xFF00) */
static inline uint32_t graph_scale_lerp_rb_bsp(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1) >> 8) & 0x00FF00FF);
}

static inline uint32_t graph_scale_lerp_ga_bsp(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1) >> 8)
            & 0x00FF00FF) << 8);
}

/* same, with +128 rounding per lane to match the NEON vmla/rnd paths
   bit-exactly (per-lane max is 255*256 + 128 = 0xFF80, still carry-free) */
static inline uint32_t graph_scale_lerp_rb_bsp_r(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1 + 0x00800080u) >> 8)
            & 0x00FF00FF);
}

static inline uint32_t graph_scale_lerp_ga_bsp_r(uint32_t a, uint32_t b, uint32_t w1) {
    uint32_t w0 = 256 - w1;
    return ((((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1 + 0x00800080u) >> 8)
            & 0x00FF00FF) << 8);
}

static inline uint32_t graph_scale_bilinear_interp_bsp(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
        uint32_t fx, uint32_t fy) {
    /* quantize 16.16 fractions to 8-bit rounded weights, matching the NEON
       path precision; 8x32-bit multiplies instead of 16x64-bit */
    uint32_t fx8 = (fx + 128) >> 8;
    uint32_t fy8 = (fy + 128) >> 8;

    uint32_t top_rb = graph_scale_lerp_rb_bsp(p00, p01, fx8);
    uint32_t top_ga = graph_scale_lerp_ga_bsp(p00, p01, fx8);
    uint32_t bot_rb = graph_scale_lerp_rb_bsp(p10, p11, fx8);
    uint32_t bot_ga = graph_scale_lerp_ga_bsp(p10, p11, fx8);

    return graph_scale_lerp_rb_bsp(top_rb, bot_rb, fy8) |
           graph_scale_lerp_ga_bsp(top_ga, bot_ga, fy8);
}

static void graph_scale_prepare_axis_bsp(int dst_len, int src_max, uint32_t inv_scale,
        int *idx0, int *idx1, uint32_t *frac) {
    uint32_t pos = 0;

    for(int i = 0; i < dst_len; i++) {
        int base = (int)(pos >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t f = pos & GRAPH_SCALE_FIXED_MASK;
        int next = base + 1;

        if(base >= src_max) {
            base = src_max;
            next = src_max;
            f = 0;
        }
        else if(next > src_max) {
            next = src_max;
        }

        idx0[i] = base;
        idx1[i] = next;
        frac[i] = f;
        pos += inv_scale;
    }
}

static int graph_scale_integer_downsample_bsp(graph_t* g, graph_t* dst, uint32_t inv_scale) {
    if((inv_scale & GRAPH_SCALE_FIXED_MASK) != 0)
        return 0;

    uint32_t step = inv_scale >> GRAPH_SCALE_FIXED_SHIFT;
    if(step < 2)
        return 0;

    int src_w = g->w;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int is_pow2 = (step & (step - 1)) == 0;

    if(is_pow2) {
        unsigned step_shift = 0;
        while((1U << step_shift) < step)
            step_shift++;

        for(int y = 0; y < dst_h; y++) {
            uint32_t *src_row = &g->buffer[(y << step_shift) * src_w];
            uint32_t *dst_row = &dst->buffer[y * dst_w];
            int x = 0;

            for(; x <= dst_w - 4; x += 4) {
                dst_row[x] = src_row[x << step_shift];
                dst_row[x + 1] = src_row[(x + 1) << step_shift];
                dst_row[x + 2] = src_row[(x + 2) << step_shift];
                dst_row[x + 3] = src_row[(x + 3) << step_shift];
            }

            for(; x < dst_w; x++) {
                dst_row[x] = src_row[x << step_shift];
            }
        }

        return 1;
    }

    uint32_t src_y = 0;
    for(int y = 0; y < dst_h; y++) {
        uint32_t *src_row = &g->buffer[src_y * src_w];
        uint32_t *dst_row = &dst->buffer[y * dst_w];
        uint32_t src_x = 0;
        int x = 0;

        for(; x <= dst_w - 4; x += 4) {
            dst_row[x] = src_row[src_x];
            src_x += step;
            dst_row[x + 1] = src_row[src_x];
            src_x += step;
            dst_row[x + 2] = src_row[src_x];
            src_x += step;
            dst_row[x + 3] = src_row[src_x];
            src_x += step;
        }

        for(; x < dst_w; x++) {
            dst_row[x] = src_row[src_x];
            src_x += step;
        }

        src_y += step;
    }

    return 1;
}

/* Separable two-pass upscale for scale >= 1 (inv_scale <= FIXED_SCALE):
   each source row feeds ~scale destination rows, so the horizontal lerp is
   computed once per source row into a 2-slot row cache (gi0 advances <= 1
   per destination row), and the vertical pass becomes a contiguous scan.
   All scratch comes from a single malloc. Returns 0 on malloc failure so
   the caller falls back to the gather path. */
static int graph_scale_separable_upscale_bsp(graph_t* g, graph_t* dst, uint32_t inv_scale) {
    int src_w = g->w;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int hmax = g->h - 1;
    int wmax = src_w - 1;

    size_t cols = (size_t)dst_w;
    size_t fx8_bytes = (cols * sizeof(uint16_t) + 3u) & ~(size_t)3u; /* keep u32 rows aligned */
    uint8_t *mem = (uint8_t*)malloc(cols * 2 * sizeof(int) + fx8_bytes +
                                    2 * cols * sizeof(uint32_t));
    if(mem == NULL)
        return 0;

    int *x0 = (int*)mem;
    int *x1 = x0 + cols;
    uint16_t *fx8 = (uint16_t*)(x1 + cols);
    uint32_t *hrow[2];
    hrow[0] = (uint32_t*)((uint8_t*)fx8 + fx8_bytes);
    hrow[1] = hrow[0] + cols;
    int hrow_y[2] = {-2, -2}; /* source row cached in each slot */

    /* column mapping + 8-bit rounded weights, computed once */
    uint32_t pos = 0;
    for(int j = 0; j < dst_w; j++) {
        int base = (int)(pos >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t f = pos & GRAPH_SCALE_FIXED_MASK;
        int next = base + 1;

        if(base >= wmax) {
            base = wmax;
            next = wmax;
            f = 0;
        }
        else if(next > wmax) {
            next = wmax;
        }

        x0[j] = base;
        x1[j] = next;
        fx8[j] = (uint16_t)((f + 128) >> 8); /* 0..256 */
        pos += inv_scale;
    }

    uint32_t src_y = 0;
    for(int i = 0; i < dst_h; i++) {
        int gi0 = (int)(src_y >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t gi_frac = src_y & GRAPH_SCALE_FIXED_MASK;

        if(gi0 >= hmax) {
            gi0 = hmax;
            gi_frac = 0;
        }
        int gi1 = (gi0 < hmax) ? gi0 + 1 : hmax;
        uint16_t fy8 = (uint16_t)((gi_frac + 128) >> 8);
        if(fy8 == 256) {
            /* folds exactly into the next source row (bit-exact) */
            gi0 = gi1;
            fy8 = 0;
        }

        /* make sure both needed rows are cached; row access is monotonic,
           so evicting the slot holding the lowest row is always safe */
        for(int n = 0; n < 2; n++) {
            int y = (n == 0) ? gi0 : gi1;
            if(hrow_y[0] == y || hrow_y[1] == y)
                continue;

            int slot = (hrow_y[0] < hrow_y[1]) ? 0 : 1;
            const uint32_t *srow = g->buffer + y * src_w;
            uint32_t *hr = hrow[slot];

            for(int j = 0; j < dst_w; j++) {
                uint32_t a = srow[x0[j]];
                uint32_t b = srow[x1[j]];

                if(a == b)
                    hr[j] = a;
                else
                    hr[j] = graph_scale_lerp_rb_bsp_r(a, b, fx8[j]) |
                            graph_scale_lerp_ga_bsp_r(a, b, fx8[j]);
            }
            hrow_y[slot] = y;
        }

        const uint32_t *r0 = (hrow_y[0] == gi0) ? hrow[0] : hrow[1];
        const uint32_t *r1 = (hrow_y[0] == gi1) ? hrow[0] : hrow[1];
        uint32_t *drow = dst->buffer + i * dst_w;

        if(fy8 == 0 || r0 == r1) {
            memcpy(drow, r0, cols * sizeof(uint32_t));
        }
        else {
            uint16x8_t wv0 = vdupq_n_u16((uint16_t)(256 - fy8));
            uint16x8_t wv1 = vdupq_n_u16(fy8);
            uint16x8_t rnd = vdupq_n_u16(128);

            int j = 0;
            for(; j <= dst_w - 4; j += 4) {
                uint8x16_t b0 = vreinterpretq_u8_u32(vld1q_u32(r0 + j));
                uint8x16_t b1 = vreinterpretq_u8_u32(vld1q_u32(r1 + j));

                uint16x8_t t0l = vmovl_u8(vget_low_u8(b0));
                uint16x8_t t0h = vmovl_u8(vget_high_u8(b0));
                uint16x8_t t1l = vmovl_u8(vget_low_u8(b1));
                uint16x8_t t1h = vmovl_u8(vget_high_u8(b1));

                uint16x8_t ol = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, t0l, wv0), t1l, wv1), 8);
                uint16x8_t oh = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, t0h, wv0), t1h, wv1), 8);

                vst1q_u32(drow + j,
                        vreinterpretq_u32_u8(vcombine_u8(vmovn_u16(ol), vmovn_u16(oh))));
            }

            for(; j < dst_w; j++) {
                drow[j] = graph_scale_lerp_rb_bsp_r(r0[j], r1[j], fy8) |
                          graph_scale_lerp_ga_bsp_r(r0[j], r1[j], fy8);
            }
        }

        src_y += inv_scale;
    }

    free(mem);
    return 1;
}

static inline uint16x8_t graph_scale_expand_w0_bsp(uint8x8_t w1_bytes) {
    /* 256 - w1 per u16 lane (w1 in 0..255) */
    return vsubq_u16(vdupq_n_u16(256), vmovl_u8(w1_bytes));
}

#if defined(__GNUC__) && !defined(__clang__)
/* GCC with -mstrict-align lowers vld1_u32 on a pointer without proven 8-byte
   alignment to 2 scalar loads + orr + fmov. LD1 itself supports unaligned
   addresses in normal memory, so emit the single-instruction load directly. */
static inline uint32x2_t graph_scale_ld1_u32_pair_bsp(const uint32_t *p) {
    uint32x2_t v;
    __asm__("ld1 {%0.2s}, [%1]" : "=w"(v) : "r"(p) : );
    return v;
}
#else
static inline uint32x2_t graph_scale_ld1_u32_pair_bsp(const uint32_t *p) {
    return vld1_u32(p);
}
#endif

/* Difference-form u8 lerp, same integer arithmetic as the scalar w8 path:
   out = a + round((b - a) * w1 / 256), w1 unsigned 0..255. NEON has no
   mixed-sign u8xs8 widening multiply, so take the difference of two unsigned
   widening products; it lands in s16 range (|.| <= 65025) and vrshrn applies
   the +128 rounding shift. No pixel widening, no w0 weight vectors. */
static inline uint8x8_t graph_scale_lerp8_bsp(uint8x8_t a, uint8x8_t b, uint8x8_t w1) {
    int16x8_t acc = vreinterpretq_s16_u16(vsubq_u16(vmull_u8(b, w1), vmull_u8(a, w1)));
    return vadd_u8(a, vreinterpret_u8_s8(vrshrn_n_s16(acc, 8)));
}

void graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    int src_w = g->w;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int hmax = g->h - 1;
    int wmax = src_w - 1;
    uint32_t inv_scale = (uint32_t)((1.0f / scale) * GRAPH_SCALE_FIXED_SCALE);

    if(inv_scale == GRAPH_SCALE_FIXED_SCALE && dst_w == src_w && dst_h == g->h) {
        memcpy(dst->buffer, g->buffer, (size_t)src_w * (size_t)g->h * sizeof(uint32_t));
        return;
    }

    if(scale < 1.0 && graph_scale_integer_downsample_bsp(g, dst, inv_scale))
        return;

    /* scale >= 1: separable two-pass with row cache; falls through to the
       gather path below on malloc failure */
    if(inv_scale <= GRAPH_SCALE_FIXED_SCALE &&
            graph_scale_separable_upscale_bsp(g, dst, inv_scale))
        return;

    int *x0 = (int*)malloc((size_t)dst_w * sizeof(int));
    int *x1 = (int*)malloc((size_t)dst_w * sizeof(int));
    uint32_t *x_frac = (uint32_t*)malloc((size_t)dst_w * sizeof(uint32_t));
    uint8_t *fx8 = (uint8_t*)malloc((size_t)dst_w * sizeof(uint8_t));

    if(x0 != NULL && x1 != NULL && x_frac != NULL && fx8 != NULL) {
        graph_scale_prepare_axis_bsp(dst_w, wmax, inv_scale, x0, x1, x_frac);

        /* per-column 8-bit horizontal weights, expanded in-register per
           iteration; a weight of 256 folds exactly into the next column */
        for(int j = 0; j < dst_w; j++) {
            uint32_t f = (x_frac[j] + 128) >> 8; /* 0..256 */
            if(f == 256) {
                f = 0;
                if(x0[j] < wmax) {
                    x0[j]++;
                    x1[j] = (x0[j] < wmax) ? x0[j] + 1 : wmax;
                }
            }
            fx8[j] = (uint8_t)f;
        }

        /* pair-load of (x0[j], x0[j]+1) stays in bounds while x0[j] < wmax;
           x0 is non-decreasing, so the NEON-safe prefix is contiguous */
        int j_safe = 0;
        while(j_safe < dst_w && x0[j_safe] < wmax)
            j_safe++;
        int neon_w = j_safe & ~3;

        uint32_t src_y = 0;
        for(int i = 0; i < dst_h; i++) {
            int gi0 = (int)(src_y >> GRAPH_SCALE_FIXED_SHIFT);
            uint32_t gi_frac = src_y & GRAPH_SCALE_FIXED_MASK;
            int gi1 = gi0 + 1;

            if(gi0 >= hmax) {
                gi0 = hmax;
                gi1 = hmax;
                gi_frac = 0;
            }
            else if(gi1 > hmax) {
                gi1 = hmax;
            }

            const uint32_t *row0 = g->buffer + gi0 * src_w;
            const uint32_t *row1 = g->buffer + gi1 * src_w;
            uint32_t *drow = dst->buffer + i * dst_w;

            uint16_t fy = (uint16_t)((gi_frac + 128) >> 8);
            if(fy == 256) {
                /* folds exactly into the next source row (bit-exact, same as
                   the scalar w8 path); keeps the weight in u8 range */
                gi1 = gi0;
                row1 = row0;
                fy = 0;
            }
            uint8x8_t wy = vdup_n_u8((uint8_t)fy);

            int j = 0;
            for(; j < neon_w; j += 4) {
                /* gather (p00,p01) pairs for 4 output columns from both rows;
                   x1[j] == x0[j]+1 in the NEON-safe prefix */
                uint32x4_t ab0 = vcombine_u32(graph_scale_ld1_u32_pair_bsp(row0 + x0[j]),
                                              graph_scale_ld1_u32_pair_bsp(row0 + x0[j + 1]));
                uint32x4_t cd0 = vcombine_u32(graph_scale_ld1_u32_pair_bsp(row0 + x0[j + 2]),
                                              graph_scale_ld1_u32_pair_bsp(row0 + x0[j + 3]));
                uint32x4x2_t u0 = vuzpq_u32(ab0, cd0);

                uint32x4_t ab1 = vcombine_u32(graph_scale_ld1_u32_pair_bsp(row1 + x0[j]),
                                              graph_scale_ld1_u32_pair_bsp(row1 + x0[j + 1]));
                uint32x4_t cd1 = vcombine_u32(graph_scale_ld1_u32_pair_bsp(row1 + x0[j + 2]),
                                              graph_scale_ld1_u32_pair_bsp(row1 + x0[j + 3]));
                uint32x4x2_t u1 = vuzpq_u32(ab1, cd1);

                /* flat 4-pixel block: skip weights and the whole lerp chain */
                uint32x4_t neq = vorrq_u32(
                        vorrq_u32(veorq_u32(u0.val[0], u0.val[1]),
                                  veorq_u32(u0.val[0], u1.val[0])),
                        veorq_u32(u0.val[0], u1.val[1]));
                if(vmaxvq_u32(neq) == 0) {
                    vst1q_u32(drow + j, u0.val[0]);
                    continue;
                }

                /* horizontal weight vectors from 4 bytes of fx8:
                   [a b c d] -> l: [aaaa bbbb], h: [cccc dddd] as w1 bytes.
                   lane-load keeps the read within the fx8 array bounds */
                uint8x8_t f8 = vreinterpret_u8_u32(
                        vld1_lane_u32((const uint32_t*)(fx8 + j), vdup_n_u32(0), 0));
                uint8x8x2_t fz = vzip_u8(f8, f8);
                uint8x8x2_t wl = vzip_u8(fz.val[0], fz.val[0]);

                /* horizontal lerp in u8 (2 pixels per d-register half) */
                uint8x16_t tl = vcombine_u8(
                        graph_scale_lerp8_bsp(vget_low_u8(vreinterpretq_u8_u32(u0.val[0])),
                                              vget_low_u8(vreinterpretq_u8_u32(u0.val[1])),
                                              wl.val[0]),
                        graph_scale_lerp8_bsp(vget_high_u8(vreinterpretq_u8_u32(u0.val[0])),
                                              vget_high_u8(vreinterpretq_u8_u32(u0.val[1])),
                                              wl.val[1]));
                uint8x16_t bl = vcombine_u8(
                        graph_scale_lerp8_bsp(vget_low_u8(vreinterpretq_u8_u32(u1.val[0])),
                                              vget_low_u8(vreinterpretq_u8_u32(u1.val[1])),
                                              wl.val[0]),
                        graph_scale_lerp8_bsp(vget_high_u8(vreinterpretq_u8_u32(u1.val[0])),
                                              vget_high_u8(vreinterpretq_u8_u32(u1.val[1])),
                                              wl.val[1]));

                /* vertical lerp in u8 + pack */
                uint8x8_t ol = graph_scale_lerp8_bsp(vget_low_u8(tl), vget_low_u8(bl), wy);
                uint8x8_t oh = graph_scale_lerp8_bsp(vget_high_u8(tl), vget_high_u8(bl), wy);

                vst1q_u32(drow + j, vreinterpretq_u32_u8(vcombine_u8(ol, oh)));
            }

            for(; j < dst_w; j++) {
                uint32_t p00 = row0[x0[j]];
                uint32_t p01 = row0[x1[j]];
                uint32_t p10 = row1[x0[j]];
                uint32_t p11 = row1[x1[j]];

                if(p00 == p01 && p00 == p10 && p00 == p11) {
                    drow[j] = p00;
                }
                else {
                    drow[j] = graph_scale_bilinear_interp_bsp(p00, p01, p10, p11, x_frac[j], gi_frac);
                }
            }

            src_y += inv_scale;
        }

        free(x0);
        free(x1);
        free(x_frac);
        free(fx8);
        return;
    }

    free(x0);
    free(x1);
    free(x_frac);
    free(fx8);

    uint32_t src_y = 0;
    for(int i = 0; i < dst_h; i++) {
        int gi0 = (int)(src_y >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t gi_frac = src_y & GRAPH_SCALE_FIXED_MASK;
        int gi1 = gi0 + 1;

        if(gi0 >= hmax) {
            gi0 = hmax;
            gi1 = hmax;
            gi_frac = 0;
        }
        else if(gi1 > hmax) {
            gi1 = hmax;
        }

        int gi0w = gi0 * src_w;
        int gi1w = gi1 * src_w;
        int dst_row = i * dst_w;
        uint32_t src_x = 0;

        for(int j = 0; j < dst_w; j++) {
            int gj0 = (int)(src_x >> GRAPH_SCALE_FIXED_SHIFT);
            uint32_t gj_frac = src_x & GRAPH_SCALE_FIXED_MASK;
            int gj1 = gj0 + 1;

            if(gj0 >= wmax) {
                gj0 = wmax;
                gj1 = wmax;
                gj_frac = 0;
            }
            else if(gj1 > wmax) {
                gj1 = wmax;
            }

            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + gj1];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + gj1];

            if(p00 == p01 && p00 == p10 && p00 == p11) {
                dst->buffer[dst_row + j] = p00;
            }
            else {
                dst->buffer[dst_row + j] = graph_scale_bilinear_interp_bsp(p00, p01, p10, p11, gj_frac, gi_frac);
            }

            src_x += inv_scale;
        }

        src_y += inv_scale;
    }
}

static inline uint32x4_t neon_reverse_u32x4(uint32x4_t v) {
    return vcombine_u32(vrev64_u32(vget_high_u32(v)), vrev64_u32(vget_low_u32(v)));
}

static inline uint8x8_t neon_rgb_to_y8(uint8x8_t b8, uint8x8_t g8, uint8x8_t r8) {
    uint16x8_t b16 = vmovl_u8(b8);
    uint16x8_t g16 = vmovl_u8(g8);
    uint16x8_t r16 = vmovl_u8(r8);

    uint32x4_t y0 = vmulq_n_u32(vmovl_u16(vget_low_u16(r16)), 306);
    y0 = vmlaq_n_u32(y0, vmovl_u16(vget_low_u16(g16)), 601);
    y0 = vmlaq_n_u32(y0, vmovl_u16(vget_low_u16(b16)), 117);

    uint32x4_t y1 = vmulq_n_u32(vmovl_u16(vget_high_u16(r16)), 306);
    y1 = vmlaq_n_u32(y1, vmovl_u16(vget_high_u16(g16)), 601);
    y1 = vmlaq_n_u32(y1, vmovl_u16(vget_high_u16(b16)), 117);

    y0 = vshrq_n_u32(y0, 10);
    y1 = vshrq_n_u32(y1, 10);

    return vmovn_u16(vcombine_u16(vmovn_u32(y0), vmovn_u32(y1)));
}

static inline uint8x16_t neon_rgb_to_y16(uint8x16_t b, uint8x16_t g, uint8x16_t r) {
    return vcombine_u8(neon_rgb_to_y8(vget_low_u8(b), vget_low_u8(g), vget_low_u8(r)),
                       neon_rgb_to_y8(vget_high_u8(b), vget_high_u8(g), vget_high_u8(r)));
}

static inline uint8_t rgb_to_y_scalar_bsp(uint32_t pixel) {
    uint32_t b = pixel & 0xff;
    uint32_t g = (pixel >> 8) & 0xff;
    uint32_t r = (pixel >> 16) & 0xff;
    return (uint8_t)((306 * r + 601 * g + 117 * b) >> 10);
}

static inline void rgb_to_uv_scalar_bsp(uint32_t pixel, uint8_t *uv) {
    int32_t b = (int32_t)(pixel & 0xff);
    int32_t g = (int32_t)((pixel >> 8) & 0xff);
    int32_t r = (int32_t)((pixel >> 16) & 0xff);

    uv[0] = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
    uv[1] = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

static inline uint32_t rgb2nv12_get_src_pixel(const uint32_t *in, int w, int h, int y, int x) {
    return in[(h - 1 - y) * w + (w - 1 - x)];
}

void argb_2_nv12_arch(uint8_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    uint8_t *y_plane = out;
    uint8_t *uv_plane = out + w * h;

    for(int y = 0; y < h; y += 2) {
        uint8_t *y_row0 = y_plane + y * w;
        uint8_t *y_row1 = (y + 1 < h) ? (y_row0 + w) : NULL;
        uint8_t *uv_row = uv_plane + (y >> 1) * w;
        int x = 0;

        if(y + 1 < h) {
            for(; x + 15 < w; x += 16) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                const uint32_t *src_row1 = in + (h - 2 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[16];
                uint32_t row1_pixels[16];
                uint8x16_t yv0;
                uint8x16_t yv1;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));
                vst1q_u32(row0_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row0 - 11)));
                vst1q_u32(row0_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row0 - 15)));
                vst1q_u32(row1_pixels, neon_reverse_u32x4(vld1q_u32(src_row1 - 3)));
                vst1q_u32(row1_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row1 - 7)));
                vst1q_u32(row1_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row1 - 11)));
                vst1q_u32(row1_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row1 - 15)));

                uint8x16x4_t bgra0 = vld4q_u8((const uint8_t*)row0_pixels);
                uint8x16x4_t bgra1 = vld4q_u8((const uint8_t*)row1_pixels);

                yv0 = neon_rgb_to_y16(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                yv1 = neon_rgb_to_y16(bgra1.val[0], bgra1.val[1], bgra1.val[2]);

                vst1q_u8(y_row0 + x, yv0);
                vst1q_u8(y_row1 + x, yv1);

                rgb_to_uv_scalar_bsp(row0_pixels[0], uv_row + x);
                rgb_to_uv_scalar_bsp(row0_pixels[2], uv_row + x + 2);
                rgb_to_uv_scalar_bsp(row0_pixels[4], uv_row + x + 4);
                rgb_to_uv_scalar_bsp(row0_pixels[6], uv_row + x + 6);
                rgb_to_uv_scalar_bsp(row0_pixels[8], uv_row + x + 8);
                rgb_to_uv_scalar_bsp(row0_pixels[10], uv_row + x + 10);
                rgb_to_uv_scalar_bsp(row0_pixels[12], uv_row + x + 12);
                rgb_to_uv_scalar_bsp(row0_pixels[14], uv_row + x + 14);
            }

            for(; x + 1 < w; x += 2) {
                uint32_t p00 = rgb2nv12_get_src_pixel(in, w, h, y, x);
                uint32_t p01 = rgb2nv12_get_src_pixel(in, w, h, y, x + 1);
                uint32_t p10 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x);
                uint32_t p11 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x + 1);

                y_row0[x] = rgb_to_y_scalar_bsp(p00);
                y_row0[x + 1] = rgb_to_y_scalar_bsp(p01);
                y_row1[x] = rgb_to_y_scalar_bsp(p10);
                y_row1[x + 1] = rgb_to_y_scalar_bsp(p11);
                rgb_to_uv_scalar_bsp(p00, uv_row + x);
            }

            if(x < w) {
                uint32_t p0 = rgb2nv12_get_src_pixel(in, w, h, y, x);
                uint32_t p1 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x);
                y_row0[x] = rgb_to_y_scalar_bsp(p0);
                y_row1[x] = rgb_to_y_scalar_bsp(p1);
            }
        }
        else {
            for(; x + 15 < w; x += 16) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[16];
                uint8x16_t yv0;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));
                vst1q_u32(row0_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row0 - 11)));
                vst1q_u32(row0_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row0 - 15)));

                uint8x16x4_t bgra0 = vld4q_u8((const uint8_t*)row0_pixels);
                yv0 = neon_rgb_to_y16(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                vst1q_u8(y_row0 + x, yv0);
            }

            for(; x < w; ++x) {
                y_row0[x] = rgb_to_y_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
            }
        }
    }
}

static inline uint16_t rgb_to_555_scalar_bsp(uint32_t pixel) {
    uint32_t b = pixel & 0xff;
    uint32_t g = (pixel >> 8) & 0xff;
    uint32_t r = (pixel >> 16) & 0xff;
    return (uint16_t)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

static inline uint16x8_t neon_rgb_to_555_u8x8(uint8x8_t b8, uint8x8_t g8, uint8x8_t r8) {
    /* 0RRRRRGGGGGBBBBB: widen each channel into the high byte, then
       shift-right-insert keeps the top bits of the previous channels */
    uint16x8_t d = vshrq_n_u16(vshll_n_u8(r8, 8), 1);
    d = vsriq_n_u16(d, vshll_n_u8(g8, 8), 6);
    d = vsriq_n_u16(d, vshll_n_u8(b8, 8), 11);
    return d;
}

void argb_2_rgb15_arch(uint16_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    for(int y = 0; y < h; y++) {
        uint16_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            const uint32_t *src_row = in + (h - 1 - y) * w + (w - 1 - x);
            uint32_t row_pixels[16];

            vst1q_u32(row_pixels, neon_reverse_u32x4(vld1q_u32(src_row - 3)));
            vst1q_u32(row_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row - 7)));
            vst1q_u32(row_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row - 11)));
            vst1q_u32(row_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row - 15)));

            uint8x16x4_t bgra = vld4q_u8((const uint8_t*)row_pixels);
            vst1q_u16(dst_row + x,
                neon_rgb_to_555_u8x8(vget_low_u8(bgra.val[0]),
                    vget_low_u8(bgra.val[1]), vget_low_u8(bgra.val[2])));
            vst1q_u16(dst_row + x + 8,
                neon_rgb_to_555_u8x8(vget_high_u8(bgra.val[0]),
                    vget_high_u8(bgra.val[1]), vget_high_u8(bgra.val[2])));
        }

        for(; x < w; ++x) {
            dst_row[x] = rgb_to_555_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
        }
    }
}

static inline uint32x4_t rotate_rev4_u32(uint32x4_t v) {
    /* reverse the four 32-bit lanes: {a,b,c,d} -> {d,c,b,a} */
    return vcombine_u32(vget_high_u32(vrev64q_u32(v)), vget_low_u32(vrev64q_u32(v)));
}

static inline void rotate_90_cw_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
    /* dst is (height x width); dst[y][x] = src[height-1-x][y].
       The outer loop walks DESTINATION row bands so every dst row is
       filled left-to-right (ascending): scan-out mappings are usually
       Normal Non-Cacheable, and write-combine only merges stores into
       DRAM bursts when they arrive sequentially. The source lives in
       cacheable memory, so its strided reads are absorbed by L1/L2. */
    int y = 0;
    for(; y + 4 <= width; y += 4) {
        uint32_t* d0 = dst + (y + 0) * height;
        uint32_t* d1 = dst + (y + 1) * height;
        uint32_t* d2 = dst + (y + 2) * height;
        uint32_t* d3 = dst + (y + 3) * height;
        int x = 0;

        for(; x + 4 <= height; x += 4) {
            const uint32_t* s0 = src + (height - 1 - x) * width + y;
            const uint32_t* s1 = s0 - width;
            const uint32_t* s2 = s1 - width;
            const uint32_t* s3 = s2 - width;
            uint32x4_t v0 = vld1q_u32(s0);
            uint32x4_t v1 = vld1q_u32(s1);
            uint32x4_t v2 = vld1q_u32(s2);
            uint32x4_t v3 = vld1q_u32(s3);

            /* transpose the 4x4 pixel block into columns */
            uint32x4x2_t t01 = vtrnq_u32(v0, v1);
            uint32x4x2_t t23 = vtrnq_u32(v2, v3);
            uint32x4_t c0 = vcombine_u32(vget_low_u32(t01.val[0]), vget_low_u32(t23.val[0]));
            uint32x4_t c1 = vcombine_u32(vget_low_u32(t01.val[1]), vget_low_u32(t23.val[1]));
            uint32x4_t c2 = vcombine_u32(vget_high_u32(t01.val[0]), vget_high_u32(t23.val[0]));
            uint32x4_t c3 = vcombine_u32(vget_high_u32(t01.val[1]), vget_high_u32(t23.val[1]));

            vst1q_u32(d0 + x, c0);
            vst1q_u32(d1 + x, c1);
            vst1q_u32(d2 + x, c2);
            vst1q_u32(d3 + x, c3);
        }

        for(; x < height; ++x) {
            d0[x] = src[(height - 1 - x) * width + y + 0];
            d1[x] = src[(height - 1 - x) * width + y + 1];
            d2[x] = src[(height - 1 - x) * width + y + 2];
            d3[x] = src[(height - 1 - x) * width + y + 3];
        }
    }

    for(; y < width; ++y) {
        uint32_t* d = dst + y * height;
        for(int x = 0; x < height; ++x)
            d[x] = src[(height - 1 - x) * width + y];
    }
}

static inline void rotate_270_cw_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
    /* dst is (height x width); dst[y][x] = src[x][width-1-y], dst rows ascending */
    int y = 0;
    for(; y + 4 <= width; y += 4) {
        uint32_t* d0 = dst + (y + 0) * height;
        uint32_t* d1 = dst + (y + 1) * height;
        uint32_t* d2 = dst + (y + 2) * height;
        uint32_t* d3 = dst + (y + 3) * height;
        int x = 0;

        for(; x + 4 <= height; x += 4) {
            const uint32_t* s0 = src + (x + 0) * width + (width - 4 - y);
            const uint32_t* s1 = s0 + width;
            const uint32_t* s2 = s1 + width;
            const uint32_t* s3 = s2 + width;
            uint32x4_t v0 = vld1q_u32(s0);
            uint32x4_t v1 = vld1q_u32(s1);
            uint32x4_t v2 = vld1q_u32(s2);
            uint32x4_t v3 = vld1q_u32(s3);

            /* transpose the 4x4 pixel block into columns */
            uint32x4x2_t t01 = vtrnq_u32(v0, v1);
            uint32x4x2_t t23 = vtrnq_u32(v2, v3);
            uint32x4_t c0 = vcombine_u32(vget_low_u32(t01.val[0]), vget_low_u32(t23.val[0]));
            uint32x4_t c1 = vcombine_u32(vget_low_u32(t01.val[1]), vget_low_u32(t23.val[1]));
            uint32x4_t c2 = vcombine_u32(vget_high_u32(t01.val[0]), vget_high_u32(t23.val[0]));
            uint32x4_t c3 = vcombine_u32(vget_high_u32(t01.val[1]), vget_high_u32(t23.val[1]));

            /* dst[y+i][x+j] = src[x+3-j][width-1-(y+i)]: row i takes column 3-i */
            vst1q_u32(d0 + x, c3);
            vst1q_u32(d1 + x, c2);
            vst1q_u32(d2 + x, c1);
            vst1q_u32(d3 + x, c0);
        }

        for(; x < height; ++x) {
            d0[x] = src[(x + 0) * width + width - 1 - (y + 0)];
            d1[x] = src[(x + 0) * width + width - 1 - (y + 1)];
            d2[x] = src[(x + 0) * width + width - 1 - (y + 2)];
            d3[x] = src[(x + 0) * width + width - 1 - (y + 3)];
        }
    }

    for(; y < width; ++y) {
        uint32_t* d = dst + y * height;
        for(int x = 0; x < height; ++x)
            d[x] = src[x * width + width - 1 - y];
    }
}

static inline void rotate_180_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
    /* dst[y][x] = src[height-1-y][width-1-x], dst rows ascending */
    int y = 0;
    for(; y + 4 <= height; y += 4) {
        uint32_t* d0 = dst + (y + 0) * width;
        uint32_t* d1 = dst + (y + 1) * width;
        uint32_t* d2 = dst + (y + 2) * width;
        uint32_t* d3 = dst + (y + 3) * width;
        const uint32_t* s0 = src + (height - 1 - (y + 0)) * width;
        const uint32_t* s1 = src + (height - 1 - (y + 1)) * width;
        const uint32_t* s2 = src + (height - 1 - (y + 2)) * width;
        const uint32_t* s3 = src + (height - 1 - (y + 3)) * width;
        int x = 0;

        for(; x + 4 <= width; x += 4) {
            vst1q_u32(d0 + x, rotate_rev4_u32(vld1q_u32(s0 + width - x - 4)));
            vst1q_u32(d1 + x, rotate_rev4_u32(vld1q_u32(s1 + width - x - 4)));
            vst1q_u32(d2 + x, rotate_rev4_u32(vld1q_u32(s2 + width - x - 4)));
            vst1q_u32(d3 + x, rotate_rev4_u32(vld1q_u32(s3 + width - x - 4)));
        }

        for(; x < width; ++x) {
            d0[x] = s0[width - 1 - x];
            d1[x] = s1[width - 1 - x];
            d2[x] = s2[width - 1 - x];
            d3[x] = s3[width - 1 - x];
        }
    }

    for(; y < height; ++y) {
        uint32_t* d = dst + y * width;
        const uint32_t* s = src + (height - 1 - y) * width;
        for(int x = 0; x < width; ++x)
            d[x] = s[width - 1 - x];
    }
}

/*
 *  XRGB1555 -> ARGB8888 (NEON, 16 pixels per iteration).
 *  Straight linear scan (no rotation), the inverse of argb_2_rgb15_arch.
 */
void rgb15_2_argb_arch(uint32_t *out, uint16_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint16x8_t mask_r = vdupq_n_u16(0x7c00);
    const uint16x8_t mask_g = vdupq_n_u16(0x03e0);
    const uint16x8_t mask_b = vdupq_n_u16(0x001f);

    for(int y = 0; y < h; y++) {
        const uint16_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            uint16x8_t px0 = vld1q_u16(src_row + x);
            uint16x8_t px1 = vld1q_u16(src_row + x + 8);

            /* --- first 8 pixels --- */
            uint16x8_t r5_0 = vshrq_n_u16(vandq_u16(px0, mask_r), 10);
            uint16x8_t g5_0 = vshrq_n_u16(vandq_u16(px0, mask_g), 5);
            uint16x8_t b5_0 = vandq_u16(px0, mask_b);
            uint16x8_t r8_0 = vorrq_u16(vshlq_n_u16(r5_0, 3), vshrq_n_u16(r5_0, 2));
            uint16x8_t g8_0 = vorrq_u16(vshlq_n_u16(g5_0, 3), vshrq_n_u16(g5_0, 2));
            uint16x8_t b8_0 = vorrq_u16(vshlq_n_u16(b5_0, 3), vshrq_n_u16(b5_0, 2));
            uint16x8_t ar0 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_0);
            uint16x8_t gb0 = vorrq_u16(vshlq_n_u16(g8_0, 8), b8_0);
            uint32x4_t lo0 = vorrq_u32(vshll_n_u16(vget_low_u16(ar0), 16),
                                       vmovl_u16(vget_low_u16(gb0)));
            uint32x4_t hi0 = vorrq_u32(vshll_n_u16(vget_high_u16(ar0), 16),
                                       vmovl_u16(vget_high_u16(gb0)));

            /* --- second 8 pixels --- */
            uint16x8_t r5_1 = vshrq_n_u16(vandq_u16(px1, mask_r), 10);
            uint16x8_t g5_1 = vshrq_n_u16(vandq_u16(px1, mask_g), 5);
            uint16x8_t b5_1 = vandq_u16(px1, mask_b);
            uint16x8_t r8_1 = vorrq_u16(vshlq_n_u16(r5_1, 3), vshrq_n_u16(r5_1, 2));
            uint16x8_t g8_1 = vorrq_u16(vshlq_n_u16(g5_1, 3), vshrq_n_u16(g5_1, 2));
            uint16x8_t b8_1 = vorrq_u16(vshlq_n_u16(b5_1, 3), vshrq_n_u16(b5_1, 2));
            uint16x8_t ar1 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_1);
            uint16x8_t gb1 = vorrq_u16(vshlq_n_u16(g8_1, 8), b8_1);
            uint32x4_t lo1 = vorrq_u32(vshll_n_u16(vget_low_u16(ar1), 16),
                                       vmovl_u16(vget_low_u16(gb1)));
            uint32x4_t hi1 = vorrq_u32(vshll_n_u16(vget_high_u16(ar1), 16),
                                       vmovl_u16(vget_high_u16(gb1)));

            vst1q_u32(dst_row + x,      lo0);
            vst1q_u32(dst_row + x +  4, hi0);
            vst1q_u32(dst_row + x +  8, lo1);
            vst1q_u32(dst_row + x + 12, hi1);
        }

        for(; x < w; ++x) {
            uint16_t v = src_row[x];
            uint32_t r = (v >> 10) & 0x1f;
            uint32_t g = (v >>  5) & 0x1f;
            uint32_t b =  v        & 0x1f;
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            dst_row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

/*
 *  ARGB8888 -> RGB24 (NEON, 8 pixels per iteration): strip alpha.
 */
void argb_2_rgb24_arch(uint32_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t mask = vdupq_n_u32(0x00ffffffu);

    for(int y = 0; y < h; y++) {
        const uint32_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint32x4_t lo = vld1q_u32(src_row + x);
            uint32x4_t hi = vld1q_u32(src_row + x + 4);
            vst1q_u32(dst_row + x,     vandq_u32(lo, mask));
            vst1q_u32(dst_row + x + 4, vandq_u32(hi, mask));
        }

        for(; x < w; ++x)
            dst_row[x] = src_row[x] & 0x00ffffffu;
    }
}

/*
 *  RGB24 -> ARGB8888 (NEON, 8 pixels per iteration): set alpha to 0xFF.
 */
void rgb24_2_argb_arch(uint32_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t alpha = vdupq_n_u32(0xff000000u);
    const uint32x4_t mask  = vdupq_n_u32(0x00ffffffu);

    for(int y = 0; y < h; y++) {
        const uint32_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint32x4_t lo = vld1q_u32(src_row + x);
            uint32x4_t hi = vld1q_u32(src_row + x + 4);
            vst1q_u32(dst_row + x,     vorrq_u32(vandq_u32(lo, mask), alpha));
            vst1q_u32(dst_row + x + 4, vorrq_u32(vandq_u32(hi, mask), alpha));
        }

        for(; x < w; ++x)
            dst_row[x] = 0xff000000u | (src_row[x] & 0x00ffffffu);
    }
}

/*
 * Big-endian [00][RR][GG][BB] byte stream -> ARGB8888.
 * vrev32_u8 reverses each 32-bit word from [00][RR][GG][BB] to [RR][GG][BB][00],
 * then we OR in the alpha byte.
 */
void rgb24be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t alpha = vdupq_n_u32(0xff000000u);

    for(int y = 0; y < h; y++) {
        const uint8_t *src_row = in + y * bpr;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint8x16_t raw0 = vld1q_u8(src_row +  x      * 4);
            uint8x16_t raw1 = vld1q_u8(src_row + (x + 4) * 4);
            uint32x4_t r0 = vorrq_u32(vreinterpretq_u32_u8(vrev32q_u8(raw0)), alpha);
            uint32x4_t r1 = vorrq_u32(vreinterpretq_u32_u8(vrev32q_u8(raw1)), alpha);
            vst1q_u32(dst_row + x,     r0);
            vst1q_u32(dst_row + x + 4, r1);
        }

        for(; x < w; ++x) {
            const uint8_t *p = src_row + x * 4;
            dst_row[x] = 0xff000000u |
                         ((uint32_t)p[1] << 16) |
                         ((uint32_t)p[2] <<  8) |
                          (uint32_t)p[3];
        }
    }
}

/*
 * Big-endian XRGB1555 byte stream -> ARGB8888.
 * vrev32_u8 swaps each 32-bit word so the two 16-bit halves
 * become host-order XRGB1555, then expands 5-bit channels to 8.
 */
void rgb15be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint16x8_t mask_r = vdupq_n_u16(0x7c00);
    const uint16x8_t mask_g = vdupq_n_u16(0x03e0);
    const uint16x8_t mask_b = vdupq_n_u16(0x001f);

    for(int y = 0; y < h; y++) {
        const uint8_t *src_row = in + y * bpr;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            /* Load 16 pixels as raw bytes, swap hi/lo within each 16-bit pixel */
            uint8x16_t raw0 = vld1q_u8(src_row +  x      * 2);
            uint8x16_t raw1 = vld1q_u8(src_row + (x + 8) * 2);
            uint16x8_t px0 = vreinterpretq_u16_u8(vrev16q_u8(raw0));
            uint16x8_t px1 = vreinterpretq_u16_u8(vrev16q_u8(raw1));

            /* --- first 8 pixels --- */
            uint16x8_t r5_0 = vshrq_n_u16(vandq_u16(px0, mask_r), 10);
            uint16x8_t g5_0 = vshrq_n_u16(vandq_u16(px0, mask_g), 5);
            uint16x8_t b5_0 = vandq_u16(px0, mask_b);
            uint16x8_t r8_0 = vorrq_u16(vshlq_n_u16(r5_0, 3), vshrq_n_u16(r5_0, 2));
            uint16x8_t g8_0 = vorrq_u16(vshlq_n_u16(g5_0, 3), vshrq_n_u16(g5_0, 2));
            uint16x8_t b8_0 = vorrq_u16(vshlq_n_u16(b5_0, 3), vshrq_n_u16(b5_0, 2));
            uint16x8_t ar0 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_0);
            uint16x8_t gb0 = vorrq_u16(vshlq_n_u16(g8_0, 8), b8_0);
            uint32x4_t lo0 = vorrq_u32(vshll_n_u16(vget_low_u16(ar0), 16),
                                       vmovl_u16(vget_low_u16(gb0)));
            uint32x4_t hi0 = vorrq_u32(vshll_n_u16(vget_high_u16(ar0), 16),
                                       vmovl_u16(vget_high_u16(gb0)));

            /* --- second 8 pixels --- */
            uint16x8_t r5_1 = vshrq_n_u16(vandq_u16(px1, mask_r), 10);
            uint16x8_t g5_1 = vshrq_n_u16(vandq_u16(px1, mask_g), 5);
            uint16x8_t b5_1 = vandq_u16(px1, mask_b);
            uint16x8_t r8_1 = vorrq_u16(vshlq_n_u16(r5_1, 3), vshrq_n_u16(r5_1, 2));
            uint16x8_t g8_1 = vorrq_u16(vshlq_n_u16(g5_1, 3), vshrq_n_u16(g5_1, 2));
            uint16x8_t b8_1 = vorrq_u16(vshlq_n_u16(b5_1, 3), vshrq_n_u16(b5_1, 2));
            uint16x8_t ar1 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_1);
            uint16x8_t gb1 = vorrq_u16(vshlq_n_u16(g8_1, 8), b8_1);
            uint32x4_t lo1 = vorrq_u32(vshll_n_u16(vget_low_u16(ar1), 16),
                                       vmovl_u16(vget_low_u16(gb1)));
            uint32x4_t hi1 = vorrq_u32(vshll_n_u16(vget_high_u16(ar1), 16),
                                       vmovl_u16(vget_high_u16(gb1)));

            vst1q_u32(dst_row + x,      lo0);
            vst1q_u32(dst_row + x +  4, hi0);
            vst1q_u32(dst_row + x +  8, lo1);
            vst1q_u32(dst_row + x + 12, hi1);
        }

        for(; x < w; ++x) {
            const uint8_t *p = src_row + x * 2;
            uint16_t v = ((uint16_t)p[0] << 8) | p[1];
            uint32_t r = (v >> 10) & 0x1f;
            uint32_t g = (v >>  5) & 0x1f;
            uint32_t b =  v        & 0x1f;
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            dst_row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

void graph_rotate_to_arch(graph_t* g, graph_t* ret, int rot) {
    if(g == NULL || ret == NULL ||
            g->buffer == NULL || ret->buffer == NULL ||
            g->w <= 0 || g->h <= 0)
        return;

    if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
        if(ret->w < g->h || ret->h < g->w)
            return;
        if(rot == G_ROTATE_90)
            rotate_90_cw_neon(g->buffer, ret->buffer, g->w, g->h);
        else
            rotate_270_cw_neon(g->buffer, ret->buffer, g->w, g->h);
    }
    else if(rot == G_ROTATE_180) {
        if(ret->w < g->w || ret->h < g->h)
            return;
        rotate_180_neon(g->buffer, ret->buffer, g->w, g->h);
    }
}

#endif

#ifdef __cplusplus
}
#endif
