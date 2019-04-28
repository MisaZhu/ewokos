#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>

typedef struct Graph {
	uint32_t *buffer;
	uint32_t w;
	uint32_t h;

	int32_t fd;
	int32_t shm_id;
} graph_t;

uint32_t rgb(uint32_t r, uint32_t g, uint32_t b);
uint32_t rgb_int(uint32_t c);

graph_t* graph_open(const char* fname);

void graph_flush(graph_t* g);

void graph_close(graph_t* g);

void pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);

void clear(graph_t* g, uint32_t color);

void box(graph_t* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void fill(graph_t* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

void draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color);

void draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color);

#endif
