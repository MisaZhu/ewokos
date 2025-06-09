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

static inline void neon_alpha_16(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    __asm volatile(
        // 加载前景色(R,G,B,A)
        "ld4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%1]\n\t"
        
        // 将alpha_more加载到w23并广播到v30
        "mov w23, %w3\n\t"
        "dup v30.16b, w23\n\t"
        
        // 计算叠加后的alpha: alpha = alpha * alpha_more / 255
        "umull v0.8h, v23.8b, v30.8b\n\t"    // alpha * alpha_more (低8位)
        "umull2 v1.8h, v23.16b, v30.16b\n\t" // alpha * alpha_more (高8位)
        "ushr v0.8h, v0.8h, #8\n\t"          // 除以256
        "ushr v1.8h, v1.8h, #8\n\t"
        "xtn v23.8b, v0.8h\n\t"              // 窄化到8位
        "xtn2 v23.16b, v1.8h\n\t"
        
        // 后续处理(保持原有代码不变)
        "movi v28.16b, #255\n\t"
        "sub v28.16b, v28.16b, v23.16b\n\t"
        
        // Multiply foreground by its alpha
        "umull2 v1.8h, v20.16b, v23.16b\n\t"               // R high * A
        "umull v0.8h, v20.8b, v23.8b\n\t"                 // R low * A
        "umull2 v3.8h, v21.16b, v23.16b\n\t"              // G high * A
        "umull v2.8h, v21.8b, v23.8b\n\t"                 // G low * A
        "umull2 v5.8h, v22.16b, v23.16b\n\t"              // B high * A
        "umull v4.8h, v22.8b, v23.8b\n\t"                 // B low * A
        
        "ld4 {v24.16b, v25.16b, v26.16b, v27.16b}, [%0]\n\t" // load background (R,G,B,A)
        "movi v29.16b, #255\n\t"
        "sub v29.16b, v29.16b, v27.16b\n\t"                // 255 - background alpha
        
        // Multiply background by inverse of foreground alpha
        "umull2 v7.8h, v24.16b, v28.16b\n\t"              // R high * (1-A)
        "umull v6.8h, v24.8b, v28.8b\n\t"                // R low * (1-A)
        "umull2 v9.8h, v25.16b, v28.16b\n\t"              // G high * (1-A)
        "umull v8.8h, v25.8b, v28.8b\n\t"                // G low * (1-A)
        "umull2 v11.8h, v26.16b, v28.16b\n\t"             // B high * (1-A)
        "umull v10.8h, v26.8b, v28.8b\n\t"               // B low * (1-A)
        
        // Calculate resulting alpha: oa = oa + (255 - oa) * a / 255
        "umull2 v13.8h, v29.16b, v23.16b\n\t"             // (255-oa)*a high
        "umull v12.8h, v29.8b, v23.8b\n\t"               // (255-oa)*a low
        "ushr v13.8h, v13.8h, #8\n\t"                    // >> 8 (/256) high
        "ushr v12.8h, v12.8h, #8\n\t"                    // >> 8 (/256) low
        "movi v29.16b, #1\n\t"
        "umull2 v15.8h, v29.16b, v27.16b\n\t"             // 1 * oa high
        "umull v14.8h, v29.8b, v27.8b\n\t"               // 1 * oa low
        "add v13.8h, v13.8h, v15.8h\n\t"                 // sum high
        "add v12.8h, v12.8h, v14.8h\n\t"                 // sum low
        
        // Sum foreground and background components
        "add v1.8h, v1.8h, v7.8h\n\t"
        "add v0.8h, v0.8h, v6.8h\n\t"
        "add v3.8h, v3.8h, v9.8h\n\t"
        "add v2.8h, v2.8h, v8.8h\n\t"
        "add v5.8h, v5.8h, v11.8h\n\t"
        "add v4.8h, v4.8h, v10.8h\n\t"
        
        // Shift right by 8 (/256)
        "ushr v1.8h, v1.8h, #8\n\t"
        "ushr v0.8h, v0.8h, #8\n\t"
        "ushr v3.8h, v3.8h, #8\n\t"
        "ushr v2.8h, v2.8h, #8\n\t"
        "ushr v5.8h, v5.8h, #8\n\t"
        "ushr v4.8h, v4.8h, #8\n\t"
        
        // Narrow to 8-bit
        "xtn v20.8b, v0.8h\n\t"
        "xtn2 v20.16b, v1.8h\n\t"
        "xtn v21.8b, v2.8h\n\t"
        "xtn2 v21.16b, v3.8h\n\t"
        "xtn v22.8b, v4.8h\n\t"
        "xtn2 v22.16b, v5.8h\n\t"
        "xtn v23.8b, v12.8h\n\t"
        "xtn2 v23.16b, v13.8h\n\t"
        
        // Store result
        "st4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%2]\n\t"
        :
        : "r"(b), "r"(f), "r"(d), "r"(alpha_more)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", 
          "v10", "v11", "v12", "v13", "v14", "v15",
          "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "w23");
}

static inline void neon_16(uint32_t *s, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%0]\n\t"
        "st4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%1]\n\t"
        :
        : "r"(s), "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_load_16(uint32_t *s)
{
    __asm volatile(
        "ld4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%0]\n\t"
        :
        : "r"(s)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_store_16(uint32_t *d)
{
    __asm volatile(
        "st4 {v20.16b, v21.16b, v22.16b, v23.16b}, [%0]\n\t"
        :
        : "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}


static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
								  uint32_t *src, int size, uint8_t alpha_more)
{
	uint32_t fg[16] = {0};
	uint32_t bg[16] = {0};
	uint32_t *dst = &graph->buffer[y * graph->w + x];

	if (size >= 16)
	{
		neon_alpha_16(dst, src, dst, alpha_more);
	}
	else
	{
		memcpy(fg, src, 4*size);
		memcpy(bg, dst, 4*size);
		neon_alpha_16(bg, fg, bg, alpha_more);
		memcpy(dst, bg, 4*size);
	}
}

static inline void graph_pixel_neon(graph_t *graph, int32_t x, int32_t y,
								  uint32_t *src, int size)
{
	uint32_t *dst = &graph->buffer[y * graph->w + x];

	if (size == 16)
	{
		neon_16(src, dst);
	}
	else
	{
		memcpy(dst, src, 4 * size);
	}
}

void graph_fill_bsp(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint32_t buf[16] = {0};

	if(g == NULL || w <= 0 || h <= 0)
		return;
	grect_t r = {x, y, w, h};
	if(!graph_insect(g, &r))
		return;

	register int32_t ex, ey;
	y = r.y;
	ex = r.x + r.w;
	ey = r.y + r.h;

	for(int i = 0; i < 16; i++)
		buf[i] = color;
	
	if(color_a(color) == 0xff) {
		neon_fill_load_16(buf);
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x+=16) {
				uint32_t *dst = &g->buffer[y * g->w + x];
				int pixels = ex -x;
				if(pixels >= 16)
					neon_fill_store_16(dst);
				else
					memcpy(dst, buf, pixels * 4);
			}
		}
	}
	else {
		for(; y < ey; y++) {
			x = r.x;
			for(; x < ex; x+=16) {
				graph_pixel_argb_neon(g, x, y, buf, MIN(ex-x, 16), 0xFF);
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
		for(; sx < ex; sx+=16, dx+=16) {
			graph_pixel_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 16));	
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
		for(; sx < ex; sx+=16, dx+=16) {
			graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 16), alpha);	
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
