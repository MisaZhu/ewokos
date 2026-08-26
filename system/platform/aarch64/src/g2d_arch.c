/* aarch64 NEON back end for the g2d arch hooks. The semantics mirror the
   portable C reference in the bsp library (bsp_g2d.c) bit-for-bit where it
   matters: 14-bit fixed point trig, ceiling bounding-box sizes, the
   div255-free (a*x)>>8 effective alpha and the per-pixel /255 blend. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARCH_BOOST
#include <arm_neon.h>

#define MIN(a, b) (((a) > (b))?(b):(a))
#define MAX(a, b) (((a) > (b))?(a):(b))

#if defined(__GNUC__) && !defined(__clang__)
/* GCC with -mstrict-align lowers vld1q_u32/vst1q_u32 on pointers without
   proven 16-byte alignment to scalar ldp w/orr/stp w sequences. LD1/ST1
   tolerate unaligned addresses in Normal memory (the kernel boots with
   SCTLR_EL1.A=0), so emit them directly. NOTE: do NOT use LDP/STP q-form
   here — unlike LD1/ST1 they require 16-byte alignment even with A=0.
   Callers must only reach these helpers with 16-byte aligned pointers on
   buffers that can sit in Device-mapped memory (see g2d_row_copy_neon). */
static inline uint32x4_t g2d_ld1q_u32(const uint32_t *p) {
    uint32x4_t v;
    /* the "memory" clobber is required even for a pure load: without it the
       compiler assumes the asm touches no memory and may reorder it above
       preceding stores to *p issued by plain C code in the same function. */
    __asm__("ld1 {%0.4s}, [%1]" : "=w"(v) : "r"(p) : "memory");
    return v;
}
static inline void g2d_st1q_u32(uint32_t *p, uint32x4_t v) {
    __asm__("st1 {%1.4s}, [%0]" :: "r"(p), "w"(v) : "memory");
}
static inline void g2d_ld1q_x4_u32(const uint32_t *p, uint32x4_t *a, uint32x4_t *b, uint32x4_t *c, uint32x4_t *d) {
    /* 4 independent single-register LD1s: GCC "=w" constraints do NOT
       guarantee consecutive register allocation, so the ld1 {vA.4s-vD.4s}
       range syntax breaks as soon as register pressure rises. */
    *a = g2d_ld1q_u32(p);
    *b = g2d_ld1q_u32(p + 4);
    *c = g2d_ld1q_u32(p + 8);
    *d = g2d_ld1q_u32(p + 12);
}
static inline void g2d_st1q_x4_u32(uint32_t *p, uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d) {
    g2d_st1q_u32(p, a);
    g2d_st1q_u32(p + 4, b);
    g2d_st1q_u32(p + 8, c);
    g2d_st1q_u32(p + 12, d);
}
#else
static inline uint32x4_t g2d_ld1q_u32(const uint32_t *p) {
    return vld1q_u32(p);
}
static inline void g2d_st1q_u32(uint32_t *p, uint32x4_t v) {
    vst1q_u32(p, v);
}
static inline void g2d_ld1q_x4_u32(const uint32_t *p, uint32x4_t *a, uint32x4_t *b, uint32x4_t *c, uint32x4_t *d) {
    *a = vld1q_u32(p);
    *b = vld1q_u32(p + 4);
    *c = vld1q_u32(p + 8);
    *d = vld1q_u32(p + 12);
}
static inline void g2d_st1q_x4_u32(uint32_t *p, uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d) {
    vst1q_u32(p, a);
    vst1q_u32(p + 4, b);
    vst1q_u32(p + 8, c);
    vst1q_u32(p + 12, d);
}
#endif

/* ---------------------------------------------------------------- fill --- */

/* software back end has nothing to set up, the hook exists so platforms
   with a 2D hardware engine can prepare it before first use. */
int32_t arch_g2d_init(void) {
	return 0;
}

void arch_g2d_fill(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(argb == NULL || argb_w <= 0 || argb_h <= 0 || w <= 0 || h <= 0)
		return;

	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;
	if(w <= 0 || h <= 0)
		return;

	uint8_t cb = (uint8_t)color;
	int same_bytes = (((color >> 8) & 0xff) == cb) &&
			(((color >> 16) & 0xff) == cb) &&
			(((color >> 24) & 0xff) == cb);

	/* whole-width rectangles are one contiguous block in memory */
	int full_rows = (x == 0 && w == argb_w);

	if(same_bytes) {
		/* all four channel bytes equal: a byte memset fills whole pixels */
		if(full_rows) {
			memset(argb + y * argb_w, cb, (size_t)w * h * 4);
		} else {
			for(int32_t row = 0; row < h; row++)
				memset(argb + (y + row) * argb_w + x, cb, (size_t)w * 4);
		}
		return;
	}

	/* broadcast once, then the row loop only stores. Do NOT round-trip the
	   color through a stack buffer: the ld1 helpers are opaque asm to the
	   compiler, and without a visible memory dependency GCC hoisted the load
	   above the buffer init, filling the first 4 pixels of every 16px block
	   with stale stack data (seen as vertical stripes on screen). */
	uint32x4_t vc = vdupq_n_u32(color);

	if(full_rows) {
		/* contiguous w*h pixel block: one aligned-store run instead of
		   per-row setup */
		uint32_t* dp = argb + y * argb_w;
		int32_t total = w * h;
		int32_t cx = 0;
		uintptr_t dpa = (uintptr_t)dp;
		if(dpa & 0xF) {
			int32_t head = (int32_t)((16 - (dpa & 0xF)) >> 2);
			if(head > total)
				head = total;
			for(; cx < head; cx++)
				dp[cx] = color;
		}
		for(; cx <= total - 16; cx += 16)
			g2d_st1q_x4_u32(dp + cx, vc, vc, vc, vc);
		for(; cx < total; cx++)
			dp[cx] = color;
		return;
	}

	for(int32_t row = 0; row < h; row++) {
		uint32_t* dp = argb + (y + row) * argb_w + x;
		int32_t cx = 0;

		/* 16px SIMD stores need a 16-byte aligned start: the surface may
		   be Device-mapped framebuffer memory where any unaligned access
		   faults (odd x or odd surface width). A scalar head of at most
		   3 pixels realigns the row instead of falling back to a full
		   scalar loop. */
		uintptr_t dpa = (uintptr_t)dp;
		if(dpa & 0xF) {
			int32_t head = (int32_t)((16 - (dpa & 0xF)) >> 2);
			if(head > w)
				head = w;
			for(; cx < head; cx++)
				dp[cx] = color;
		}

		for(; cx <= w - 16; cx += 16)
			g2d_st1q_x4_u32(dp + cx, vc, vc, vc, vc);
		for(; cx < w; cx++)
			dp[cx] = color;
	}
}

/* ----------------------------------------------------------------- blt --- */

#define G2D_PREFETCH(srcp) \
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(srcp))

/* aligned-load shift: assemble 16 bytes at (base + off) from two aligned
   16-byte loads, so misaligned sources never need unaligned NEON loads
   (Device-mapped surfaces fault on ANY unaligned access) */
static inline uint32x4_t g2d_ld1_shift(uint32x4_t prev, uint32x4_t cur, int off) {
    if(off == 4)
        return vextq_u32(prev, cur, 1);
    if(off == 8)
        return vextq_u32(prev, cur, 2);
    return vextq_u32(prev, cur, 3);
}

