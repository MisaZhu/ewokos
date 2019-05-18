#include <graph/graph.h>
#include <stdlib.h>
#include <kstring.h>
#include <vfs/fs.h>
#include <fbinfo.h>
#include <unistd.h>
#include <shm.h>

inline uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) {
	return b << 16 | g << 8 | r;
}

uint32_t rgb_int(uint32_t c) {
	return rgb((c>>16)&0xff, (c>>8)&0xff, c&0xff);
}

int32_t fb_open(const char* fname, fb_t* fb) {
	fb_info_t fb_info;
	int fd = fs_open(fname, 0);
	if(fd < 0) {
		return -1;
	}
	if(fs_ctrl(fd, FS_CTRL_INFO, NULL, 0, &fb_info, sizeof(fb_info_t)) != 0) {
		fs_close(fd);
		return -1;
	}

	uint32_t sz;
	int shm_id = fs_dma(fd, &sz);
	if(shm_id < 0 || sz == 0) {
		fs_close(fd);
		return -1;
	}
	
	void* p = shm_map(shm_id);
	if(p == NULL) {
		fs_close(fd);
		return -1;
	}
	fb->fd = fd;
	fb->shm_id = shm_id;
	fb->w = fb_info.width;
	fb->h = fb_info.height;
	return 0;
}

void fb_flush(fb_t* fb) {
	fs_flush(fb->fd);
}

void fb_close(fb_t* fb) {
	shm_unmap(fb->shm_id);
	fs_close(fb->fd);
}

graph_t* graph_new(uint32_t* buffer, uint32_t w, uint32_t h) {
	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	ret->w = w;
	ret->h = h;
	if(buffer != NULL) {
		ret->buffer = buffer;
		ret->type = GRAPH_REF;
	}
	else {
		ret->buffer = (uint32_t*)malloc(w*h*4);
		ret->type = GRAPH_MEM;
	}
	return ret;
}

graph_t* graph_from_fb(fb_t* fb) {
	void* p = shm_map(fb->shm_id);
	if(p == NULL) {
		return NULL;
	}
	return graph_new(p, fb->w, fb->h);
}

void graph_free(graph_t* g) {
	if(g->type == GRAPH_MEM && g->buffer != NULL)
		free(g->buffer);
	free(g);
}

inline void pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	if(x < 0 ||  (uint32_t)x >= g->w || y < 0 || (uint32_t)y >= g->h)
		return;
	g->buffer[y * g->w + x] = color;
}

static inline void pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	g->buffer[y * g->w + x] = color;
}

void clear(graph_t* g, uint32_t color) {
	uint8_t byte = color & 0xff;	
	if(((color >> 8)&0xff) == byte && ((color >> 16)&0xff) == byte) {
		memset(g->buffer, byte, g->w*g->h*4);
		return;
	}

	if(g->w == 0 || g->w == 0)
		return;
	uint32_t i = 0;
	uint32_t sz = g->w * 4;
	while(i<g->w) {
		g->buffer[i] = color;
		++i;
	}
	char* p = (char*)g->buffer;
	for(i=1; i<g->h; ++i) {
		memcpy(p+(i*sz), p, sz);
	}
}

void line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	int32_t dx, dy, x, y, s1, s2, e, temp, swap, i;

	dy = y2 - y1;
	if (dy<0) { dy = -dy; s2 = -1; }
	else s2 = 1;
	dx = x2 - x1;
	if (dx<0) { dx = -dx; s1 = -1; }
	else s1 = 1;

	x = x1;
	y = y1;
	if (dy > dx) {
		temp=dx;
		dx=dy;
		dy=temp;
		swap=1;
	}
	else swap=0;

	e = 2 * dy - dx;
	for (i=0; i<=dx; i++) {
		pixel_safe(g, x, y, color);
		while (e>=0) {
			if (swap==1) x += s1;
			else y += s2;
			e -= 2*dx;
		}
		if (swap==1) y += s2;
		else x += s1;
		e += 2*dy;
	}
}

void box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	line(g, x, y, x+w, y, color);
	line(g, x, y+1, x, y+h, color);
	line(g, x+1, y+h, x+w, y+h, color);
	line(g, x+w, y+1, x+w, y+h, color);
}

/*will change the value of sr, dr.
	return false for none-insection-area.
*/
static bool graph_insect(graph_t* g, rect_t* r) {
	//insect src;
	if(r->x >= (int32_t)g->w || r->y >= (int32_t)g->h) //check x, y
		return false;

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
		return false;
	return true;
}

/*will change the value of sr, dr.
	return false for none-insection-area.
*/

static bool insect(graph_t* src, rect_t* sr, graph_t* dst, rect_t* dr) {
	//insect src;
	int32_t dx = dr->x;
	int32_t dy = dr->y;
	if(!graph_insect(src, sr))
		return false;
	if(!graph_insect(dst, dr))
		return false;

	if(dx < 0) {
		sr->x -= dx;
		sr->w += dx;
	}
	if(dy < 0) {
		sr->y -= dy;
		sr->h += dy;
	}
	if(sr->w <= 0 || sr->h <= 0)
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

void fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	rect_t r = {x, y, w, h};
	if(!graph_insect(g, &r))
		return;

	int32_t ex, ey;
	y = r.y;
	ex = r.x + r.w;
	ey = r.y + r.h;

	for(; y < ey; y++) {
		x = r.x;
		for(; x < ex; x++) {
			pixel(g, x, y, color);
		}
	}
}

static void draw_char8(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h;
	ypart = y;

	for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
		xpart = x;
		pmask = 1 << (8-1);
		check = pdata[index+ychar];
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				pixel_safe(g, xpart, ypart, color);
			//else
			//	pixel_safe(g, xpart, ypart, bg);
			xpart++;
			pmask >>= 1;
		}
		ypart++;
	}
}

static void draw_char16(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h * 2;
	ypart = y;

	for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
		xpart = x;
		pmask = 1 << (8-1);
		check = pdata[index+ychar];
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				pixel_safe(g, xpart, ypart, color);
			//else
			//	pixel_safe(g, xpart, ypart, bg);
			xpart++;
			pmask >>= 1;
		}

		index++;
		check = pdata[index+ychar];
		pmask = 1 << (8-1);
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				pixel_safe(g, xpart, ypart, color);
			//else
			//	pixel_safe(g, xpart, ypart, bg);
			xpart++;
			pmask >>= 1;
		}
		ypart++;
	}
}

void draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(font->w <= 8)
		draw_char8(g, x, y, c, font, color);
	else if(font->w <= 16)
		draw_char16(g, x, y, c, font, color);
}

void draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color) {
	while(*str) {
		draw_char(g, x, y, *str, font, color);
		x += font->w;
		str++;
	}
}

void blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	rect_t sr = {sx, sy, sw, sh};
	rect_t dr = {dx, dy, dw, dh};
	if(!insect(src, &sr, dst, &dr))
		return;

	int32_t ex, ey;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		int32_t sx = sr.x;
		int32_t dx = dr.x;
		for(; sx < ex; sx++, dx++) {
			dst->buffer[dy * dst->w + dx] = src->buffer[sy * src->w + sx];
		}
	}
}
