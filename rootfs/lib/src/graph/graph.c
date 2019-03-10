#include <graph/graph.h>
#include <stdlib.h>
#include <kstring.h>
#include <vfs/fs.h>
#include <fbinfo.h>
#include <shm.h>

GraphT* graphOpen(const char* fname) {
	FBInfoT fbInfo;
	int fd = fsOpen(fname, 0);
	if(fd < 0) {
		return NULL;
	}
	if(fsCtrl(fd, 0, NULL, 0, &fbInfo, sizeof(FBInfoT)) != 0) {
		fsClose(fd);
		return NULL;
	}

	uint32_t sz;
	int shmID = fsDMA(fd, &sz);
	if(shmID < 0 || sz == 0) {
		fsClose(fd);
		return NULL;
	}
	
	void* p = shmMap(shmID);
	if(p == NULL) {
		fsClose(fd);
		return NULL;
	}

	GraphT* ret = (GraphT*)malloc(sizeof(GraphT));
	ret->w = fbInfo.width;
	ret->h = fbInfo.height;
	ret->buffer = p;
	ret->fd = fd;
	ret->shmID = shmID;

	return ret;
}

void graphFlush(GraphT* graph) {
	fsFlush(graph->fd);
}

void graphClose(GraphT* graph) {
	shmUnmap(graph->shmID);
	fsClose(graph->fd);
	free(graph);
}

inline void pixel(GraphT* g, int32_t x, int32_t y, uint32_t color) {
	if(x < 0 ||  (uint32_t)x >= g->w || y < 0 || (uint32_t)y >= g->h)
		return;
	g->buffer[y * g->w + x] = color;
}

void clear(GraphT* g, uint32_t color) {
	uint32_t loopy,loopx,index;
	index = 0;
	for (loopy=0;loopy<g->h;loopy++) {
		for (loopx=0;loopx<g->w;loopx++) {
			g->buffer[index++] = color;
		}
	}
}

void line(GraphT* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
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

void box(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color) {
	line(g, x, y, x+w, y, color);
	line(g, x, y+1, x, y+h, color);
	line(g, x+1, y+h, x+w, y+h, color);
	line(g, x+w, y+1, x+w, y+h, color);
}

void fill(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color) {
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

static inline void drawChar(GraphT* g, int32_t x, int32_t y, char c, FontT* font, uint32_t color) {
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h;
	ypart = y;

	for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
		xpart = x;
		pmask = 1 << (font->w-1);
		check = pdata[index+ychar];
		for(xchar=0; (uint32_t)xchar<font->w; xchar++) {
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

void drawText(GraphT* g, int32_t x, int32_t y, const char* str, FontT* font, uint32_t color) {
	while(*str) {
		drawChar(g, x, y, *str, font, color);
		x += font->w;
		str++;
	}
}
