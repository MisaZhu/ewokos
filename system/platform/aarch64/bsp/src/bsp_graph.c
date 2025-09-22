#include <graph/bsp_graph.h>
#include <ewoksys/core.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openlibm.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef BSP_BOOST
#include <arm_neon.h>

#define MIN(a, b) (((a) > (b))?(b):(a))

static inline void neon_alpha_8(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    __asm volatile(
        "ld4 {v20.8b-v23.8b}, [%1]\n\t"    // Load foreground (R,G,B,A)
        
        // Load alpha_more and calculate combined alpha
        "dup v31.8b, %w3\n\t"             // Duplicate alpha_more
        "umull v0.8h, v23.8b, v31.8b\n\t"  // alpha * alpha_more
        "ushr v0.8h, v0.8h, #8\n\t"        // Divide by 256
        "xtn v23.8b, v0.8h\n\t"            // Narrow to 8-bit
        
        "movi v28.8b, #0xff\n\t"
        "sub v28.8b, v28.8b, v23.8b\n\t"   // 255 - alpha
        
        // Multiply foreground by alpha
        "umull v1.8h, v20.8b, v23.8b\n\t"
        "umull v3.8h, v21.8b, v23.8b\n\t"
        "umull v5.8h, v22.8b, v23.8b\n\t"
        
        // Load background
        "ld4 {v24.8b-v27.8b}, [%0]\n\t"
        "movi v29.8b, #0xff\n\t"
        "sub v29.8b, v29.8b, v27.8b\n\t"  // 255 - background alpha
        
        // Multiply background by inverse alpha
        "umull v2.8h, v24.8b, v28.8b\n\t"
        "umull v4.8h, v25.8b, v28.8b\n\t"
        "umull v6.8h, v26.8b, v28.8b\n\t"
        
        // Calculate resulting alpha: oa = oa + (255 - oa) * a / 255
        "umull v7.8h, v29.8b, v23.8b\n\t"
        "ushr v7.8h, v7.8h, #8\n\t"
        "movi v29.8b, #1\n\t"
        "umull v8.8h, v29.8b, v27.8b\n\t"
        "add v7.8h, v7.8h, v8.8h\n\t"
        
        // Combine foreground and background
        "add v1.8h, v1.8h, v2.8h\n\t"
        "add v3.8h, v3.8h, v4.8h\n\t"
        "add v5.8h, v5.8h, v6.8h\n\t"
        
        // Shift right by 8
        "ushr v1.8h, v1.8h, #8\n\t"
        "ushr v3.8h, v3.8h, #8\n\t"
        "ushr v5.8h, v5.8h, #8\n\t"
        
        // Narrow to 8-bit
        "xtn v20.8b, v1.8h\n\t"
        "xtn v21.8b, v3.8h\n\t"
        "xtn v22.8b, v5.8h\n\t"
        "xtn v23.8b, v7.8h\n\t"
        
        // Store result
        "st4 {v20.8b-v23.8b}, [%2]\n\t"
        :
        : "r"(b), "r"(f), "r"(d), "r"(alpha_more)
        : "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", 
          "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v31");
}

