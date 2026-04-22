#include <graph/graph.h>
#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

bool grect_insect(const grect_t* src, grect_t* dst) {
	//insect src;
	if(dst->x >= (int32_t)(src->x+src->w))
		dst->w = 0;
	if(dst->y >= (int32_t)(src->y+src->h))
		dst->h = 0;
	
	if(dst->w == 0 || dst->h == 0)
		return false;

	int32_t rx, ry;  //chehck w, h
	rx = dst->x + dst->w;
	ry = dst->y + dst->h;
	if(rx >= (int32_t)(src->x+src->w))
		dst->w -= (rx - (src->x+src->w));
	if(ry >= (int32_t)(src->y+src->h))
		dst->h -= (ry - (src->y+src->h));

	if(dst->x < src->x) {
		dst->w -= (src->x - dst->x);
		dst->x = src->x;
	}

	if(dst->y < src->y) {
		dst->h -= (src->y - dst->y);
		dst->y = src->y;
	}

	if(dst->w < 0)
		dst->w = 0;
	if(dst->h < 0)
		dst->h = 0;
	if(dst->w == 0 || dst->h == 0)
		return false;
	return true;
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/
inline bool graph_insect(graph_t* g, grect_t* r) {
	grect_t gr = {0, 0, g->w, g->h};
	return grect_insect(&gr, r);
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/

bool graph_insect_with(graph_t* src, grect_t* sr, graph_t* dst, grect_t* dr) {
	int32_t dx = sr->x < dr->x ? sr->x:dr->x;
	int32_t dy = sr->y < dr->y ? sr->y:dr->y;

	if(dx < 0) {
		sr->x -= dx;
		dr->x -= dx;
		sr->w += dx;
		dr->w += dx;
	}

	if(dy < 0) {
		sr->y -= dy;
		dr->y -= dy;
		sr->h += dy;
		dr->h += dy;
	}

	//insect src;
	if(!graph_insect(src, sr))
		return false;
	if(!graph_insect(dst, dr))
		return false;

	if(sr->w <= 0 || sr->h <= 0 ||
			dr->w <= 0 || dr->h <= 0)
		return false;

	if(sr->w > dr->w)
		sr->w = dr->w;
	else
		dr->w = sr->w;
	
	if(sr->h > dr->h)
		sr->h = dr->h;
	else
		dr->h = sr->h;
	return true;
}

void graph_fill_cpu(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
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

	if(color_a(color) == 0xff) {
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x++) {
				graph_set_pixel(g, x, y, color);
			}
		}
	}
	else {
		register uint8_t ca = (color >> 24) & 0xff;
		register uint8_t cr = (color >> 16) & 0xff;
		register uint8_t cg = (color >> 8) & 0xff;
		register uint8_t cb = (color) & 0xff;
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x++) {
				graph_pixel_argb_raw(g, x, y, ca, cr, cg, cb);
			}
		}
	}
}

void graph_set(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
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

	for(; y < ey; y++) {
		x = r.x;
		for(; x < ex; x++) {
			g->buffer[y*g->w+x] = color;
		}
	}
}

inline void graph_fill_rect(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
#ifdef BSP_BOOST
		graph_fill_bsp(g, x, y, w, h, color);
#else
		graph_fill_cpu(g, x, y, w, h, color);
#endif
}

