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

inline void graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	g->buffer[y * g->w + x] = color;
}

inline void graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	if(g == NULL)
		return;
	if(x < 0 || x >= g->w || y < 0 || y >= g->h)
		return;
	graph_pixel(g, x, y, color);
}

static inline void pixel_argb(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	register uint32_t oc = graph->buffer[y * graph->w + x];
	register uint8_t oa = (oc >> 24) & 0xff;
	register uint8_t or = (oc >> 16) & 0xff;
	register uint8_t og = (oc >> 8)  & 0xff;
	register uint8_t ob = oc & 0xff;

	oa = oa + (255 - oa) * a / 255;
	or = r*a/255 + or*(255-a)/255;
	og = g*a/255 + og*(255-a)/255;
	ob = b*a/255 + ob*(255-a)/255;

	graph->buffer[y * graph->w + x] = argb(oa, or, og, ob);
}

static inline void pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	if(graph == NULL)
		return;
	if(x < 0 || x >= graph->w || y < 0 || y >= graph->h)
		return;
	pixel_argb(graph, x, y, a, r, g, b);
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

void graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if(g == NULL)
		return;

	if(x1 == x2 && y1 == y2) {
		if(!has_alpha(color))
			graph_pixel_safe(g, x1, y1, color);
		else
			pixel_argb_safe(g, x1, y1,
					(color >> 24) & 0xff,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
		return;
	}

	int32_t dx, dy, x, y, s1, s2, e, temp, swap, i;

	dy = y2 - y1;
	if (dy<0) {
		dy = -dy; 
		s2 = -1; 
	}
	else
		s2 = 1;

	dx = x2 - x1;
	if (dx<0) {
		dx = -dx; 
		s1 = -1;
	}
	else
		s1 = 1;

	x = x1;
	y = y1;
	if (dy > dx) {
		temp=dx;
		dx=dy;
		dy=temp;
		swap=1;
	}
	else
		swap=0;

	e = 2 * dy - dx;
	if(!has_alpha(color)) {
		for (i=0; i<=dx; i++) {
			graph_pixel_safe(g, x, y, color);
			while (e>=0) {
				if (swap==1)
					x += s1;
				else
					y += s2;
				e -= 2*dx;
			}
			if (swap==1)
				y += s2;
			else
				x += s1;
			e += 2*dy;
		}
	}
	else {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;
	
		for (i=0; i<=dx; i++) {
			pixel_argb_safe(g, x, y, ca, cr, cg, cb);
			while (e>=0) {
				if (swap==1)
					x += s1;
				else 
					y += s2;
				e -= 2*dx;
			}
			if (swap==1)
				y += s2;
			else
				x += s1;
			e += 2*dy;
		}
	}
}

void graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL || w <= 0 || h <= 0)
		return;

	graph_line(g, x, y, x+w-1, y, color);
	graph_line(g, x, y+1, x, y+h-1, color);
	graph_line(g, x+1, y+h-1, x+w-1, y+h-1, color);
	graph_line(g, x+w-1, y+1, x+w-1, y+h-1, color);
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
				pixel_argb(g, x, y, ca, cr, cg, cb);
			}
		}
	}
}

static void draw_char8(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h;
	ypart = y;

	if(has_alpha(color)) {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;
		for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
			xpart = x;
			pmask = 1 << (8-1);
			check = pdata[index+ychar];
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					pixel_argb_safe(g, xpart, ypart,
							ca, cr, cg, cb);
				xpart++;
				pmask >>= 1;
			}
			ypart++;
		}
	}
	else {
		for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
			xpart = x;
			pmask = 1 << (8-1);
			check = pdata[index+ychar];
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					graph_pixel_safe(g, xpart, ypart, color);
				xpart++;
				pmask >>= 1;
			}
			ypart++;
		}
	}
}

