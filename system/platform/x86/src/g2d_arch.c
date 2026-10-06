#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/ewokdef.h>

#ifdef ARCH_BOOST
#include <emmintrin.h>
#endif

/* scalar per-pixel blend, same math as the C reference: effective alpha
   (src_a * alpha) >> 8 is applied by the callers, then the /255 blend.
   A fully transparent dest carries no colour (its RGB is usually zeroed
   by alpha masks), so blending against it would darken the source: take
   the source colour with the effective alpha instead, same as
   graph_pixel_argb_raw. */
static inline uint32_t g2d_blend_argb_scalar(uint32_t dst_color, uint8_t a,
		uint8_t r, uint8_t g, uint8_t b) {
	uint32_t oa = (dst_color >> 24) & 0xff;
	if(oa == 0)
		return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
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

/* proportional clip of a src->dst blit against both buffer bounds, keeping
   the src/dst rects in lock-step (same helper the other arch back ends use).
   returns 0 when nothing is left to draw. plain C: shared by the simd and
   the portable blit paths. */
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

/* one source pixel over one destination pixel with a constant global alpha,
   bit-identical to graph_blt_alpha_cpu's 1:1 path: effective alpha is
   (src_a * alpha) >> 8 (exactly 255 for a fully opaque source pixel when
   alpha == 0xff, matching the reference's opaque-copy shortcut), an
   effective alpha of 0 leaves the destination untouched, and a fully
   transparent destination takes the source RGB with the effective alpha.
   used for the scalar head/tail of the simd rows and by the portable path. */
static inline uint32_t x86_blt_alpha_px(uint32_t dst_color, uint32_t color, uint32_t alpha) {
	uint32_t src_a = (color >> 24) & 0xff;
	uint32_t sa;

	if(src_a == 0)
		return dst_color;
	if(alpha == 0xff && src_a == 0xff)
		return color;

	sa = (src_a * alpha) >> 8;
	if(sa == 0)
		return dst_color;

	return g2d_blend_argb_scalar(dst_color, (uint8_t)sa,
			(uint8_t)((color >> 16) & 0xff),
			(uint8_t)((color >> 8) & 0xff),
			(uint8_t)(color & 0xff));
}

/* 16.16 fixed-point source stepping, same constants as scale.c */
enum {
	G2D_SCALE_FIXED_SHIFT = 16,
	G2D_SCALE_FIXED_SCALE = 1 << G2D_SCALE_FIXED_SHIFT,
	G2D_SCALE_FIXED_MASK = G2D_SCALE_FIXED_SCALE - 1
};

/* truncating lerp of two packed argb pixels with an 8-bit weight (w0+w1=256),
   bit-identical to scale.c's scale_lerp_rb | scale_lerp_ga pair: each 16-bit
   half peaks at 255*256 = 0xFF00, so the packed channels never carry. */
static inline uint32_t x86_scale_lerp(uint32_t a, uint32_t b, uint32_t w1) {
	uint32_t w0 = 256 - w1;
	uint32_t rb = (((a & 0x00FF00FF) * w0 + (b & 0x00FF00FF) * w1) >> 8) & 0x00FF00FF;
	uint32_t ga = (((((a >> 8) & 0x00FF00FF) * w0 + ((b >> 8) & 0x00FF00FF) * w1) >> 8)
			& 0x00FF00FF) << 8;
	return rb | ga;
}

#ifdef ARCH_BOOST
/* scalar blend of one pixel against a constant color, bit-identical math
   to x86_blend_4px (used for the head/tail alignment pixels) */
static inline uint32_t x86_fill_blend_px(uint32_t dst, uint32_t color, uint32_t a) {
	uint32_t inv_a = 255 - a;
	uint32_t db = dst & 0xff;
	uint32_t dg = (dst >> 8) & 0xff;
	uint32_t dr = (dst >> 16) & 0xff;
	uint32_t da = (dst >> 24) & 0xff;
	uint32_t div255;

	/* transparent dest: source colour with the fill alpha, unblended */
	if(da == 0)
		return (a << 24) | (color & 0x00ffffff);

	div255 = ((color & 0xff) * a + db * inv_a);
	db = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (((color >> 8) & 0xff) * a + dg * inv_a);
	dg = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (((color >> 16) & 0xff) * a + dr * inv_a);
	dr = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (255 - da) * a;
	da = da + ((div255 + 1 + (div255 >> 8)) >> 8);
	return (da << 24) | (dr << 16) | (dg << 8) | db;
}

/* div255 for eight unsigned 16-bit lanes, same rounding as the scalar
   helper: (v + 1 + (v >> 8)) >> 8 (max input 65025, no lane overflow) */
static inline __m128i x86_div255_epu16(__m128i v) {
	__m128i t = _mm_add_epi16(v, _mm_set1_epi16(1));
	t = _mm_add_epi16(t, _mm_srli_epi16(v, 8));
	return _mm_srli_epi16(t, 8);
}

/* Source-over blend of 4 pixels against a constant color. Lanes hold
   [b,g,r,a,b,g,r,a] after the byte unpack; the alpha lane is replaced
   with bg_a + div255((255-bg_a)*a) via alpha_mask. Pixels whose dest
   alpha is 0 take fg32 (the fill colour with its alpha) unblended. dst
   must be 16-byte aligned. */
static inline void x86_blend_4px(uint32_t* dst, __m128i cpa16, __m128i a16,
		__m128i inv_a16, __m128i alpha_mask, __m128i full16, __m128i zero, __m128i fg32) {
	__m128i bg = _mm_load_si128((const __m128i*)dst);
	__m128i bg_clear = _mm_cmpeq_epi32(_mm_srli_epi32(bg, 24), zero);
	__m128i lo = _mm_unpacklo_epi8(bg, zero);
	__m128i hi = _mm_unpackhi_epi8(bg, zero);

	__m128i blend_lo = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(lo, inv_a16)));
	__m128i blend_hi = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(hi, inv_a16)));

	__m128i oa_lo = _mm_add_epi16(lo, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, lo), a16)));
	__m128i oa_hi = _mm_add_epi16(hi, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, hi), a16)));

	lo = _mm_or_si128(_mm_and_si128(alpha_mask, oa_lo), _mm_andnot_si128(alpha_mask, blend_lo));
	hi = _mm_or_si128(_mm_and_si128(alpha_mask, oa_hi), _mm_andnot_si128(alpha_mask, blend_hi));

	__m128i out = _mm_packus_epi16(lo, hi);
	out = _mm_or_si128(_mm_and_si128(bg_clear, fg32), _mm_andnot_si128(bg_clear, out));
	_mm_store_si128((__m128i*)dst, out);
}