/* Row copy that never falls back to libc memcpy: the EwokOS libc one is a
   plain byte loop, while 128-bit NEON load/stores move 16 pixels in a few
   instructions and merge cleanly into write-combine bursts on non-cacheable
   memory.

   Alignment strategy (Device-mapped destinations fault on any unaligned
   access): a scalar head of at most 3 pixels brings dp to a 16-byte
   boundary so every SIMD store stays aligned. Aligned sources use plain
   loads; misaligned sources are assembled from aligned load pairs via
   VEXT (the look-behind stays inside the buffer because graph bases are
   at least 16-byte aligned), so no path ever issues an unaligned access. */
static inline void g2d_row_copy_neon(uint32_t *dp, const uint32_t *sp, int32_t w) {
    int32_t x = 0;
    uintptr_t dpa = (uintptr_t)dp;
    if(dpa & 0xF) {
        int32_t head = (int32_t)((16 - (dpa & 0xF)) >> 2);
        if(head > w)
            head = w;
        for(; x < head; x++)
            dp[x] = sp[x];
        if(x >= w)
            return;
    }

    uintptr_t spa = (uintptr_t)(sp + x);
    if((spa & 0xF) == 0) {
        /* aligned fast path: 32 pixels (128 bytes) per iteration with a
           rolling prefetch 4 cache lines ahead, loads issued ahead of the
           stores so the load pipe overlaps with the store drain */
        for(; x <= w - 32; x += 32) {
            G2D_PREFETCH(sp + x);
            uint32x4_t v0, v1, v2, v3, v4, v5, v6, v7;
            g2d_ld1q_x4_u32(sp + x, &v0, &v1, &v2, &v3);
            g2d_ld1q_x4_u32(sp + x + 16, &v4, &v5, &v6, &v7);
            g2d_st1q_x4_u32(dp + x, v0, v1, v2, v3);
            g2d_st1q_x4_u32(dp + x + 16, v4, v5, v6, v7);
        }
        /* 16 pixels */
        if(x <= w - 16) {
            uint32x4_t v0, v1, v2, v3;
            g2d_ld1q_x4_u32(sp + x, &v0, &v1, &v2, &v3);
            g2d_st1q_x4_u32(dp + x, v0, v1, v2, v3);
            x += 16;
        }
        /* 8 pixels */
        if(x <= w - 8) {
            g2d_st1q_u32(dp + x, g2d_ld1q_u32(sp + x));
            g2d_st1q_u32(dp + x + 4, g2d_ld1q_u32(sp + x + 4));
            x += 8;
        }
        /* 4 pixels */
        if(x <= w - 4) {
            g2d_st1q_u32(dp + x, g2d_ld1q_u32(sp + x));
            x += 4;
        }
        for(; x < w; x++)
            dp[x] = sp[x];
        return;
    }

    /* misaligned source: vext-shifted aligned loads, 16 pixels/iteration */
    int off = (int)(spa & 0xF);
    const uint32_t *base = (const uint32_t *)(spa & ~(uintptr_t)0xF);
    uint32x4_t prev = g2d_ld1q_u32(base);
    for(; x <= w - 16; x += 16) {
        G2D_PREFETCH(sp + x);
        uint32x4_t cur = g2d_ld1q_u32(base + 4);
        g2d_st1q_u32(dp + x, g2d_ld1_shift(prev, cur, off));
        prev = cur;
        base += 4;
        cur = g2d_ld1q_u32(base + 4);
        g2d_st1q_u32(dp + x + 4, g2d_ld1_shift(prev, cur, off));
        prev = cur;
        base += 4;
        cur = g2d_ld1q_u32(base + 4);
        g2d_st1q_u32(dp + x + 8, g2d_ld1_shift(prev, cur, off));
        prev = cur;
        base += 4;
        cur = g2d_ld1q_u32(base + 4);
        g2d_st1q_u32(dp + x + 12, g2d_ld1_shift(prev, cur, off));
        prev = cur;
        base += 4;
    }
    for(; x < w; x++)
        dp[x] = sp[x];
}

/* backward variant for overlapping self-copies (scroll down): mirrored
   alignment handling — a scalar tail right-aligns dp to a 16-byte end
   boundary, then aligned stores walk downwards. Misaligned sources use
   look-ahead aligned load pairs + VEXT. */
static inline void g2d_row_copy_back_neon(uint32_t *dp, const uint32_t *sp, int32_t w) {
    int32_t x = w;
    uintptr_t dpe = (uintptr_t)(dp + x);
    if(dpe & 0xF) {
        int32_t tail = (int32_t)((dpe & 0xF) >> 2);
        if(tail > w)
            tail = w;
        for(int32_t i = 0; i < tail; i++) {
            x--;
            dp[x] = sp[x];
        }
        if(x <= 0)
            return;
    }

    uintptr_t spe = (uintptr_t)(sp + x);
    if((spe & 0xF) == 0) {
        for(; x >= 32; x -= 32) {
            G2D_PREFETCH(sp + x - 32);
            uint32x4_t v0, v1, v2, v3, v4, v5, v6, v7;
            g2d_ld1q_x4_u32(sp + x - 16, &v4, &v5, &v6, &v7);
            g2d_ld1q_x4_u32(sp + x - 32, &v0, &v1, &v2, &v3);
            g2d_st1q_x4_u32(dp + x - 32, v0, v1, v2, v3);
            g2d_st1q_x4_u32(dp + x - 16, v4, v5, v6, v7);
        }
        if(x >= 16) {
            uint32x4_t v0, v1, v2, v3;
            g2d_ld1q_x4_u32(sp + x - 16, &v0, &v1, &v2, &v3);
            g2d_st1q_x4_u32(dp + x - 16, v0, v1, v2, v3);
            x -= 16;
        }
        if(x >= 8) {
            g2d_st1q_u32(dp + x - 8, g2d_ld1q_u32(sp + x - 8));
            g2d_st1q_u32(dp + x - 4, g2d_ld1q_u32(sp + x - 4));
            x -= 8;
        }
        if(x >= 4) {
            g2d_st1q_u32(dp + x - 4, g2d_ld1q_u32(sp + x - 4));
            x -= 4;
        }
        while(x > 0) {
            x--;
            dp[x] = sp[x];
        }
        return;
    }

    /* misaligned source: assemble each 16-byte chunk from the aligned pair
       straddling its END (look-ahead, safe to the end of the buffer) */
    int off = (int)(spe & 0xF);
    const uint32_t *base = (const uint32_t *)(spe & ~(uintptr_t)0xF);
    uint32x4_t next = g2d_ld1q_u32(base);
    while(x >= 16) {
        uint32x4_t cur;
        x -= 16;
        base -= 4;
        cur = g2d_ld1q_u32(base);
        g2d_st1q_u32(dp + x + 12, g2d_ld1_shift(cur, next, off));
        next = cur;
        base -= 4;
        cur = g2d_ld1q_u32(base);
        g2d_st1q_u32(dp + x + 8, g2d_ld1_shift(cur, next, off));
        next = cur;
        base -= 4;
        cur = g2d_ld1q_u32(base);
        g2d_st1q_u32(dp + x + 4, g2d_ld1_shift(cur, next, off));
        next = cur;
        base -= 4;
        cur = g2d_ld1q_u32(base);
        g2d_st1q_u32(dp + x, g2d_ld1_shift(cur, next, off));
        next = cur;
    }
    while(x > 0) {
        x--;
        dp[x] = sp[x];
    }
}

