#ifndef GRAPH_H
#define GRAPH_H

#include <graph/font.h>
#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	G_ROTATE_NONE = 0,
	G_ROTATE_90,
	G_ROTATE_180,
	G_ROTATE_N90
};

typedef struct {
	uint8_t *buffer;
	int32_t w;
	int32_t h;
} bitmap_t;

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

int32_t has_alpha(uint32_t c);

uint32_t argb_int(uint32_t c);

void     graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h);
graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h);
void     graph_free(graph_t* g);

void     graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color);
void     graph_pixel_argb(graph_t* graph, int32_t x, int32_t y,
			uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     graph_pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
			uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);

void     graph_clear(graph_t* g, uint32_t color);

void     graph_reverse(graph_t* g);
graph_t* graph_rotate(graph_t* g, int rot);
void     graph_scale_to(graph_t* src, graph_t* dst, int scale);
graph_t* graph_scale(graph_t* g, int scale);

void     graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void     graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

void     graph_draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color);
void     graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color);

void     graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);
void     graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);

void     graph_fill_round(graph_t* g, int32_t x, int32_t y, 
			int32_t w, int32_t h,
			int32_t radius, uint32_t color); 
void     graph_round(graph_t* g, int32_t x, int32_t y,
			int32_t w, int32_t h,
			int32_t radius, uint32_t color);

void     graph_blt(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void     graph_blt_alpha(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);
	
bool     check_in_rect(int32_t x, int32_t y, grect_t* rect);

graph_t* graph_from_fb(int fd, int *dma_id);

void      bitmap_free(bitmap_t* b);
bitmap_t* graph_to_bitmap(graph_t* g);
graph_t*  graph_from_bitmap(bitmap_t* b, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
