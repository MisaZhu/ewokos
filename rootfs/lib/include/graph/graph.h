#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>

typedef struct Graph {
	uint32_t *buffer;
	uint32_t w;
	uint32_t h;
} GraphT;

GraphT* graphNew(uint32_t w, uint32_t h);

void graphFree(GraphT* g);

void pixel(GraphT* g, int32_t x, int32_t y, uint32_t color);

void clear(GraphT* g, uint32_t color);

void box(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void fill(GraphT* g, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);

void line(GraphT* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

void drawText(GraphT* g, int32_t x, int32_t y, const char* str, FontT* font, uint32_t color);

#endif
