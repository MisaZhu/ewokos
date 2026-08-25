#include <graph/graph_arch.h>
#include <g2d_arch.h>
#include <ewoksys/core.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openlibm.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef ARCH_BOOST
#include <arm_neon.h>

#define MIN(a, b) (((a) > (b))?(b):(a))

static inline uint16x8_t neon_div255_u16(uint16x8_t v)
{
    uint16x8_t t = vaddq_u16(v, vdupq_n_u16(1));
    t = vaddq_u16(t, vshrq_n_u16(v, 8));
    return vshrq_n_u16(t, 8);
}

static inline void neon_alpha_16(uint32_t *b, uint32_t *f, uint32_t *d, uint8_t alpha_more)
{
    uint8x16x4_t fg = vld4q_u8((const uint8_t*)f);
    uint8x16x4_t bg = vld4q_u8((const uint8_t*)b);
    uint8x16x4_t out;
    uint8x16_t full = vdupq_n_u8(0xff);
    uint8x16_t scaled = vdupq_n_u8(alpha_more);

    uint8x8_t a_lo = vmovn_u16(neon_div255_u16(vmull_u8(vget_low_u8(fg.val[3]), vget_low_u8(scaled))));
    uint8x8_t a_hi = vmovn_u16(neon_div255_u16(vmull_u8(vget_high_u8(fg.val[3]), vget_high_u8(scaled))));
    uint8x16_t a = vcombine_u8(a_lo, a_hi);
    uint8x16_t inv_a = vsubq_u8(full, a);

    uint16x8_t oa_lo = neon_div255_u16(vmull_u8(vsub_u8(vget_low_u8(full), vget_low_u8(bg.val[3])), a_lo));
    uint16x8_t oa_hi = neon_div255_u16(vmull_u8(vsub_u8(vget_high_u8(full), vget_high_u8(bg.val[3])), a_hi));

    /* out = div255(fg*a + bg*(255-a)) per channel, low/high halves widened */
    for(int c = 0; c < 3; c++) {
        uint16x8_t lo = vaddq_u16(vmull_u8(vget_low_u8(fg.val[c]), a_lo),
                                  vmull_u8(vget_low_u8(bg.val[c]), vget_low_u8(inv_a)));
        uint16x8_t hi = vaddq_u16(vmull_u8(vget_high_u8(fg.val[c]), a_hi),
                                  vmull_u8(vget_high_u8(bg.val[c]), vget_high_u8(inv_a)));
        out.val[c] = vcombine_u8(vmovn_u16(neon_div255_u16(lo)), vmovn_u16(neon_div255_u16(hi)));
    }
    /* out_a = bg_a + div255((255-bg_a)*a) */
    out.val[3] = vcombine_u8(
        vmovn_u16(vaddq_u16(vmovl_u8(vget_low_u8(bg.val[3])), oa_lo)),
        vmovn_u16(vaddq_u16(vmovl_u8(vget_high_u8(bg.val[3])), oa_hi)));

    vst4q_u8((uint8_t*)d, out);
}

static inline void graph_pixel_argb_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size, uint8_t alpha_more)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 16)
    {
        neon_alpha_16(dst, src, dst, alpha_more);
    }
    else
    {
        // For size < 16, use memcpy to handle the edge case safely.
        uint32_t fg[16] = {0};
        uint32_t bg[16] = {0};
        memcpy(fg, src, 4*size);
        memcpy(bg, dst, 4*size);
        neon_alpha_16(bg, fg, bg, alpha_more);
        memcpy(dst, bg, 4*size);
    }
}

void graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if(g == NULL || w <= 0 || h <= 0)
        return;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);

    /* opaque fill: handled by the g2d engine (memset / NEON row stores) */
    if(color_a(color) == 0xff) {
        arch_g2d_fill(g->buffer, g->w, g->h, r.x, r.y, r.w, r.h, color);
        return;
    }

    /* translucent fill: blend the solid color against what is there */
    uint32_t buf[16];
    for(int i = 0; i < 16; i++)
        buf[i] = color;

    register int32_t ex, ey;
    ey = r.y + r.h;
    ex = r.x + r.w;
    for(int32_t yy = r.y; yy < ey; yy++) {
        for(int32_t xx = r.x; xx < ex; xx += 16) {
            graph_pixel_argb_neon(g, xx, yy, buf, MIN(ex - xx, 16), 0xFF);
        }
    }
}

inline void graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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

    /* 1:1 copy of the clipped region; the g2d engine keeps overlapping
       copies within one buffer safe (memmove ordering) */
    arch_g2d_blt(src->buffer, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h);
}

inline void graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
    if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    /* Global alpha 0: nothing visible, skip entirely */
    if(alpha == 0)
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

    /* 1:1 blend of the clipped region; the g2d engine applies the same
       per-block transparent/opaque fast paths and div255 blend math */
    arch_g2d_blt_alpha(src->buffer, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h, alpha);
}


static inline void neon_mask_alpha_16(uint32_t *dst, uint32_t *src)
{
    __asm volatile(
        // Load 16 dst pixels (RGBA)
        "ld4 {v20.16b-v23.16b}, [%0]\n\t"
        // Load 16 src pixels (RGBA)
        "ld4 {v24.16b-v27.16b}, [%1]\n\t"
        
        // v23 = dst_a, v27 = src_a
        // Create zero vector
        "movi v28.16b, #0\n\t"
        // Compare src_a > 0 (cmhi returns 0xFF where true, 0x00 where false)
        "cmhi v28.16b, v27.16b, v28.16b\n\t"  // v28: 0xFF where src_a > 0, 0x00 where src_a == 0
        
        // Compare dst_a > src_a
        "cmhi v29.16b, v23.16b, v27.16b\n\t"  // v29: 0xFF where dst_a > src_a
        
        // Create mask for src_a == 0: set all channels to 0
        "and v20.16b, v20.16b, v28.16b\n\t"    // R
        "and v21.16b, v21.16b, v28.16b\n\t"    // G
        "and v22.16b, v22.16b, v28.16b\n\t"    // B
        "and v23.16b, v23.16b, v28.16b\n\t"    // A
        
        // For dst_a > src_a: keep dst RGB, replace alpha with src_a
        // First, mask src_a where condition is true
        "and v30.16b, v27.16b, v29.16b\n\t"
        // Mask dst_a where condition is false
        "mvn v28.16b, v29.16b\n\t"
        "and v23.16b, v23.16b, v28.16b\n\t"
        // Combine
        "orr v23.16b, v23.16b, v30.16b\n\t"
        
        // Store result
        "st4 {v20.16b-v23.16b}, [%0]\n\t"
        :
        : "r"(dst), "r"(src)
        : "memory", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", 
          "v28", "v29", "v30");
}

static inline void graph_pixel_alpha_mask_neon(graph_t *graph, int32_t x, int32_t y,
                                  uint32_t *src, int size)
{
    uint32_t *dst = &graph->buffer[y * graph->w + x];

    if (size == 16)
    {
        neon_mask_alpha_16(dst, src);
    }
    else
    {
        // For size < 16, use memcpy to handle boundaries
        uint32_t src_buf[16] = {0};
        uint32_t dst_buf[16] = {0};
        memcpy(src_buf, src, 4 * size);
        memcpy(dst_buf, dst, 4 * size);
        neon_mask_alpha_16(dst_buf, src_buf);
        memcpy(dst, dst_buf, 4 * size);
    }
}