void graph_blt_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	if(sx == 0 && sy == 0 && dx == 0 && dy == 0 &&
			sw == dw && sh == dh && src->w == sw && src->h == sh &&
				dst->w == dw && dst->h == dh) {
		memcpy(dst->buffer, src->buffer, 4*sw*sh);	
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

	int32_t src_w = src->w;
	int32_t src_h = src->h;
	float scale_x = (float)sr.w / (float)dr.w;
	float scale_y = (float)sr.h / (float)dr.h;
	int is_downscale = (scale_x > 1.0f || scale_y > 1.0f);
	int32_t ey = dr.y + dr.h;
	int32_t ex = dr.x + dr.w;

	if(!is_downscale && scale_x <= 1.0f && scale_y <= 1.0f && scale_x > 0.9f && scale_y > 0.9f) {
		int32_t y = dr.y;
		for(; y < ey; y++) {
			int32_t src_y = (int32_t)((y - dr.y) * scale_y + sr.y);
			if(src_y >= src_h) src_y = src_h - 1;
			int32_t dst_offset_y = y * dst->w;
			int32_t x = dr.x;
			int32_t src_offset = src_y * src_w;
			for(; x < ex; x++) {
				int32_t src_x = (int32_t)((x - dr.x) * scale_x + sr.x);
				if(src_x >= sr.w + sr.x) src_x = sr.w + sr.x - 1;
				dst->buffer[dst_offset_y + x] = src->buffer[src_offset + src_x];
			}
		}
	} else {
		if(!is_downscale) {
			for(int32_t y = dr.y; y < ey; y++) {
				float src_fy = (y - dr.y) * scale_y + sr.y;
				int32_t src_y0 = (int32_t)src_fy;
				int32_t src_y1 = src_y0 + 1;
				if(src_y1 >= sr.y + sr.h) src_y1 = src_y0;
				float ty = src_fy - src_y0;
				int32_t dst_offset_y = y * dst->w;

				for(int32_t x = dr.x; x < ex; x++) {
					float src_fx = (x - dr.x) * scale_x + sr.x;
					int32_t src_x0 = (int32_t)src_fx;
					int32_t src_x1 = src_x0 + 1;
					if(src_x1 >= sr.x + sr.w) src_x1 = src_x0;
					float tx = src_fx - src_x0;

					uint32_t p00 = src->buffer[src_y0 * src_w + src_x0];
					uint32_t p10 = src->buffer[src_y0 * src_w + src_x1];
					uint32_t p01 = src->buffer[src_y1 * src_w + src_x0];
					uint32_t p11 = src->buffer[src_y1 * src_w + src_x1];

					float a00 = (1.0f - tx) * (1.0f - ty);
					float a10 = tx * (1.0f - ty);
					float a01 = (1.0f - tx) * ty;
					float a11 = tx * ty;

					uint8_t r = (uint8_t)(((p00 >> 16) & 0xff) * a00 + ((p10 >> 16) & 0xff) * a10 + ((p01 >> 16) & 0xff) * a01 + ((p11 >> 16) & 0xff) * a11);
					uint8_t g = (uint8_t)(((p00 >> 8) & 0xff) * a00 + ((p10 >> 8) & 0xff) * a10 + ((p01 >> 8) & 0xff) * a01 + ((p11 >> 8) & 0xff) * a11);
					uint8_t b = (uint8_t)((p00 & 0xff) * a00 + (p10 & 0xff) * a10 + (p01 & 0xff) * a01 + (p11 & 0xff) * a11);
					uint8_t a = (uint8_t)(((p00 >> 24) & 0xff) * a00 + ((p10 >> 24) & 0xff) * a10 + ((p01 >> 24) & 0xff) * a01 + ((p11 >> 24) & 0xff) * a11);

					dst->buffer[dst_offset_y + x] = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}
		} else {
			for(int32_t y = dr.y; y < ey; y++) {
				float src_start_y = (y - dr.y) * scale_y + sr.y;
				float src_end_y = (y - dr.y + 1) * scale_y + sr.y;
				if(src_start_y < sr.y) src_start_y = sr.y;
				if(src_end_y > sr.y + sr.h) src_end_y = sr.y + sr.h;
				int32_t src_y_start = (int32_t)src_start_y;
				int32_t src_y_end = (int32_t)src_end_y;
				if(src_y_end >= sr.y + sr.h) src_y_end = sr.y + sr.h - 1;
				int32_t dst_offset_y = y * dst->w;

				for(int32_t x = dr.x; x < ex; x++) {
					float src_start_x = (x - dr.x) * scale_x + sr.x;
					float src_end_x = (x - dr.x + 1) * scale_x + sr.x;
					if(src_start_x < sr.x) src_start_x = sr.x;
					if(src_end_x > sr.x + sr.w) src_end_x = sr.x + sr.w;
					int32_t src_x_start = (int32_t)src_start_x;
					int32_t src_x_end = (int32_t)src_end_x;
					if(src_x_end >= sr.x + sr.w) src_x_end = sr.x + sr.w - 1;

					uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
					uint32_t count = 0;

					for(int32_t src_y = src_y_start; src_y <= src_y_end; src_y++) {
						int32_t row_offset = src_y * src_w;
						for(int32_t src_x = src_x_start; src_x <= src_x_end; src_x++) {
							uint32_t p = src->buffer[row_offset + src_x];
							sum_r += (p >> 16) & 0xff;
							sum_g += (p >> 8) & 0xff;
							sum_b += p & 0xff;
							sum_a += (p >> 24) & 0xff;
							count++;
						}
					}

					if(count > 0) {
						uint8_t r = (uint8_t)(sum_r / count);
						uint8_t g = (uint8_t)(sum_g / count);
						uint8_t b = (uint8_t)(sum_b / count);
						uint8_t a = (uint8_t)(sum_a / count);
						dst->buffer[dst_offset_y + x] = (a << 24) | (r << 16) | (g << 8) | b;
					}
				}
			}
		}
	}
}

inline void graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
#ifdef BSP_BOOST
		graph_blt_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#else
		graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#endif
}

