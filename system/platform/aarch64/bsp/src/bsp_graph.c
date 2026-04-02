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

static inline void neon_8(uint32_t *s, uint32_t *d)
{
    __asm volatile(
        "ld4 {v20.8b-v23.8b}, [%0]\n\t"    // Load source
        "st4 {v20.8b-v23.8b}, [%1]\n\t"    // Store to destination
        :
        : "r"(s), "r"(d)
        : "memory", "v20", "v21", "v22", "v23");
}

static inline void neon_alpha_8(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    __asm volatile(
        // 预加载更多数据到缓存
        "prfm pldl1keep, [%1, #128]\n\t"
        "prfm pldl1keep, [%0, #128]\n\t"
        
        // Load foreground (R,G,B,A)
        "ld4 {v20.8b-v23.8b}, [%1]\n\t"
        
        // Load alpha_more and calculate combined alpha
        "dup v31.8b, %w3\n\t"             // Duplicate alpha_more
        "umull v0.8h, v23.8b, v31.8b\n\t"  // alpha * alpha_more
        "ushr v0.8h, v0.8h, #8\n\t"        // Divide by 256
        "xtn v23.8b, v0.8h\n\t"            // Narrow to 8-bit
        
        "movi v28.8b, #0xff\n\t"
        "sub v28.8b, v28.8b, v23.8b\n\t"   // 255 - alpha
        
        // Load background
        "ld4 {v24.8b-v27.8b}, [%0]\n\t"
        "movi v29.8b, #0xff\n\t"
        "sub v29.8b, v29.8b, v27.8b\n\t"  // 255 - background alpha
        
        // 优化指令顺序，提高并行度
        "umull v1.8h, v20.8b, v23.8b\n\t"  // 前景 R * alpha
        "umull v2.8h, v24.8b, v28.8b\n\t"  // 背景 R * (255 - alpha)
        "umull v3.8h, v21.8b, v23.8b\n\t"  // 前景 G * alpha
        "umull v4.8h, v25.8b, v28.8b\n\t"  // 背景 G * (255 - alpha)
        "umull v5.8h, v22.8b, v23.8b\n\t"  // 前景 B * alpha
        "umull v6.8h, v26.8b, v28.8b\n\t"  // 背景 B * (255 - alpha)
        
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
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 8)
    {
        neon_alpha_8(dst, src, dst, alpha_more);
    }
    else
    {
        // 对于 size < 8 的情况，使用 memcpy 处理边界
        uint32_t fg[8] = {0};
        uint32_t bg[8] = {0};
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

    // Loop unrolling, process 2 rows at a time for better instruction-level parallelism
    for(; sy < ey - 1; sy += 2, dy += 2) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset1 = sy * src->w;
        register int32_t offset2 = (sy + 1) * src->w;
        
        // Preload next row data to cache
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&src->buffer[offset1]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[dy * dst->w + dx]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[(dy + 1) * dst->w + dx]));
        
        for(; sx < ex - 7; sx += 8, dx += 8) {
            // Process two rows in parallel
            graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset1 + sx], 8, alpha);
            graph_pixel_argb_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], 8, alpha);
        }
        
        // Process remaining pixels
        if(sx < ex) {
            int remain = ex - sx;
            graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset1 + sx], remain, alpha);
            graph_pixel_argb_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], remain, alpha);
        }
    }
    
    // Process last row (if total rows is odd)
    if(sy < ey) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset = sy * src->w;
        
        for(; sx < ex; sx += 8, dx += 8) {
            graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex - sx, 8), alpha);
        }
    }
}