inline void graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
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
        
        for(; sx < ex - 15; sx += 16, dx += 16) {
            // Process two rows in parallel
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset1 + sx], 16);
            graph_pixel_alpha_mask_neon(dst, dx, dy + 1, &src->buffer[offset2 + sx], 16);
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
        
        for(; sx < ex; sx += 16, dx += 16) {
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex - sx, 16));
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
        
        for (int i = x; i <= x_end; i += 16) {
            // Handle the remaining pixels when fewer than 16 are left.
            int remaining = x_end - i + 1;
            if (remaining < 16) {
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
            
            // Generate random offsets for 16 pixels.
            int rand_x[16], rand_y[16];
            for (int k = 0; k < 16; k++) {
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
            
            // Process pixels 4-7.
            int32x4_t vrand_x1 = vld1q_s32(&rand_x[4]);
            int32x4_t vrand_y1 = vld1q_s32(&rand_y[4]);
            int32x4_t vi1 = {i+4, i+5, i+6, i+7};
            int32x4_t rx1 = vaddq_s32(vi1, vrand_x1);
            int32x4_t ry1 = vaddq_s32(vj, vrand_y1);
            rx1 = vmaxq_s32(vx, vminq_s32(vx_end, rx1));
            ry1 = vmaxq_s32(vy, vminq_s32(vy_end, ry1));
            int32x4_t rpos1 = vmlaq_s32(rx1, ry1, vwidth);
            
            // Process pixels 8-11.
            int32x4_t vrand_x2 = vld1q_s32(&rand_x[8]);
            int32x4_t vrand_y2 = vld1q_s32(&rand_y[8]);
            int32x4_t vi2 = {i+8, i+9, i+10, i+11};
            int32x4_t rx2 = vaddq_s32(vi2, vrand_x2);
            int32x4_t ry2 = vaddq_s32(vj, vrand_y2);
            rx2 = vmaxq_s32(vx, vminq_s32(vx_end, rx2));
            ry2 = vmaxq_s32(vy, vminq_s32(vy_end, ry2));
            int32x4_t rpos2 = vmlaq_s32(rx2, ry2, vwidth);
            
            // Process the last 4 pixels.
            int32x4_t vrand_x3 = vld1q_s32(&rand_x[12]);
            int32x4_t vrand_y3 = vld1q_s32(&rand_y[12]);
            int32x4_t vi3 = {i+12, i+13, i+14, i+15};
            int32x4_t rx3 = vaddq_s32(vi3, vrand_x3);
            int32x4_t ry3 = vaddq_s32(vj, vrand_y3);
            rx3 = vmaxq_s32(vx, vminq_s32(vx_end, rx3));
            ry3 = vmaxq_s32(vy, vminq_s32(vy_end, ry3));
            int32x4_t rpos3 = vmlaq_s32(rx3, ry3, vwidth);
            
            // Extract the positions into scalar arrays.
            int rpos_arr0[4], rpos_arr1[4], rpos_arr2[4], rpos_arr3[4];
            vst1q_s32(rpos_arr0, rpos0);
            vst1q_s32(rpos_arr1, rpos1);
            vst1q_s32(rpos_arr2, rpos2);
            vst1q_s32(rpos_arr3, rpos3);
            
            // Gather the pixel values in a batch.
            uint32_t pixels[16];
            for (int k = 0; k < 4; k++) {
                pixels[k]      = args[rpos_arr0[k]];
                pixels[k + 4]  = args[rpos_arr1[k]];
                pixels[k + 8]  = args[rpos_arr2[k]];
                pixels[k + 12] = args[rpos_arr3[k]];
            }
            
            // Store the results in a batch.
            uint32x4_t pixels0 = vld1q_u32(pixels);
            uint32x4_t pixels1 = vld1q_u32(&pixels[4]);
            uint32x4_t pixels2 = vld1q_u32(&pixels[8]);
            uint32x4_t pixels3 = vld1q_u32(&pixels[12]);
            vst1q_u32(&args[j * width + i],      pixels0);
            vst1q_u32(&args[j * width + i + 4],  pixels1);
            vst1q_u32(&args[j * width + i + 8],  pixels2);
            vst1q_u32(&args[j * width + i + 12], pixels3);
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
    
    // NEON-optimized horizontal blur, processing 16 pixels at a time.
    for (int j = 0; j < h; j++) {
        // Preload data into the cache.
        __asm volatile("prfm pldl1keep, [%0, #256]\n\t" : : "r"(&pixels[(y + j) * width + x]));
        
        for (int i = 0; i < w; i += 16) {
            if (i + 16 > w) {
                // Handle the remaining pixels when fewer than 16 are left.
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
            
            // Process 16 pixels in parallel.
            float32x4_t accum[16] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // Process 16 pixels in parallel.
                for (int k = 0; k < 16; k++) {
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
            }
            
            // 转换为整数并存储
            for (int k = 0; k < 16; k++) {
                uint32x4_t result = vcvtq_u32_f32(accum[k]);
                uint8x8_t res8 = vmovn_u16(vcombine_u16(
                    vmovn_u32(result),
                    vmovn_u32(result)
                ));
                temp[j * w + i + k] = vget_lane_u32(vreinterpret_u32_u8(res8), 0);
            }
        }
    }
    
    // NEON优化垂直模糊，并行处理16个像素
    for (int j = 0; j < h; j += 16) {
        if (j + 16 > h) {
            // 处理剩余不足16个像素的情况
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
            
            // 并行处理16个像素
            float32x4_t accum[16] = {
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f),
                vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f), vdupq_n_f32(0.0f)
            };
            
            for (int m = -radius; m <= radius; m++) {
                float weight = kernel[m + radius];
                
                // 处理16个像素
                for (int k = 0; k < 16; k++) {
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
            for (int k = 0; k < 16; k++) {
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

inline void graph_glass_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_glass_neon(g, x, y, w, h, r);
}

inline void graph_gaussian_arch(graph_t* g, int x, int y, int w, int h, int r) {
    graph_gaussian_neon(g, x, y, w, h, r);
}

void graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    /* whole-surface scale: handled by the g2d engine (bilinear with
       integer-decimation and separable-upscale fast paths) */
    arch_g2d_scale_to(g->buffer, g->w, g->h, dst->buffer, dst->w, dst->h);
}

void graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    arch_g2d_scale_to(g->buffer, g->w, g->h, dst->buffer, dst->w, dst->h);
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

static inline uint8x16_t neon_rgb_to_y16(uint8x16_t b, uint8x16_t g, uint8x16_t r) {
    return vcombine_u8(neon_rgb_to_y8(vget_low_u8(b), vget_low_u8(g), vget_low_u8(r)),
                       neon_rgb_to_y8(vget_high_u8(b), vget_high_u8(g), vget_high_u8(r)));
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

void argb_2_nv12_arch(uint8_t *out, uint32_t *in, int w, int h) {
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
            for(; x + 15 < w; x += 16) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                const uint32_t *src_row1 = in + (h - 2 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[16];
                uint32_t row1_pixels[16];
                uint8x16_t yv0;
                uint8x16_t yv1;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));
                vst1q_u32(row0_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row0 - 11)));
                vst1q_u32(row0_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row0 - 15)));
                vst1q_u32(row1_pixels, neon_reverse_u32x4(vld1q_u32(src_row1 - 3)));
                vst1q_u32(row1_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row1 - 7)));
                vst1q_u32(row1_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row1 - 11)));
                vst1q_u32(row1_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row1 - 15)));

                uint8x16x4_t bgra0 = vld4q_u8((const uint8_t*)row0_pixels);
                uint8x16x4_t bgra1 = vld4q_u8((const uint8_t*)row1_pixels);

                yv0 = neon_rgb_to_y16(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                yv1 = neon_rgb_to_y16(bgra1.val[0], bgra1.val[1], bgra1.val[2]);

                vst1q_u8(y_row0 + x, yv0);
                vst1q_u8(y_row1 + x, yv1);

                rgb_to_uv_scalar_bsp(row0_pixels[0], uv_row + x);
                rgb_to_uv_scalar_bsp(row0_pixels[2], uv_row + x + 2);
                rgb_to_uv_scalar_bsp(row0_pixels[4], uv_row + x + 4);
                rgb_to_uv_scalar_bsp(row0_pixels[6], uv_row + x + 6);
                rgb_to_uv_scalar_bsp(row0_pixels[8], uv_row + x + 8);
                rgb_to_uv_scalar_bsp(row0_pixels[10], uv_row + x + 10);
                rgb_to_uv_scalar_bsp(row0_pixels[12], uv_row + x + 12);
                rgb_to_uv_scalar_bsp(row0_pixels[14], uv_row + x + 14);
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
            for(; x + 15 < w; x += 16) {
                const uint32_t *src_row0 = in + (h - 1 - y) * w + (w - 1 - x);
                uint32_t row0_pixels[16];
                uint8x16_t yv0;

                vst1q_u32(row0_pixels, neon_reverse_u32x4(vld1q_u32(src_row0 - 3)));
                vst1q_u32(row0_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row0 - 7)));
                vst1q_u32(row0_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row0 - 11)));
                vst1q_u32(row0_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row0 - 15)));

                uint8x16x4_t bgra0 = vld4q_u8((const uint8_t*)row0_pixels);
                yv0 = neon_rgb_to_y16(bgra0.val[0], bgra0.val[1], bgra0.val[2]);
                vst1q_u8(y_row0 + x, yv0);
            }

            for(; x < w; ++x) {
                y_row0[x] = rgb_to_y_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
            }
        }
    }
}

