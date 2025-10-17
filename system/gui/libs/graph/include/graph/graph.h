#ifndef GRAPH_H
#define GRAPH_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	G_ROTATE_0 = 0,
	G_ROTATE_90,
	G_ROTATE_180,
	G_ROTATE_270
};

typedef struct {
	uint8_t *buffer;
	int32_t w;
	int32_t h;
} bitmap_t;

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

typedef struct {
	uint32_t *buffer;
	int32_t w;
	int32_t h;
	grect_t clip;
	bool need_free;
} graph_t;


bool grect_insect(const grect_t* src, grect_t* dst);

uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b);

uint8_t  color_a(uint32_t c);
uint8_t  color_r(uint32_t c);
uint8_t  color_g(uint32_t c);
uint8_t  color_b(uint32_t c);
uint32_t color_gray(uint32_t c);
uint32_t color_reverse(uint32_t c);
uint32_t color_reverse_rgb(uint32_t c);

uint32_t argb_int(uint32_t c);

void     graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h);
graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h);
void     graph_free(graph_t* g);
graph_t* graph_dup(graph_t* g);

bool  graph_insect_with(graph_t* src, grect_t* sr, graph_t* dst, grect_t* dr);
bool  graph_insect(graph_t* g, grect_t* r);
void     graph_set_clip(graph_t* g, int x, int y, int w, int h);
void     graph_unset_clip(graph_t* g);

void     graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color);
void     graph_pixel_argb(graph_t* graph, int32_t x, int32_t y,
			uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     graph_pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
			uint8_t a, uint8_t r, uint8_t g, uint8_t b);
void     graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color);
uint32_t graph_get_pixel(graph_t* g, int32_t x, int32_t y);

void     graph_clear(graph_t* g, uint32_t color);

void     graph_reverse(graph_t* g);
void     graph_reverse_rgb(graph_t* g);
void     graph_gray(graph_t* g);
void     graph_rotate_to(graph_t* g, graph_t* dst, int rot);
graph_t* graph_rotate(graph_t* g, int rot);
void     graph_scale_to(graph_t* src, graph_t* dst, int scale);
graph_t* graph_scale(graph_t* g, int scale);
void     graph_scale_tof(graph_t* src, graph_t* dst, float scale);
graph_t* graph_scalef(graph_t* g, float scale);

void     graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_fill(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_set(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void     graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
void     graph_wline(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, uint32_t w);

void     graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);
void     graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color);

void     graph_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color);
void     graph_fill_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color);

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

void     graph_fill_cpu(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void     graph_blt_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void     graph_blt_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	

void     graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale);

bool     check_in_rect(int32_t x, int32_t y, grect_t* rect);

graph_t* graph_from_fb(int fd, int *dma_id);

void      bitmap_free(bitmap_t* b);
bitmap_t* graph_to_bitmap(graph_t* g);
graph_t*  graph_from_bitmap(bitmap_t* b, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
