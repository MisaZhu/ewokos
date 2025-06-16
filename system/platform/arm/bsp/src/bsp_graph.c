#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef NEON_BOOST
#include <arm_neon.h>

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

static bool seeded = false;
static void glass_neon(uint32_t* args, int width, int height, 
                int x, int y, int w, int h, int r) {
    // 参数检查
    if (!args || r <= 0 || w <= 0 || h <= 0 || width <= 0 || height <= 0) 
        return;
    if (x < 0 || y < 0 || x + w > width || y + h > height)
        return;

	uint32_t *tmp = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	if(tmp == NULL)
		return;
	memcpy(tmp, args, width * height * sizeof(uint32_t));

    // 使用固定随机种子确保每次效果相同
    if (!seeded) {
        srand(0x12345678);  // 固定种子值
    }

    // 预计算常用值
    int range = 2*r;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    // NEON寄存器初始化
    int32x4_t vradius = vdupq_n_s32(r);
    int32x4_t vrange = vdupq_n_s32(range);
    int32x4_t vx = vdupq_n_s32(x);
    int32x4_t vy = vdupq_n_s32(y);
    int32x4_t vx_end = vdupq_n_s32(x_end);
    int32x4_t vy_end = vdupq_n_s32(y_end);
    int32x4_t vwidth = vdupq_n_s32(width);

    // 预生成所有需要的随机数
    int total_pixels = w * h;
    int* rand_offsets = malloc(total_pixels * 2 * sizeof(int));

	for (int i = 0; i < total_pixels * 2; i++) {
		rand_offsets[i] = (rand() % range) - r;
	}

    // 处理图像区域
    int offset_index = 0;
    for (int j = y; j <= y_end; j++) {
        int32x4_t vj = vdupq_n_s32(j);
        
        for (int i = x; i <= x_end; i += 4) {
            // 处理剩余不足4个像素的情况
            int remaining = x_end - i + 1;
            if (remaining < 4) {
                for (int k = 0; k < remaining; k++) {
                    int rx = i + k + rand_offsets[offset_index++];
                    int ry = j + rand_offsets[offset_index++];
                    
                    // 边界检查
                    rx = (rx < x) ? x : ((rx > x_end) ? x_end : rx);
                    ry = (ry < y) ? y : ((ry > y_end) ? y_end : ry);
                    
                    args[j * width + i + k] = args[ry * width + rx];
                }
                break;
            }
            
            // 为4个像素生成随机偏移
            int rand_x[4], rand_y[4];
            for (int k = 0; k < 4; k++) {
                rand_x[k] = rand_offsets[offset_index++];
                rand_y[k] = rand_offsets[offset_index++];
            }
            
            // 加载随机偏移到NEON寄存器
            int32x4_t vrand_x = vld1q_s32(rand_x);
            int32x4_t vrand_y = vld1q_s32(rand_y);
            
            // 计算当前x位置
            int32x4_t vi = {i, i+1, i+2, i+3};
            
            // 计算随机位置
            int32x4_t rx = vaddq_s32(vi, vrand_x);
            int32x4_t ry = vaddq_s32(vj, vrand_y);
            
            // 边界检查
            rx = vmaxq_s32(vx, vminq_s32(vx_end, rx));
            ry = vmaxq_s32(vy, vminq_s32(vy_end, ry));
            
            // 计算随机像素位置(ry * width + rx)
            int32x4_t rpos = vmlaq_s32(rx, ry, vwidth);
            
            // 提取位置到数组
            int rpos_arr[4];
            vst1q_s32(rpos_arr, rpos);
            
            // 手动收集像素值
            uint32_t pixels[4];
            for (int k = 0; k < 4; k++) {
                pixels[k] = tmp[rpos_arr[k]];
            }
            
            // 存储结果
            vst1q_u32(&args[j * width + i], vld1q_u32(pixels));
        }
    }
    
	free(tmp);
    free(rand_offsets);
}

static void graph_glass_neon(graph_t* g, int x, int y, int w, int h, int r) {
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

	glass_neon(g->buffer, g->w, g->h, x, y, w, h, 2);
}

static void graph_gaussian_neon(graph_t* g, int x, int y, int w, int h, int r) {
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

inline void graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int r) {
    graph_glass_neon(g, x, y, w, h, r);
}

inline void graph_gaussian_bsp(graph_t* g, int x, int y, int w, int h, int r) {
    graph_gaussian_neon(g, x, y, w, h, r);
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

inline void graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int r) {
	graph_glass_cpu(g, x, y, w, h, r);
}

inline void graph_gaussian_bsp(graph_t* g, int x, int y, int w, int h, int r) {
    graph_gaussian_cpu(g, x, y, w, h, r);
}

bool  graph_2d_boosted_bsp(void) {
	return false;
}
#endif

#ifdef __cplusplus
}
#endif