static inline uint16_t rgb_to_555_scalar_bsp(uint32_t pixel) {
    uint32_t b = pixel & 0xff;
    uint32_t g = (pixel >> 8) & 0xff;
    uint32_t r = (pixel >> 16) & 0xff;
    return (uint16_t)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
}

static inline uint16x8_t neon_rgb_to_555_u8x8(uint8x8_t b8, uint8x8_t g8, uint8x8_t r8) {
    /* 0RRRRRGGGGGBBBBB: widen each channel into the high byte, then
       shift-right-insert keeps the top bits of the previous channels */
    uint16x8_t d = vshrq_n_u16(vshll_n_u8(r8, 8), 1);
    d = vsriq_n_u16(d, vshll_n_u8(g8, 8), 6);
    d = vsriq_n_u16(d, vshll_n_u8(b8, 8), 11);
    return d;
}

void argb_2_rgb15_arch(uint16_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    for(int y = 0; y < h; y++) {
        uint16_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            const uint32_t *src_row = in + (h - 1 - y) * w + (w - 1 - x);
            uint32_t row_pixels[16];

            vst1q_u32(row_pixels, neon_reverse_u32x4(vld1q_u32(src_row - 3)));
            vst1q_u32(row_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row - 7)));
            vst1q_u32(row_pixels + 8, neon_reverse_u32x4(vld1q_u32(src_row - 11)));
            vst1q_u32(row_pixels + 12, neon_reverse_u32x4(vld1q_u32(src_row - 15)));

            uint8x16x4_t bgra = vld4q_u8((const uint8_t*)row_pixels);
            vst1q_u16(dst_row + x,
                neon_rgb_to_555_u8x8(vget_low_u8(bgra.val[0]),
                    vget_low_u8(bgra.val[1]), vget_low_u8(bgra.val[2])));
            vst1q_u16(dst_row + x + 8,
                neon_rgb_to_555_u8x8(vget_high_u8(bgra.val[0]),
                    vget_high_u8(bgra.val[1]), vget_high_u8(bgra.val[2])));
        }

        for(; x < w; ++x) {
            dst_row[x] = rgb_to_555_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
        }
    }
}

