#include <graph/bsp_graph.h>
#include <ewoksys/core.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openlibm.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef NEON_BOOST
#include <arm_neon.h>

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

static void gaussian_blur_neon(uint32_t* pixels, int width, int height,
                       int x, int y, int w, int h, int radius) {
    if (radius <= 0) return;
    
    // 边界检查
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    if (w <= 0 || h <= 0) return;
    
    // 创建高斯核
    int kernel_size = radius * 2 + 1;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    float sigma = radius / 2.0f;
    float sum = 0.0f;
    
    for (int i = -radius; i <= radius; i++) {
        float val = expf(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = val;
        sum += val;
    }
    
    // 归一化
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    // 临时缓冲区
    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    
    // NEON优化水平模糊
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            float32x4_t accum = vdupq_n_f32(0.0f);
            
            for (int k = -radius; k <= radius; k++) {
                int px = x + i + k;
                if (px < x) px = x;
                if (px >= x + w) px = x + w - 1;
                
                uint32_t pixel = pixels[(y + j) * width + px];
                float weight = kernel[k + radius];
                
                // 提取ARGB通道
                uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                uint16x8_t vPixel16 = vmovl_u8(vPixel);
                uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                
                // 乘以权重并累加
                accum = vmlaq_n_f32(accum, vPixelF, weight);
            }
            
            // 转换为整数并存储
            uint32x4_t result = vcvtq_u32_f32(accum);
            uint8x8_t res8 = vmovn_u16(vcombine_u16(
                vmovn_u32(result),
                vmovn_u32(result)
            ));
            temp[j * w + i] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
        }
    }
    
    // NEON优化垂直模糊
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            float32x4_t accum = vdupq_n_f32(0.0f);
            
            for (int k = -radius; k <= radius; k++) {
                int py = y + j + k;
                if (py < y) py = y;
                if (py >= y + h) py = y + h - 1;
                
                uint32_t pixel = temp[(py - y) * w + i];
                float weight = kernel[k + radius];
                
                // 提取ARGB通道
                uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                uint16x8_t vPixel16 = vmovl_u8(vPixel);
                uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                
                // 乘以权重并累加
                accum = vmlaq_n_f32(accum, vPixelF, weight);
            }
            
            // 转换为整数并存储
            uint32x4_t result = vcvtq_u32_f32(accum);
            uint8x8_t res8 = vmovn_u16(vcombine_u16(
                vmovn_u32(result),
                vmovn_u32(result)
            ));
            pixels[(y + j) * width + (x + i)] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
        }
    }
    
    free(temp);
    free(kernel);
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

	gaussian_blur_neon(g->buffer, g->w, g->h, x, y, w, h, r);
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