/* clip src/dst pair against their buffers, keeping the mapping aligned.
   returns false when nothing is left to draw. */
static int g2d_blt_clip(int32_t src_w, int32_t src_h,
		int32_t* sx, int32_t* sy, int32_t* sw, int32_t* sh,
		int32_t dst_w, int32_t dst_h,
		int32_t* dx, int32_t* dy, int32_t* dw, int32_t* dh) {
	/* cut left/top of source, adjust destination proportionally */
	if(*sx < 0) {
		int32_t cut = (int32_t)((int64_t)(-*sx) * *dw / *sw);
		*dx += cut; *dw -= cut;
		*sw += *sx; *sx = 0;
	}
	if(*sy < 0) {
		int32_t cut = (int32_t)((int64_t)(-*sy) * *dh / *sh);
		*dy += cut; *dh -= cut;
		*sh += *sy; *sy = 0;
	}
	/* cut right/bottom of source */
	if(*sx + *sw > src_w) {
		int32_t over = *sx + *sw - src_w;
		int32_t cut = (int32_t)((int64_t)over * *dw / *sw);
		*dw -= cut;
		*sw -= over;
	}
	if(*sy + *sh > src_h) {
		int32_t over = *sy + *sh - src_h;
		int32_t cut = (int32_t)((int64_t)over * *dh / *sh);
		*dh -= cut;
		*sh -= over;
	}
	if(*sw <= 0 || *sh <= 0 || *dw <= 0 || *dh <= 0)
		return 0;

	/* cut left/top of destination, adjust source proportionally */
	if(*dx < 0) {
		int32_t cut = (int32_t)((int64_t)(-*dx) * *sw / *dw);
		*sx += cut; *sw -= cut;
		*dx += cut; *dw -= cut;
	}
	if(*dy < 0) {
		int32_t cut = (int32_t)((int64_t)(-*dy) * *sh / *dh);
		*sy += cut; *sh -= cut;
		*dy += cut; *dh -= cut;
	}
	/* cut right/bottom of destination */
	if(*dx + *dw > dst_w) {
		int32_t over = *dx + *dw - dst_w;
		int32_t cut = (int32_t)((int64_t)over * *sw / *dw);
		*dw -= over;
		*sw -= cut;
	}
	if(*dy + *dh > dst_h) {
		int32_t over = *dy + *dh - dst_h;
		int32_t cut = (int32_t)((int64_t)over * *sh / *dh);
		*dh -= over;
		*sh -= cut;
	}
	return (*sw > 0 && *sh > 0 && *dw > 0 && *dh > 0);
}

void arch_g2d_blt(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	if(!g2d_blt_clip(src_w, src_h, &sx, &sy, &sw, &sh,
			dst_w, dst_h, &dx, &dy, &dw, &dh))
		return;

	/* 1:1 row copy (overlap-safe) */
	if(sw == dw && sh == dh) {
		int backward = (argb_src == argb_dst) &&
			(dy > sy || (dy == sy && dx > sx));
		if(!backward) {
			/* whole region contiguous on both sides: one streaming copy */
			if(dx == 0 && sx == 0 && sw == src_w && sw == dst_w) {
				g2d_row_copy_neon(&argb_dst[dy * dst_w],
						&argb_src[sy * src_w], sh * sw);
				return;
			}
			for(int32_t row = 0; row < sh; row++)
				g2d_row_copy_neon(
						argb_dst + (dy + row) * dst_w + dx,
						argb_src + (sy + row) * src_w + sx, sw);
		}
		else {
			/* overlapping copy going downwards: reverse row order, NEON rows */
			for(int32_t row = 0; row < sh; row++) {
				int32_t r = sh - 1 - row;
				g2d_row_copy_back_neon(
						argb_dst + (dy + r) * dst_w + dx,
						argb_src + (sy + r) * src_w + sx, sw);
			}
		}
		return;
	}

	/* nearest-neighbor scaling */
	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * src_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;

		/* integer up/downscale: dst columns map to evenly spaced src
		   columns, gather 4 (or step) instead of one div per column */
		if(sw == dw * 2) {
			int32_t col = 0;
			for(; col <= dw - 4; col += 4) {
				drow[col] = srow[sx + col * 2];
				drow[col + 1] = srow[sx + col * 2 + 1];
				drow[col + 2] = srow[sx + col * 2 + 2];
				drow[col + 3] = srow[sx + col * 2 + 3];
			}
			for(; col < dw; col++)
				drow[col] = srow[sx + col * 2];
			continue;
		}
		if(dw == sw * 2) {
			int32_t col = 0;
			for(; col <= dw - 8; col += 8) {
				uint32_t p0 = srow[sx + (col >> 1)];
				uint32_t p1 = srow[sx + (col >> 1) + 1];
				uint32_t p2 = srow[sx + (col >> 1) + 2];
				uint32_t p3 = srow[sx + (col >> 1) + 3];
				drow[col] = p0; drow[col + 1] = p0;
				drow[col + 2] = p1; drow[col + 3] = p1;
				drow[col + 4] = p2; drow[col + 5] = p2;
				drow[col + 6] = p3; drow[col + 7] = p3;
			}
			for(; col < dw; col++)
				drow[col] = srow[sx + (col >> 1)];
			continue;
		}

		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;
			drow[col] = srow[src_x];
		}
	}
}

/* ---------------------------------------------------------- blt alpha --- */

static inline uint16x8_t neon_div255_u16(uint16x8_t v)
{
    uint16x8_t t = vaddq_u16(v, vdupq_n_u16(1));
    t = vaddq_u16(t, vshrq_n_u16(v, 8));
    return vshrq_n_u16(t, 8);
}

/* Blend 16 pixels whose effective alpha is already known to be neither
   all-zero nor all-opaque. a_lo/a_hi are the per-pixel effective alphas
   (src_a scaled by the global alpha). Channels use vmull+vmlal instead of
   vmull+vmull+vaddq: one fewer instruction per channel half. Same /255
   blend math as the C reference. */
static inline void g2d_alpha16_blend_core(uint32_t *dp, uint8x16x4_t fg,
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
static inline void g2d_alpha16_blend_a255(uint32_t *dp, uint8x16x4_t fg)
{
    g2d_alpha16_blend_core(dp, fg,
            vget_low_u8(fg.val[3]), vget_high_u8(fg.val[3]));
}

/* 16px blend with a global alpha < 0xff */
static inline void g2d_alpha16_blend_scaled(uint32_t *dp, uint8x16x4_t fg,
        uint8x16_t alpha_vec)
{
    uint8x8_t a_lo = vmovn_u16(neon_div255_u16(vmull_u8(vget_low_u8(fg.val[3]), vget_low_u8(alpha_vec))));
    uint8x8_t a_hi = vmovn_u16(neon_div255_u16(vmull_u8(vget_high_u8(fg.val[3]), vget_high_u8(alpha_vec))));
    g2d_alpha16_blend_core(dp, fg, a_lo, a_hi);
}

/* 16px sub-block already loaded interleaved (cheap ld1). Alphas are the
   top byte of each u32 lane: max-of-OR == 0 means all transparent,
   min-of-AND == 0xff means all opaque. Uniform blocks never touch the
   expensive ld4/st4 de-interleave path: transparent skips all memory
   access, opaque is a plain st1 copy. Only truly mixed blocks reload the
   source de-interleaved for the blend math. */
static inline void g2d_alpha16_a255_regs(uint32_t *dp, const uint32_t *sp,
        uint32x4_t s0, uint32x4_t s1, uint32x4_t s2, uint32x4_t s3)
{
    uint32x4_t or_a = vorrq_u32(vorrq_u32(s0, s1), vorrq_u32(s2, s3));
    if(vmaxvq_u32(vshrq_n_u32(or_a, 24)) == 0)
        return;
    uint32x4_t and_a = vandq_u32(vandq_u32(s0, s1), vandq_u32(s2, s3));
    if(vminvq_u32(vshrq_n_u32(and_a, 24)) == 0xff) {
        g2d_st1q_x4_u32(dp, s0, s1, s2, s3);
        return;
    }
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)sp);
    g2d_alpha16_blend_a255(dp, fg);
}

