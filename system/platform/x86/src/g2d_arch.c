#include <stdint.h>
#include <string.h>
#include <ewoksys/ewokdef.h>

#ifdef ARCH_BOOST
#include <emmintrin.h>
#endif

/* scalar per-pixel blend, same math as the C reference: effective alpha
   (src_a * alpha) >> 8 is applied by the callers, then the /255 blend. */
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
   with bg_a + div255((255-bg_a)*a) via alpha_mask. dst must be 16-byte
   aligned. */
static inline void x86_blend_4px(uint32_t* dst, __m128i cpa16, __m128i a16,
		__m128i inv_a16, __m128i alpha_mask, __m128i full16, __m128i zero) {
	__m128i bg = _mm_load_si128((const __m128i*)dst);
	__m128i lo = _mm_unpacklo_epi8(bg, zero);
	__m128i hi = _mm_unpackhi_epi8(bg, zero);

	__m128i blend_lo = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(lo, inv_a16)));
	__m128i blend_hi = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(hi, inv_a16)));

	__m128i oa_lo = _mm_add_epi16(lo, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, lo), a16)));
	__m128i oa_hi = _mm_add_epi16(hi, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, hi), a16)));

	lo = _mm_or_si128(_mm_and_si128(alpha_mask, oa_lo), _mm_andnot_si128(alpha_mask, blend_lo));
	hi = _mm_or_si128(_mm_and_si128(alpha_mask, oa_hi), _mm_andnot_si128(alpha_mask, blend_hi));

	_mm_store_si128((__m128i*)dst, _mm_packus_epi16(lo, hi));
}

/* alpha fill: simd rows of 4 pixels with a scalar head for 16-byte
   realignment and a scalar tail; the fill color is constant so the
   per-channel color*a constants are set up once per call. */
void arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint32_t a;

	if(argb == NULL)
		return;
	a = (color >> 24) & 0xff;
	if(a == 0)
		return;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return;
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

		for(int32_t row = y; row < y + h; row++) {
			uint32_t* dp = argb + row * argb_w + x;
			int32_t i = 0;

			/* scalar head realigns the row start to 16 bytes */
			while(i < w && (((uintptr_t)(dp + i)) & 0x0F) != 0) {
				dp[i] = x86_fill_blend_px(dp[i], color, a);
				i++;
			}

			for(; i + 4 <= w; i += 4)
				x86_blend_4px(dp + i, cpa16, a16, inv_a16, alpha_mask, full16, zero);

			/* scalar tail, at most 3 pixels per row */
			for(; i < w; i++)
				dp[i] = x86_fill_blend_px(dp[i], color, a);
		}
	}
}
#endif /* ARCH_BOOST */

int32_t arch_g2d_init(void) {
    return 0;
}

void  arch_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {}

void  arch_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) {}

void  arch_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {}

void  arch_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h) {}

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
void  arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h) {}

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by arch_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
void  arch_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree) {}

/* cpu back end for sub-alignment tails and narrow copies: scalar 1:1
   copy or blend with exact per-pixel access, no alignment, contiguity
   or simd requirements. the 1:1 rect is clipped against both buffer
   bounds first (callers hand in unclipped window rects: a row that
   overruns the right edge would otherwise wrap into the left edge of
   the next row). use_alpha == 0 is a plain copy; otherwise the same
   math as the simd paths (effective alpha (src_a * alpha) >> 8, then
   the /255 blend). */
void arch_g2d_blt_cpu(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, uint8_t use_alpha, uint8_t alpha) {
	if(argb_src == NULL || argb_dst == NULL || sw <= 0 || sh <= 0)
		return;
	if(use_alpha != 0 && alpha == 0)
		return;

	/* 1:1 mapping: cutting one side shifts the other surface's origin by
	   the same delta, cutting right/bottom just shrinks the size */
	if(dx < 0) { sx -= dx; sw += dx; dx = 0; }
	if(dy < 0) { sy -= dy; sh += dy; dy = 0; }
	if(sx < 0) { dx -= sx; sw += sx; sx = 0; }
	if(sy < 0) { dy -= sy; sh += sy; sy = 0; }
	if(sx + sw > src_w) sw = src_w - sx;
	if(sy + sh > src_h) sh = src_h - sy;
	if(dx + sw > dst_w) sw = dst_w - dx;
	if(dy + sh > dst_h) sh = dst_h - dy;
	if(sw <= 0 || sh <= 0)
		return;

	for(int32_t row = 0; row < sh; row++) {
		const uint32_t* sp = argb_src + (sy + row) * src_w + sx;
		uint32_t* dp = argb_dst + (dy + row) * dst_w + dx;

		if(use_alpha == 0) {
			memcpy(dp, sp, (size_t)sw * sizeof(uint32_t));
			continue;
		}
		for(int32_t col = 0; col < sw; col++) {
			uint32_t color = sp[col];
			uint32_t src_a = (color >> 24) & 0xff;
			uint8_t sa;

			if(src_a == 0)
				continue;
			sa = (uint8_t)((alpha == 0xff) ? src_a : (src_a * alpha) >> 8);
			if(sa == 0)
				continue;
			if(sa == 0xff) {
				dp[col] = color;
				continue;
			}
			dp[col] = g2d_blend_argb_scalar(dp[col], sa,
					(uint8_t)((color >> 16) & 0xff),
					(uint8_t)((color >> 8) & 0xff),
					(uint8_t)(color & 0xff));
		}
	}
}

#ifndef ARCH_BOOST
/* cpu back end for alpha fills: blend a solid color over a sub-rect,
   clipped to the buffer bounds, exact per-pixel access with the same
   blend math as the simd paths. alpha == 0 or an empty/fully clipped
   rect is a no-op. */
void arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint8_t a;

	if(argb == NULL)
		return;
	a = (uint8_t)((color >> 24) & 0xff);
	if(a == 0)
		return;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return;
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
}
#endif /* !ARCH_BOOST */