/* alpha fill: simd rows of 4 pixels with a scalar head for 16-byte
   realignment and a scalar tail; the fill color is constant so the
   per-channel color*a constants are set up once per call. */
int32_t arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint32_t a;

	if(argb == NULL)
		return 0;
	a = (color >> 24) & 0xff;
	if(a == 0)
		return 0;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return 0;
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;

	{
		uint32_t inv_a = 255 - a;
		uint32_t cb = color & 0xff;
		uint32_t cg = (color >> 8) & 0xff;
		uint32_t cr = (color >> 16) & 0xff;

		/* per-lane color*a constants for the [b,g,r,a,b,g,r,a] lane
		   layout; the alpha lanes are don't-care (masked away), so
		   they hold 0 */
		__m128i cpa16 = _mm_set_epi32((int)(cr * a), (int)((cg * a) << 16 | (cb * a)),
				(int)(cr * a), (int)((cg * a) << 16 | (cb * a)));
		__m128i a16 = _mm_set1_epi16((short)a);
		__m128i inv_a16 = _mm_set1_epi16((short)inv_a);
		__m128i alpha_mask = _mm_set_epi32((int)0xFFFF0000, 0, (int)0xFFFF0000, 0);
		__m128i full16 = _mm_set1_epi16(255);
		__m128i zero = _mm_setzero_si128();
		__m128i fg32 = _mm_set1_epi32((int)color);

		for(int32_t row = y; row < y + h; row++) {
			uint32_t* dp = argb + row * argb_w + x;
			int32_t i = 0;

			/* scalar head realigns the row start to 16 bytes */
			while(i < w && (((uintptr_t)(dp + i)) & 0x0F) != 0) {
				dp[i] = x86_fill_blend_px(dp[i], color, a);
				i++;
			}

			for(; i + 4 <= w; i += 4)
				x86_blend_4px(dp + i, cpa16, a16, inv_a16, alpha_mask, full16, zero, fg32);

			/* scalar tail, at most 3 pixels per row */
			for(; i < w; i++)
				dp[i] = x86_fill_blend_px(dp[i], color, a);
		}
	}
	return 0;
}