/* 32px block, global alpha 0xff: combined transparent/opaque checks over
   all 8 interleaved vectors first (one reduction each), then per-16px
   sub-block checks only when the block is mixed. Blending preserves
   block-level semantics bit-exactly: fg_a==0 is the identity on dst, and
   fg_a==0xff gives div255(x*255) == x per channel with
   out_a = bg_a + (255-bg_a) == 255. */
static inline void g2d_alpha32_a255(uint32_t *dp, const uint32_t *sp)
{
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(sp));

    uint32x4_t s0, s1, s2, s3, s4, s5, s6, s7;
    g2d_ld1q_x4_u32(sp, &s0, &s1, &s2, &s3);
    g2d_ld1q_x4_u32(sp + 16, &s4, &s5, &s6, &s7);

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
        g2d_st1q_x4_u32(dp, s0, s1, s2, s3);
        g2d_st1q_x4_u32(dp + 16, s4, s5, s6, s7);
        return;
    }

    /* Mixed block: blend reads dst too, prefetch it as well */
    __asm volatile("prfm pldl1keep, [%0, #256]" : : "r"(dp));
    g2d_alpha16_a255_regs(dp, sp, s0, s1, s2, s3);
    g2d_alpha16_a255_regs(dp + 16, sp + 16, s4, s5, s6, s7);
}

/* 32px block with a global alpha < 0xff */
static inline void g2d_alpha32_scaled(uint32_t *dp, const uint32_t *sp,
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

    g2d_alpha16_blend_scaled(dp, fg0, alpha_vec);
    g2d_alpha16_blend_scaled(dp + 16, fg1, alpha_vec);
}

/* Remaining 16px block with per-block checks, global alpha 0xff */
static inline void g2d_alpha16_a255_checked(uint32_t *dp, const uint32_t *sp)
{
    uint32x4_t s0, s1, s2, s3;
    g2d_ld1q_x4_u32(sp, &s0, &s1, &s2, &s3);
    g2d_alpha16_a255_regs(dp, sp, s0, s1, s2, s3);
}

/* Remaining 16px block with per-block checks, global alpha < 0xff */
static inline void g2d_alpha16_scaled_checked(uint32_t *dp, const uint32_t *sp,
        uint8x16_t alpha_vec)
{
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)sp);
    if(vmaxvq_u8(fg.val[3]) == 0)
        return;
    g2d_alpha16_blend_scaled(dp, fg, alpha_vec);
}

/* scalar tail blending one pixel at a time, same math as the C reference:
   effective alpha (src_a * alpha) >> 8, then the /255 blend. */
static inline uint32_t g2d_blend_argb_scalar(uint32_t dst_color, uint8_t a,
		uint8_t r, uint8_t g, uint8_t b) {
	uint32_t oa = (dst_color >> 24) & 0xff;
	uint32_t dr = (dst_color >> 16) & 0xff;
	uint32_t dg = (dst_color >> 8) & 0xff;
	uint32_t db = dst_color & 0xff;
	uint32_t inv_a = 255 - a;

	oa = oa + (255 - oa) * a / 255;
	dr = (r * a + dr * inv_a) / 255;
	dg = (g * a + dg * inv_a) / 255;
	db = (b * a + db * inv_a) / 255;
	return (oa << 24) | (dr << 16) | (dg << 8) | db;
}

void arch_g2d_blt_alpha(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || alpha == 0)
		return;

	if(!g2d_blt_clip(src_w, src_h, &sx, &sy, &sw, &sh,
			dst_w, dst_h, &dx, &dy, &dw, &dh))
		return;

	/* 1:1 blit: 16/32px SIMD blocks with per-block alpha checks */
	if(sw == dw && sh == dh) {
		uint8x16_t alpha_vec16 = vdupq_n_u8(alpha);

		for(int32_t row = 0; row < sh; row++) {
			const uint32_t *sp = argb_src + (sy + row) * src_w + sx;
			uint32_t *dp = argb_dst + (dy + row) * dst_w + dx;
			int32_t x = 0;

			if(alpha == 0xff) {
				for(; x <= sw - 32; x += 32)
					g2d_alpha32_a255(dp + x, sp + x);
				if(x <= sw - 16) {
					g2d_alpha16_a255_checked(dp + x, sp + x);
					x += 16;
				}
			}
			else {
				for(; x <= sw - 32; x += 32)
					g2d_alpha32_scaled(dp + x, sp + x, alpha_vec16);
				if(x <= sw - 16) {
					g2d_alpha16_scaled_checked(dp + x, sp + x, alpha_vec16);
					x += 16;
				}
			}

			/* Tail: zero-padded block; padding lanes blend as identity */
			if(x < sw) {
				int remain = sw - x;
				uint32_t fg[16] = {0}, bg[16] = {0};
				memcpy(fg, sp + x, 4 * remain);
				memcpy(bg, dp + x, 4 * remain);
				uint8x16x4_t fgv = vld4q_u8((const uint8_t*)fg);
				if(alpha == 0xff)
					g2d_alpha16_blend_a255(bg, fgv);
				else
					g2d_alpha16_blend_scaled(bg, fgv, alpha_vec16);
				memcpy(dp + x, bg, 4 * remain);
			}
		}
		return;
	}

	/* scaled blit: nearest-neighbor gather + scalar blend per pixel */
	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * src_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;
		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;

			uint32_t color = srow[src_x];
			uint32_t src_a = (color >> 24) & 0xff;
			if(src_a == 0)
				continue;
			if(alpha == 0xff && src_a == 0xff) {
				drow[col] = color;
				continue;
			}

			uint8_t sa = (uint8_t)((src_a * alpha) >> 8);
			if(sa == 0)
				continue;

			drow[col] = g2d_blend_argb_scalar(drow[col], sa,
				(uint8_t)((color >> 16) & 0xff),
				(uint8_t)((color >> 8) & 0xff),
				(uint8_t)(color & 0xff));
		}
	}
}

/* -------------------------------------------------------------- scale --- */

/* bilinear scaler filling the whole destination, with per-axis 16.16
   fixed-point stepping and 8-bit quantized weights */
enum {
	G2D_SCALE_FIXED_SHIFT = 16,
	G2D_SCALE_FIXED_SCALE = 1 << G2D_SCALE_FIXED_SHIFT,
	G2D_SCALE_FIXED_MASK = G2D_SCALE_FIXED_SCALE - 1
};

/* interpolate the packed (byte 0, byte 2) channels with an 8-bit weight;
   w0+w1 = 256 keeps each 16-bit lane carry-free (255*256 = 0xFF00) */