/*
 *  XRGB1555 -> ARGB8888 (NEON, 16 pixels per iteration).
 *  Straight linear scan (no rotation), the inverse of argb_2_rgb15_arch.
 */
void rgb15_2_argb_arch(uint32_t *out, uint16_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint16x8_t mask_r = vdupq_n_u16(0x7c00);
    const uint16x8_t mask_g = vdupq_n_u16(0x03e0);
    const uint16x8_t mask_b = vdupq_n_u16(0x001f);

    for(int y = 0; y < h; y++) {
        const uint16_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            uint16x8_t px0 = vld1q_u16(src_row + x);
            uint16x8_t px1 = vld1q_u16(src_row + x + 8);

            /* --- first 8 pixels --- */
            uint16x8_t r5_0 = vshrq_n_u16(vandq_u16(px0, mask_r), 10);
            uint16x8_t g5_0 = vshrq_n_u16(vandq_u16(px0, mask_g), 5);
            uint16x8_t b5_0 = vandq_u16(px0, mask_b);
            uint16x8_t r8_0 = vorrq_u16(vshlq_n_u16(r5_0, 3), vshrq_n_u16(r5_0, 2));
            uint16x8_t g8_0 = vorrq_u16(vshlq_n_u16(g5_0, 3), vshrq_n_u16(g5_0, 2));
            uint16x8_t b8_0 = vorrq_u16(vshlq_n_u16(b5_0, 3), vshrq_n_u16(b5_0, 2));
            uint16x8_t ar0 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_0);
            uint16x8_t gb0 = vorrq_u16(vshlq_n_u16(g8_0, 8), b8_0);
            uint32x4_t lo0 = vorrq_u32(vshll_n_u16(vget_low_u16(ar0), 16),
                                       vmovl_u16(vget_low_u16(gb0)));
            uint32x4_t hi0 = vorrq_u32(vshll_n_u16(vget_high_u16(ar0), 16),
                                       vmovl_u16(vget_high_u16(gb0)));

            /* --- second 8 pixels --- */
            uint16x8_t r5_1 = vshrq_n_u16(vandq_u16(px1, mask_r), 10);
            uint16x8_t g5_1 = vshrq_n_u16(vandq_u16(px1, mask_g), 5);
            uint16x8_t b5_1 = vandq_u16(px1, mask_b);
            uint16x8_t r8_1 = vorrq_u16(vshlq_n_u16(r5_1, 3), vshrq_n_u16(r5_1, 2));
            uint16x8_t g8_1 = vorrq_u16(vshlq_n_u16(g5_1, 3), vshrq_n_u16(g5_1, 2));
            uint16x8_t b8_1 = vorrq_u16(vshlq_n_u16(b5_1, 3), vshrq_n_u16(b5_1, 2));
            uint16x8_t ar1 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_1);
            uint16x8_t gb1 = vorrq_u16(vshlq_n_u16(g8_1, 8), b8_1);
            uint32x4_t lo1 = vorrq_u32(vshll_n_u16(vget_low_u16(ar1), 16),
                                       vmovl_u16(vget_low_u16(gb1)));
            uint32x4_t hi1 = vorrq_u32(vshll_n_u16(vget_high_u16(ar1), 16),
                                       vmovl_u16(vget_high_u16(gb1)));

            vst1q_u32(dst_row + x,      lo0);
            vst1q_u32(dst_row + x +  4, hi0);
            vst1q_u32(dst_row + x +  8, lo1);
            vst1q_u32(dst_row + x + 12, hi1);
        }

        for(; x < w; ++x) {
            uint16_t v = src_row[x];
            uint32_t r = (v >> 10) & 0x1f;
            uint32_t g = (v >>  5) & 0x1f;
            uint32_t b =  v        & 0x1f;
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            dst_row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

/*
 *  ARGB8888 -> RGB24 (NEON, 8 pixels per iteration): strip alpha.
 */
void argb_2_rgb24_arch(uint32_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t mask = vdupq_n_u32(0x00ffffffu);

    for(int y = 0; y < h; y++) {
        const uint32_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint32x4_t lo = vld1q_u32(src_row + x);
            uint32x4_t hi = vld1q_u32(src_row + x + 4);
            vst1q_u32(dst_row + x,     vandq_u32(lo, mask));
            vst1q_u32(dst_row + x + 4, vandq_u32(hi, mask));
        }

        for(; x < w; ++x)
            dst_row[x] = src_row[x] & 0x00ffffffu;
    }
}

