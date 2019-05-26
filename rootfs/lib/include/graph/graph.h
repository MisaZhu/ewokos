#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>

typedef struct {
	int32_t fd;
	int32_t shm_id;
	uint32_t w;
	uint32_t h;
} fb_t;

enum {
	GRAPH_MEM = 0,
	GRAPH_REF
};

typedef struct {
	uint32_t *buffer;
	uint32_t w;
	uint32_t h;
	uint32_t type;
} graph_t;

typedef struct {
	int32_t w;	
	int32_t h;	
} gsize_t;

typedef struct {
	int32_t x;	
	int32_t y;	
} pos_t;

typedef struct {
	int32_t x;	
	int32_t y;	
	int32_t w;	
	int32_t h;	
} rect_t;

uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b);

bool has_alpha(uint32_t c);

uint32_t argb_int(uint32_t c);

int32_t fb_open(const char* fname, fb_t* fb);

void fb_close(fb_t* fb);

void fb_flush(fb_t* fb);

graph_t* graph_from_fb(fb_t* fb);

graph_t* graph_new(uint32_t* buffer, uint32_t w, uint32_t h);

void graph_free(graph_t* g);

void pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color);

void pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);

void clear(graph_t* g, uint32_t color);

void box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

void draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color);

void draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color);

void blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);

void blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);
#endif