static inline void neon_mask_alpha_8(uint32_t *dst, uint32_t *src)
{
    __asm volatile(
        // Load 8 dst pixels (RGBA)
        "ld4 {v20.8b-v23.8b}, [%0]\n\t"
        // Load 8 src pixels (RGBA)
        "ld4 {v24.8b-v27.8b}, [%1]\n\t"
        
        // v23 = dst_a, v27 = src_a
        // Create zero vector
        "movi v28.8b, #0\n\t"
        // Compare src_a > 0 (cmhi returns 0xFF where true, 0x00 where false)
        "cmhi v28.8b, v27.8b, v28.8b\n\t"  // v28: 0xFF where src_a > 0, 0x00 where src_a == 0
        
        // Compare dst_a > src_a
        "cmhi v29.8b, v23.8b, v27.8b\n\t"  // v29: 0xFF where dst_a > src_a
        
        // Create mask for src_a == 0: set all channels to 0
        "and v20.8b, v20.8b, v28.8b\n\t"    // R
        "and v21.8b, v21.8b, v28.8b\n\t"    // G
        "and v22.8b, v22.8b, v28.8b\n\t"    // B
        "and v23.8b, v23.8b, v28.8b\n\t"    // A
        
        // For dst_a > src_a: keep dst RGB, replace alpha with src_a
        // First, mask src_a where condition is true
        "and v30.8b, v27.8b, v29.8b\n\t"
        // Mask dst_a where condition is false
        "mvn v28.8b, v29.8b\n\t"
        "and v23.8b, v23.8b, v28.8b\n\t"
        // Combine
        "orr v23.8b, v23.8b, v30.8b\n\t"
        
        // Store result
        "st4 {v20.8b-v23.8b}, [%0]\n\t"
        :
        : "r"(dst), "r"(src)
        : "memory", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", 
          "v28", "v29", "v30");
}

static inline void graph_pixel_alpha_mask_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 8)
    {
        neon_mask_alpha_8(dst, src);
    }
    else
    {
        // For size < 8, use memcpy to handle boundaries
        uint32_t src_buf[8] = {0};
        uint32_t dst_buf[8] = {0};
        memcpy(src_buf, src, 4 * size);
        memcpy(dst_buf, dst, 4 * size);
        neon_mask_alpha_8(dst_buf, src_buf);
        memcpy(dst, dst_buf, 4 * size);
    }
}

inline void graph_blt_alpha_mask_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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

    // Loop unrolling, process 2 rows at a time for better instruction-level parallelism
    for(; sy < ey - 1; sy += 2, dy += 2) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset1 = sy * src->w;
        register int32_t offset2 = (sy + 1) * src->w;
        
        // Preload next row data to cache
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&src->buffer[offset1]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[dy * dst->w + dx]));
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&dst->buffer[(dy + 1) * dst->w + dx]));
        
        for(; sx < ex - 7; sx += 8, dx += 8) {
            // Process two rows in parallel
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset1 + sx], 8);
            graph_pixel_alpha_mask_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], 8);
        }
        
        // Process remaining pixels
        if(sx < ex) {
            int remain = ex - sx;
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset1 + sx], remain);
            graph_pixel_alpha_mask_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], remain);
        }
    }
    
    // Process last row (if total rows is odd)
    if(sy < ey) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset = sy * src->w;
        
        for(; sx < ex; sx += 8, dx += 8) {
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex - sx, 8));
        }
    }
}