/* Blend one 8-lane half (two [b,g,r,a] pixels) of a source-over alpha blit.
   sa is the per-pixel effective alpha broadcast across each pixel's four
   channel lanes; a255 selects the alpha == 0xff specialisation where a fully
   opaque source pixel blends with sa == 255 (== the reference opaque copy).
   Bit-identical to x86_blt_alpha_px. */
static inline __m128i x86_blt_alpha_half(__m128i f, __m128i b, __m128i alpha16,
		int a255, __m128i alpha_mask, __m128i full16, __m128i zero) {
	/* per-pixel source alpha, broadcast over the pixel's four lanes:
	   shufflelo replicates lane 3, shufflehi replicates lane 7 */
	__m128i sa_b = _mm_shufflehi_epi16(_mm_shufflelo_epi16(f, 0xFF), 0xFF);
	__m128i sa = _mm_srli_epi16(_mm_mullo_epi16(sa_b, alpha16), 8);
	if(a255) {
		__m128i opq = _mm_cmpeq_epi16(sa_b, full16);
		sa = _mm_or_si128(_mm_and_si128(opq, full16), _mm_andnot_si128(opq, sa));
	}
	__m128i inv_sa = _mm_sub_epi16(full16, sa);

	/* rgb lanes: div255(f*sa + b*inv_sa); f*sa + b*inv_sa <= 255*255 = 65025
	   (a convex combination), so no 16-bit lane overflows */
	__m128i blend = x86_div255_epu16(_mm_add_epi16(_mm_mullo_epi16(f, sa),
			_mm_mullo_epi16(b, inv_sa)));
	/* alpha lane: b_a + div255((255 - b_a)*sa); == sa when b_a == 0 */
	__m128i oa = _mm_add_epi16(b,
			x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, b), sa)));
	__m128i out = _mm_or_si128(_mm_and_si128(alpha_mask, oa),
			_mm_andnot_si128(alpha_mask, blend));

	/* transparent destination: rgb lanes take the source rgb (the alpha lane
	   already holds sa); the alpha lane must not be overwritten */
	__m128i oa_b = _mm_shufflehi_epi16(_mm_shufflelo_epi16(b, 0xFF), 0xFF);
	__m128i clear = _mm_andnot_si128(alpha_mask, _mm_cmpeq_epi16(oa_b, zero));
	out = _mm_or_si128(_mm_and_si128(clear, f), _mm_andnot_si128(clear, out));

	/* effective alpha 0: leave the destination completely untouched */
	__m128i skip = _mm_cmpeq_epi16(sa, zero);
	out = _mm_or_si128(_mm_and_si128(skip, b), _mm_andnot_si128(skip, out));
	return out;
}

/* Source-over alpha blit of 4 pixels, dst 16-byte aligned (temporal stores,
   never non-temporal: NT stores commit lazily through the write-combine
   buffers of a UC-mapped scan-out buffer and show up as tearing). */
static inline void x86_blt_alpha_4px(uint32_t* dst, const uint32_t* src,
		__m128i alpha16, int a255, __m128i alpha_mask, __m128i full16, __m128i zero) {
	__m128i fg = _mm_loadu_si128((const __m128i*)src);
	__m128i bg;

	/* all four source pixels transparent: destination untouched, no read */
	if(_mm_movemask_epi8(_mm_cmpeq_epi32(_mm_srli_epi32(fg, 24), zero)) == 0xFFFF)
		return;

	bg = _mm_load_si128((const __m128i*)dst);
	{
		__m128i lo = x86_blt_alpha_half(_mm_unpacklo_epi8(fg, zero),
				_mm_unpacklo_epi8(bg, zero), alpha16, a255, alpha_mask, full16, zero);
		__m128i hi = x86_blt_alpha_half(_mm_unpackhi_epi8(fg, zero),
				_mm_unpackhi_epi8(bg, zero), alpha16, a255, alpha_mask, full16, zero);
		_mm_store_si128((__m128i*)dst, _mm_packus_epi16(lo, hi));
	}
}

