#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef NEON_BOOST

#define MIN(a, b) (((a) > (b))?(b):(a))

static inline void neon_alpha_8(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
	__asm volatile(
		"vld4.8    {d20-d23},[%1]\n\t" // 加载前景色(R,G,B,A)
		
		// 新增：将alpha_more加载到d31并计算叠加alpha
		"mov        r3, %3\n\t"       // 加载alpha_more
		"vdup.u8    d31, r3\n\t"

		"vmull.u8   q0, d23, d31\n\t"  // alpha * alpha_more
		"vshr.u16   q0, q0, #8\n\t"    // 除以256
		"vmovn.u16  d23, q0\n\t"       // 窄化到8位
		
		"vmov.u8	d28, 0xff\n\t"
		"vsub.u8	d28, d28, d23\n\t"
		"vmull.u8  q1,d20,d23\n\t"
		"vmull.u8  q3,d21,d23\n\t"
		"vmull.u8  q5,d22,d23\n\t"

		"vld4.8    {d24-d27},[%0]\n\t" // load background
		"vmov.u8	d29, 0xff\n\t"
		"vsub.u8	d29, d29, d27\n\t"
		"vmull.u8  q2,d24,d28\n\t"
		"vmull.u8  q4,d25,d28\n\t"
		"vmull.u8  q6,d26,d28\n\t" // apply alpha

		/*oa = oa + (255 - oa) * a / 255;*/
		"vmull.u8	q7, d29, d23\n\t"
		"vshr.u16  	q7,q7,#8\n\t"
		"vmov.u8	d29, 0x1\n\t"
		"vmull.u8	q8, d29, d27\n\t"
		"vadd.u16  	q7,q7,q8\n\t"

		"vadd.u16  q1,q1,q2\n\t"
		"vadd.u16  q3,q3,q4\n\t"
		"vadd.u16  q5,q5,q6\n\t"

		"vshr.u16  q1,q1,#8\n\t"
		"vshr.u16  q3,q3,#8\n\t"
		"vshr.u16  q5,q5,#8\n\t"

		"vmovn.u16 d20,q1\n\t"
		"vmovn.u16 d21,q3\n\t"
		"vmovn.u16 d22,q5\n\t"
		"vmovn.u16   d23,q7\n\t"

		"vst4.8   {d20-d23},[%2]\n\t"
		:
		: "r"(b), "r"(f), "r"(d), "r"(alpha_more)
		: "memory", "q0", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d31");
}

static inline void neon_8(uint32_t *s, uint32_t *d)
{
	__asm volatile(
		"vld4.8    {d20-d23},[%0]\n\t" // load foreground
		"vst4.8   {d20-d23},[%1]\n\t"
		:
		: "r"(s), "r"(d)
		: "memory");
	return;
}

static inline void neon_fill_load(uint32_t *s)
{
	__asm volatile(
		"vld4.8    {d20-d23},[%0]\n\t" // load foreground
		:
		: "r"(s)
		: "memory");
	return;
}


static inline void neon_fill_store(uint32_t *d)
{
	__asm volatile(
		"vst4.8   {d20-d23},[%0]\n\t"
		:
		: "r"(d)
		: "memory");
	return;
}

static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
								  uint32_t *src, int size, uint8_t alpha)
{
	uint32_t fg[8];
	uint32_t bg[8];
	uint32_t *dst = &graph->buffer[y * graph->w + x];

	if (size >= 8)
	{
		neon_alpha_8(dst, src, dst, alpha);
	}
	else
	{
		memcpy(fg, src, 4 * size);
		memcpy(bg, dst, 4 * size);
		neon_alpha_8(bg, fg, bg, alpha);
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
				graph_pixel_argb_neon(g, x, y, buf, MIN(ex-x, 8), 0xFF);
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
			graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 8), alpha);	
		}
	}
}

static void graph_glass_neon(graph_t* g, int x, int y, int w, int h, int8_t r) {
    if (g == NULL || r == 0) {
        return;
    }

	grect_t ir = {x, y, w, h};
	if(!graph_insect(g, &ir))
		return;
	x = ir.x;
	y = ir.y;
	w = ir.w;
	h = ir.h;

    // 分配临时缓冲区
	uint32_t sz = w * h * sizeof(uint32_t);
    uint32_t* buffer = (uint32_t*)malloc(sz);
    if (buffer == NULL) {
        return;
    }

	if(ir.x == 0 && ir.y == 0 && ir.w == g->w && ir.h == g->h) {
		memcpy(buffer, g->buffer, sz);
	}
	else {
		for (int iy = 0; iy < h; iy++) {
			int off = iy*w;
			for (int ix = 0; ix < w; ix++) {
				buffer[off + ix] = graph_get_pixel(g, x + ix, y + iy);
			}
		}
	}

    // 分离的盒模糊：水平方向
    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    if (temp == NULL) {
        free(buffer);
        return;
    }

    for (int iy = 0; iy < h; iy++) {
		int off = iy*w;
        for (int ix = 0; ix < w; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
			uint8_t alpha = 0xFF;
            for (int dx = -r; dx <= r; dx++) {
                int nx = ix + dx;
                if (nx >= 0 && nx < w) {
                    uint32_t pixel = buffer[off + nx];
                    alpha = (pixel >> 24) & 0xff;
                    sumR += (pixel >> 16) & 0xff;
                    sumG += (pixel >> 8) & 0xff;
                    sumB += pixel & 0xff;
                    count++;
                }
            }
            uint8_t avgR = sumR / count;
            uint8_t avgG = sumG / count;
            uint8_t avgB = sumB / count;
            temp[off + ix] = (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB;
        }
    }

    // 分离的盒模糊：垂直方向
    for (int iy = 0; iy < h; iy++) {
        for (int ix = 0; ix < w; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
			uint8_t alpha = 0xFF;
            for (int dy = -r; dy <= r; dy++) {
                int ny = iy + dy;
                if (ny >= 0 && ny < h) {
                    uint32_t pixel = temp[ny * w + ix];
                    alpha = (pixel >> 24) & 0xff;
                    sumR += (pixel >> 16) & 0xff;
                    sumG += (pixel >> 8) & 0xff;
                    sumB += pixel & 0xff;
                    count++;
                }
            }
            uint8_t avgR = sumR / count;
            uint8_t avgG = sumG / count;
            uint8_t avgB = sumB / count;
            graph_pixel(g, x + ix, y + iy, (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB);
        }
    }

    // 释放缓冲区
    free(buffer);
    free(temp);
}

inline void graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int8_t r) {
    graph_glass_neon(g, x, y, w, h, r);
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

inline void graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int8_t r) {
	graph_glass_cpu(g, x, y, w, h, r);
}

bool  graph_2d_boosted_bsp(void) {
	return false;
}
#endif

#ifdef __cplusplus
}
#endif