static inline uint32_t g2d_scale_lerp_rb(uint32_t a, uint32_t b, uint32_t w1) {
	uint32_t w0 = 256 - w1;
	return ((((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1) >> 8) & 0x00FF00FF);
}

static inline uint32_t g2d_scale_lerp_ga(uint32_t a, uint32_t b, uint32_t w1) {
	uint32_t w0 = 256 - w1;
	return ((((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1) >> 8)
			& 0x00FF00FF) << 8);
}

/* same, with +128 rounding per lane to match the NEON vmla/rnd paths
   bit-exactly (per-lane max is 255*256 + 128 = 0xFF80, still carry-free) */
static inline uint32_t g2d_scale_lerp_rb_r(uint32_t a, uint32_t b, uint32_t w1) {
	uint32_t w0 = 256 - w1;
	return ((((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1 + 0x00800080u) >> 8)
			& 0x00FF00FF);
}

static inline uint32_t g2d_scale_lerp_ga_r(uint32_t a, uint32_t b, uint32_t w1) {
	uint32_t w0 = 256 - w1;
	return ((((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1 + 0x00800080u) >> 8)
			& 0x00FF00FF) << 8);
}

static inline uint32_t g2d_scale_bilinear_interp(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
		uint32_t fx, uint32_t fy) {
	/* quantize 16.16 fractions to 8-bit rounded weights, matching the NEON
	   path precision; 8x32-bit multiplies instead of 16x64-bit */
	uint32_t fx8 = (fx + 128) >> 8;
	uint32_t fy8 = (fy + 128) >> 8;

	uint32_t top_rb = g2d_scale_lerp_rb(p00, p01, fx8);
	uint32_t top_ga = g2d_scale_lerp_ga(p00, p01, fx8);
	uint32_t bot_rb = g2d_scale_lerp_rb(p10, p11, fx8);
	uint32_t bot_ga = g2d_scale_lerp_ga(p10, p11, fx8);

	return g2d_scale_lerp_rb(top_rb, bot_rb, fy8) |
			g2d_scale_lerp_ga(top_ga, bot_ga, fy8);
}

static void g2d_scale_prepare_axis(int dst_len, int src_max, uint32_t inv_scale,
		int *idx0, int *idx1, uint32_t *frac) {
	uint32_t pos = 0;

	for(int i = 0; i < dst_len; i++) {
		int base = (int)(pos >> G2D_SCALE_FIXED_SHIFT);
		uint32_t f = pos & G2D_SCALE_FIXED_MASK;
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

/* plain decimation when both axes are exact integer steps >= 2 */
static int g2d_scale_integer_downsample(const uint32_t* argb_src, int32_t src_w,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		uint32_t inv_x, uint32_t inv_y) {
	if((inv_x & G2D_SCALE_FIXED_MASK) != 0 || (inv_y & G2D_SCALE_FIXED_MASK) != 0)
		return 0;

	uint32_t step_x = inv_x >> G2D_SCALE_FIXED_SHIFT;
	uint32_t step_y = inv_y >> G2D_SCALE_FIXED_SHIFT;
	if(step_x < 2 || step_y < 2)
		return 0;

	int is_pow2_x = (step_x & (step_x - 1)) == 0;
	int is_pow2_y = (step_y & (step_y - 1)) == 0;

	if(is_pow2_x && is_pow2_y) {
		unsigned shift_x = 0;
		unsigned shift_y = 0;
		while((1U << shift_x) < step_x)
			shift_x++;
		while((1U << shift_y) < step_y)
			shift_y++;

		for(int32_t y = 0; y < dst_h; y++) {
			const uint32_t *src_row = argb_src + (y << shift_y) * src_w;
			uint32_t *dst_row = argb_dst + y * dst_w;
			int32_t x = 0;

			for(; x <= dst_w - 4; x += 4) {
				dst_row[x] = src_row[x << shift_x];
				dst_row[x + 1] = src_row[(x + 1) << shift_x];
				dst_row[x + 2] = src_row[(x + 2) << shift_x];
				dst_row[x + 3] = src_row[(x + 3) << shift_x];
			}

			for(; x < dst_w; x++)
				dst_row[x] = src_row[x << shift_x];
		}

		return 1;
	}

	uint32_t src_y = 0;
	for(int32_t y = 0; y < dst_h; y++) {
		const uint32_t *src_row = argb_src + src_y * src_w;
		uint32_t *dst_row = argb_dst + y * dst_w;
		uint32_t src_x = 0;
		int32_t x = 0;

		for(; x <= dst_w - 4; x += 4) {
			dst_row[x] = src_row[src_x];
			src_x += step_x;
			dst_row[x + 1] = src_row[src_x];
			src_x += step_x;
			dst_row[x + 2] = src_row[src_x];
			src_x += step_x;
			dst_row[x + 3] = src_row[src_x];
			src_x += step_x;
		}

		for(; x < dst_w; x++) {
			dst_row[x] = src_row[src_x];
			src_x += step_x;
		}

		src_y += step_y;
	}

	return 1;
}

/* Separable two-pass upscale for scale >= 1 on both axes (inv <= FIXED_SCALE):
   each source row feeds ~scale destination rows, so the horizontal lerp is
   computed once per source row into a 2-slot row cache (gi0 advances <= 1
   per destination row), and the vertical pass becomes a contiguous scan.
   All scratch comes from a single malloc. Returns 0 on malloc failure so
   the caller falls back to the gather path. */
static int g2d_scale_separable_upscale(const uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		uint32_t inv_x, uint32_t inv_y) {
	int hmax = src_h - 1;
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
		int base = (int)(pos >> G2D_SCALE_FIXED_SHIFT);
		uint32_t f = pos & G2D_SCALE_FIXED_MASK;
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
		pos += inv_x;
	}

	uint32_t src_y = 0;
	for(int i = 0; i < dst_h; i++) {
		int gi0 = (int)(src_y >> G2D_SCALE_FIXED_SHIFT);
		uint32_t gi_frac = src_y & G2D_SCALE_FIXED_MASK;

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
			const uint32_t *srow = argb_src + y * src_w;
			uint32_t *hr = hrow[slot];

			for(int j = 0; j < dst_w; j++) {
				uint32_t a = srow[x0[j]];
				uint32_t b = srow[x1[j]];

				if(a == b)
					hr[j] = a;
				else
					hr[j] = g2d_scale_lerp_rb_r(a, b, fx8[j]) |
							g2d_scale_lerp_ga_r(a, b, fx8[j]);
			}
			hrow_y[slot] = y;
		}

		const uint32_t *r0 = (hrow_y[0] == gi0) ? hrow[0] : hrow[1];
		const uint32_t *r1 = (hrow_y[0] == gi1) ? hrow[0] : hrow[1];
		uint32_t *drow = argb_dst + i * dst_w;

		if(fy8 == 0 || r0 == r1) {
			memcpy(drow, r0, cols * sizeof(uint32_t));
		}
		else if(((uintptr_t)drow & 0xF) != 0) {
			/* unaligned row start: SIMD stores can fault on Device-mapped
			   framebuffer memory */
			for(int j = 0; j < dst_w; j++)
				drow[j] = g2d_scale_lerp_rb_r(r0[j], r1[j], fy8) |
						g2d_scale_lerp_ga_r(r0[j], r1[j], fy8);
		}
		else {
			uint16x8_t wv0 = vdupq_n_u16((uint16_t)(256 - fy8));
			uint16x8_t wv1 = vdupq_n_u16(fy8);
			uint16x8_t rnd = vdupq_n_u16(128);

			int j = 0;
			for(; j <= dst_w - 4; j += 4) {
				uint8x16_t b0 = vreinterpretq_u8_u32(g2d_ld1q_u32(r0 + j));
				uint8x16_t b1 = vreinterpretq_u8_u32(g2d_ld1q_u32(r1 + j));

				uint16x8_t t0l = vmovl_u8(vget_low_u8(b0));
				uint16x8_t t0h = vmovl_u8(vget_high_u8(b0));
				uint16x8_t t1l = vmovl_u8(vget_low_u8(b1));
				uint16x8_t t1h = vmovl_u8(vget_high_u8(b1));

				uint16x8_t ol = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, t0l, wv0), t1l, wv1), 8);
				uint16x8_t oh = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, t0h, wv0), t1h, wv1), 8);

				g2d_st1q_u32(drow + j,
						vreinterpretq_u32_u8(vcombine_u8(vmovn_u16(ol), vmovn_u16(oh))));
			}

			for(; j < dst_w; j++) {
				drow[j] = g2d_scale_lerp_rb_r(r0[j], r1[j], fy8) |
						g2d_scale_lerp_ga_r(r0[j], r1[j], fy8);
			}
		}

		src_y += inv_y;
	}

	free(mem);
	return 1;
}