static inline void neon_8(uint32_t *s, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.8b-v23.8b}, [%0]\n\t"    // Load source
        "st4 {v20.8b-v23.8b}, [%1]\n\t"    // Store to destination
        :
        : "r"(s), "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_load_8(uint32_t *s)
{
    __asm volatile(
        "ld4 {v20.8b-v23.8b}, [%0]\n\t"    // Load source
        :
        : "r"(s)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_fill_store_8(uint32_t *d)
{
    __asm volatile(
        "st4 {v20.8b-v23.8b}, [%0]\n\t"    // Store to destination
        :
        : "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size, uint8_t alpha_more)
{
    uint32_t fg[8] = {0};
    uint32_t bg[8] = {0};
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 8)
    {
        neon_alpha_8(dst, src, dst, alpha_more);
    }
    else
    {
        memcpy(fg, src, 4*size);
        memcpy(bg, dst, 4*size);
        neon_alpha_8(bg, fg, bg, alpha_more);
        memcpy(dst, bg, 4*size);
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
    uint32_t buf[8] = {0};

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
        neon_fill_load_8(buf);
        for(; y < ey; y++) {
            x = r.x;
            for(; x < ex; x+=8) {
                uint32_t *dst = &g->buffer[y * g->w + x];
                int pixels = ex -x;
                if(pixels >= 8)
                    neon_fill_store_8(dst);
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
    
    // NEON优化水平模糊，并行处理4个像素
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i += 4) {
            if (i + 4 > w) {
                // 处理剩余不足4个像素的情况
                for (int k = i; k < w; k++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int px = x + k + m;
                        if (px < x) px = x;
                        if (px >= x + w) px = x + w - 1;
                        
                        uint32_t pixel = pixels[(y + j) * width + px];
                        float weight = kernel[m + radius];
                        
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
                    temp[j * w + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
                }
                break;
            }
            
            float32x4_t accum[4] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            
            for (int m = -radius; m <= radius; m++) {
                for (int k = 0; k < 4; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    float weight = kernel[m + radius];
                    
                    // 提取ARGB通道
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // 乘以权重并累加
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // 转换为整数并存储
            for (int k = 0; k < 4; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                temp[j * w + i + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
        }
    }
    
    // NEON优化垂直模糊，并行处理4个像素
    for (int j = 0; j < h; j += 4) {
        if (j + 4 > h) {
            // 处理剩余不足4个像素的情况
            for (int k = j; k < h; k++) {
                for (int i = 0; i < w; i++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int py = y + k + m;
                        if (py < y) py = y;
                        if (py >= y + h) py = y + h - 1;
                        
                        uint32_t pixel = temp[(py - y) * w + i];
                        float weight = kernel[m + radius];
                        
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
                    pixels[(y + k) * width + (x + i)] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
                }
            }
            break;
        }
        
        for (int i = 0; i < w; i++) {
            float32x4_t accum[4] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            
            for (int m = -radius; m <= radius; m++) {
                for (int k = 0; k < 4; k++) {
                    int py = y + j + k + m;
                    if (py < y) py = y;
                    if (py >= y + h) py = y + h - 1;
                    
                    uint32_t pixel = temp[(py - y) * w + i];
                    float weight = kernel[m + radius];
                    
                    // 提取ARGB通道
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // 乘以权重并累加
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // 转换为整数并存储
            for (int k = 0; k < 4; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                pixels[(y + j + k) * width + (x + i)] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
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

void graph_scale_tof_bsp(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0f ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;
    
    int dst_w = dst->w;
    int dst_h = dst->h;
    int src_w = g->w;
    int src_h = g->h;
    uint32_t* src_buffer = g->buffer;
    uint32_t* dst_buffer = dst->buffer;
    
    // Process 4 pixels at a time using NEON
    for(int i = 0; i < dst_h; i++) {
        float gi = (float)i / scale;
        int gi0 = (int)gi;
        float gi_frac = gi - gi0;
        
        if(gi0 >= src_h-1) gi0 = src_h-2;
        if(gi0 < 0) gi0 = 0;
        
        int j = 0;
        // Process 4 pixels in parallel using NEON
        for(; j <= dst_w - 4; j += 4) {
            // Calculate source coordinates for 4 pixels
            float gj[4];
            int gj0[4];
            float gj_frac[4];
            
            for(int k = 0; k < 4; k++) {
                gj[k] = (float)(j + k) / scale;
                gj0[k] = (int)gj[k];
                gj_frac[k] = gj[k] - gj0[k];
                
                if(gj0[k] >= src_w-1) gj0[k] = src_w-2;
                if(gj0[k] < 0) gj0[k] = 0;
            }
            
            // Load 4 sets of 4 neighboring pixels (16 pixels total)
            // Each set: p00, p01, p10, p11
            uint8x8_t pixels_r[4][4];
            uint8x8_t pixels_g[4][4];
            uint8x8_t pixels_b[4][4];
            uint8x8_t pixels_a[4][4];
            
            for(int k = 0; k < 4; k++) {
                // Load 4 pixels for each set
                uint32_t p00 = src_buffer[gi0 * src_w + gj0[k]];
                uint32_t p01 = src_buffer[gi0 * src_w + gj0[k] + 1];
                uint32_t p10 = src_buffer[(gi0 + 1) * src_w + gj0[k]];
                uint32_t p11 = src_buffer[(gi0 + 1) * src_w + gj0[k] + 1];
                
                // Extract RGBA components and duplicate to 8x8 vectors
                pixels_r[k][0] = vdup_n_u8((p00 >> 16) & 0xFF);
                pixels_g[k][0] = vdup_n_u8((p00 >> 8) & 0xFF);
                pixels_b[k][0] = vdup_n_u8(p00 & 0xFF);
                pixels_a[k][0] = vdup_n_u8((p00 >> 24) & 0xFF);
                
                pixels_r[k][1] = vdup_n_u8((p01 >> 16) & 0xFF);
                pixels_g[k][1] = vdup_n_u8((p01 >> 8) & 0xFF);
                pixels_b[k][1] = vdup_n_u8(p01 & 0xFF);
                pixels_a[k][1] = vdup_n_u8((p01 >> 24) & 0xFF);
                
                pixels_r[k][2] = vdup_n_u8((p10 >> 16) & 0xFF);
                pixels_g[k][2] = vdup_n_u8((p10 >> 8) & 0xFF);
                pixels_b[k][2] = vdup_n_u8(p10 & 0xFF);
                pixels_a[k][2] = vdup_n_u8((p10 >> 24) & 0xFF);
                
                pixels_r[k][3] = vdup_n_u8((p11 >> 16) & 0xFF);
                pixels_g[k][3] = vdup_n_u8((p11 >> 8) & 0xFF);
                pixels_b[k][3] = vdup_n_u8(p11 & 0xFF);
                pixels_a[k][3] = vdup_n_u8((p11 >> 24) & 0xFF);
            }
            
            // Create interpolation weight vectors
            uint8x8_t gi_frac_vec = vdup_n_u8((uint8_t)(gi_frac * 255));
            uint8x8_t gi_frac_inv_vec = vdup_n_u8((uint8_t)((1.0f - gi_frac) * 255));
            
            uint32_t result_pixels[4];
            
            // Perform bilinear interpolation for each of the 4 pixels
            for(int k = 0; k < 4; k++) {
                uint8x8_t gj_frac_vec = vdup_n_u8((uint8_t)(gj_frac[k] * 255));
                uint8x8_t gj_frac_inv_vec = vdup_n_u8((uint8_t)((1.0f - gj_frac[k]) * 255));
                
                // Interpolate horizontally first
                uint16x8_t r_h0 = vmull_u8(pixels_r[k][0], gj_frac_inv_vec);
                uint16x8_t r_h1 = vmull_u8(pixels_r[k][1], gj_frac_vec);
                uint16x8_t r_horizontal = vaddq_u16(r_h0, r_h1);
                
                uint16x8_t g_h0 = vmull_u8(pixels_g[k][0], gj_frac_inv_vec);
                uint16x8_t g_h1 = vmull_u8(pixels_g[k][1], gj_frac_vec);
                uint16x8_t g_horizontal = vaddq_u16(g_h0, g_h1);
                
                uint16x8_t b_h0 = vmull_u8(pixels_b[k][0], gj_frac_inv_vec);
                uint16x8_t b_h1 = vmull_u8(pixels_b[k][1], gj_frac_vec);
                uint16x8_t b_horizontal = vaddq_u16(b_h0, b_h1);
                
                uint16x8_t a_h0 = vmull_u8(pixels_a[k][0], gj_frac_inv_vec);
                uint16x8_t a_h1 = vmull_u8(pixels_a[k][1], gj_frac_vec);
                uint16x8_t a_horizontal = vaddq_u16(a_h0, a_h1);
                
                uint16x8_t r_h2 = vmull_u8(pixels_r[k][2], gj_frac_inv_vec);
                uint16x8_t r_h3 = vmull_u8(pixels_r[k][3], gj_frac_vec);
                uint16x8_t r_horizontal2 = vaddq_u16(r_h2, r_h3);
                
                uint16x8_t g_h2 = vmull_u8(pixels_g[k][2], gj_frac_inv_vec);
                uint16x8_t g_h3 = vmull_u8(pixels_g[k][3], gj_frac_vec);
                uint16x8_t g_horizontal2 = vaddq_u16(g_h2, g_h3);
                
                uint16x8_t b_h2 = vmull_u8(pixels_b[k][2], gj_frac_inv_vec);
                uint16x8_t b_h3 = vmull_u8(pixels_b[k][3], gj_frac_vec);
                uint16x8_t b_horizontal2 = vaddq_u16(b_h2, b_h3);
                
                uint16x8_t a_h2 = vmull_u8(pixels_a[k][2], gj_frac_inv_vec);
                uint16x8_t a_h3 = vmull_u8(pixels_a[k][3], gj_frac_vec);
                uint16x8_t a_horizontal2 = vaddq_u16(a_h2, a_h3);
                
                // Interpolate vertically
                uint16x8_t r_v1 = vmull_u8(vshrn_n_u16(r_horizontal, 8), gi_frac_inv_vec);
                uint16x8_t r_v2 = vmull_u8(vshrn_n_u16(r_horizontal2, 8), gi_frac_vec);
                uint16x8_t r_final = vaddq_u16(r_v1, r_v2);
                
                uint16x8_t g_v1 = vmull_u8(vshrn_n_u16(g_horizontal, 8), gi_frac_inv_vec);
                uint16x8_t g_v2 = vmull_u8(vshrn_n_u16(g_horizontal2, 8), gi_frac_vec);
                uint16x8_t g_final = vaddq_u16(g_v1, g_v2);
                
                uint16x8_t b_v1 = vmull_u8(vshrn_n_u16(b_horizontal, 8), gi_frac_inv_vec);
                uint16x8_t b_v2 = vmull_u8(vshrn_n_u16(b_horizontal2, 8), gi_frac_vec);
                uint16x8_t b_final = vaddq_u16(b_v1, b_v2);
                
                uint16x8_t a_v1 = vmull_u8(vshrn_n_u16(a_horizontal, 8), gi_frac_inv_vec);
                uint16x8_t a_v2 = vmull_u8(vshrn_n_u16(a_horizontal2, 8), gi_frac_vec);
                uint16x8_t a_final = vaddq_u16(a_v1, a_v2);
                
                // Extract final pixel value
                uint8x8_t r_final_8 = vshrn_n_u16(r_final, 8);
                uint8x8_t g_final_8 = vshrn_n_u16(g_final, 8);
                uint8x8_t b_final_8 = vshrn_n_u16(b_final, 8);
                uint8x8_t a_final_8 = vshrn_n_u16(a_final, 8);
                
                uint8_t r = vget_lane_u8(r_final_8, 0);
                uint8_t g_val = vget_lane_u8(g_final_8, 0);
                uint8_t b = vget_lane_u8(b_final_8, 0);
                uint8_t a = vget_lane_u8(a_final_8, 0);
                
                result_pixels[k] = (a << 24) | (r << 16) | (g_val << 8) | b;
            }
            
            // Store 4 result pixels
            vst1q_u32(&dst_buffer[i * dst_w + j], vld1q_u32(result_pixels));
        }
        
        // Handle remaining pixels
        for(; j < dst_w; j++) {
            float gj = (float)j / scale;
            int gj0 = (int)gj;
            float gj_frac = gj - gj0;
            
            if(gj0 >= src_w-1) gj0 = src_w-2;
            if(gj0 < 0) gj0 = 0;
            
            uint32_t p00 = src_buffer[gi0 * src_w + gj0];
            uint32_t p01 = src_buffer[gi0 * src_w + gj0 + 1];
            uint32_t p10 = src_buffer[(gi0 + 1) * src_w + gj0];
            uint32_t p11 = src_buffer[(gi0 + 1) * src_w + gj0 + 1];
            
            uint8_t r00 = (p00 >> 16) & 0xFF;
            uint8_t g00 = (p00 >> 8) & 0xFF;
            uint8_t b00 = p00 & 0xFF;
            uint8_t a00 = (p00 >> 24) & 0xFF;
            
            uint8_t r01 = (p01 >> 16) & 0xFF;
            uint8_t g01 = (p01 >> 8) & 0xFF;
            uint8_t b01 = p01 & 0xFF;
            uint8_t a01 = (p01 >> 24) & 0xFF;
            
            uint8_t r10 = (p10 >> 16) & 0xFF;
            uint8_t g10 = (p10 >> 8) & 0xFF;
            uint8_t b10 = p10 & 0xFF;
            uint8_t a10 = (p10 >> 24) & 0xFF;
            
            uint8_t r11 = (p11 >> 16) & 0xFF;
            uint8_t g11 = (p11 >> 8) & 0xFF;
            uint8_t b11 = p11 & 0xFF;
            uint8_t a11 = (p11 >> 24) & 0xFF;
            
            uint8_t r = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * r00 + gj_frac * r01) + 
                                 gi_frac * ((1 - gj_frac) * r10 + gj_frac * r11));
            uint8_t g_val = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * g00 + gj_frac * g01) + 
                                     gi_frac * ((1 - gj_frac) * g10 + gj_frac * g11));
            uint8_t b = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * b00 + gj_frac * b01) + 
                                 gi_frac * ((1 - gj_frac) * b10 + gj_frac * b11));
            uint8_t a = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * a00 + gj_frac * a01) + 
                                 gi_frac * ((1 - gj_frac) * a10 + gj_frac * a11));
            
            dst_buffer[i * dst_w + j] = (a << 24) | (r << 16) | (g_val << 8) | b;
        }
    }
}

#endif

#ifdef __cplusplus
}
#endif
