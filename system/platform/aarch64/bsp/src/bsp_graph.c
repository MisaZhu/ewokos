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

static inline uint16x8_t neon_div255_u16(uint16x8_t v)
{
    uint16x8_t t = vaddq_u16(v, vdupq_n_u16(1));
    t = vaddq_u16(t, vshrq_n_u16(v, 8));
    return vshrq_n_u16(t, 8);
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

static inline void neon_alpha_8(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    uint8x8x4_t fg = vld4_u8((const uint8_t*)f);
    uint8x8x4_t bg = vld4_u8((const uint8_t*)b);
    uint8x8x4_t out;
    uint8x8_t full = vdup_n_u8(0xff);
    uint8x8_t scaled = vdup_n_u8(alpha_more);
    uint8x8_t a = vmovn_u16(neon_div255_u16(vmull_u8(fg.val[3], scaled)));
    uint8x8_t inv_a = vsub_u8(full, a);
    uint16x8_t oa_add = neon_div255_u16(vmull_u8(vsub_u8(full, bg.val[3]), a));

    out.val[0] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[0], a), vmull_u8(bg.val[0], inv_a))));
    out.val[1] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[1], a), vmull_u8(bg.val[1], inv_a))));
    out.val[2] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[2], a), vmull_u8(bg.val[2], inv_a))));
    out.val[3] = vmovn_u16(vaddq_u16(vmovl_u8(bg.val[3]), oa_add));

    vst4_u8((uint8_t*)d, out);
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
        // For size < 8, use memcpy to handle the edge case safely.
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
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);

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
	if(dst->clip.w > 0 && dst->clip.h > 0)
		grect_insect(&dst->clip, &dr);

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
    int32_t w = ex - sr.x;

    for(; sy < ey; sy++, dy++) {
        const uint32_t *sp = &src->buffer[sy * src->w + sr.x];
        uint32_t *dp = &dst->buffer[dy * dst->w + dr.x];

        /* Wide rows: libc memcpy uses cache-managed assembly */
        if(w >= 64) {
            memcpy(dp, sp, w * 4);
            continue;
        }

        int32_t x = 0;
        /* 16 pixels (64 bytes) per iteration */
        for(; x <= w - 16; x += 16) {
            uint32x4_t v0 = vld1q_u32(sp + x);
            uint32x4_t v1 = vld1q_u32(sp + x + 4);
            uint32x4_t v2 = vld1q_u32(sp + x + 8);
            uint32x4_t v3 = vld1q_u32(sp + x + 12);
            vst1q_u32(dp + x, v0);
            vst1q_u32(dp + x + 4, v1);
            vst1q_u32(dp + x + 8, v2);
            vst1q_u32(dp + x + 12, v3);
        }
        /* 8 pixels */
        if(x <= w - 8) {
            uint32x4_t v0 = vld1q_u32(sp + x);
            uint32x4_t v1 = vld1q_u32(sp + x + 4);
            vst1q_u32(dp + x, v0);
            vst1q_u32(dp + x + 4, v1);
            x += 8;
        }
        /* Tail */
        if(x < w)
            memcpy(dp + x, sp + x, (w - x) * 4);
    }
}

/* Inline alpha blend of 8 pixels: dst = fg over bg, with global alpha scaling.
   alpha_vec must be vdup_n_u8(alpha) — created once outside the loop. */
static inline void blt_alpha_8_inline(uint32_t *dp, const uint32_t *sp, uint8x8_t alpha_vec)
{
    uint8x8x4_t fg = vld4_u8((const uint8_t*)sp);
    uint8x8x4_t bg = vld4_u8((const uint8_t*)dp);
    uint8x8_t full = vdup_n_u8(0xff);
    uint8x8_t a = vmovn_u16(neon_div255_u16(vmull_u8(fg.val[3], alpha_vec)));
    uint8x8_t inv_a = vsub_u8(full, a);
    uint16x8_t oa_add = neon_div255_u16(vmull_u8(vsub_u8(full, bg.val[3]), a));
    uint8x8x4_t out;
    out.val[0] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[0], a), vmull_u8(bg.val[0], inv_a))));
    out.val[1] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[1], a), vmull_u8(bg.val[1], inv_a))));
    out.val[2] = vmovn_u16(neon_div255_u16(vaddq_u16(vmull_u8(fg.val[2], a), vmull_u8(bg.val[2], inv_a))));
    out.val[3] = vmovn_u16(vaddq_u16(vmovl_u8(bg.val[3]), oa_add));
    vst4_u8((uint8_t*)dp, out);
}