void graph_blt_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
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

	int32_t src_w = src->w;
	int32_t src_h = src->h;
	float scale_x = (float)sr.w / (float)dr.w;
	float scale_y = (float)sr.h / (float)dr.h;
	int is_downscale = (scale_x > 1.0f || scale_y > 1.0f);
	float alpha_f = (float)alpha / 255.0f;
	int32_t ey = dr.y + dr.h;
	int32_t ex = dr.x + dr.w;

	if(!is_downscale && scale_x <= 1.0f && scale_y <= 1.0f && scale_x > 0.9f && scale_y > 0.9f) {
		for(int32_t y = dr.y; y < ey; y++) {
			int32_t src_y = (int32_t)((y - dr.y) * scale_y + sr.y);
			if(src_y >= src_h) src_y = src_h - 1;
			int32_t src_offset = src_y * src_w;
			for(int32_t x = dr.x; x < ex; x++) {
				int32_t src_x = (int32_t)((x - dr.x) * scale_x + sr.x);
				if(src_x >= sr.x + sr.w) src_x = sr.x + sr.w - 1;
				uint32_t color = src->buffer[src_offset + src_x];
				uint8_t sa = (uint8_t)(((color >> 24) & 0xff) * alpha >> 8);
				if(sa > 0)
					graph_pixel_argb_raw(dst, x, y, sa, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
			}
		}
	} else {
		if(!is_downscale) {
			for(int32_t y = dr.y; y < ey; y++) {
				float src_fy = (y - dr.y) * scale_y + sr.y;
				int32_t src_y0 = (int32_t)src_fy;
				int32_t src_y1 = src_y0 + 1;
				if(src_y1 >= sr.y + sr.h) src_y1 = src_y0;
				float ty = src_fy - src_y0;

				for(int32_t x = dr.x; x < ex; x++) {
					float src_fx = (x - dr.x) * scale_x + sr.x;
					int32_t src_x0 = (int32_t)src_fx;
					int32_t src_x1 = src_x0 + 1;
					if(src_x1 >= sr.x + sr.w) src_x1 = src_x0;
					float tx = src_fx - src_x0;

					uint32_t p00 = src->buffer[src_y0 * src_w + src_x0];
					uint32_t p10 = src->buffer[src_y0 * src_w + src_x1];
					uint32_t p01 = src->buffer[src_y1 * src_w + src_x0];
					uint32_t p11 = src->buffer[src_y1 * src_w + src_x1];

					float a00 = (1.0f - tx) * (1.0f - ty);
					float a10 = tx * (1.0f - ty);
					float a01 = (1.0f - tx) * ty;
					float a11 = tx * ty;

					uint8_t r = (uint8_t)(((p00 >> 16) & 0xff) * a00 + ((p10 >> 16) & 0xff) * a10 + ((p01 >> 16) & 0xff) * a01 + ((p11 >> 16) & 0xff) * a11);
					uint8_t g = (uint8_t)(((p00 >> 8) & 0xff) * a00 + ((p10 >> 8) & 0xff) * a10 + ((p01 >> 8) & 0xff) * a01 + ((p11 >> 8) & 0xff) * a11);
					uint8_t b = (uint8_t)((p00 & 0xff) * a00 + (p10 & 0xff) * a10 + (p01 & 0xff) * a01 + (p11 & 0xff) * a11);
					uint8_t a = (uint8_t)(((p00 >> 24) & 0xff) * a00 + ((p10 >> 24) & 0xff) * a10 + ((p01 >> 24) & 0xff) * a01 + ((p11 >> 24) & 0xff) * a11);

					uint8_t sa = (uint8_t)((float)a * alpha_f);
					if(sa > 0)
						graph_pixel_argb_raw(dst, x, y, sa, r, g, b);
				}
			}
		} else {
			for(int32_t y = dr.y; y < ey; y++) {
				float src_start_y = (y - dr.y) * scale_y + sr.y;
				float src_end_y = (y - dr.y + 1) * scale_y + sr.y;
				if(src_start_y < sr.y) src_start_y = sr.y;
				if(src_end_y > sr.y + sr.h) src_end_y = sr.y + sr.h;
				int32_t src_y_start = (int32_t)src_start_y;
				int32_t src_y_end = (int32_t)src_end_y;
				if(src_y_end >= sr.y + sr.h) src_y_end = sr.y + sr.h - 1;

				for(int32_t x = dr.x; x < ex; x++) {
					float src_start_x = (x - dr.x) * scale_x + sr.x;
					float src_end_x = (x - dr.x + 1) * scale_x + sr.x;
					if(src_start_x < sr.x) src_start_x = sr.x;
					if(src_end_x > sr.x + sr.w) src_end_x = sr.x + sr.w;
					int32_t src_x_start = (int32_t)src_start_x;
					int32_t src_x_end = (int32_t)src_end_x;
					if(src_x_end >= sr.x + sr.w) src_x_end = sr.x + sr.w - 1;

					uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
					uint32_t count = 0;

					for(int32_t src_y = src_y_start; src_y <= src_y_end; src_y++) {
						int32_t row_offset = src_y * src_w;
						for(int32_t src_x = src_x_start; src_x <= src_x_end; src_x++) {
							uint32_t p = src->buffer[row_offset + src_x];
							sum_r += (p >> 16) & 0xff;
							sum_g += (p >> 8) & 0xff;
							sum_b += p & 0xff;
							sum_a += (p >> 24) & 0xff;
							count++;
						}
					}

					if(count > 0) {
						uint8_t r = (uint8_t)(sum_r / count);
						uint8_t g = (uint8_t)(sum_g / count);
						uint8_t b = (uint8_t)(sum_b / count);
						uint8_t a = (uint8_t)((float)(sum_a / count) * alpha_f);
						if(a > 0)
							graph_pixel_argb_raw(dst, x, y, a, r, g, b);
					}
				}
			}
		}
	}
}