/*
 *  RGB24 -> ARGB8888 (NEON, 8 pixels per iteration): set alpha to 0xFF.
 */
void rgb24_2_argb_arch(uint32_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t alpha = vdupq_n_u32(0xff000000u);
    const uint32x4_t mask  = vdupq_n_u32(0x00ffffffu);

    for(int y = 0; y < h; y++) {
        const uint32_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint32x4_t lo = vld1q_u32(src_row + x);
            uint32x4_t hi = vld1q_u32(src_row + x + 4);
            vst1q_u32(dst_row + x,     vorrq_u32(vandq_u32(lo, mask), alpha));
            vst1q_u32(dst_row + x + 4, vorrq_u32(vandq_u32(hi, mask), alpha));
        }

        for(; x < w; ++x)
            dst_row[x] = 0xff000000u | (src_row[x] & 0x00ffffffu);
    }
}

/*
 * Big-endian [00][RR][GG][BB] byte stream -> ARGB8888.
 * vrev32_u8 reverses each 32-bit word from [00][RR][GG][BB] to [RR][GG][BB][00],
 * then we OR in the alpha byte.
 */
void rgb24be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t alpha = vdupq_n_u32(0xff000000u);

    for(int y = 0; y < h; y++) {
        const uint8_t *src_row = in + y * bpr;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 7 < w; x += 8) {
            uint8x16_t raw0 = vld1q_u8(src_row +  x      * 4);
            uint8x16_t raw1 = vld1q_u8(src_row + (x + 4) * 4);
            uint32x4_t r0 = vorrq_u32(vreinterpretq_u32_u8(vrev32q_u8(raw0)), alpha);
            uint32x4_t r1 = vorrq_u32(vreinterpretq_u32_u8(vrev32q_u8(raw1)), alpha);
            vst1q_u32(dst_row + x,     r0);
            vst1q_u32(dst_row + x + 4, r1);
        }

        for(; x < w; ++x) {
            const uint8_t *p = src_row + x * 4;
            dst_row[x] = 0xff000000u |
                         ((uint32_t)p[1] << 16) |
                         ((uint32_t)p[2] <<  8) |
                          (uint32_t)p[3];
        }
    }
}

