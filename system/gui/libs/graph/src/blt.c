#include <graph/graph.h>
#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#define BSP_SIZE 256

int32_t grect_insect(const grect_t* src, grect_t* dst) {
	//insect src;
	if(dst->x >= (int32_t)(src->x+src->w))
		dst->w = 0;
	if(dst->y >= (int32_t)(src->y+src->h))
		dst->h = 0;
	
	if(dst->w == 0 || dst->h == 0)
		return 0;

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
		return 0;
	return 1;
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/
inline int32_t graph_insect(graph_t* g, grect_t* r) {
	grect_t gr = {0, 0, g->w, g->h};
	return grect_insect(&gr, r);
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/

int32_t graph_insect_with(graph_t* src, grect_t* sr, graph_t* dst, grect_t* dr) {
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
		return 0;
	if(!graph_insect(dst, dr))
		return 0;

	if(sr->w <= 0 || sr->h <= 0 ||
			dr->w <= 0 || dr->h <= 0)
		return 0;

	if(sr->w > dr->w)
		sr->w = dr->w;
	else
		dr->w = sr->w;
	
	if(sr->h > dr->h)
		sr->h = dr->h;
	else
		dr->h = sr->h;
	return 1;
}

void graph_fill_cpu(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL || w <= 0 || h <= 0)
		return;
	grect_t r = {x, y, w, h};
	if(!graph_insect(g, &r))
		return;

	register int32_t ex, ey;
	y = r.y;
	ex = r.x + r.w;
	ey = r.y + r.h;

	if(color_a(color) == 0xff) {
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x++) {
				graph_pixel(g, x, y, color);
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
				graph_pixel_argb(g, x, y, ca, cr, cg, cb);
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

inline void graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(w < BSP_SIZE || h < BSP_SIZE)
		graph_fill_cpu(g, x, y, w, h, color);
	else
		graph_fill_bsp(g, x, y, w, h, color);
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
	if(!graph_insect_with(src, &sr, dst, &dr))
		return;

	if(dx < 0)
		sr.x -= dx;
	if(dy < 0)
		sr.y -= dy;

	register int32_t ex, ey, offset_d, offset_r;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		sx = sr.x;
		dx = dr.x;
		offset_d = dy * dst->w;
		offset_r = sy * src->w;
		for(; sx < ex; sx++, dx++) {
			dst->buffer[offset_d + dx] = src->buffer[offset_r + sx];
		}
		//memcpy(&dst->buffer[offset_d]+dx, &src->buffer[offset_r], (ex-sx)*4);
	}
}

inline void graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(dw < BSP_SIZE || dh < BSP_SIZE)
		graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
	else
		graph_blt_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

void graph_blt_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	grect_t sr = {sx, sy, sw, sh};
	grect_t dr = {dx, dy, dw, dh};
	graph_insect(dst, &dr);
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
		register int32_t sx = sr.x;
		register int32_t dx = dr.x;
		register int32_t offset = sy * src->w;
		for(; sx < ex; sx++, dx++) {
			register uint32_t color = src->buffer[offset + sx];
			graph_pixel_argb(dst, dx, dy,
					//(((color >> 24) & 0xff) * alpha)/0xff,
					(((color >> 24) & 0xff) * alpha)>>8,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
		}
	}
}

inline void graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	graph_blt_alpha_bsp(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
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