#if defined(__GNUC__) && !defined(__clang__)
/* GCC with -mstrict-align lowers vld1_u32 on a pointer without proven 8-byte
   alignment to 2 scalar loads + orr + fmov. LD1 itself supports unaligned
   addresses in normal memory, so emit the single-instruction load directly. */
static inline uint32x2_t g2d_scale_ld1_u32_pair(const uint32_t *p) {
	uint32x2_t v;
	__asm__("ld1 {%0.2s}, [%1]" : "=w"(v) : "r"(p) : );
	return v;
}
#else
static inline uint32x2_t g2d_scale_ld1_u32_pair(const uint32_t *p) {
	return vld1_u32(p);
}
#endif

/* Difference-form u8 lerp, same integer arithmetic as the scalar w8 path:
   out = a + round((b - a) * w1 / 256), w1 unsigned 0..255. NEON has no
   mixed-sign u8xs8 widening multiply, so take the difference of two unsigned
   widening products; it lands in s16 range (|.| <= 65025) and vrshrn applies
   the +128 rounding shift. No pixel widening, no w0 weight vectors. */
static inline uint8x8_t g2d_scale_lerp8(uint8x8_t a, uint8x8_t b, uint8x8_t w1) {
	int16x8_t acc = vreinterpretq_s16_u16(vsubq_u16(vmull_u8(b, w1), vmull_u8(a, w1)));
	return vadd_u8(a, vreinterpret_u8_s8(vrshrn_n_s16(acc, 8)));
}

void arch_g2d_scale_to(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return;

	if(src_w == dst_w && src_h == dst_h) {
		g2d_row_copy_neon(argb_dst, argb_src, src_w * src_h);
		return;
	}

	/* per-axis 16.16 fixed-point source step */
	uint32_t inv_x = (uint32_t)(((uint64_t)src_w << G2D_SCALE_FIXED_SHIFT) / dst_w);
	uint32_t inv_y = (uint32_t)(((uint64_t)src_h << G2D_SCALE_FIXED_SHIFT) / dst_h);

	/* integer decimation: no interpolation needed at all */
	if(g2d_scale_integer_downsample(argb_src, src_w, argb_dst, dst_w, dst_h, inv_x, inv_y))
		return;

	/* scale >= 1 on both axes: separable two-pass with row cache; falls
	   through to the gather path below on malloc failure */
	if(inv_x <= G2D_SCALE_FIXED_SCALE && inv_y <= G2D_SCALE_FIXED_SCALE &&
			g2d_scale_separable_upscale(argb_src, src_w, src_h, argb_dst, dst_w, dst_h, inv_x, inv_y))
		return;

	int wmax = src_w - 1;
	int hmax = src_h - 1;

	int *x0 = (int*)malloc((size_t)dst_w * sizeof(int));
	int *x1 = (int*)malloc((size_t)dst_w * sizeof(int));
	uint32_t *x_frac = (uint32_t*)malloc((size_t)dst_w * sizeof(uint32_t));
	uint8_t *fx8 = (uint8_t*)malloc((size_t)dst_w * sizeof(uint8_t));

	if(x0 != NULL && x1 != NULL && x_frac != NULL && fx8 != NULL) {
		g2d_scale_prepare_axis(dst_w, wmax, inv_x, x0, x1, x_frac);

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
			int gi0 = (int)(src_y >> G2D_SCALE_FIXED_SHIFT);
			uint32_t gi_frac = src_y & G2D_SCALE_FIXED_MASK;
			int gi1 = gi0 + 1;

			if(gi0 >= hmax) {
				gi0 = hmax;
				gi1 = hmax;
				gi_frac = 0;
			}
			else if(gi1 > hmax) {
				gi1 = hmax;
			}

			const uint32_t *row0 = argb_src + gi0 * src_w;
			const uint32_t *row1 = argb_src + gi1 * src_w;
			uint32_t *drow = argb_dst + i * dst_w;

			uint16_t fy = (uint16_t)((gi_frac + 128) >> 8);
			if(fy == 256) {
				/* folds exactly into the next source row (bit-exact, same as
				   the scalar w8 path); keeps the weight in u8 range */
				gi1 = gi0;
				row1 = row0;
				fy = 0;
			}
			uint8x8_t wy = vdup_n_u8((uint8_t)fy);

			/* 16-byte SIMD stores need an aligned row start: the surface may
			   be Device-mapped framebuffer memory where unaligned accesses
			   fault */
			int rw = (((uintptr_t)drow & 0xF) == 0) ? neon_w : 0;

			int j = 0;
			for(; j < rw; j += 4) {
				/* gather (p00,p01) pairs for 4 output columns from both rows;
				   x1[j] == x0[j]+1 in the NEON-safe prefix */
				uint32x4_t ab0 = vcombine_u32(g2d_scale_ld1_u32_pair(row0 + x0[j]),
						g2d_scale_ld1_u32_pair(row0 + x0[j + 1]));
				uint32x4_t cd0 = vcombine_u32(g2d_scale_ld1_u32_pair(row0 + x0[j + 2]),
						g2d_scale_ld1_u32_pair(row0 + x0[j + 3]));
				uint32x4x2_t u0 = vuzpq_u32(ab0, cd0);

				uint32x4_t ab1 = vcombine_u32(g2d_scale_ld1_u32_pair(row1 + x0[j]),
						g2d_scale_ld1_u32_pair(row1 + x0[j + 1]));
				uint32x4_t cd1 = vcombine_u32(g2d_scale_ld1_u32_pair(row1 + x0[j + 2]),
						g2d_scale_ld1_u32_pair(row1 + x0[j + 3]));
				uint32x4x2_t u1 = vuzpq_u32(ab1, cd1);

				/* flat 4-pixel block: skip weights and the whole lerp chain */
				uint32x4_t neq = vorrq_u32(
						vorrq_u32(veorq_u32(u0.val[0], u0.val[1]),
								veorq_u32(u0.val[0], u1.val[0])),
						veorq_u32(u0.val[0], u1.val[1]));
				if(vmaxvq_u32(neq) == 0) {
					g2d_st1q_u32(drow + j, u0.val[0]);
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
						g2d_scale_lerp8(vget_low_u8(vreinterpretq_u8_u32(u0.val[0])),
								vget_low_u8(vreinterpretq_u8_u32(u0.val[1])),
								wl.val[0]),
						g2d_scale_lerp8(vget_high_u8(vreinterpretq_u8_u32(u0.val[0])),
								vget_high_u8(vreinterpretq_u8_u32(u0.val[1])),
								wl.val[1]));
				uint8x16_t bl = vcombine_u8(
						g2d_scale_lerp8(vget_low_u8(vreinterpretq_u8_u32(u1.val[0])),
								vget_low_u8(vreinterpretq_u8_u32(u1.val[1])),
								wl.val[0]),
						g2d_scale_lerp8(vget_high_u8(vreinterpretq_u8_u32(u1.val[0])),
								vget_high_u8(vreinterpretq_u8_u32(u1.val[1])),
								wl.val[1]));

				/* vertical lerp in u8 + pack */
				uint8x8_t ol = g2d_scale_lerp8(vget_low_u8(tl), vget_low_u8(bl), wy);
				uint8x8_t oh = g2d_scale_lerp8(vget_high_u8(tl), vget_high_u8(bl), wy);

				g2d_st1q_u32(drow + j, vreinterpretq_u32_u8(vcombine_u8(ol, oh)));
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
					drow[j] = g2d_scale_bilinear_interp(p00, p01, p10, p11, x_frac[j], gi_frac);
				}
			}

			src_y += inv_y;
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

	/* scalar fallback on malloc failure */
	uint32_t src_y = 0;
	for(int i = 0; i < dst_h; i++) {
		int gi0 = (int)(src_y >> G2D_SCALE_FIXED_SHIFT);
		uint32_t gi_frac = src_y & G2D_SCALE_FIXED_MASK;
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
			int gj0 = (int)(src_x >> G2D_SCALE_FIXED_SHIFT);
			uint32_t gj_frac = src_x & G2D_SCALE_FIXED_MASK;
			int gj1 = gj0 + 1;

			if(gj0 >= wmax) {
				gj0 = wmax;
				gj1 = wmax;
				gj_frac = 0;
			}
			else if(gj1 > wmax) {
				gj1 = wmax;
			}

			uint32_t p00 = argb_src[gi0w + gj0];
			uint32_t p01 = argb_src[gi0w + gj1];
			uint32_t p10 = argb_src[gi1w + gj0];
			uint32_t p11 = argb_src[gi1w + gj1];

			if(p00 == p01 && p00 == p10 && p00 == p11) {
				argb_dst[dst_row + j] = p00;
			}
			else {
				argb_dst[dst_row + j] = g2d_scale_bilinear_interp(p00, p01, p10, p11, gj_frac, gi_frac);
			}

			src_x += inv_x;
		}

		src_y += inv_y;
	}
}