static void draw_char16(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h * 2;
	ypart = y;

	if(has_alpha(color)) {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;
		for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
			xpart = x;
			pmask = 1 << (8-1);
			check = pdata[index+ychar];
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					pixel_argb_safe(g, xpart, ypart, ca, cr, cg, cb);
				xpart++;
				pmask >>= 1;
			}

			index++;
			check = pdata[index+ychar];
			pmask = 1 << (8-1);
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					pixel_argb_safe(g, xpart, ypart, ca, cr, cg, cb);
				xpart++;
				pmask >>= 1;
			}
			ypart++;
		}
	}
	else {
		for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
			xpart = x;
			pmask = 1 << (8-1);
			check = pdata[index+ychar];
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					graph_pixel_safe(g, xpart, ypart, color);
				xpart++;
				pmask >>= 1;
			}

			index++;
			check = pdata[index+ychar];
			pmask = 1 << (8-1);
			for(xchar=0; (uint32_t)xchar<8; xchar++) {
				if(check&pmask)
					graph_pixel_safe(g, xpart, ypart, color);
				xpart++;
				pmask >>= 1;
			}
			ypart++;
		}
	}
}

void graph_draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	if(font->w <= 8)
		draw_char8(g, x, y, c, font, color);
	else if(font->w <= 16)
		draw_char16(g, x, y, c, font, color);
}

void graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t ox = x;
	while(*str) {
		if(*str == '\n') {
			x = ox;
			y += font->h;
		}
		else {
			graph_draw_char(g, x, y, *str, font, color);
			x += font->w;
		}
		str++;
	}
}

void graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
	if(radius <= 0)
		return;

	int32_t a, b, P;
	a = 1;
	b = radius;
	P = 1 - radius;

	graph_line(g, x-radius, y, x+radius, y, color);
	do {
		graph_line(g, x-b, y+a, x+b, y+a, color);
		graph_line(g, x-b, y-a, x+b, y-a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, x - a, y + b, x + a, y + b, color);
				graph_line(g, x - a, y - b, x + a, y - b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);
}

void graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
	if(radius <= 0)
		return;
	int32_t a, b, P;
	a = 1;
	b = radius;
	P = 1 - radius;

	if(has_alpha(color)) {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;

		pixel_argb_safe(g, x, radius+y, ca, cr, cg, cb);
		pixel_argb_safe(g, x, y-radius, ca, cr, cg, cb);
		pixel_argb_safe(g, x+radius, y, ca, cr, cg, cb);
		pixel_argb_safe(g, x-radius, y, ca, cr, cg, cb);

		do {
			pixel_argb_safe(g, b+x, a+y, ca, cr, cg, cb);
			pixel_argb_safe(g, x-b, a+y, ca, cr, cg, cb);
			pixel_argb_safe(g, b+x, y-a, ca, cr, cg, cb);
			pixel_argb_safe(g, x-b, y-a, ca, cr, cg, cb);

			pixel_argb_safe(g, a+x, b+y, ca, cr, cg, cb);
			pixel_argb_safe(g, x-a, b+y, ca, cr, cg, cb);
			pixel_argb_safe(g, a+x, y-b, ca, cr, cg, cb);
			pixel_argb_safe(g, x-a, y-b, ca, cr, cg, cb);

			if(P < 0)
				P+= 3 + 2*a++;
			else
				P+= 5 + 2*(a++ - b--);
		} while(a < b);
		pixel_argb_safe(g, b+x, a+y, ca, cr, cg, cb);
		pixel_argb_safe(g, x-b, a+y, ca, cr, cg, cb);
		pixel_argb_safe(g, b+x, y-a, ca, cr, cg, cb);
		pixel_argb_safe(g, x-b, y-a, ca, cr, cg, cb);
	}
	else {
		graph_pixel_safe(g, x, radius+y, color);
		graph_pixel_safe(g, x, y-radius, color);
		graph_pixel_safe(g, x+radius, y, color);
		graph_pixel_safe(g, x-radius, y, color);

		do {
			graph_pixel_safe(g, b+x, a+y, color);
			graph_pixel_safe(g, x-b, a+y, color);
			graph_pixel_safe(g, b+x, y-a, color);
			graph_pixel_safe(g, x-b, y-a, color);

			graph_pixel_safe(g, a+x, b+y, color);
			graph_pixel_safe(g, x-a, b+y, color);
			graph_pixel_safe(g, a+x, y-b, color);
			graph_pixel_safe(g, x-a, y-b, color);

			if(P < 0)
				P+= 3 + 2*a++;
			else
				P+= 5 + 2*(a++ - b--);
		} while(a < b);
		graph_pixel_safe(g, b+x, a+y, color);
		graph_pixel_safe(g, x-b, a+y, color);
		graph_pixel_safe(g, b+x, y-a, color);
		graph_pixel_safe(g, x-b, y-a, color);
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
			pixel_argb(dst, dx, dy,
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