static void glass_neon(uint32_t* args, int width, int height, 
                int x, int y, int w, int h, int r) {
    // 参数检查
    if (!args || r <= 0 || w <= 0 || h <= 0 || width <= 0 || height <= 0) 
        return;
    if (x < 0 || y < 0 || x + w > width || y + h > height)
        return;

    // 使用固定随机种子确保每次效果相同
    srand(0x12345678);  // 每次调用都使用相同的种子值

    // 预计算常用值
    int range = 2*r;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    // NEON寄存器初始化
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
        
        // 预加载数据到缓存
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&args[j * width + x]));
        
        for (int i = x; i <= x_end; i += 8) {
            // 处理剩余不足8个像素的情况
            int remaining = x_end - i + 1;
            if (remaining < 8) {
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
            
            // 为8个像素生成随机偏移
            int rand_x[8], rand_y[8];
            for (int k = 0; k < 8; k++) {
                rand_x[k] = rand_offsets[offset_index++];
                rand_y[k] = rand_offsets[offset_index++];
            }
            
            // 处理前4个像素
            int32x4_t vrand_x0 = vld1q_s32(rand_x);
            int32x4_t vrand_y0 = vld1q_s32(rand_y);
            int32x4_t vi0 = {i, i+1, i+2, i+3};
            int32x4_t rx0 = vaddq_s32(vi0, vrand_x0);
            int32x4_t ry0 = vaddq_s32(vj, vrand_y0);
            rx0 = vmaxq_s32(vx, vminq_s32(vx_end, rx0));
            ry0 = vmaxq_s32(vy, vminq_s32(vy_end, ry0));
            int32x4_t rpos0 = vmlaq_s32(rx0, ry0, vwidth);
            
            // 处理后4个像素
            int32x4_t vrand_x1 = vld1q_s32(&rand_x[4]);
            int32x4_t vrand_y1 = vld1q_s32(&rand_y[4]);
            int32x4_t vi1 = {i+4, i+5, i+6, i+7};
            int32x4_t rx1 = vaddq_s32(vi1, vrand_x1);
            int32x4_t ry1 = vaddq_s32(vj, vrand_y1);
            rx1 = vmaxq_s32(vx, vminq_s32(vx_end, rx1));
            ry1 = vmaxq_s32(vy, vminq_s32(vy_end, ry1));
            int32x4_t rpos1 = vmlaq_s32(rx1, ry1, vwidth);
            
            // 提取位置到数组
            int rpos_arr0[4], rpos_arr1[4];
            vst1q_s32(rpos_arr0, rpos0);
            vst1q_s32(rpos_arr1, rpos1);
            
            // 批量收集像素值
            uint32_t pixels[8];
            for (int k = 0; k < 4; k++) {
                pixels[k] = args[rpos_arr0[k]];
                pixels[k + 4] = args[rpos_arr1[k]];
            }
            
            // 批量存储结果
            uint32x4_t pixels0 = vld1q_u32(pixels);
            uint32x4_t pixels1 = vld1q_u32(&pixels[4]);
            vst1q_u32(&args[j * width + i], pixels0);
            vst1q_u32(&args[j * width + i + 4], pixels1);
        }
    }
    
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
    
    // NEON优化水平模糊，并行处理8个像素
    for (int j = 0; j < h; j++) {
        // 预加载数据到缓存
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&pixels[(y + j) * width + x]));
        
        for (int i = 0; i < w; i += 8) {
            if (i + 8 > w) {
                // 处理剩余不足8个像素的情况
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
            
            // 并行处理8个像素
            float32x4_t accum[8] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // 批量加载8个像素
                uint32x4_t pixels0 = vld1q_u32(&pixels[(y + j) * width + x + i]);
                uint32x4_t pixels1 = vld1q_u32(&pixels[(y + j) * width + x + i + 4]);
                
                // 处理前4个像素
                for (int k = 0; k < 4; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    
                    // 提取ARGB通道
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // 乘以权重并累加
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
                
                // 处理后4个像素
                for (int k = 4; k < 8; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    
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
            for (int k = 0; k < 8; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                temp[j * w + i + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
        }
    }
    
    // NEON优化垂直模糊，并行处理8个像素
    for (int j = 0; j < h; j += 8) {
        if (j + 8 > h) {
            // 处理剩余不足8个像素的情况
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
            // 预加载数据到缓存
            __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&temp[i]));
            
            // 并行处理8个像素
            float32x4_t accum[8] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // 处理8个像素
                for (int k = 0; k < 8; k++) {
                    int py = y + j + k + m;
                    if (py < y) py = y;
                    if (py >= y + h) py = y + h - 1;
                    
                    uint32_t pixel = temp[(py - y) * w + i];
                    
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
            for (int k = 0; k < 8; k++) {
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

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define WEIGHT_TABLE_SIZE 256
static float weight_table[WEIGHT_TABLE_SIZE];
static int weight_table_initialized = 0;

static inline float fast_sinf(float x) {
    const float PI = 3.141592653589793f;
    const float TWO_PI = 6.283185307179586f;
    
    x = fmodf(x, TWO_PI);
    if(x < -PI) x += TWO_PI;
    else if(x > PI) x -= TWO_PI;
    
    const float x2 = x * x;
    const float x4 = x2 * x2;
    return x * (0.99999994f + x2 * (-0.16666665f + x2 * 0.008333329f));
}

static inline float lanczos_sinc(float x) {
    x = fabsf(x);
    if(x < 0.0001f)
        return 1.0f;
    if(x >= 3.0f)
        return 0.0f;
    const float PI = 3.141592653589793f;
    float pi_x = PI * x;
    return fast_sinf(pi_x) / pi_x;
}

static inline float lanczos_kernel(float x, int a) {
    x = x < 0 ? -x : x;
    if(x < (float)a) {
        return lanczos_sinc(x) * lanczos_sinc(x / (float)a);
    }
    return 0.0f;
}

static void init_weight_table(void) {
    if(weight_table_initialized) return;
    
    for(int i = 0; i < WEIGHT_TABLE_SIZE; i++) {
        float x = (float)i / (WEIGHT_TABLE_SIZE - 1) * 6.0f - 3.0f;
        weight_table[i] = lanczos_kernel(x, 3);
    }
    
    weight_table_initialized = 1;
}

static inline float get_weight_fast(float x) {
    float normalized = (x + 3.0f) * (1.0f / 6.0f);
    int index = (int)(normalized * (WEIGHT_TABLE_SIZE - 1));
    if(index < 0) index = 0;
    if(index >= WEIGHT_TABLE_SIZE) index = WEIGHT_TABLE_SIZE - 1;
    return weight_table[index];
}

void graph_scale_tof_bsp(graph_t* g, graph_t* dst, double scale) {
    init_weight_table();
    
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    float effective_scale = scale < 1.0f ? scale : 1.0f;
    int lanczos_a = 2;  // Changed from 3 to 2 for Lanczos2
    float inv_scale = 1.0f / scale;
    float inv_weight_table_size = 1.0f / (WEIGHT_TABLE_SIZE - 1);

    for(int i = 0; i < dst->h; i++) {
        float gi = (float)i * inv_scale;

        int j = 0;
        for(; j <= dst->w - 8; j += 8) {
            float center_x[8];
            float center_y = gi;
            int start_x[8], end_x[8];
            int start_y, end_y;

            for(int k = 0; k < 8; k++) {
                center_x[k] = (float)(j + k) * inv_scale;
            }

            float adjusted_a = (float)lanczos_a / effective_scale;
            start_y = (int)floorf(center_y - adjusted_a);
            end_y = (int)floorf(center_y + adjusted_a) + 1;

            for(int k = 0; k < 8; k++) {
                start_x[k] = (int)floorf(center_x[k] - adjusted_a);
                end_x[k] = (int)floorf(center_x[k] + adjusted_a) + 1;
            }

            float32x4_t sum_r[2] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_g[2] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_b[2] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_a[2] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};
            float32x4_t sum_weight[2] = {vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)};

            for(int y = start_y; y <= end_y; y++) {
                float ky_arg = (y - center_y) * effective_scale;
                float ky = get_weight_fast(ky_arg);
                if(ky == 0.0f) continue;
                float32x4_t ky_vec = vdupq_n_f32(ky);

                int max_end_x = start_x[0];
                for(int k = 1; k < 8; k++) {
                    if(end_x[k] > max_end_x) max_end_x = end_x[k];
                }

                int cy = CLAMP(y, 0, g->h - 1);
                uint32_t* row_ptr = &g->buffer[cy * g->w];

                for(int x = start_x[0]; x <= max_end_x; x++) {
                    float kx[8];
                    for(int k = 0; k < 8; k++) {
                        if(x >= start_x[k] && x <= end_x[k]) {
                            float kx_arg = ((float)x - center_x[k]) * effective_scale;
                            kx[k] = get_weight_fast(kx_arg);
                        } else {
                            kx[k] = 0.0f;
                        }
                    }

                    float32x4_t kx0 = (float32x4_t){kx[0], kx[1], kx[2], kx[3]};
                    float32x4_t kx1 = (float32x4_t){kx[4], kx[5], kx[6], kx[7]};

                    int cx = CLAMP(x, 0, g->w - 1);
                    uint32_t p = row_ptr[cx];

                    float r_val = (float)((p >> 16) & 0xFF);
                    float g_val = (float)((p >> 8) & 0xFF);
                    float b_val = (float)(p & 0xFF);
                    float a_val = (float)((p >> 24) & 0xFF);

                    float32x4_t r_f = vdupq_n_f32(r_val);
                    float32x4_t g_f = vdupq_n_f32(g_val);
                    float32x4_t b_f = vdupq_n_f32(b_val);
                    float32x4_t a_f = vdupq_n_f32(a_val);

                    float32x4_t weight0 = vmulq_f32(kx0, ky_vec);
                    float32x4_t weight1 = vmulq_f32(kx1, ky_vec);

                    sum_r[0] = vfmaq_f32(sum_r[0], r_f, weight0);
                    sum_r[1] = vfmaq_f32(sum_r[1], r_f, weight1);
                    sum_g[0] = vfmaq_f32(sum_g[0], g_f, weight0);
                    sum_g[1] = vfmaq_f32(sum_g[1], g_f, weight1);
                    sum_b[0] = vfmaq_f32(sum_b[0], b_f, weight0);
                    sum_b[1] = vfmaq_f32(sum_b[1], b_f, weight1);
                    sum_a[0] = vfmaq_f32(sum_a[0], a_f, weight0);
                    sum_a[1] = vfmaq_f32(sum_a[1], a_f, weight1);
                    sum_weight[0] = vfmaq_f32(sum_weight[0], vdupq_n_f32(1.0f), weight0);
                    sum_weight[1] = vfmaq_f32(sum_weight[1], vdupq_n_f32(1.0f), weight1);
                }
            }

            float32x4_t inv_weight0 = vrecpeq_f32(sum_weight[0]);
            float32x4_t inv_weight1 = vrecpeq_f32(sum_weight[1]);
            inv_weight0 = vmulq_f32(inv_weight0, vrecpsq_f32(sum_weight[0], inv_weight0));
            inv_weight1 = vmulq_f32(inv_weight1, vrecpsq_f32(sum_weight[1], inv_weight1));
            
            sum_r[0] = vmulq_f32(sum_r[0], inv_weight0);
            sum_r[1] = vmulq_f32(sum_r[1], inv_weight1);
            sum_g[0] = vmulq_f32(sum_g[0], inv_weight0);
            sum_g[1] = vmulq_f32(sum_g[1], inv_weight1);
            sum_b[0] = vmulq_f32(sum_b[0], inv_weight0);
            sum_b[1] = vmulq_f32(sum_b[1], inv_weight1);
            sum_a[0] = vmulq_f32(sum_a[0], inv_weight0);
            sum_a[1] = vmulq_f32(sum_a[1], inv_weight1);

            uint16x8_t result_r = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_r[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_r[1]))
            );
            uint16x8_t result_g = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_g[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_g[1]))
            );
            uint16x8_t result_b = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_b[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_b[1]))
            );
            uint16x8_t result_a = vcombine_u16(
                vqmovn_u32(vcvtq_u32_f32(sum_a[0])),
                vqmovn_u32(vcvtq_u32_f32(sum_a[1]))
            );

            uint8x8_t r8 = vqmovn_u16(result_r);
            uint8x8_t g8 = vqmovn_u16(result_g);
            uint8x8_t b8 = vqmovn_u16(result_b);
            uint8x8_t a8 = vqmovn_u16(result_a);

            uint16x8_t ga = vshlq_n_u16(vmovl_u8(g8), 8);
            uint16x8_t ba = vmovl_u8(b8);
            uint16x8_t aa = vshlq_n_u16(vmovl_u8(a8), 8);
            uint16x8_t ra = vmovl_u8(r8);
            
            uint16x8_t gb = vorrq_u16(ga, ba);
            uint16x8_t ar = vorrq_u16(aa, ra);
            
            uint16x4_t gb_lo = vget_low_u16(gb);
            uint16x4_t gb_hi = vget_high_u16(gb);
            uint16x4_t ar_lo = vget_low_u16(ar);
            uint16x4_t ar_hi = vget_high_u16(ar);
            
            uint32x4_t result_lo = vshlq_n_u32(vmovl_u16(ar_lo), 16);
            uint32x4_t result_hi = vshlq_n_u32(vmovl_u16(ar_hi), 16);
            
            result_lo = vorrq_u32(result_lo, vmovl_u16(gb_lo));
            result_hi = vorrq_u32(result_hi, vmovl_u16(gb_hi));
            
            vst1q_u32(&dst->buffer[i * dst->w + j], result_lo);
            vst1q_u32(&dst->buffer[i * dst->w + j + 4], result_hi);
        }

        for(; j < dst->w; j++) {
            float gj = (float)j * inv_scale;
            float center_x = gj;
            float center_y = gi;

            float adjusted_a = (float)lanczos_a / effective_scale;
            int start_x = (int)floorf(center_x - adjusted_a);
            int end_x = (int)floorf(center_x + adjusted_a) + 1;
            int start_y = (int)floorf(center_y - adjusted_a);
            int end_y = (int)floorf(center_y + adjusted_a) + 1;

            float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f, sum_a = 0.0f;
            float sum_weight = 0.0f;

            for(int y = start_y; y <= end_y; y++) {
                float ky = lanczos_kernel((y - center_y) * effective_scale, lanczos_a);
                for(int x = start_x; x <= end_x; x++) {
                    float kx = lanczos_kernel((x - center_x) * effective_scale, lanczos_a);
                    float weight = ky * kx;

                    int cx = CLAMP(x, 0, g->w - 1);
                    int cy = CLAMP(y, 0, g->h - 1);
                    uint32_t p = g->buffer[cy * g->w + cx];

                    uint8_t r = (p >> 16) & 0xFF;
                    uint8_t g_val = (p >> 8) & 0xFF;
                    uint8_t b = p & 0xFF;
                    uint8_t a_val = (p >> 24) & 0xFF;

                    sum_r += (float)r * weight;
                    sum_g += (float)g_val * weight;
                    sum_b += (float)b * weight;
                    sum_a += (float)a_val * weight;
                    sum_weight += weight;
                }
            }

            if(sum_weight != 0.0f) {
                float inv_weight = 1.0f / sum_weight;
                sum_r *= inv_weight;
                sum_g *= inv_weight;
                sum_b *= inv_weight;
                sum_a *= inv_weight;
            }

            uint8_t r = (uint8_t)CLAMP(sum_r, 0.0f, 255.0f);
            uint8_t g_val = (uint8_t)CLAMP(sum_g, 0.0f, 255.0f);
            uint8_t b = (uint8_t)CLAMP(sum_b, 0.0f, 255.0f);
            uint8_t a_val = (uint8_t)CLAMP(sum_a, 0.0f, 255.0f);

            dst->buffer[i*dst->w + j] = (a_val << 24) | (r << 16) | (g_val << 8) | b;
        }
    }
}

#endif

#ifdef __cplusplus
}
#endif