/* ------------------------------------------------------------- rotate --- */

/* fixed point trig: table[i] = round(sin(i degree) * 16384), avoids a
   libm dependency */
#define G2D_FP_BITS 14
#define G2D_FP_ONE  (1 << G2D_FP_BITS)
#define G2D_FP_HALF (1 << (G2D_FP_BITS - 1))

static const int16_t g2d_sin_table[91] = {
	0, 286, 572, 857, 1143, 1428, 1713, 1997,
	2280, 2563, 2845, 3126, 3406, 3686, 3964, 4240,
	4516, 4790, 5063, 5334, 5604, 5872, 6138, 6402,
	6664, 6924, 7182, 7438, 7692, 7943, 8192, 8438,
	8682, 8923, 9162, 9397, 9630, 9860, 10087, 10311,
	10531, 10749, 10963, 11174, 11381, 11585, 11786, 11982,
	12176, 12365, 12551, 12733, 12911, 13085, 13255, 13421,
	13583, 13741, 13894, 14044, 14189, 14330, 14466, 14598,
	14726, 14849, 14968, 15082, 15191, 15296, 15396, 15491,
	15582, 15668, 15749, 15826, 15897, 15964, 16026, 16083,
	16135, 16182, 16225, 16262, 16294, 16322, 16344, 16362,
	16374, 16382, 16384
};

static inline int32_t g2d_norm_degree(int32_t degree) {
	return ((degree % 360) + 360) % 360;
}

static inline int32_t g2d_sin_fp(int32_t degree) {
	degree = g2d_norm_degree(degree);
	if(degree <= 90)
		return g2d_sin_table[degree];
	if(degree <= 180)
		return g2d_sin_table[180 - degree];
	if(degree <= 270)
		return -g2d_sin_table[degree - 180];
	return -g2d_sin_table[360 - degree];
}

static inline int32_t g2d_cos_fp(int32_t degree) {
	return g2d_sin_fp(degree + 90);
}

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
void arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
		int32_t* dst_w, int32_t* dst_h) {
	int32_t c;
	int32_t s;
	int32_t w;
	int32_t h;

	if(dst_w == NULL || dst_h == NULL)
		return;
	*dst_w = 0;
	*dst_h = 0;
	if(src_w <= 0 || src_h <= 0)
		return;

	degree = g2d_norm_degree(degree);
	if(degree % 90 == 0) {
		if(degree == 90 || degree == 270) {
			*dst_w = src_h;
			*dst_h = src_w;
		}
		else {
			*dst_w = src_w;
			*dst_h = src_h;
		}
		return;
	}

	c = g2d_cos_fp(degree);
	s = g2d_sin_fp(degree);
	if(c < 0) c = -c;
	if(s < 0) s = -s;
	/* round up so the rotated corners always fit */
	w = (int32_t)(((int64_t)src_w * c + (int64_t)src_h * s + G2D_FP_ONE - 1) >> G2D_FP_BITS);
	h = (int32_t)(((int64_t)src_w * s + (int64_t)src_h * c + G2D_FP_ONE - 1) >> G2D_FP_BITS);
	*dst_w = (w < 1) ? 1 : w;
	*dst_h = (h < 1) ? 1 : h;
}

static inline uint32x4_t rotate_rev4_u32(uint32x4_t v) {
    /* reverse the four 32-bit lanes: {a,b,c,d} -> {d,c,b,a} */
    return vcombine_u32(vget_high_u32(vrev64q_u32(v)), vget_low_u32(vrev64q_u32(v)));
}

static inline void g2d_rotate_90_cw_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
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
            uint32x4_t v0 = g2d_ld1q_u32(s0);
            uint32x4_t v1 = g2d_ld1q_u32(s1);
            uint32x4_t v2 = g2d_ld1q_u32(s2);
            uint32x4_t v3 = g2d_ld1q_u32(s3);

            /* transpose the 4x4 pixel block into columns */
            uint32x4x2_t t01 = vtrnq_u32(v0, v1);
            uint32x4x2_t t23 = vtrnq_u32(v2, v3);
            uint32x4_t c0 = vcombine_u32(vget_low_u32(t01.val[0]), vget_low_u32(t23.val[0]));
            uint32x4_t c1 = vcombine_u32(vget_low_u32(t01.val[1]), vget_low_u32(t23.val[1]));
            uint32x4_t c2 = vcombine_u32(vget_high_u32(t01.val[0]), vget_high_u32(t23.val[0]));
            uint32x4_t c3 = vcombine_u32(vget_high_u32(t01.val[1]), vget_high_u32(t23.val[1]));

            g2d_st1q_u32(d0 + x, c0);
            g2d_st1q_u32(d1 + x, c1);
            g2d_st1q_u32(d2 + x, c2);
            g2d_st1q_u32(d3 + x, c3);
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