/* alpha blit: 1:1 runs through 4px simd blocks with a scalar head for
   16-byte realignment and a scalar tail; scaled blits gather nearest-neighbour
   and blend per pixel (the g2d engine's scaled path, matching the other arch
   back ends). bit-identical to graph_blt_alpha_cpu for the 1:1 case. */
int32_t arch_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || alpha == 0)
		return 0;

	if(!g2d_blt_clip(src_w, src_h, &sx, &sy, &sw, &sh,
			dst_w, dst_h, &dx, &dy, &dw, &dh))
		return 0;

	if(sw == dw && sh == dh) {
		__m128i alpha16 = _mm_set1_epi16((short)alpha);
		__m128i alpha_mask = _mm_set_epi32((int)0xFFFF0000, 0, (int)0xFFFF0000, 0);
		__m128i full16 = _mm_set1_epi16(255);
		__m128i zero = _mm_setzero_si128();
		int a255 = (alpha == 0xff);

		for(int32_t row = 0; row < sh; row++) {
			const uint32_t* sp = argb_src + (sy + row) * src_w + sx;
			uint32_t* dp = argb_dst + (dy + row) * dst_w + dx;
			int32_t i = 0;

			/* scalar head realigns the destination row start to 16 bytes */
			while(i < sw && (((uintptr_t)(dp + i)) & 0x0F) != 0) {
				dp[i] = x86_blt_alpha_px(dp[i], sp[i], alpha);
				i++;
			}
			for(; i + 4 <= sw; i += 4)
				x86_blt_alpha_4px(dp + i, sp + i, alpha16, a255, alpha_mask, full16, zero);
			for(; i < sw; i++)
				dp[i] = x86_blt_alpha_px(dp[i], sp[i], alpha);
		}
		return 0;
	}

	/* scaled blit: nearest-neighbour gather + scalar blend per pixel */
	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * src_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;
		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;
			drow[col] = x86_blt_alpha_px(drow[col], srow[src_x], alpha);
		}
	}
	return 0;
}

/* -------------------------------------------------------------- scale --- */

/* vertical lerp of one destination row from two horizontally-lerped source
   rows with an 8-bit weight, truncating (matches scale.c exactly). dst row
   may be unaligned (Device-mapped framebuffer): a scalar head brings it to a
   16-byte boundary so the temporal aligned stores stay legal. */
static inline void x86_scale_vlerp_row(uint32_t* drow, const uint32_t* r0,
		const uint32_t* r1, int32_t dst_w, uint32_t w1) {
	__m128i wv0 = _mm_set1_epi16((short)(256 - w1));
	__m128i wv1 = _mm_set1_epi16((short)w1);
	__m128i zero = _mm_setzero_si128();
	int32_t j = 0;

	while(j < dst_w && (((uintptr_t)(drow + j)) & 0x0F) != 0) {
		drow[j] = x86_scale_lerp(r0[j], r1[j], w1);
		j++;
	}
	for(; j + 4 <= dst_w; j += 4) {
		__m128i a = _mm_loadu_si128((const __m128i*)(r0 + j));
		__m128i b = _mm_loadu_si128((const __m128i*)(r1 + j));
		__m128i olo = _mm_srli_epi16(_mm_add_epi16(
				_mm_mullo_epi16(_mm_unpacklo_epi8(a, zero), wv0),
				_mm_mullo_epi16(_mm_unpacklo_epi8(b, zero), wv1)), 8);
		__m128i ohi = _mm_srli_epi16(_mm_add_epi16(
				_mm_mullo_epi16(_mm_unpackhi_epi8(a, zero), wv0),
				_mm_mullo_epi16(_mm_unpackhi_epi8(b, zero), wv1)), 8);
		_mm_store_si128((__m128i*)(drow + j), _mm_packus_epi16(olo, ohi));
	}
	for(; j < dst_w; j++)
		drow[j] = x86_scale_lerp(r0[j], r1[j], w1);
}

