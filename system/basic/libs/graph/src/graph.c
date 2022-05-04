#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
	return a << 24 | r << 16 | g << 8 | b;
}

inline int32_t has_alpha(uint32_t c) {
	if(((c >> 24) & 0xff) != 0xff)
		return 1;
	return 0;
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return NULL;

	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	ret->w = w;
	ret->h = h;
	if(buffer != NULL) {
		ret->buffer = buffer;
		ret->need_free = false;
	}
	else {
		ret->buffer = (uint32_t*)malloc(w*h*4);
		ret->need_free = true;
	}
	return ret;
}

void graph_free(graph_t* g) {
	if(g == NULL)
		return;
	if(g->buffer != NULL && g->need_free)
		free(g->buffer);
	free(g);
}

void graph_clear(graph_t* g, uint32_t color) {
	if(g == NULL)
		return;
	if(g->w == 0 || g->w == 0)
		return;

	int32_t i = 0;
	int32_t sz = g->w * 4;
	while(i<g->w) {
		g->buffer[i] = color;
		++i;
	}
	char* p = (char*)g->buffer;
	for(i=1; i<g->h; ++i) {
		memcpy(p+(i*sz), p, sz);
	}
}

void graph_reverse(graph_t* g) {
	if(g == NULL)
		return;
	int32_t i = 0;
	while(i < g->w*g->h) {
		uint32_t oc = g->buffer[i];
		uint8_t oa = (oc >> 24) & 0xff;
		uint8_t or = 0xff - ((oc >> 16) & 0xff);
		uint8_t og = 0xff - ((oc >> 8)  & 0xff);
		uint8_t ob = 0xff - (oc & 0xff);
		g->buffer[i] = argb(oa, or, og, ob);
		i++;
	}
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/
static int32_t graph_insect(graph_t* g, grect_t* r) {
	//insect src;
	if(g == NULL || r->x >= (int32_t)g->w || r->y >= (int32_t)g->h) //check x, y
		return 0;

	int32_t rx, ry;  //chehck w, h
	rx = r->x + r->w;
	ry = r->y + r->h;
	if(rx >= (int32_t)g->w)
		r->w -= rx - g->w;
	if(ry >= (int32_t)g->h)
		r->h -= ry - g->h;

	if(r->x < 0) {
		r->w += r->x;
		r->x = 0;
	}

	if(r->y < 0) {
		r->h += r->y;
		r->y = 0;
	}

	if(r->w <= 0 || r->h <= 0)
		return 0;
	return 1;
}

/*will change the value of sr, dr.
	return 0 for none-insection-area.
*/

static int32_t insect(graph_t* src, grect_t* sr, graph_t* dst, grect_t* dr) {
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

void graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL || w <= 0 || h <= 0)
		return;
	grect_t r = {x, y, w, h};
	if(!graph_insect(g, &r))
		return;

	int32_t ex, ey;
	y = r.y;
	ex = r.x + r.w;
	ey = r.y + r.h;

	if(!has_alpha(color)) {
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x++) {
				graph_pixel(g, x, y, color);
			}
		}
	}
	else {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x++) {
				graph_pixel_argb(g, x, y, ca, cr, cg, cb);
			}
		}
	}
}

inline void graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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
	if(!insect(src, &sr, dst, &dr))
		return;

	register int32_t ex, ey;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		sx = sr.x;
		dx = dr.x;
		for(; sx < ex; sx++, dx++) {
			dst->buffer[dy * dst->w + dx] = src->buffer[sy * src->w + sx];
		}
	}
}

inline void graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	grect_t sr = {sx, sy, sw, sh};
	grect_t dr = {dx, dy, dw, dh};
	if(!insect(src, &sr, dst, &dr))
		return;

	register int32_t ex, ey;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		register int32_t sx = sr.x;
		register int32_t dx = dr.x;
		for(; sx < ex; sx++, dx++) {
			register uint32_t color = src->buffer[sy * src->w + sx];
			graph_pixel_argb(dst, dx, dy,
					(((color >> 24) & 0xff) * alpha)/0xff,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
		}
	}
}

bool check_in_rect(int32_t x, int32_t y, grect_t* rect) {
	if(x >= rect->x && x < (rect->x+rect->w) && 
			y >= rect->y && y < (rect->y+rect->h))
		return true;
	return false;
}

#ifdef __cplusplus
}
#endif