static inline void g2d_rotate_270_cw_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
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
            uint32x4_t v0 = g2d_ld1q_u32(s0);
            uint32x4_t v1 = g2d_ld1q_u32(s1);
            uint32x4_t v2 = g2d_ld1q_u32(s2);
            uint32x4_t v3 = g2d_ld1q_u32(s3);

            /* transpose the 4x4 pixel block into columns */
            uint32x4x2_t t01 = vtrnq_u32(v0, v1);
            uint32x4x2_t t23 = vtrnq_u32(v2, v3);
            uint32x4_t c0 = vcombine_u32(vget_low_u32(t01.val[0]), vget_low_u32(t23.val[0]));
            uint32x4_t c1 = vcombine_u32(vget_low_u32(t01.val[1]), vget_low_u32(t23.val[1]));
            uint32x4_t c2 = vcombine_u32(vget_high_u32(t01.val[0]), vget_high_u32(t23.val[0]));
            uint32x4_t c3 = vcombine_u32(vget_high_u32(t01.val[1]), vget_high_u32(t23.val[1]));

            /* dst[y+i][x+j] = src[x+3-j][width-1-(y+i)]: row i takes column 3-i */
            g2d_st1q_u32(d0 + x, c3);
            g2d_st1q_u32(d1 + x, c2);
            g2d_st1q_u32(d2 + x, c1);
            g2d_st1q_u32(d3 + x, c0);
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

static inline void g2d_rotate_180_neon(const uint32_t* src, uint32_t* dst, int width, int height) {
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
            g2d_st1q_u32(d0 + x, rotate_rev4_u32(g2d_ld1q_u32(s0 + width - x - 4)));
            g2d_st1q_u32(d1 + x, rotate_rev4_u32(g2d_ld1q_u32(s1 + width - x - 4)));
            g2d_st1q_u32(d2 + x, rotate_rev4_u32(g2d_ld1q_u32(s2 + width - x - 4)));
            g2d_st1q_u32(d3 + x, rotate_rev4_u32(g2d_ld1q_u32(s3 + width - x - 4)));
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

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by arch_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
void arch_g2d_rotate(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h, int32_t degree) {
	int32_t bw;
	int32_t bh;
	int32_t c;
	int32_t s;
	int32_t scx;
	int32_t scy;
	int32_t dcx;
	int32_t dcy;

	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return;

	degree = g2d_norm_degree(degree);
	if(degree == 0) {
		/* plain copy */
		if(dst_w < src_w || dst_h < src_h)
			return;
		if(argb_src != argb_dst)
			g2d_row_copy_neon(argb_dst, argb_src, src_w * src_h);
		return;
	}

	if(degree == 180) {
		if(dst_w < src_w || dst_h < src_h)
			return;
		if(argb_src == argb_dst) {
			/* in-place reversal */
			uint32_t* lo = argb_src;
			uint32_t* hi = argb_src + (size_t)src_w * src_h - 1;
			while(lo < hi) {
				uint32_t t = *lo;
				*lo = *hi;
				*hi = t;
				lo++;
				hi--;
			}
			return;
		}
		g2d_rotate_180_neon(argb_src, argb_dst, src_w, src_h);
		return;
	}

	if(degree == 90 || degree == 270) {
		/* 90/270 change surface dimensions, in-place is not supported */
		if(argb_src == argb_dst || dst_w < src_h || dst_h < src_w)
			return;

		if(degree == 90)
			g2d_rotate_90_cw_neon(argb_src, argb_dst, src_w, src_h);
		else
			g2d_rotate_270_cw_neon(argb_src, argb_dst, src_w, src_h);
		return;
	}

	/* arbitrary angle: inverse-mapped nearest neighbor, rotation around
	   the center, content written into the top-left bw x bh bounding box. */
	if(argb_src == argb_dst)
		return;
	arch_g2d_rotated_size(src_w, src_h, degree, &bw, &bh);
	if(bw <= 0 || bh <= 0 || dst_w < bw || dst_h < bh)
		return;

	memset(argb_dst, 0, (size_t)dst_w * dst_h * sizeof(uint32_t));

	c = g2d_cos_fp(degree);
	s = g2d_sin_fp(degree);
	/* centers in fixed point (value * G2D_FP_ONE) */
	scx = (src_w - 1) << (G2D_FP_BITS - 1);
	scy = (src_h - 1) << (G2D_FP_BITS - 1);
	dcx = (bw - 1) << (G2D_FP_BITS - 1);
	dcy = (bh - 1) << (G2D_FP_BITS - 1);

	for(int32_t y = 0; y < bh; y++) {
		uint32_t* drow = argb_dst + y * dst_w;
		int64_t dy = (int64_t)y * G2D_FP_ONE - dcy;
		for(int32_t x = 0; x < bw; x++) {
			int64_t dx = (int64_t)x * G2D_FP_ONE - dcx;
			/* inverse of clockwise rotation: src = R(-degree) * dst */
			int64_t sxf = ((dx * c + dy * s) >> G2D_FP_BITS) + scx;
			int64_t syf = ((dy * c - dx * s) >> G2D_FP_BITS) + scy;
			int32_t sx = (int32_t)((sxf + G2D_FP_HALF) >> G2D_FP_BITS);
			int32_t sy = (int32_t)((syf + G2D_FP_HALF) >> G2D_FP_BITS);
			if(sx >= 0 && sx < src_w && sy >= 0 && sy < src_h)
				drow[x] = argb_src[sy * src_w + sx];
		}
	}
}

#else /* !ARCH_BOOST: portable fallback, same semantics as the C reference */

int32_t arch_g2d_init(void) {
	return 0;
}

void arch_g2d_fill(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(argb == NULL || argb_w <= 0 || argb_h <= 0 || w <= 0 || h <= 0)
		return;

	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;
	if(w <= 0 || h <= 0)
		return;

	for(int32_t row = 0; row < h; row++) {
		uint32_t* p = argb + (y + row) * argb_w + x;
		for(int32_t col = 0; col < w; col++)
			p[col] = color;
	}
}

void arch_g2d_blt(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	(void)argb_src; (void)src_w; (void)src_h;
	(void)sx; (void)sy; (void)sw; (void)sh;
	(void)argb_dst; (void)dst_w; (void)dst_h;
	(void)dx; (void)dy; (void)dw; (void)dh;
}

void arch_g2d_blt_alpha(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	(void)argb_src; (void)src_w; (void)src_h;
	(void)sx; (void)sy; (void)sw; (void)sh;
	(void)argb_dst; (void)dst_w; (void)dst_h;
	(void)dx; (void)dy; (void)dw; (void)dh; (void)alpha;
}

void arch_g2d_scale_to(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h) {
	(void)argb_src; (void)src_w; (void)src_h;
	(void)argb_dst; (void)dst_w; (void)dst_h;
}

void arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
		int32_t* dst_w, int32_t* dst_h) {
	(void)src_w; (void)src_h; (void)degree;
	(void)dst_w; (void)dst_h;
}

void arch_g2d_rotate(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h, int32_t degree) {
	(void)argb_src; (void)src_w; (void)src_h;
	(void)argb_dst; (void)dst_w; (void)dst_h; (void)degree;
}

#endif /* ARCH_BOOST */

#ifdef __cplusplus
}
#endif