/* fill one cache row: horizontal truncating lerp of src[x0[j]]/src[x1[j]]
   with the rounded 8-bit weight fx8[j], identical to scale.c's top/bot lerp */
static inline void x86_scale_hrow(const uint32_t* srow, const int32_t* x0,
		const int32_t* x1, const uint16_t* fx8, int32_t dst_w, uint32_t* out) {
	for(int32_t j = 0; j < dst_w; j++) {
		uint32_t a = srow[x0[j]];
		uint32_t b = srow[x1[j]];
		out[j] = (a == b) ? a : x86_scale_lerp(a, b, fx8[j]);
	}
}

/* bilinear scale of the whole src buffer into dst with explicit per-axis 16.16
   source steps, bit-identical to graph_scale_tof_cpu (rounded 8-bit weights,
   truncating packed lerps, separable horizontal-then-vertical with a 2-slot
   row cache). returns 0 on success, non-zero on malloc failure so the caller
   can fall back to the cpu reference. */
static int g2d_scale_fixed(const uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h, uint32_t inv_x, uint32_t inv_y) {
	int32_t wmax = src_w - 1;
	int32_t hmax = src_h - 1;
	int32_t* x0;
	int32_t* x1;
	uint16_t* fx8;
	uint32_t* hrow[2];
	int32_t hrow_y[2] = {-2, -2};
	uint8_t* mem;
	size_t cols = (size_t)dst_w;

	if(inv_x == (uint32_t)G2D_SCALE_FIXED_SCALE && inv_y == (uint32_t)G2D_SCALE_FIXED_SCALE &&
			dst_w == src_w && dst_h == src_h) {
		memcpy(argb_dst, argb_src, (size_t)src_w * (size_t)src_h * sizeof(uint32_t));
		return 0;
	}

	/* one allocation: x0[cols] + x1[cols] + fx8 (u16, padded) + 2 cache rows */
	{
		size_t fx8_bytes = (cols * sizeof(uint16_t) + 15u) & ~(size_t)15u;
		size_t total = cols * 2 * sizeof(int32_t) + fx8_bytes + 2 * cols * sizeof(uint32_t);
		mem = (uint8_t*)malloc(total);
		if(mem == NULL)
			return 1;
		x0 = (int32_t*)mem;
		x1 = x0 + cols;
		fx8 = (uint16_t*)((uint8_t*)x1 + cols * sizeof(int32_t));
		hrow[0] = (uint32_t*)((uint8_t*)fx8 + fx8_bytes);
		hrow[1] = hrow[0] + cols;
	}

	/* column mapping + rounded 8-bit weights, computed once */
	{
		uint32_t pos = 0;
		for(int32_t j = 0; j < dst_w; j++) {
			int32_t base = (int32_t)(pos >> G2D_SCALE_FIXED_SHIFT);
			uint32_t f = pos & G2D_SCALE_FIXED_MASK;
			if(base >= wmax) { base = wmax; f = 0; }
			x0[j] = base;
			x1[j] = (base < wmax) ? base + 1 : wmax;
			fx8[j] = (uint16_t)((f + 128) >> 8);
			pos += inv_x;
		}
	}

	{
		uint32_t sy_f = 0;
		for(int32_t oy = 0; oy < dst_h; oy++) {
			int32_t y0 = (int32_t)(sy_f >> G2D_SCALE_FIXED_SHIFT);
			uint32_t fy = sy_f & G2D_SCALE_FIXED_MASK;
			int32_t y1;
			uint32_t fy8;
			const uint32_t* r0;
			const uint32_t* r1;
			uint32_t* drow = argb_dst + (size_t)oy * dst_w;

			if(y0 >= hmax) { y0 = hmax; fy = 0; }
			y1 = (y0 < hmax) ? y0 + 1 : hmax;
			fy8 = (fy + 128) >> 8;

			/* make sure both source rows are cached; row access is monotonic,
			   so evicting the slot holding the lower row is always safe */
			for(int32_t n = 0; n < 2; n++) {
				int32_t y = (n == 0) ? y0 : y1;
				int32_t slot;
				if(hrow_y[0] == y || hrow_y[1] == y)
					continue;
				slot = (hrow_y[0] < hrow_y[1]) ? 0 : 1;
				x86_scale_hrow(argb_src + (size_t)y * src_w, x0, x1, fx8, dst_w, hrow[slot]);
				hrow_y[slot] = y;
			}

			r0 = (hrow_y[0] == y0) ? hrow[0] : hrow[1];
			r1 = (hrow_y[0] == y1) ? hrow[0] : hrow[1];

			if(fy8 == 0 || r0 == r1)
				memcpy(drow, r0, cols * sizeof(uint32_t));
			else
				x86_scale_vlerp_row(drow, r0, r1, dst_w, fy8);

			sy_f += inv_y;
		}
	}

	free(mem);
	return 0;
}

