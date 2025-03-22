#include <graph/graph.h>
#include <mm/kmalloc.h>
#include <kstring.h>

inline uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
	return a << 24 | r << 16 | g << 8 | b;
}

void graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return;

	g->w = w;
	g->h = h;
	if(buffer != NULL) {
		g->buffer = (uint32_t*)buffer;
		g->need_free = false;
	}
	else {
		g->buffer = (uint32_t*)kmalloc(w*h*4);
		g->need_free = true;
	}
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return NULL;

	graph_t* ret = (graph_t*)kmalloc(sizeof(graph_t));
	if(ret != NULL)
		graph_init(ret, buffer, w, h);
	
	return ret;
}

void graph_free(graph_t* g) {
	if(g == NULL)
		return;
	if(g->buffer != NULL && g->need_free)
		kfree(g->buffer);
	kfree(g);
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

void blt16(uint32_t* src, uint16_t* dst, uint32_t w, uint32_t h) {
	uint32_t sz = w * h;
	uint32_t i;

	for (i = 0; i < sz; i++) {
		uint32_t s = src[i];
		uint8_t r = (s >> 16) & 0xff;
		uint8_t g = (s >> 8)  & 0xff;
		uint8_t b = s & 0xff;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}

void graph_rotate_to(graph_t* g, graph_t* ret, int rot) {
	if(g == NULL || ret == NULL)
		return;

	if(rot == G_ROTATE_90) {
		for(int i=0; i<g->w; i++) {
			for(int j=0; j<g->h; j++) {
				ret->buffer[(ret->h-i-1)*ret->w + (ret->w-j)] = g->buffer[j*g->w + (g->w-i-1)];
			}
		}
	}
	else if(rot == G_ROTATE_N90) {
		for(int i=0; i<g->w; i++) {
			for(int j=0; j<g->h; j++) {
				ret->buffer[i*ret->w + j] = g->buffer[j*g->w + (g->w-i-1)];
			}
		}
	}
	else if(rot == G_ROTATE_180) {
		for(int i=0; i<g->h; i++) {
			for(int j=0; j<g->w; j++) {
				ret->buffer[i*g->w + j] = g->buffer[(g->h-i-1)*g->w + (g->w-j-1)];
			}
		}
	}
}

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
	if(dst->w <= 0 || dst->h <= 0)
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

void graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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