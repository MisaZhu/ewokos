#include <graph/bsp_graph.h>
#include <ewoksys/core.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef NEON_BOOST

#define MIN(a, b) (((a) > (b))?(b):(a))

static inline void neon_alpha_8(uint32_t *b, uint32_t *f, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%1]\n\t"  // load foreground (R,G,B,A)
        "movi v28.8b, #255\n\t"
        "sub v28.8b, v28.8b, v23.8b\n\t"                 // 255 - alpha
        
        // Multiply foreground by its alpha
        "umull v1.8h, v20.8b, v23.8b\n\t"                // R * A
        "umull v3.8h, v21.8b, v23.8b\n\t"                // G * A
        "umull v5.8h, v22.8b, v23.8b\n\t"                // B * A
        
        "ld4 {v24.8b, v25.8b, v26.8b, v27.8b}, [%0]\n\t"  // load background (R,G,B,A)
        "movi v29.8b, #255\n\t"
        "sub v29.8b, v29.8b, v27.8b\n\t"                 // 255 - background alpha
        
        // Multiply background by inverse of foreground alpha
        "umull v2.8h, v24.8b, v28.8b\n\t"                // R * (1-A)
        "umull v4.8h, v25.8b, v28.8b\n\t"                // G * (1-A)
        "umull v6.8h, v26.8b, v28.8b\n\t"                // B * (1-A)
        
        // Calculate resulting alpha: oa = oa + (255 - oa) * a / 255
        "umull v7.8h, v29.8b, v23.8b\n\t"                // (255-oa)*a
        "ushr v7.8h, v7.8h, #8\n\t"                     // >> 8 (/256)
        "movi v29.8b, #1\n\t"
        "umull v8.8h, v29.8b, v27.8b\n\t"                // 1 * oa
        "add v7.8h, v7.8h, v8.8h\n\t"                    // sum
        
        // Sum foreground and background components
        "add v1.8h, v1.8h, v2.8h\n\t"
        "add v3.8h, v3.8h, v4.8h\n\t"
        "add v5.8h, v5.8h, v6.8h\n\t"
        
        // Shift right by 8 (/256)
        "ushr v1.8h, v1.8h, #8\n\t"
        "ushr v3.8h, v3.8h, #8\n\t"
        "ushr v5.8h, v5.8h, #8\n\t"
        
        // Narrow to 8-bit
        "xtn v20.8b, v1.8h\n\t"
        "xtn v21.8b, v3.8h\n\t"
        "xtn v22.8b, v5.8h\n\t"
        "xtn v23.8b, v7.8h\n\t"
        
        // Store result
        "st4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%2]\n\t"
        :
        : "r"(b), "r"(f), "r"(d)
        : "memory", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", 
          "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29");
}

static inline void neon_8(uint32_t *s, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%0]\n\t"
        "st4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%1]\n\t"
        :
        : "r"(s), "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_load(uint32_t *s)
{
    __asm volatile(
        "ld4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%0]\n\t"
        :
        : "r"(s)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_store(uint32_t *d)
{
    __asm volatile(
        "st4 {v20.8b, v21.8b, v22.8b, v23.8b}, [%0]\n\t"
        :
        : "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
								  uint32_t *src, int size)
{
	uint32_t fg[8];
	uint32_t bg[8];
	uint32_t *dst = &graph->buffer[y * graph->w + x];

	if (size == 8)
	{
		neon_alpha_8(dst, src, dst);
	}
	else
	{
		memcpy(fg, src, 4 * size);
		memcpy(bg, dst, 4 * size);
		neon_alpha_8(bg, fg, bg);
		memcpy(dst, bg, 4 * size);
	}
}

static inline void graph_pixel_neon(graph_t *graph, int32_t x, int32_t y,
								  uint32_t *src, int size)
{
	uint32_t *dst = &graph->buffer[y * graph->w + x];

	if (size == 8)
	{
		neon_8(src, dst);
	}
	else
	{
		memcpy(dst, src, 4 * size);
	}
}

void graph_fill_bsp(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint32_t buf[8];

	if(g == NULL || w <= 0 || h <= 0)
		return;
	grect_t r = {x, y, w, h};
	if(!graph_insect(g, &r))
		return;

	register int32_t ex, ey;
	y = r.y;
	ex = r.x + r.w;
	ey = r.y + r.h;

	for(int i = 0; i < 8; i++)
		buf[i] = color;
	
	if(color_a(color) == 0xff) {
		neon_fill_load(buf);
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x+=8) {
				uint32_t *dst = &g->buffer[y * g->w + x];
				int pixels = ex -x;
				if(pixels >= 8)
					neon_fill_store(dst);
				else
					memcpy(dst, buf, pixels * 4);
			}
		}
	}
	else {
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x+=8) {
				graph_pixel_argb_neon(g, x, y, buf, MIN(ex-x, 8));
			}
		}
	}
}

inline void graph_blt_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	grect_t sr = {sx, sy, sw, sh};
	grect_t dr = {dx, dy, dw, dh};
	graph_insect(dst, &dr);
	if(!graph_insect_with(src, &sr, dst, &dr))
		return;

	if(dx < 0)
		sr.x -= dx;
	if(dy < 0)
		sr.y -= dy;

	register int32_t ex, ey;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		register int32_t sx = sr.x;
		register int32_t dx = dr.x;
		register int32_t offset = sy * src->w;
		for(; sx < ex; sx+=8, dx+=8) {
			graph_pixel_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 8));	
		}
	}
}

inline void graph_blt_alpha_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;

	grect_t sr = {sx, sy, sw, sh};
	grect_t dr = {dx, dy, dw, dh};
	graph_insect(dst, &dr);
	if(!graph_insect_with(src, &sr, dst, &dr))
		return;

	if(dx < 0)
		sr.x -= dx;
	if(dy < 0)
		sr.y -= dy;

	register int32_t ex, ey;
	sy = sr.y;
	dy = dr.y;
	ex = sr.x + sr.w;
	ey = sr.y + sr.h;

	for(; sy < ey; sy++, dy++) {
		register int32_t sx = sr.x;
		register int32_t dx = dr.x;
		register int32_t offset = sy * src->w;
		for(; sx < ex; sx+=8, dx+=8) {
			graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 8));	
		}
	}
}

bool  graph_2d_boosted_bsp(void) {
	return true;
}

#else

inline void graph_fill_bsp(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	graph_fill_cpu(g, x, y, w, h, color);
}

inline void graph_blt_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

inline void graph_blt_alpha_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	graph_blt_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
}

bool  graph_2d_boosted_bsp(void) {
	return false;
}
#endif

#ifdef __cplusplus
}
#endif