int32_t arch_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h) {
	uint32_t inv_x, inv_y;

	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return 0;
	if(src_w == dst_w && src_h == dst_h) {
		memcpy(argb_dst, argb_src, (size_t)src_w * (size_t)src_h * sizeof(uint32_t));
		return 0;
	}

	inv_x = (uint32_t)(((uint64_t)src_w << G2D_SCALE_FIXED_SHIFT) / (uint32_t)dst_w);
	inv_y = (uint32_t)(((uint64_t)src_h << G2D_SCALE_FIXED_SHIFT) / (uint32_t)dst_h);
	g2d_scale_fixed(argb_src, src_w, src_h, argb_dst, dst_w, dst_h, inv_x, inv_y);
	return 0;
}

/* scale with an explicit per-axis 16.16 source step, so graph_scale_tof_arch
   can pass FIXED_SCALE/scale and stay bit-exact with graph_scale_tof_cpu
   (whose step is derived from the scale factor, not the dst/src ratio).
   returns non-zero on malloc failure so the caller can use the cpu path. */
int32_t arch_g2d_scale_inv(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
		uint32_t inv_x, uint32_t inv_y) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return 0;
	return g2d_scale_fixed(argb_src, src_w, src_h, argb_dst, dst_w, dst_h, inv_x, inv_y);
}
#endif /* ARCH_BOOST */

int32_t arch_g2d_init(void) {
    return 0;
}

int32_t arch_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) { return 0; }

int32_t arch_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) { return 0; }

int32_t arch_g2d_gaussian(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius) { return 0; }

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
int32_t arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h) { return 0; }

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by arch_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
int32_t arch_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree) { return 0; }

#ifndef ARCH_BOOST
/* cpu back end for alpha fills: blend a solid color over a sub-rect,
   clipped to the buffer bounds, exact per-pixel access with the same
   blend math as the simd paths. alpha == 0 or an empty/fully clipped
   rect is a no-op. */
int32_t arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint8_t a;

	if(argb == NULL)
		return 0;
	a = (uint8_t)((color >> 24) & 0xff);
	if(a == 0)
		return 0;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return 0;
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;

	for(int32_t row = y; row < y + h; row++) {
		uint32_t* dp = argb + row * argb_w + x;
		for(int32_t col = 0; col < w; col++) {
			dp[col] = g2d_blend_argb_scalar(dp[col], a,
					(uint8_t)((color >> 16) & 0xff),
					(uint8_t)((color >> 8) & 0xff),
					(uint8_t)(color & 0xff));
		}
	}
	return 0;
}

/* portable alpha blit: same clipping and per-pixel math as the simd path */
int32_t arch_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || alpha == 0)
		return 0;

	if(!g2d_blt_clip(src_w, src_h, &sx, &sy, &sw, &sh,
			dst_w, dst_h, &dx, &dy, &dw, &dh))
		return 0;

	if(sw == dw && sh == dh) {
		for(int32_t row = 0; row < sh; row++) {
			const uint32_t* sp = argb_src + (sy + row) * src_w + sx;
			uint32_t* dp = argb_dst + (dy + row) * dst_w + dx;
			for(int32_t col = 0; col < sw; col++)
				dp[col] = x86_blt_alpha_px(dp[col], sp[col], alpha);
		}
		return 0;
	}

	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * src_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;
		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;
			drow[col] = x86_blt_alpha_px(drow[col], srow[src_x], alpha);
		}
	}
	return 0;
}