inline void graph_blt_alpha_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    grect_t sr = {sx, sy, sw, sh};
    grect_t dr = {dx, dy, dw, dh};
    graph_insect(dst, &dr);
	if(dst->clip.w > 0 && dst->clip.h > 0)
		grect_insect(&dst->clip, &dr);

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
    int32_t w = ex - sr.x;

    /* Create alpha vector once — constant across the entire blit */
    uint8x8_t alpha_vec = vdup_n_u8(alpha);

    /* Process 2 rows at a time for instruction-level parallelism */
    for(; sy < ey - 1; sy += 2, dy += 2) {
        const uint32_t *sp1 = &src->buffer[sy * src->w + sr.x];
        const uint32_t *sp2 = sp1 + src->w;
        uint32_t *dp1 = &dst->buffer[dy * dst->w + dr.x];
        uint32_t *dp2 = dp1 + dst->w;
        int32_t x = 0;

        for(; x <= w - 8; x += 8) {
            blt_alpha_8_inline(dp1 + x, sp1 + x, alpha_vec);
            blt_alpha_8_inline(dp2 + x, sp2 + x, alpha_vec);
        }
        /* Tail */
        if(x < w) {
            int remain = w - x;
            uint32_t fg[8] = {0}, bg[8] = {0};
            memcpy(fg, sp1 + x, 4 * remain);
            memcpy(bg, dp1 + x, 4 * remain);
            blt_alpha_8_inline(bg, fg, alpha_vec);
            memcpy(dp1 + x, bg, 4 * remain);
            memcpy(fg, sp2 + x, 4 * remain);
            memcpy(bg, dp2 + x, 4 * remain);
            blt_alpha_8_inline(bg, fg, alpha_vec);
            memcpy(dp2 + x, bg, 4 * remain);
        }
    }

    /* Last row if odd height */
    if(sy < ey) {
        const uint32_t *sp = &src->buffer[sy * src->w + sr.x];
        uint32_t *dp = &dst->buffer[dy * dst->w + dr.x];
        int32_t x = 0;

        for(; x <= w - 8; x += 8)
            blt_alpha_8_inline(dp + x, sp + x, alpha_vec);
        if(x < w) {
            int remain = w - x;
            uint32_t fg[8] = {0}, bg[8] = {0};
            memcpy(fg, sp + x, 4 * remain);
            memcpy(bg, dp + x, 4 * remain);
            blt_alpha_8_inline(bg, fg, alpha_vec);
            memcpy(dp + x, bg, 4 * remain);
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
	if(dst->clip.w > 0 && dst->clip.h > 0)
		grect_insect(&dst->clip, &dr);

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
    // Validate parameters.
    if (!args || r <= 0 || w <= 0 || h <= 0 || width <= 0 || height <= 0) 
        return;
    if (x < 0 || y < 0 || x + w > width || y + h > height)
        return;

    // Use a fixed random seed so the effect stays deterministic.
    srand(0x12345678);  // Reuse the same seed on every call

    // Precompute frequently used values.
    int range = 2*r;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    // Initialize NEON registers.
    int32x4_t vx = vdupq_n_s32(x);
    int32x4_t vy = vdupq_n_s32(y);
    int32x4_t vx_end = vdupq_n_s32(x_end);
    int32x4_t vy_end = vdupq_n_s32(y_end);
    int32x4_t vwidth = vdupq_n_s32(width);

    // Pre-generate all random offsets.
    int total_pixels = w * h;
    int* rand_offsets = malloc(total_pixels * 2 * sizeof(int));

	for (int i = 0; i < total_pixels * 2; i++) {
		rand_offsets[i] = (rand() % range) - r;
	}

    // Process the image region.
    int offset_index = 0;
    for (int j = y; j <= y_end; j++) {
        int32x4_t vj = vdupq_n_s32(j);
        
        // Preload data into the cache.
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&args[j * width + x]));
        
        for (int i = x; i <= x_end; i += 8) {
            // Handle the remaining pixels when fewer than 8 are left.
            int remaining = x_end - i + 1;
            if (remaining < 8) {
                for (int k = 0; k < remaining; k++) {
                    int rx = i + k + rand_offsets[offset_index++];
                    int ry = j + rand_offsets[offset_index++];
                    
                    // Clamp to the valid bounds.
                    rx = (rx < x) ? x : ((rx > x_end) ? x_end : rx);
                    ry = (ry < y) ? y : ((ry > y_end) ? y_end : ry);
                    
                    args[j * width + i + k] = args[ry * width + rx];
                }
                break;
            }
            
            // Generate random offsets for 8 pixels.
            int rand_x[8], rand_y[8];
            for (int k = 0; k < 8; k++) {
                rand_x[k] = rand_offsets[offset_index++];
                rand_y[k] = rand_offsets[offset_index++];
            }
            
            // Process the first 4 pixels.
            int32x4_t vrand_x0 = vld1q_s32(rand_x);
            int32x4_t vrand_y0 = vld1q_s32(rand_y);
            int32x4_t vi0 = {i, i+1, i+2, i+3};
            int32x4_t rx0 = vaddq_s32(vi0, vrand_x0);
            int32x4_t ry0 = vaddq_s32(vj, vrand_y0);
            rx0 = vmaxq_s32(vx, vminq_s32(vx_end, rx0));
            ry0 = vmaxq_s32(vy, vminq_s32(vy_end, ry0));
            int32x4_t rpos0 = vmlaq_s32(rx0, ry0, vwidth);
            
            // Process the last 4 pixels.
            int32x4_t vrand_x1 = vld1q_s32(&rand_x[4]);
            int32x4_t vrand_y1 = vld1q_s32(&rand_y[4]);
            int32x4_t vi1 = {i+4, i+5, i+6, i+7};
            int32x4_t rx1 = vaddq_s32(vi1, vrand_x1);
            int32x4_t ry1 = vaddq_s32(vj, vrand_y1);
            rx1 = vmaxq_s32(vx, vminq_s32(vx_end, rx1));
            ry1 = vmaxq_s32(vy, vminq_s32(vy_end, ry1));
            int32x4_t rpos1 = vmlaq_s32(rx1, ry1, vwidth);
            
            // Extract the positions into scalar arrays.
            int rpos_arr0[4], rpos_arr1[4];
            vst1q_s32(rpos_arr0, rpos0);
            vst1q_s32(rpos_arr1, rpos1);
            
            // Gather the pixel values in a batch.
            uint32_t pixels[8];
            for (int k = 0; k < 4; k++) {
                pixels[k] = args[rpos_arr0[k]];
                pixels[k + 4] = args[rpos_arr1[k]];
            }
            
            // Store the results in a batch.
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
    
    // Clamp to valid bounds.
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    if (w <= 0 || h <= 0) return;
    
    // Build the Gaussian kernel.
    int kernel_size = radius * 2 + 1;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    float sigma = radius / 2.0f;
    float sum = 0.0f;
    
    for (int i = -radius; i <= radius; i++) {
        float val = expf(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = val;
        sum += val;
    }
    
    // Normalize the kernel.
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    // Temporary buffer.
    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    
    // NEON-optimized horizontal blur, processing 8 pixels at a time.
    for (int j = 0; j < h; j++) {
        // Preload data into the cache.
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&pixels[(y + j) * width + x]));
        
        for (int i = 0; i < w; i += 8) {
            if (i + 8 > w) {
                // Handle the remaining pixels when fewer than 8 are left.
                for (int k = i; k < w; k++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int px = x + k + m;
                        if (px < x) px = x;
                        if (px >= x + w) px = x + w - 1;
                        
                        uint32_t pixel = pixels[(y + j) * width + px];
                        float weight = kernel[m + radius];
                        
                        // Extract ARGB channels.
                        uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                        uint16x8_t vPixel16 = vmovl_u8(vPixel);
                        uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                        float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                        
                        // Multiply by the weight and accumulate.
                        accum = vmlaq_n_f32(accum, vPixelF, weight);
                    }
                    
                    // Convert back to integers and store the result.
                    uint32x4_t result = vcvtq_u32_f32(accum);
                    uint8x8_t res8 = vmovn_u16(vcombine_u16(
                        vmovn_u32(result),
                        vmovn_u32(result)
                    ));
                    temp[j * w + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
                }
                break;
            }
            
            // Process 8 pixels in parallel.
            float32x4_t accum[8] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // Load 8 pixels in a batch.
                uint32x4_t pixels0 = vld1q_u32(&pixels[(y + j) * width + x + i]);
                uint32x4_t pixels1 = vld1q_u32(&pixels[(y + j) * width + x + i + 4]);
                
                // Process the first 4 pixels.
                for (int k = 0; k < 4; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    
                    // Extract ARGB channels.
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // Multiply by the weight and accumulate.
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
                
                // Process the last 4 pixels.
                for (int k = 4; k < 8; k++) {
                    int px = x + i + k + m;
                    if (px < x) px = x;
                    if (px >= x + w) px = x + w - 1;
                    
                    uint32_t pixel = pixels[(y + j) * width + px];
                    
                    // Extract ARGB channels.
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

enum {
    GRAPH_SCALE_FIXED_SHIFT = 16,
    GRAPH_SCALE_FIXED_SCALE = 1 << GRAPH_SCALE_FIXED_SHIFT,
    GRAPH_SCALE_FIXED_MASK = GRAPH_SCALE_FIXED_SCALE - 1
};

static inline uint32_t graph_scale_bilinear_interp_bsp(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11,
        uint32_t fx, uint32_t fy) {
    uint32_t one_minus_fx = GRAPH_SCALE_FIXED_SCALE - fx;
    uint32_t one_minus_fy = GRAPH_SCALE_FIXED_SCALE - fy;

    uint32_t r00 = (p00 >> 16) & 0xFF;
    uint32_t g00 = (p00 >> 8) & 0xFF;
    uint32_t b00 = p00 & 0xFF;
    uint32_t a00 = (p00 >> 24) & 0xFF;

    uint32_t r01 = (p01 >> 16) & 0xFF;
    uint32_t g01 = (p01 >> 8) & 0xFF;
    uint32_t b01 = p01 & 0xFF;
    uint32_t a01 = (p01 >> 24) & 0xFF;

    uint32_t r10 = (p10 >> 16) & 0xFF;
    uint32_t g10 = (p10 >> 8) & 0xFF;
    uint32_t b10 = p10 & 0xFF;
    uint32_t a10 = (p10 >> 24) & 0xFF;

    uint32_t r11 = (p11 >> 16) & 0xFF;
    uint32_t g11 = (p11 >> 8) & 0xFF;
    uint32_t b11 = p11 & 0xFF;
    uint32_t a11 = (p11 >> 24) & 0xFF;

    uint64_t tmp_r = (uint64_t)one_minus_fx * one_minus_fy * r00 +
                     (uint64_t)fx * one_minus_fy * r01 +
                     (uint64_t)one_minus_fx * fy * r10 +
                     (uint64_t)fx * fy * r11;
    uint64_t tmp_g = (uint64_t)one_minus_fx * one_minus_fy * g00 +
                     (uint64_t)fx * one_minus_fy * g01 +
                     (uint64_t)one_minus_fx * fy * g10 +
                     (uint64_t)fx * fy * g11;
    uint64_t tmp_b = (uint64_t)one_minus_fx * one_minus_fy * b00 +
                     (uint64_t)fx * one_minus_fy * b01 +
                     (uint64_t)one_minus_fx * fy * b10 +
                     (uint64_t)fx * fy * b11;
    uint64_t tmp_a = (uint64_t)one_minus_fx * one_minus_fy * a00 +
                     (uint64_t)fx * one_minus_fy * a01 +
                     (uint64_t)one_minus_fx * fy * a10 +
                     (uint64_t)fx * fy * a11;

    uint32_t r = (uint32_t)(tmp_r >> (2 * GRAPH_SCALE_FIXED_SHIFT));
    uint32_t g = (uint32_t)(tmp_g >> (2 * GRAPH_SCALE_FIXED_SHIFT));
    uint32_t b = (uint32_t)(tmp_b >> (2 * GRAPH_SCALE_FIXED_SHIFT));
    uint32_t a = (uint32_t)(tmp_a >> (2 * GRAPH_SCALE_FIXED_SHIFT));

    return (a << 24) | (r << 16) | (g << 8) | b;
}

static void graph_scale_prepare_axis_bsp(int dst_len, int src_max, uint32_t inv_scale,
        int *idx0, int *idx1, uint32_t *frac) {
    uint32_t pos = 0;

    for(int i = 0; i < dst_len; i++) {
        int base = (int)(pos >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t f = pos & GRAPH_SCALE_FIXED_MASK;
        int next = base + 1;

        if(base >= src_max) {
            base = src_max;
            next = src_max;
            f = 0;
        }
        else if(next > src_max) {
            next = src_max;
        }

        idx0[i] = base;
        idx1[i] = next;
        frac[i] = f;
        pos += inv_scale;
    }
}

static int graph_scale_integer_downsample_bsp(graph_t* g, graph_t* dst, uint32_t inv_scale) {
    if((inv_scale & GRAPH_SCALE_FIXED_MASK) != 0)
        return 0;

    uint32_t step = inv_scale >> GRAPH_SCALE_FIXED_SHIFT;
    if(step < 2)
        return 0;

    int src_w = g->w;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int is_pow2 = (step & (step - 1)) == 0;

    if(is_pow2) {
        unsigned step_shift = 0;
        while((1U << step_shift) < step)
            step_shift++;

        for(int y = 0; y < dst_h; y++) {
            uint32_t *src_row = &g->buffer[(y << step_shift) * src_w];
            uint32_t *dst_row = &dst->buffer[y * dst_w];
            int x = 0;

            for(; x <= dst_w - 4; x += 4) {
                dst_row[x] = src_row[x << step_shift];
                dst_row[x + 1] = src_row[(x + 1) << step_shift];
                dst_row[x + 2] = src_row[(x + 2) << step_shift];
                dst_row[x + 3] = src_row[(x + 3) << step_shift];
            }

            for(; x < dst_w; x++) {
                dst_row[x] = src_row[x << step_shift];
            }
        }

        return 1;
    }

    uint32_t src_y = 0;
    for(int y = 0; y < dst_h; y++) {
        uint32_t *src_row = &g->buffer[src_y * src_w];
        uint32_t *dst_row = &dst->buffer[y * dst_w];
        uint32_t src_x = 0;
        int x = 0;

        for(; x <= dst_w - 4; x += 4) {
            dst_row[x] = src_row[src_x];
            src_x += step;
            dst_row[x + 1] = src_row[src_x];
            src_x += step;
            dst_row[x + 2] = src_row[src_x];
            src_x += step;
            dst_row[x + 3] = src_row[src_x];
            src_x += step;
        }

        for(; x < dst_w; x++) {
            dst_row[x] = src_row[src_x];
            src_x += step;
        }

        src_y += step;
    }

    return 1;
}

void graph_scale_tof_fast_bsp(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    int src_w = g->w;
    int dst_w = dst->w;
    int dst_h = dst->h;
    int hmax = g->h - 1;
    int wmax = src_w - 1;
    uint32_t inv_scale = (uint32_t)((1.0f / scale) * GRAPH_SCALE_FIXED_SCALE);

    if(inv_scale == GRAPH_SCALE_FIXED_SCALE && dst_w == src_w && dst_h == g->h) {
        memcpy(dst->buffer, g->buffer, (size_t)src_w * (size_t)g->h * sizeof(uint32_t));
        return;
    }

    if(scale < 1.0 && graph_scale_integer_downsample_bsp(g, dst, inv_scale))
        return;

    int *x0 = (int*)malloc((size_t)dst_w * sizeof(int));
    int *x1 = (int*)malloc((size_t)dst_w * sizeof(int));
    uint32_t *x_frac = (uint32_t*)malloc((size_t)dst_w * sizeof(uint32_t));
    uint16_t *wh0 = (uint16_t*)malloc((size_t)dst_w * 4 * sizeof(uint16_t));
    uint16_t *wh1 = (uint16_t*)malloc((size_t)dst_w * 4 * sizeof(uint16_t));

    if(x0 != NULL && x1 != NULL && x_frac != NULL && wh0 != NULL && wh1 != NULL) {
        graph_scale_prepare_axis_bsp(dst_w, wmax, inv_scale, x0, x1, x_frac);

        /* per-column 8-bit-fraction horizontal weights, replicated per channel */
        for(int j = 0; j < dst_w; j++) {
            uint32_t f = (x_frac[j] + 128) >> 8; /* 0..256 */
            uint16_t w1 = (uint16_t)f;
            uint16_t w0 = (uint16_t)(256 - f);
            int o = j * 4;
            wh0[o] = w0;
            wh0[o + 1] = w0;
            wh0[o + 2] = w0;
            wh0[o + 3] = w0;
            wh1[o] = w1;
            wh1[o + 1] = w1;
            wh1[o + 2] = w1;
            wh1[o + 3] = w1;
        }

        /* pair-load of (x0[j], x0[j]+1) stays in bounds while x0[j] < wmax;
           x0 is non-decreasing, so the NEON-safe prefix is contiguous */
        int j_safe = 0;
        while(j_safe < dst_w && x0[j_safe] < wmax)
            j_safe++;
        int neon_w = j_safe & ~3;

        uint32_t src_y = 0;
        for(int i = 0; i < dst_h; i++) {
            int gi0 = (int)(src_y >> GRAPH_SCALE_FIXED_SHIFT);
            uint32_t gi_frac = src_y & GRAPH_SCALE_FIXED_MASK;
            int gi1 = gi0 + 1;

            if(gi0 >= hmax) {
                gi0 = hmax;
                gi1 = hmax;
                gi_frac = 0;
            }
            else if(gi1 > hmax) {
                gi1 = hmax;
            }

            const uint32_t *row0 = g->buffer + gi0 * src_w;
            const uint32_t *row1 = g->buffer + gi1 * src_w;
            uint32_t *drow = dst->buffer + i * dst_w;

            uint16_t fy = (uint16_t)((gi_frac + 128) >> 8);
            uint16x8_t wv0 = vdupq_n_u16((uint16_t)(256 - fy));
            uint16x8_t wv1 = vdupq_n_u16(fy);
            uint16x8_t rnd = vdupq_n_u16(128);

            int j = 0;
            for(; j < neon_w; j += 4) {
                /* gather (p00,p01) pairs for 4 output columns from both rows;
                   x1[j] == x0[j]+1 in the NEON-safe prefix */
                uint32x4_t ab0 = vcombine_u32(vld1_u32(row0 + x0[j]), vld1_u32(row0 + x0[j + 1]));
                uint32x4_t cd0 = vcombine_u32(vld1_u32(row0 + x0[j + 2]), vld1_u32(row0 + x0[j + 3]));
                uint32x4x2_t u0 = vuzpq_u32(ab0, cd0);

                uint32x4_t ab1 = vcombine_u32(vld1_u32(row1 + x0[j]), vld1_u32(row1 + x0[j + 1]));
                uint32x4_t cd1 = vcombine_u32(vld1_u32(row1 + x0[j + 2]), vld1_u32(row1 + x0[j + 3]));
                uint32x4x2_t u1 = vuzpq_u32(ab1, cd1);

                uint32x4_t p00 = u0.val[0];
                uint32x4_t p01 = u0.val[1];
                uint32x4_t p10 = u1.val[0];
                uint32x4_t p11 = u1.val[1];

                uint32x4_t eq = vandq_u32(
                        vandq_u32(vceqq_u32(p00, p01), vceqq_u32(p00, p10)),
                        vceqq_u32(p00, p11));
                uint32x2_t eq2 = vand_u32(vget_low_u32(eq), vget_high_u32(eq));
                if((vget_lane_u32(eq2, 0) & vget_lane_u32(eq2, 1)) == 0xFFFFFFFFu) {
                    vst1q_u32(drow + j, p00);
                    continue;
                }

                uint8x16_t b00 = vreinterpretq_u8_u32(p00);
                uint8x16_t b01 = vreinterpretq_u8_u32(p01);
                uint8x16_t b10 = vreinterpretq_u8_u32(p10);
                uint8x16_t b11 = vreinterpretq_u8_u32(p11);

                uint16x8_t p00l = vmovl_u8(vget_low_u8(b00));
                uint16x8_t p00h = vmovl_u8(vget_high_u8(b00));
                uint16x8_t p01l = vmovl_u8(vget_low_u8(b01));
                uint16x8_t p01h = vmovl_u8(vget_high_u8(b01));
                uint16x8_t p10l = vmovl_u8(vget_low_u8(b10));
                uint16x8_t p10h = vmovl_u8(vget_high_u8(b10));
                uint16x8_t p11l = vmovl_u8(vget_low_u8(b11));
                uint16x8_t p11h = vmovl_u8(vget_high_u8(b11));

                uint16x8_t w0l = vld1q_u16(wh0 + j * 4);
                uint16x8_t w0h = vld1q_u16(wh0 + j * 4 + 8);
                uint16x8_t w1l = vld1q_u16(wh1 + j * 4);
                uint16x8_t w1h = vld1q_u16(wh1 + j * 4 + 8);

                /* horizontal lerp: (p00*w0 + p01*w1 + 128) >> 8, w0+w1 = 256 so no u16 overflow */
                uint16x8_t tl = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, p00l, w0l), p01l, w1l), 8);
                uint16x8_t th = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, p00h, w0h), p01h, w1h), 8);
                uint16x8_t bl = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, p10l, w0l), p11l, w1l), 8);
                uint16x8_t bh = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, p10h, w0h), p11h, w1h), 8);

                /* vertical lerp */
                uint16x8_t ol = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, tl, wv0), bl, wv1), 8);
                uint16x8_t oh = vshrq_n_u16(vmlaq_u16(vmlaq_u16(rnd, th, wv0), bh, wv1), 8);

                vst1q_u32(drow + j,
                        vreinterpretq_u32_u8(vcombine_u8(vmovn_u16(ol), vmovn_u16(oh))));
            }

            for(; j < dst_w; j++) {
                uint32_t p00 = row0[x0[j]];
                uint32_t p01 = row0[x1[j]];
                uint32_t p10 = row1[x0[j]];
                uint32_t p11 = row1[x1[j]];

                if(p00 == p01 && p00 == p10 && p00 == p11) {
                    drow[j] = p00;
                }
                else {
                    drow[j] = graph_scale_bilinear_interp_bsp(p00, p01, p10, p11, x_frac[j], gi_frac);
                }
            }

            src_y += inv_scale;
        }

        free(x0);
        free(x1);
        free(x_frac);
        free(wh0);
        free(wh1);
        return;
    }

    free(x0);
    free(x1);
    free(x_frac);
    free(wh0);
    free(wh1);

    uint32_t src_y = 0;
    for(int i = 0; i < dst_h; i++) {
        int gi0 = (int)(src_y >> GRAPH_SCALE_FIXED_SHIFT);
        uint32_t gi_frac = src_y & GRAPH_SCALE_FIXED_MASK;
        int gi1 = gi0 + 1;

        if(gi0 >= hmax) {
            gi0 = hmax;
            gi1 = hmax;
            gi_frac = 0;
        }
        else if(gi1 > hmax) {
            gi1 = hmax;
        }

        int gi0w = gi0 * src_w;
        int gi1w = gi1 * src_w;
        int dst_row = i * dst_w;
        uint32_t src_x = 0;

        for(int j = 0; j < dst_w; j++) {
            int gj0 = (int)(src_x >> GRAPH_SCALE_FIXED_SHIFT);
            uint32_t gj_frac = src_x & GRAPH_SCALE_FIXED_MASK;
            int gj1 = gj0 + 1;

            if(gj0 >= wmax) {
                gj0 = wmax;
                gj1 = wmax;
                gj_frac = 0;
            }
            else if(gj1 > wmax) {
                gj1 = wmax;
            }

            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + gj1];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + gj1];

            if(p00 == p01 && p00 == p10 && p00 == p11) {
                dst->buffer[dst_row + j] = p00;
            }
            else {
                dst->buffer[dst_row + j] = graph_scale_bilinear_interp_bsp(p00, p01, p10, p11, gj_frac, gi_frac);
            }

            src_x += inv_scale;
        }

        src_y += inv_scale;
    }
}

