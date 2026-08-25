#include <bsp/bsp_g2d.h>
#include <string.h>
#include <stdbool.h>

/* virt platform has no 2D hardware engine, all operations are done by CPU. */

/* software backend has nothing to set up, the hook exists so platforms
   with a 2D hardware engine can prepare it before first use. */
int32_t bsp_g2d_init(void) {
	return 0;
}

/* fixed point trig: table[i] = round(sin(i degree) * 16384), avoids a
   libm dependency for the bsp library. */
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

void  bsp_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
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


static inline uint32_t blend_argb(uint32_t dst_color, uint8_t a,
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

/* clip rect against buffer bounds [0, bw) x [0, bh) */
static inline void rect_clip(int32_t bw, int32_t bh,
		int32_t* x, int32_t* y, int32_t* w, int32_t* h) {
	if(*x < 0) { *w += *x; *x = 0; }
	if(*y < 0) { *h += *y; *y = 0; }
	if(*x + *w > bw) *w = bw - *x;
	if(*y + *h > bh) *h = bh - *y;
	if(*w < 0) *w = 0;
	if(*h < 0) *h = 0;
}

void  bsp_g2d_fill(uint32_t* argb, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(argb == NULL || argb_w <= 0 || argb_h <= 0 || w <= 0 || h <= 0)
		return;

	rect_clip(argb_w, argb_h, &x, &y, &w, &h);
	if(w == 0 || h == 0)
		return;

	uint8_t cb = color & 0xff;
	bool same_bytes = ((color >> 8) & 0xff) == cb &&
		((color >> 16) & 0xff) == cb && ((color >> 24) & 0xff) == cb;

	for(int32_t row = 0; row < h; row++) {
		uint32_t* p = argb + (y + row) * argb_w + x;
		if(same_bytes) {
			memset(p, cb, (size_t)w * sizeof(uint32_t));
		} else {
			for(int32_t col = 0; col < w; col++)
				p[col] = color;
		}
	}
}

/* clip src/dst pair against their buffers, keeping the mapping aligned.
   returns false when nothing is left to draw. */
static bool blt_clip(uint32_t* argb_src, int32_t src_w, int32_t src_h,
		int32_t* sx, int32_t* sy, int32_t* sw, int32_t* sh,
		uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
		int32_t* dx, int32_t* dy, int32_t* dw, int32_t* dh) {
	(void)argb_src;
	(void)argb_dst;

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
		return false;

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
	if(*sw <= 0 || *sh <= 0 || *dw <= 0 || *dh <= 0)
		return false;
	return true;
}

void  bsp_g2d_blt(uint32_t* argb_src, int32_t argb_w, int32_t argb_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(argb_src == NULL || argb_dst == NULL ||
			argb_w <= 0 || argb_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	if(!blt_clip(argb_src, argb_w, argb_h, &sx, &sy, &sw, &sh,
			argb_dst, dst_w, dst_h, &dx, &dy, &dw, &dh))
		return;

	/* 1:1 row copy (overlap-safe) */
	if(sw == dw && sh == dh) {
		size_t row_bytes = (size_t)sw * sizeof(uint32_t);
		bool backward = (argb_src == argb_dst) &&
			(dy > sy || (dy == sy && dx > sx));
		for(int32_t row = 0; row < sh; row++) {
			int32_t r = backward ? (sh - 1 - row) : row;
			memmove(argb_dst + (dy + r) * dst_w + dx,
				argb_src + (sy + r) * argb_w + sx, row_bytes);
		}
		return;
	}

	/* nearest-neighbor scaling */
	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * argb_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;
		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;
			drow[col] = srow[src_x];
		}
	}
}