/* portable bilinear scale, bit-identical to graph_scale_tof_cpu */
static int g2d_scale_scalar(const uint32_t* argb_src, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h, uint32_t inv_x, uint32_t inv_y) {
	int32_t wmax = src_w - 1;
	int32_t hmax = src_h - 1;
	int32_t* x0 = (int32_t*)malloc((size_t)dst_w * sizeof(int32_t));
	int32_t* x1 = (int32_t*)malloc((size_t)dst_w * sizeof(int32_t));
	uint16_t* fx8 = (uint16_t*)malloc((size_t)dst_w * sizeof(uint16_t));

	if(inv_x == (uint32_t)G2D_SCALE_FIXED_SCALE && inv_y == (uint32_t)G2D_SCALE_FIXED_SCALE &&
			dst_w == src_w && dst_h == src_h) {
		memcpy(argb_dst, argb_src, (size_t)src_w * (size_t)src_h * sizeof(uint32_t));
		return 0;
	}
	if(x0 == NULL || x1 == NULL || fx8 == NULL) {
		free(x0); free(x1); free(fx8);
		return 1;
	}

	{
		uint32_t pos = 0;
		for(int32_t j = 0; j < dst_w; j++) {
			int32_t base = (int32_t)(pos >> G2D_SCALE_FIXED_SHIFT);
			uint32_t f = pos & G2D_SCALE_FIXED_MASK;
			if(base >= wmax) { base = wmax; f = 0; }
			x0[j] = base;
			x1[j] = (base < wmax) ? base + 1 : wmax;
			fx8[j] = (uint16_t)((f + 128) >> 8);
			pos += inv_x;
		}
	}

	{
		uint32_t sy_f = 0;
		for(int32_t oy = 0; oy < dst_h; oy++) {
			int32_t y0 = (int32_t)(sy_f >> G2D_SCALE_FIXED_SHIFT);
			uint32_t fy = sy_f & G2D_SCALE_FIXED_MASK;
			uint32_t* drow = argb_dst + (size_t)oy * dst_w;
			int32_t y1;
			uint32_t fy8;

			if(y0 >= hmax) { y0 = hmax; fy = 0; }
			y1 = (y0 < hmax) ? y0 + 1 : hmax;
			fy8 = (fy + 128) >> 8;

			{
				const uint32_t* row0 = argb_src + (size_t)y0 * src_w;
				const uint32_t* row1 = argb_src + (size_t)y1 * src_w;
				for(int32_t j = 0; j < dst_w; j++) {
					uint32_t top = x86_scale_lerp(row0[x0[j]], row0[x1[j]], fx8[j]);
					uint32_t bot = x86_scale_lerp(row1[x0[j]], row1[x1[j]], fx8[j]);
					drow[j] = x86_scale_lerp(top, bot, fy8);
				}
			}
			sy_f += inv_y;
		}
	}

	free(x0); free(x1); free(fx8);
	return 0;
}

int32_t arch_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h) {
	uint32_t inv_x, inv_y;
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return 0;
	inv_x = (uint32_t)(((uint64_t)src_w << G2D_SCALE_FIXED_SHIFT) / (uint32_t)dst_w);
	inv_y = (uint32_t)(((uint64_t)src_h << G2D_SCALE_FIXED_SHIFT) / (uint32_t)dst_h);
	g2d_scale_scalar(argb_src, src_w, src_h, argb_dst, dst_w, dst_h, inv_x, inv_y);
	return 0;
}

int32_t arch_g2d_scale_inv(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
		uint32_t inv_x, uint32_t inv_y) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return 0;
	return g2d_scale_scalar(argb_src, src_w, src_h, argb_dst, dst_w, dst_h, inv_x, inv_y);
}
#endif /* !ARCH_BOOST */