static inline uint32x4_t neon_reverse_u32x4(uint32x4_t v) {
    return vcombine_u32(vrev64_u32(vget_high_u32(v)), vrev64_u32(vget_low_u32(v)));
}

static inline uint8x8_t neon_rgb_to_y8(uint8x8_t b8, uint8x8_t g8, uint8x8_t r8) {
    uint16x8_t b16 = vmovl_u8(b8);
    uint16x8_t g16 = vmovl_u8(g8);
    uint16x8_t r16 = vmovl_u8(r8);

    uint32x4_t y0 = vmulq_n_u32(vmovl_u16(vget_low_u16(r16)), 306);
    y0 = vmlaq_n_u32(y0, vmovl_u16(vget_low_u16(g16)), 601);
    y0 = vmlaq_n_u32(y0, vmovl_u16(vget_low_u16(b16)), 117);

    uint32x4_t y1 = vmulq_n_u32(vmovl_u16(vget_high_u16(r16)), 306);
    y1 = vmlaq_n_u32(y1, vmovl_u16(vget_high_u16(g16)), 601);
    y1 = vmlaq_n_u32(y1, vmovl_u16(vget_high_u16(b16)), 117);

    y0 = vshrq_n_u32(y0, 10);
    y1 = vshrq_n_u32(y1, 10);

    return vmovn_u16(vcombine_u16(vmovn_u32(y0), vmovn_u32(y1)));
}

