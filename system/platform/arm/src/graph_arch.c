#include <graph/graph_arch.h>
#include <g2d_arch.h>
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

static inline uint32_t fill_div255(uint32_t v)
{
    return (v + 1 + (v >> 8)) >> 8;
}

/* scalar blend of one pixel against a constant color, bit-identical math
   to neon_fill_alpha_16 (used for the head/tail alignment pixels) */
static inline uint32_t fill_blend_px(uint32_t dst, uint32_t color, uint32_t a)
{
    uint32_t inv_a = 255 - a;
    uint32_t db = dst & 0xff;
    uint32_t dg = (dst >> 8) & 0xff;
    uint32_t dr = (dst >> 16) & 0xff;
    uint32_t da = (dst >> 24) & 0xff;
    uint32_t ob = fill_div255((color & 0xff) * a + db * inv_a);
    uint32_t og = fill_div255(((color >> 8) & 0xff) * a + dg * inv_a);
    uint32_t or_ = fill_div255(((color >> 16) & 0xff) * a + dr * inv_a);
    uint32_t oa = da + fill_div255((255 - da) * a);
    return (oa << 24) | (or_ << 16) | (og << 8) | ob;
}

/* Solid-color source-over blend, 16 pixels per call. The fg color and its
   effective alpha are loop-invariant for a fill, so the per-channel fg*a
   products are precomputed once (cpa_*); per block only the bg load, three
   bg*inv_a multiplies, four div255 and the store remain. The caller must
   pass a 16-byte aligned d (Device-mapped surfaces fault on unaligned
   access). */
static inline void neon_fill_alpha_16(uint32_t *d,
        uint16x8_t cpa_b, uint16x8_t cpa_g, uint16x8_t cpa_r,
        uint8x8_t inv_a8, uint8x8_t a8)
{
    uint8x16x4_t bg = vld4q_u8((const uint8_t*)d);
    uint8x16x4_t out;
    uint8x8_t full8 = vdup_n_u8(0xff);

    uint16x8_t oa_lo = neon_div255_u16(vmull_u8(vsub_u8(full8, vget_low_u8(bg.val[3])), a8));
    uint16x8_t oa_hi = neon_div255_u16(vmull_u8(vsub_u8(full8, vget_high_u8(bg.val[3])), a8));

    uint16x8_t cpa[3];
    cpa[0] = cpa_b;
    cpa[1] = cpa_g;
    cpa[2] = cpa_r;

    /* out = div255(color_c*a + bg_c*inv_a) per channel, low/high widened */
    for(int c = 0; c < 3; c++) {
        uint16x8_t lo = vaddq_u16(cpa[c], vmull_u8(vget_low_u8(bg.val[c]), inv_a8));
        uint16x8_t hi = vaddq_u16(cpa[c], vmull_u8(vget_high_u8(bg.val[c]), inv_a8));
        out.val[c] = vcombine_u8(vmovn_u16(neon_div255_u16(lo)), vmovn_u16(neon_div255_u16(hi)));
    }
    /* out_a = bg_a + div255((255-bg_a)*a) */
    out.val[3] = vcombine_u8(
        vmovn_u16(vaddq_u16(vmovl_u8(vget_low_u8(bg.val[3])), oa_lo)),
        vmovn_u16(vaddq_u16(vmovl_u8(vget_high_u8(bg.val[3])), oa_hi)));

    vst4q_u8((uint8_t*)d, out);
}

void graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if(g == NULL || w <= 0 || h <= 0)
        return;
    grect_t r = {x, y, w, h};
    if(!graph_insect(g, &r))
        return;
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);
    if(r.w <= 0 || r.h <= 0)
        return;

    /* opaque fill: handled by the g2d engine (memset / NEON row stores) */
    if(color_a(color) == 0xff) {
        arch_g2d_fill(g->buffer, g->shm_contig ? 1 : 0, g->w, g->h, r.x, r.y, r.w, r.h, color);
        return;
    }

    /* fully transparent fill is a no-op */
    if(color_a(color) == 0)
        return;

    /* translucent fill: blend the solid color against what is there. The
       color and its effective alpha are constants, so every per-channel
       constant is hoisted out of the row loop; scalar head/tail pixels
       keep all NEON accesses 16-byte aligned. */
    uint32_t ca = color_a(color);
    uint8x8_t a8 = vdup_n_u8((uint8_t)ca);
    uint8x8_t inv_a8 = vdup_n_u8((uint8_t)(255 - ca));
    uint16x8_t cpa_b = vmull_u8(vdup_n_u8((uint8_t)color), a8);
    uint16x8_t cpa_g = vmull_u8(vdup_n_u8((uint8_t)(color >> 8)), a8);
    uint16x8_t cpa_r = vmull_u8(vdup_n_u8((uint8_t)(color >> 16)), a8);

    int32_t ey = r.y + r.h;
    for(int32_t yy = r.y; yy < ey; yy++) {
        uint32_t* row = g->buffer + yy * g->w + r.x;
        int32_t n = r.w;
        int32_t i = 0;

        uintptr_t ra = (uintptr_t)row;
        if(ra & 0xF) {
            int32_t head = (int32_t)((16 - (ra & 0xF)) >> 2);
            if(head > n)
                head = n;
            for(; i < head; i++)
                row[i] = fill_blend_px(row[i], color, ca);
        }
        for(; i <= n - 16; i += 16)
            neon_fill_alpha_16(row + i, cpa_b, cpa_g, cpa_r, inv_a8, a8);
        for(; i < n; i++)
            row[i] = fill_blend_px(row[i], color, ca);
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
    arch_g2d_blt(src->buffer, src->shm_contig ? 1 : 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, dst->shm_contig ? 1 : 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h);
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
    arch_g2d_blt_alpha(src->buffer, src->shm_contig ? 1 : 0, src->w, src->h, sr.x, sr.y, sr.w, sr.h,
            dst->buffer, dst->shm_contig ? 1 : 0, dst->w, dst->h, dr.x, dr.y, sr.w, sr.h, alpha);
}

static inline void neon_mask_alpha_8(uint32_t *dst, uint32_t *src)
{
    __asm volatile(
        // Load 8 dst pixels (RGBA)
        "vld4.8    {d20-d23},[%0]\n\t"
        // Load 8 src pixels (RGBA)
        "vld4.8    {d24-d27},[%1]\n\t"
        
        // d23 = dst_a, d27 = src_a
        // Create zero vector
        "vmov.u8   d28, #0\n\t"
        
        // Compare src_a > 0 (vcgt returns 0xFF where true, 0x00 where false)
        "vcgt.u8   d29, d27, d28\n\t"  // d29: 0xFF where src_a > 0, 0x00 where src_a == 0
        
        // Create mask for src_a == 0: set all channels to 0
        "vand.u8   d20, d20, d29\n\t"    // R
        "vand.u8   d21, d21, d29\n\t"    // G
        "vand.u8   d22, d22, d29\n\t"    // B
        "vand.u8   d23, d23, d29\n\t"    // A
        
        // Compare dst_a > src_a
        "vcgt.u8   d30, d23, d27\n\t"   // d30: 0xFF where dst_a > src_a
        
        // For dst_a > src_a: keep dst RGB, replace alpha with src_a
        // First, mask src_a where condition is true
        "vand.u8   d31, d27, d30\n\t"
        // Mask dst_a where condition is false
        "vmvn.u8   d28, d30\n\t"
        "vand.u8   d23, d23, d28\n\t"
        // Combine
        "vorr.u8   d23, d23, d31\n\t"
        
        // Store result
        "vst4.8   {d20-d23},[%0]\n\t"
        :
        : "r"(dst), "r"(src)
        : "memory", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", 
          "d28", "d29", "d30", "d31");
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

    for(; sy < ey; sy++, dy++) {
        register int32_t sx = sr.x;
        register int32_t dx = dr.x;
        register int32_t offset = sy * src->w;
        for(; sx < ex; sx+=8, dx+=8) {
            graph_pixel_alpha_mask_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 8));	
        }
    }
}