inline void graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
#ifdef BSP_BOOST
	graph_blt_alpha_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
#else
	graph_blt_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
#endif
}

void graph_blt_mask_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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

	for(; sy < ey; sy++, dy++) {
		register int32_t sx2 = sr.x;
		register int32_t dx2 = dr.x;
		register int32_t src_offset = sy * src->w;
		register int32_t dst_offset = dy * dst->w;
		for(; sx2 < ex; sx2++, dx2++) {
			register uint32_t src_color = src->buffer[src_offset + sx2];
			register uint32_t dst_color = dst->buffer[dst_offset + dx2];
			register uint8_t src_a = (src_color >> 24) & 0xff;
			register uint8_t dst_a = (dst_color >> 24) & 0xff;
			
			// Only apply mask if src alpha > 0
			if (src_a != 0) {
				if(dst_a > src_a)
					dst->buffer[dst_offset + dx2] = (src_a << 24) | (dst_color & 0xffffff);
			}
			else 
				dst->buffer[dst_offset + dx2] = 0;
		}
	}
}

inline void graph_blt_alpha_mask(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
#ifdef BSP_BOOST
	graph_blt_alpha_mask_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#else
	graph_blt_mask_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#endif
}

void graph_blt_fit_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {

	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	grect_t dr = {dx, dy, dw, dh};
	graph_insect(dst, &dr);
	if(dst->clip.w > 0 && dst->clip.h > 0)
		grect_insect(&dst->clip, &dr);

	if(dr.w <= 0 || dr.h <= 0)
		return;

	float scale_x = (float)sw / (float)dw;
	float scale_y = (float)sh / (float)dh;
	int32_t ey = dr.y + dr.h;
	int32_t ex = dr.x + dr.w;
	int src_w = src->w;

	int is_downscale = (scale_x > 1.0f || scale_y > 1.0f);

	if(!is_downscale) {
		for(int32_t y = dr.y; y < ey; y++) {
			float src_fy = (y - dr.y) * scale_y + sy;
			int32_t src_y0 = (int32_t)src_fy;
			int32_t src_y1 = src_y0 + 1;
			if(src_y1 >= sy + sh) src_y1 = src_y0;
			float ty = src_fy - src_y0;
			int32_t dst_offset_y = y * dst->w;

			for(int32_t x = dr.x; x < ex; x++) {
				float src_fx = (x - dr.x) * scale_x + sx;
				int32_t src_x0 = (int32_t)src_fx;
				int32_t src_x1 = src_x0 + 1;
				if(src_x1 >= sx + sw) src_x1 = src_x0;
				float tx = src_fx - src_x0;

				uint32_t p00 = src->buffer[src_y0 * src_w + src_x0];
				uint32_t p10 = src->buffer[src_y0 * src_w + src_x1];
				uint32_t p01 = src->buffer[src_y1 * src_w + src_x0];
				uint32_t p11 = src->buffer[src_y1 * src_w + src_x1];

				float a00 = (1.0f - tx) * (1.0f - ty);
				float a10 = tx * (1.0f - ty);
				float a01 = (1.0f - tx) * ty;
				float a11 = tx * ty;

				uint8_t r = (uint8_t)(
					((p00 >> 16) & 0xff) * a00 +
					((p10 >> 16) & 0xff) * a10 +
					((p01 >> 16) & 0xff) * a01 +
					((p11 >> 16) & 0xff) * a11
				);
				uint8_t g = (uint8_t)(
					((p00 >> 8) & 0xff) * a00 +
					((p10 >> 8) & 0xff) * a10 +
					((p01 >> 8) & 0xff) * a01 +
					((p11 >> 8) & 0xff) * a11
				);
				uint8_t b = (uint8_t)(
					(p00 & 0xff) * a00 +
					(p10 & 0xff) * a10 +
					(p01 & 0xff) * a01 +
					(p11 & 0xff) * a11
				);
				uint8_t a = (uint8_t)(
					((p00 >> 24) & 0xff) * a00 +
					((p10 >> 24) & 0xff) * a10 +
					((p01 >> 24) & 0xff) * a01 +
					((p11 >> 24) & 0xff) * a11
				);

				dst->buffer[dst_offset_y + x] = (a << 24) | (r << 16) | (g << 8) | b;
			}
		}
	} else {
		for(int32_t y = dr.y; y < ey; y++) {
			float src_start_y = (y - dr.y) * scale_y + sy;
			float src_end_y = (y - dr.y + 1) * scale_y + sy;
			if(src_start_y < sy) src_start_y = sy;
			if(src_end_y > sy + sh) src_end_y = sy + sh;
			int32_t src_y_start = (int32_t)src_start_y;
			int32_t src_y_end = (int32_t)src_end_y;
			if(src_y_end >= sy + sh) src_y_end = sy + sh - 1;

			int32_t dst_offset_y = y * dst->w;

			for(int32_t x = dr.x; x < ex; x++) {
				float src_start_x = (x - dr.x) * scale_x + sx;
				float src_end_x = (x - dr.x + 1) * scale_x + sx;
				if(src_start_x < sx) src_start_x = sx;
				if(src_end_x > sx + sw) src_end_x = sx + sw;
				int32_t src_x_start = (int32_t)src_start_x;
				int32_t src_x_end = (int32_t)src_end_x;
				if(src_x_end >= sx + sw) src_x_end = sx + sw - 1;

				uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
				uint32_t count = 0;

				for(int32_t src_y = src_y_start; src_y <= src_y_end; src_y++) {
					int32_t row_offset = src_y * src_w;
					for(int32_t src_x = src_x_start; src_x <= src_x_end; src_x++) {
						uint32_t p = src->buffer[row_offset + src_x];
						sum_r += (p >> 16) & 0xff;
						sum_g += (p >> 8) & 0xff;
						sum_b += p & 0xff;
						sum_a += (p >> 24) & 0xff;
						count++;
					}
				}

				if(count > 0) {
					uint8_t r = (uint8_t)(sum_r / count);
					uint8_t g = (uint8_t)(sum_g / count);
					uint8_t b = (uint8_t)(sum_b / count);
					uint8_t a = (uint8_t)(sum_a / count);
					dst->buffer[dst_offset_y + x] = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}
		}
	}
}

inline void graph_blt_fit(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
#ifdef BSP_BOOST
		//graph_blt_fit_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
		graph_blt_fit_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#else
		graph_blt_fit_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
#endif
}

void graph_blt_fit_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {

	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || alpha == 0)
		return;

	grect_t dr = {dx, dy, dw, dh};
	graph_insect(dst, &dr);
	if(dst->clip.w > 0 && dst->clip.h > 0)
		grect_insect(&dst->clip, &dr);

	if(dr.w <= 0 || dr.h <= 0)
		return;

	float scale_x = (float)sw / (float)dw;
	float scale_y = (float)sh / (float)dh;

	int32_t ey = dr.y + dr.h;
	int32_t ex = dr.x + dr.w;
	int src_w = src->w;
	float alpha_f = (float)alpha / 255.0f;

	int is_downscale = (scale_x > 1.0f || scale_y > 1.0f);

	if(!is_downscale) {
		for(int32_t y = dr.y; y < ey; y++) {
			float src_fy = (y - dr.y) * scale_y + sy;
			int32_t src_y0 = (int32_t)src_fy;
			int32_t src_y1 = src_y0 + 1;
			if(src_y1 >= sy + sh) src_y1 = src_y0;
			float ty = src_fy - src_y0;
			int32_t dst_offset_y = y * dst->w;

			for(int32_t x = dr.x; x < ex; x++) {
				float src_fx = (x - dr.x) * scale_x + sx;
				int32_t src_x0 = (int32_t)src_fx;
				int32_t src_x1 = src_x0 + 1;
				if(src_x1 >= sx + sw) src_x1 = src_x0;
				float tx = src_fx - src_x0;

				uint32_t p00 = src->buffer[src_y0 * src_w + src_x0];
				uint32_t p10 = src->buffer[src_y0 * src_w + src_x1];
				uint32_t p01 = src->buffer[src_y1 * src_w + src_x0];
				uint32_t p11 = src->buffer[src_y1 * src_w + src_x1];

				float a00 = (1.0f - tx) * (1.0f - ty);
				float a10 = tx * (1.0f - ty);
				float a01 = (1.0f - tx) * ty;
				float a11 = tx * ty;

				uint8_t r = (uint8_t)(
					((p00 >> 16) & 0xff) * a00 +
					((p10 >> 16) & 0xff) * a10 +
					((p01 >> 16) & 0xff) * a01 +
					((p11 >> 16) & 0xff) * a11
				);
				uint8_t g = (uint8_t)(
					((p00 >> 8) & 0xff) * a00 +
					((p10 >> 8) & 0xff) * a10 +
					((p01 >> 8) & 0xff) * a01 +
					((p11 >> 8) & 0xff) * a11
				);
				uint8_t b = (uint8_t)(
					(p00 & 0xff) * a00 +
					(p10 & 0xff) * a10 +
					(p01 & 0xff) * a01 +
					(p11 & 0xff) * a11
				);
				uint8_t a = (uint8_t)(
					((p00 >> 24) & 0xff) * a00 +
					((p10 >> 24) & 0xff) * a10 +
					((p01 >> 24) & 0xff) * a01 +
					((p11 >> 24) & 0xff) * a11
				);

				uint8_t sa = (uint8_t)((float)a * alpha_f);
				if(sa > 0) {
					graph_pixel_argb_raw(dst, x, y, sa, r, g, b);
				}
			}
		}
	} else {
		for(int32_t y = dr.y; y < ey; y++) {
			float src_start_y = (y - dr.y) * scale_y + sy;
			float src_end_y = (y - dr.y + 1) * scale_y + sy;
			if(src_start_y < sy) src_start_y = sy;
			if(src_end_y > sy + sh) src_end_y = sy + sh;
			int32_t src_y_start = (int32_t)src_start_y;
			int32_t src_y_end = (int32_t)src_end_y;
			if(src_y_end >= sy + sh) src_y_end = sy + sh - 1;

			int32_t dst_offset_y = y * dst->w;

			for(int32_t x = dr.x; x < ex; x++) {
				float src_start_x = (x - dr.x) * scale_x + sx;
				float src_end_x = (x - dr.x + 1) * scale_x + sx;
				if(src_start_x < sx) src_start_x = sx;
				if(src_end_x > sx + sw) src_end_x = sx + sw;
				int32_t src_x_start = (int32_t)src_start_x;
				int32_t src_x_end = (int32_t)src_end_x;
				if(src_x_end >= sx + sw) src_x_end = sx + sw - 1;

				uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
				uint32_t count = 0;

				for(int32_t src_y = src_y_start; src_y <= src_y_end; src_y++) {
					int32_t row_offset = src_y * src_w;
					for(int32_t src_x = src_x_start; src_x <= src_x_end; src_x++) {
						uint32_t p = src->buffer[row_offset + src_x];
						sum_r += (p >> 16) & 0xff;
						sum_g += (p >> 8) & 0xff;
						sum_b += p & 0xff;
						sum_a += (p >> 24) & 0xff;
						count++;
					}
				}

				if(count > 0) {
					uint8_t r = (uint8_t)(sum_r / count);
					uint8_t g = (uint8_t)(sum_g / count);
					uint8_t b = (uint8_t)(sum_b / count);
					uint8_t a = (uint8_t)((float)(sum_a / count) * alpha_f);
					if(a > 0) {
						graph_pixel_argb_raw(dst, x, y, a, r, g, b);
					}
				}
			}
		}
	}
}

inline void graph_blt_fit_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
#ifdef BSP_BOOST
		//graph_blt_fit_alpha_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
		graph_blt_fit_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
#else
		graph_blt_fit_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
#endif
}

inline bool check_in_rect(int32_t x, int32_t y, grect_t* rect) {
	if(x >= rect->x && x < (rect->x+rect->w) && 
			y >= rect->y && y < (rect->y+rect->h))
		return true;
	return false;
}

#ifdef __cplusplus
}
#endif