static inline uint8_t rgb_to_y_scalar_bsp(uint32_t pixel) {
    uint32_t b = pixel & 0xff;
    uint32_t g = (pixel >> 8) & 0xff;
    uint32_t r = (pixel >> 16) & 0xff;
    return (uint8_t)((306 * r + 601 * g + 117 * b) >> 10);
}

static inline void rgb_to_uv_scalar_bsp(uint32_t pixel, uint8_t *uv) {
    int32_t b = (int32_t)(pixel & 0xff);
    int32_t g = (int32_t)((pixel >> 8) & 0xff);
    int32_t r = (int32_t)((pixel >> 16) & 0xff);

    uv[0] = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
    uv[1] = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
}

static inline uint32_t rgb2nv12_get_src_pixel(const uint32_t *in, int w, int h, int y, int x) {
    return in[(h - 1 - y) * w + (w - 1 - x)];
}

void rgb2nv12_bsp(uint8_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    uint8_t *y_plane = out;
    uint8_t *uv_plane = out + w * h;

    for(int y = 0; y < h; y += 2) {
        uint8_t *y_row0 = y_plane + y * w;
        uint8_t *y_row1 = (y + 1 < h) ? (y_row0 + w) : NULL;
        uint8_t *uv_row = uv_plane + (y >> 1) * w;
        int x = 0;

        if(y + 1 < h) {
            for(; x + 7 < w; x += 8) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                const uint32_t *src_row1 = in + (h - 2 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[8];
                uint32_t row1_pixels[8];
                uint8x8_t yv0;
                uint8x8_t yv1;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));
                vst1q_u32(row1_pixels, neon_reverse_u32x4(vld1q_u32(src_row1 - 3)));
                vst1q_u32(row1_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row1 - 7)));

                uint8x8x4_t bgra0 = vld4_u8((const uint8_t*)row0_pixels);
                uint8x8x4_t bgra1 = vld4_u8((const uint8_t*)row1_pixels);

                yv0 = neon_rgb_to_y8(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                yv1 = neon_rgb_to_y8(bgra1.val[0], bgra1.val[1], bgra1.val[2]);

                vst1_u8(y_row0 + x, yv0);
                vst1_u8(y_row1 + x, yv1);

                rgb_to_uv_scalar_bsp(row0_pixels[0], uv_row + x);
                rgb_to_uv_scalar_bsp(row0_pixels[2], uv_row + x + 2);
                rgb_to_uv_scalar_bsp(row0_pixels[4], uv_row + x + 4);
                rgb_to_uv_scalar_bsp(row0_pixels[6], uv_row + x + 6);
            }

            for(; x + 1 < w; x += 2) {
                uint32_t p00 = rgb2nv12_get_src_pixel(in, w, h, y, x);
                uint32_t p01 = rgb2nv12_get_src_pixel(in, w, h, y, x + 1);
                uint32_t p10 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x);
                uint32_t p11 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x + 1);

                y_row0[x] = rgb_to_y_scalar_bsp(p00);
                y_row0[x + 1] = rgb_to_y_scalar_bsp(p01);
                y_row1[x] = rgb_to_y_scalar_bsp(p10);
                y_row1[x + 1] = rgb_to_y_scalar_bsp(p11);
                rgb_to_uv_scalar_bsp(p00, uv_row + x);
            }

            if(x < w) {
                uint32_t p0 = rgb2nv12_get_src_pixel(in, w, h, y, x);
                uint32_t p1 = rgb2nv12_get_src_pixel(in, w, h, y + 1, x);
                y_row0[x] = rgb_to_y_scalar_bsp(p0);
                y_row1[x] = rgb_to_y_scalar_bsp(p1);
            }
        }
        else {
            for(; x + 7 < w; x += 8) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[8];
                uint8x8_t yv0;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));

                uint8x8x4_t bgra0 = vld4_u8((const uint8_t*)row0_pixels);
                yv0 = neon_rgb_to_y8(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                vst1_u8(y_row0 + x, yv0);
            }

            for(; x < w; ++x) {
                y_row0[x] = rgb_to_y_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
            }
        }
    }
}

#endif

#ifdef __cplusplus
}
#endif