static bool seeded = false;
static void glass_neon(uint32_t* args, int width, int height, 
                int x, int y, int w, int h, int r) {
    // Parameter check
    if (!args || r <= 0 || w <= 0 || h <= 0 || width <= 0 || height <= 0) 
        return;
    if (x < 0 || y < 0 || x + w > width || y + h > height)
        return;

    uint32_t *tmp = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if(tmp == NULL)
        return;
    memcpy(tmp, args, width * height * sizeof(uint32_t));

    // Use fixed random seed to ensure consistent effect
    if (!seeded) {
        srand(0x12345678);  // Fixed seed value
    }

    // Pre-calculate common values
    int range = 2*r;
    int x_end = x + w - 1;
    int y_end = y + h - 1;

    // NEON register initialization
    int32x4_t vradius = vdupq_n_s32(r);
    int32x4_t vrange = vdupq_n_s32(range);
    int32x4_t vx = vdupq_n_s32(x);
    int32x4_t vy = vdupq_n_s32(y);
    int32x4_t vx_end = vdupq_n_s32(x_end);
    int32x4_t vy_end = vdupq_n_s32(y_end);
    int32x4_t vwidth = vdupq_n_s32(width);

    // Pre-generate all needed random numbers
    int total_pixels = w * h;
    int* rand_offsets = malloc(total_pixels * 2 * sizeof(int));

    for (int i = 0; i < total_pixels * 2; i++) {
        rand_offsets[i] = (rand() % range) - r;
    }

    // Process image area
    int offset_index = 0;
    for (int j = y; j <= y_end; j++) {
        int32x4_t vj = vdupq_n_s32(j);
        
        for (int i = x; i <= x_end; i += 4) {
            // Handle remaining less than 4 pixels
            int remaining = x_end - i + 1;
            if (remaining < 4) {
                for (int k = 0; k < remaining; k++) {
                    int rx = i + k + rand_offsets[offset_index++];
                    int ry = j + rand_offsets[offset_index++];
                    
                    // Boundary check
                    rx = (rx < x) ? x : ((rx > x_end) ? x_end : rx);
                    ry = (ry < y) ? y : ((ry > y_end) ? y_end : ry);
                    
                    args[j * width + i + k] = args[ry * width + rx];
                }
                break;
            }
            
            // Generate random offsets for 4 pixels
            int rand_x[4], rand_y[4];
            for (int k = 0; k < 4; k++) {
                rand_x[k] = rand_offsets[offset_index++];
                rand_y[k] = rand_offsets[offset_index++];
            }
            
            // Load random offsets to NEON registers
            int32x4_t vrand_x = vld1q_s32(rand_x);
            int32x4_t vrand_y = vld1q_s32(rand_y);
            
            // Calculate current x position
            int32x4_t vi = {i, i+1, i+2, i+3};
            
            // Calculate random position
            int32x4_t rx = vaddq_s32(vi, vrand_x);
            int32x4_t ry = vaddq_s32(vj, vrand_y);
            
            // Boundary check
            rx = vmaxq_s32(vx, vminq_s32(vx_end, rx));
            ry = vmaxq_s32(vy, vminq_s32(vy_end, ry));
            
            // Calculate random pixel position (ry * width + rx)
            int32x4_t rpos = vmlaq_s32(rx, ry, vwidth);
            
            // Extract positions to array
            int rpos_arr[4];
            vst1q_s32(rpos_arr, rpos);
            
            // Manually gather pixel values
            uint32_t pixels[4];
            for (int k = 0; k < 4; k++) {
                pixels[k] = tmp[rpos_arr[k]];
            }
            
            // Store result
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
    
    // Boundary check
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;
    if (w <= 0 || h <= 0) return;
    
    // Create Gaussian kernel
    int kernel_size = radius * 2 + 1;
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    float sigma = radius / 2.0f;
    float sum = 0.0f;
    
    for (int i = -radius; i <= radius; i++) {
        float val = expf(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = val;
        sum += val;
    }
    
    // Normalize
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    // Temporary buffer
    uint32_t* temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    
    // NEON optimized horizontal blur, process 4 pixels in parallel
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i += 4) {
            if (i + 4 > w) {
                // Handle remaining less than 4 pixels
                for (int k = i; k < w; k++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int px = x + k + m;
                        if (px < x) px = x;
                        if (px >= x + w) px = x + w - 1;
                        
                        uint32_t pixel = pixels[(y + j) * width + px];
                        float weight = kernel[m + radius];
                        
                        // Extract ARGB channels
                        uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                        uint16x8_t vPixel16 = vmovl_u8(vPixel);
                        uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                        float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                        
                        // Multiply by weight and accumulate
                        accum = vmlaq_n_f32(accum, vPixelF, weight);
                    }
                    
                    // Convert to integer and store
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
                    
                    // Extract ARGB channels
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // Multiply by weight and accumulate
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // Convert to integer and store
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
    
    // NEON optimized vertical blur, process 4 pixels in parallel
    for (int j = 0; j < h; j += 4) {
        if (j + 4 > h) {
            // Handle remaining less than 4 pixels
            for (int k = j; k < h; k++) {
                for (int i = 0; i < w; i++) {
                    float32x4_t accum = vdupq_n_f32(0.0f);
                    
                    for (int m = -radius; m <= radius; m++) {
                        int py = y + k + m;
                        if (py < y) py = y;
                        if (py >= y + h) py = y + h - 1;
                        
                        uint32_t pixel = temp[(py - y) * w + i];
                        float weight = kernel[m + radius];
                        
                        // Extract ARGB channels
                        uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                        uint16x8_t vPixel16 = vmovl_u8(vPixel);
                        uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                        float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                        
                        // Multiply by weight and accumulate
                        accum = vmlaq_n_f32(accum, vPixelF, weight);
                    }
                    
                    // Convert to integer and store
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
                    
                    // Extract ARGB channels
                    uint8x8_t vPixel = vreinterpret_u8_u32(vdup_n_u32(pixel));
                    uint16x8_t vPixel16 = vmovl_u8(vPixel);
                    uint32x4_t vPixel32 = vmovl_u16(vget_low_u16(vPixel16));
                    float32x4_t vPixelF = vcvtq_f32_u32(vPixel32);
                    
                    // Multiply by weight and accumulate
                    accum[k] = vmlaq_n_f32(accum[k], vPixelF, weight);
                }
            }
            
            // Convert to integer and store
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
    arch_g2d_scale_to(g->buffer, g->shm_contig ? 1 : 0, g->w, g->h, dst->buffer, dst->shm_contig ? 1 : 0, dst->w, dst->h);
}

void graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    arch_g2d_scale_to(g->buffer, g->shm_contig ? 1 : 0, g->w, g->h, dst->buffer, dst->shm_contig ? 1 : 0, dst->w, dst->h);
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

        for(; x + 7 < w; x += 8) {
            const uint32_t *src_row = in + (h - 1 - y) * w + (w - 1 - x);
            uint32_t row_pixels[8];

            vst1q_u32(row_pixels, neon_reverse_u32x4(vld1q_u32(src_row - 3)));
            vst1q_u32(row_pixels + 4, neon_reverse_u32x4(vld1q_u32(src_row - 7)));

            uint8x8x4_t bgra = vld4_u8((const uint8_t*)row_pixels);
            vst1q_u16(dst_row + x,
                neon_rgb_to_555_u8x8(bgra.val[0], bgra.val[1], bgra.val[2]));
        }

        for(; x < w; ++x) {
            dst_row[x] = rgb_to_555_scalar_bsp(rgb2nv12_get_src_pixel(in, w, h, y, x));
        }
    }
}
/*
 *  XRGB1555 -> ARGB8888 (NEON, 8 pixels per iteration).
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

        for(; x + 7 < w; x += 8) {
            uint16x8_t px = vld1q_u16(src_row + x);

            /* 5-bit fields, shifted down to bits 0-4 */
            uint16x8_t r5 = vshrq_n_u16(vandq_u16(px, mask_r), 10);
            uint16x8_t g5 = vshrq_n_u16(vandq_u16(px, mask_g), 5);
            uint16x8_t b5 = vandq_u16(px, mask_b);

            /* (x << 3) | (x >> 2) within 16-bit lanes */
            uint16x8_t r8 = vorrq_u16(vshlq_n_u16(r5, 3), vshrq_n_u16(r5, 2));
            uint16x8_t g8 = vorrq_u16(vshlq_n_u16(g5, 3), vshrq_n_u16(g5, 2));
            uint16x8_t b8 = vorrq_u16(vshlq_n_u16(b5, 3), vshrq_n_u16(b5, 2));

            /* pack into 0xAARRGGBB */
            uint16x8_t ar = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8);
            uint16x8_t gb = vorrq_u16(vshlq_n_u16(g8, 8), b8);
            uint32x4_t lo = vorrq_u32(vshll_n_u16(vget_low_u16(ar), 16),
                                      vmovl_u16(vget_low_u16(gb)));
            uint32x4_t hi = vorrq_u32(vshll_n_u16(vget_high_u16(ar), 16),
                                      vmovl_u16(vget_high_u16(gb)));

            vst1q_u32(dst_row + x,     lo);
            vst1q_u32(dst_row + x + 4, hi);
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
 *  ARGB8888 -> RGB24 (NEON, 4 pixels per iteration): strip alpha.
 */
void argb_2_rgb24_arch(uint32_t *out, uint32_t *in, int w, int h) {
    if(out == NULL || in == NULL || w <= 0 || h <= 0)
        return;

    const uint32x4_t mask = vdupq_n_u32(0x00ffffffu);

    for(int y = 0; y < h; y++) {
        const uint32_t *src_row = in + y * w;
        uint32_t *dst_row = out + y * w;
        int x = 0;

        for(; x + 3 < w; x += 4) {
            uint32x4_t px = vld1q_u32(src_row + x);
            vst1q_u32(dst_row + x, vandq_u32(px, mask));
        }

        for(; x < w; ++x)
            dst_row[x] = src_row[x] & 0x00ffffffu;
    }
}

/*
 *  RGB24 -> ARGB8888 (NEON, 4 pixels per iteration): set alpha to 0xFF.
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

        for(; x + 3 < w; x += 4) {
            uint32x4_t px = vld1q_u32(src_row + x);
            vst1q_u32(dst_row + x, vorrq_u32(vandq_u32(px, mask), alpha));
        }

        for(; x < w; ++x)
            dst_row[x] = 0xff000000u | (src_row[x] & 0x00ffffffu);
    }
}

/*
 * Big-endian [00][RR][GG][BB] byte stream -> ARGB8888.
 * vrev32_u8 reverses each 32-bit word, then OR in alpha.
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

        for(; x + 3 < w; x += 4) {
            uint8x16_t raw = vld1q_u8(src_row + x * 4);
            uint8x16_t rev = vrev32q_u8(raw);
            uint32x4_t px  = vorrq_u32(vreinterpretq_u32_u8(rev), alpha);
            vst1q_u32(dst_row + x, px);
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
 * vrev32_u8 swaps each 32-bit word to host-order XRGB1555,
 * then expands 5-bit channels to 8.
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

        for(; x + 7 < w; x += 8) {
            /* Load 8 pixels as raw bytes, swap hi/lo within each 16-bit pixel */
            uint8x16_t raw = vld1q_u8(src_row + x * 2);
            uint16x8_t px  = vreinterpretq_u16_u8(vrev16q_u8(raw));

            /* 5-bit fields, shifted down to bits 0-4 */
            uint16x8_t r5 = vshrq_n_u16(vandq_u16(px, mask_r), 10);
            uint16x8_t g5 = vshrq_n_u16(vandq_u16(px, mask_g), 5);
            uint16x8_t b5 = vandq_u16(px, mask_b);

            /* (x << 3) | (x >> 2) within 16-bit lanes */
            uint16x8_t r8 = vorrq_u16(vshlq_n_u16(r5, 3), vshrq_n_u16(r5, 2));
            uint16x8_t g8 = vorrq_u16(vshlq_n_u16(g5, 3), vshrq_n_u16(g5, 2));
            uint16x8_t b8 = vorrq_u16(vshlq_n_u16(b5, 3), vshrq_n_u16(b5, 2));

            /* pack into 0xAARRGGBB */
            uint16x8_t ar = vorrq_u16(vdupq_n_u16((int16_t)0xff00), r8);
            uint16x8_t gb = vorrq_u16(vshlq_n_u16(g8, 8), b8);
            uint32x4_t lo = vorrq_u32(vshll_n_u16(vget_low_u16(ar), 16),
                                      vmovl_u16(vget_low_u16(gb)));
            uint32x4_t hi = vorrq_u32(vshll_n_u16(vget_high_u16(ar), 16),
                                      vmovl_u16(vget_high_u16(gb)));

            vst1q_u32(dst_row + x,     lo);
            vst1q_u32(dst_row + x + 4, hi);
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
    arch_g2d_rotate(g->buffer, g->shm_contig ? 1 : 0, g->w, g->h, ret->buffer, ret->shm_contig ? 1 : 0, ret->w, ret->h, rot * 90);
}

#endif

#ifdef __cplusplus
}
#endif
