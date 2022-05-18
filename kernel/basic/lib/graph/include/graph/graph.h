#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>
#include <stdbool.h>

typedef struct {
	uint32_t *buffer;
	int32_t w;
	int32_t h;
	bool need_free;
} graph_t;

typedef struct {
	int32_t w;	
	int32_t h;	
} gsize_t;

typedef struct {
	int32_t x;	
	int32_t y;	
} gpos_t;

typedef struct {
	int32_t x;	
	int32_t y;	
	int32_t w;	
	int32_t h;	
} grect_t;

uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b);

uint32_t argb_int(uint32_t c);

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h);

void graph_free(graph_t* g);

void graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color);

void graph_pixel_argb(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b);

void graph_pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b);

void graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);

void graph_clear(graph_t* g, uint32_t color);

void graph_reverse(graph_t* g);

void graph_draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color);

void graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color);

#endif