/*
 * Big-endian XRGB1555 byte stream -> ARGB8888.
 * vrev32_u8 swaps each 32-bit word so the two 16-bit halves
 * become host-order XRGB1555, then expands 5-bit channels to 8.
 */
void rgb15be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h)
{
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint16x8_t mask_r = vdupq_n_u16(0x7c00);
    const uint16x8_t mask_g = vdupq_n_u16(0x03e0);
    const uint16x8_t mask_b = vdupq_n_u16(0x001f);

    for(int y = 0; y < h; y++) {
        const uint8_t *src_row = in + y * bpr;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 15 < w; x += 16) {
            /* Load 16 pixels as raw bytes, swap hi/lo within each 16-bit pixel */
            uint8x16_t raw0 = vld1q_u8(src_row +  x      * 2);
            uint8x16_t raw1 = vld1q_u8(src_row + (x + 8) * 2);
            uint16x8_t px0 = vreinterpretq_u16_u8(vrev16q_u8(raw0));
            uint16x8_t px1 = vreinterpretq_u16_u8(vrev16q_u8(raw1));

            /* --- first 8 pixels --- */
            uint16x8_t r5_0 = vshrq_n_u16(vandq_u16(px0, mask_r), 10);
            uint16x8_t g5_0 = vshrq_n_u16(vandq_u16(px0, mask_g), 5);
            uint16x8_t b5_0 = vandq_u16(px0, mask_b);
            uint16x8_t r8_0 = vorrq_u16(vshlq_n_u16(r5_0, 3), vshrq_n_u16(r5_0, 2));
            uint16x8_t g8_0 = vorrq_u16(vshlq_n_u16(g5_0, 3), vshrq_n_u16(g5_0, 2));
            uint16x8_t b8_0 = vorrq_u16(vshlq_n_u16(b5_0, 3), vshrq_n_u16(b5_0, 2));
            uint16x8_t ar0 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_0);
            uint16x8_t gb0 = vorrq_u16(vshlq_n_u16(g8_0, 8), b8_0);
            uint32x4_t lo0 = vorrq_u32(vshll_n_u16(vget_low_u16(ar0), 16),
                                       vmovl_u16(vget_low_u16(gb0)));
            uint32x4_t hi0 = vorrq_u32(vshll_n_u16(vget_high_u16(ar0), 16),
                                       vmovl_u16(vget_high_u16(gb0)));

            /* --- second 8 pixels --- */
            uint16x8_t r5_1 = vshrq_n_u16(vandq_u16(px1, mask_r), 10);
            uint16x8_t g5_1 = vshrq_n_u16(vandq_u16(px1, mask_g), 5);
            uint16x8_t b5_1 = vandq_u16(px1, mask_b);
            uint16x8_t r8_1 = vorrq_u16(vshlq_n_u16(r5_1, 3), vshrq_n_u16(r5_1, 2));
            uint16x8_t g8_1 = vorrq_u16(vshlq_n_u16(g5_1, 3), vshrq_n_u16(g5_1, 2));
            uint16x8_t b8_1 = vorrq_u16(vshlq_n_u16(b5_1, 3), vshrq_n_u16(b5_1, 2));
            uint16x8_t ar1 = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8_1);
            uint16x8_t gb1 = vorrq_u16(vshlq_n_u16(g8_1, 8), b8_1);
            uint32x4_t lo1 = vorrq_u32(vshll_n_u16(vget_low_u16(ar1), 16),
                                       vmovl_u16(vget_low_u16(gb1)));
            uint32x4_t hi1 = vorrq_u32(vshll_n_u16(vget_high_u16(ar1), 16),
                                       vmovl_u16(vget_high_u16(gb1)));

            vst1q_u32(dst_row + x,      lo0);
            vst1q_u32(dst_row + x +  4, hi0);
            vst1q_u32(dst_row + x +  8, lo1);
            vst1q_u32(dst_row + x + 12, hi1);
        }

        for(; x < w; ++x) {
            const uint8_t *p = src_row + x * 2;
            uint16_t v = ((uint16_t)p[0] << 8) | p[1];
            uint32_t r = (v >> 10) & 0x1f;
            uint32_t g = (v >>  5) & 0x1f;
            uint32_t b =  v        & 0x1f;
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            dst_row[x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

void graph_rotate_to_arch(graph_t* g, graph_t* ret, int rot) {
    if(g == NULL || ret == NULL ||
            g->buffer == NULL || ret->buffer == NULL ||
            g->w <= 0 || g->h <= 0)
        return;

    if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
        if(ret->w < g->h || ret->h < g->w)
            return;
    }
    else if(rot == G_ROTATE_180) {
        if(ret->w < g->w || ret->h < g->h)
            return;
    }
    else
        return;

    /* quadrant rotations are implemented by the g2d engine (rot codes map
       1:1 to clockwise degrees) */
    arch_g2d_rotate(g->buffer, g->w, g->h, ret->buffer, ret->w, ret->h, rot * 90);
}

#endif

#ifdef __cplusplus
}
#endif