void  bsp_g2d_blt_alpha(uint32_t* argb_src, int32_t argb_w, int32_t argb_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(argb_src == NULL || argb_dst == NULL ||
			argb_w <= 0 || argb_h <= 0 || dst_w <= 0 || dst_h <= 0 ||
			sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || alpha == 0)
		return;

	if(!blt_clip(argb_src, argb_w, argb_h, &sx, &sy, &sw, &sh,
			argb_dst, dst_w, dst_h, &dx, &dy, &dw, &dh))
		return;

	for(int32_t row = 0; row < dh; row++) {
		int32_t src_y = sy + (int32_t)((int64_t)row * sh / dh);
		if(src_y >= sy + sh) src_y = sy + sh - 1;
		const uint32_t* srow = argb_src + src_y * argb_w;
		uint32_t* drow = argb_dst + (dy + row) * dst_w + dx;
		for(int32_t col = 0; col < dw; col++) {
			int32_t src_x = (sw == dw) ? (sx + col) :
				sx + (int32_t)((int64_t)col * sw / dw);
			if(src_x >= sx + sw) src_x = sx + sw - 1;

			uint32_t color = srow[src_x];
			uint8_t src_a = (color >> 24) & 0xff;
			if(src_a == 0)
				continue;
			if(alpha == 0xff && src_a == 0xff) {
				drow[col] = color;
				continue;
			}

			uint8_t sa = (uint8_t)((src_a * alpha) >> 8);
			if(sa == 0)
				continue;

			drow[col] = blend_argb(drow[col], sa,
				(color >> 16) & 0xff,
				(color >> 8) & 0xff,
				color & 0xff);
		}
	}
}

void  bsp_g2d_scale_to(uint32_t* argb_src, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, int32_t dst_w, int32_t dst_h) {
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return;

	if(src_w == dst_w && src_h == dst_h) {
		memcpy(argb_dst, argb_src, (size_t)src_w * src_h * sizeof(uint32_t));
		return;
	}

	/* nearest-neighbor scaling of the whole surface */
	for(int32_t row = 0; row < dst_h; row++) {
		int32_t src_y = (int32_t)((int64_t)row * src_h / dst_h);
		if(src_y >= src_h) src_y = src_h - 1;
		const uint32_t* srow = argb_src + src_y * src_w;
		uint32_t* drow = argb_dst + row * dst_w;
		for(int32_t col = 0; col < dst_w; col++) {
			int32_t src_x = (int32_t)((int64_t)col * src_w / dst_w);
			if(src_x >= src_w) src_x = src_w - 1;
			drow[col] = srow[src_x];
		}
	}
}

void  bsp_g2d_rotate(uint32_t* argb_src, int32_t src_w, int32_t src_h,
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
			memcpy(argb_dst, argb_src, (size_t)src_w * src_h * sizeof(uint32_t));
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
		for(int32_t y = 0; y < src_h; y++) {
			const uint32_t* srow = argb_src + (src_h - 1 - y) * src_w;
			uint32_t* drow = argb_dst + y * dst_w;
			for(int32_t x = 0; x < src_w; x++)
				drow[x] = srow[src_w - 1 - x];
		}
		return;
	}

	if(degree == 90 || degree == 270) {
		/* 90/270 change surface dimensions, in-place is not supported */
		if(argb_src == argb_dst || dst_w < src_h || dst_h < src_w)
			return;

		if(degree == 90) {
			/* dst is src_h x src_w, dst[y][x] = src[src_h-1-x][y] */
			for(int32_t y = 0; y < src_w; y++) {
				uint32_t* drow = argb_dst + y * dst_w;
				for(int32_t x = 0; x < src_h; x++)
					drow[x] = argb_src[(src_h - 1 - x) * src_w + y];
			}
		}
		else { /* 270: dst[y][x] = src[x][src_w-1-y] */
			for(int32_t y = 0; y < src_w; y++) {
				uint32_t* drow = argb_dst + y * dst_w;
				int32_t src_col = src_w - 1 - y;
				for(int32_t x = 0; x < src_h; x++)
					drow[x] = argb_src[x * src_w + src_col];
			}
		}
		return;
	}

	/* arbitrary angle: inverse-mapped nearest neighbor, rotation around
	   the center, content written into the top-left bw x bh bounding box. */
	if(argb_src == argb_dst)
		return;
	bsp_g2d_rotated_size(src_w, src_h, degree, &bw, &bh);
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

