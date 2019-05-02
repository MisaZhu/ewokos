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

graph_t* graph_open(const char* fname) {
	fb_info_t fbInfo;
	int fd = fs_open(fname, 0);
	if(fd < 0) {
		return NULL;
	}
	if(fs_ctrl(fd, 0, NULL, 0, &fbInfo, sizeof(fb_info_t)) != 0) {
		fs_close(fd);
		return NULL;
	}

	uint32_t sz;
	int shm_id = fs_dma(fd, &sz);
	if(shm_id < 0 || sz == 0) {
		fs_close(fd);
		return NULL;
	}
	
	void* p = shm_map(shm_id);
	if(p == NULL) {
		fs_close(fd);
		return NULL;
	}

	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	ret->w = fbInfo.width;
	ret->h = fbInfo.height;
	ret->buffer = p;
	ret->fd = fd;
	ret->shm_id = shm_id;

	return ret;
}

void graph_flush(graph_t* graph) {
	fs_flush(graph->fd);
}

void graph_close(graph_t* graph) {
	shm_unmap(graph->shm_id);
	fs_close(graph->fd);
	free(graph);
}

inline void pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	if(x < 0 ||  (uint32_t)x >= g->w || y < 0 || (uint32_t)y >= g->h)
		return;
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
		pixel(g, x, y, color);
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

void box(graph_t* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color) {
	line(g, x, y, x+w, y, color);
	line(g, x, y+1, x, y+h, color);
	line(g, x+1, y+h, x+w, y+h, color);
	line(g, x+w, y+1, x+w, y+h, color);
}

void fill(graph_t* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color) {
	int32_t ex, ey;
	ex = x + w;
	ey = y + h;

	for(; y < ey; y++) {
		int32_t sx = x;
		for(; sx < ex; sx++) {
			pixel(g, sx, y, color);
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
				pixel(g, xpart, ypart, color);
			//else
			//	pixel(g, xpart, ypart, bg);
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
				pixel(g, xpart, ypart, color);
			//else
			//	pixel(g, xpart, ypart, bg);
			xpart++;
			pmask >>= 1;
		}

		index++;
		check = pdata[index+ychar];
		pmask = 1 << (8-1);
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				pixel(g, xpart, ypart, color);
			//else
			//	pixel(g, xpart, ypart, bg);
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
