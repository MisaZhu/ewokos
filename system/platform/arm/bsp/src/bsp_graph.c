#include <graph/bsp_graph.h>
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
		"vld4.8    {d20-d23},[%1]\n\t" // Load foreground (R,G,B,A)
		
		// Load alpha_more and calculate combined alpha
		"mov        r3, %3\n\t"       // Load alpha_more
		"vdup.u8    d31, r3\n\t"

		"vmull.u8   q0, d23, d31\n\t"  // alpha * alpha_more
		"vshr.u16   q0, q0, #8\n\t"    // Divide by 256
		"vmovn.u16  d23, q0\n\t"       // Narrow to 8-bit
		
		"vmov.u8	d28, 0xff\n\t"
		"vsub.u8	d28, d28, d23\n\t"
		"vmull.u8  q1,d20,d23\n\t"
		"vmull.u8  q3,d21,d23\n\t"
		"vmull.u8  q5,d22,d23\n\t"

		"vld4.8    {d24-d27},[%0]\n\t" // Load background
		"vmov.u8	d29, 0xff\n\t"
		"vsub.u8	d29, d29, d27\n\t"
		"vmull.u8  q2,d24,d28\n\t"
		"vmull.u8  q4,d25,d28\n\t"
		"vmull.u8  q6,d26,d28\n\t" // Apply alpha

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
		"vld4.8    {d20-d23},[%0]\n\t" // Load foreground
		"vst4.8   {d20-d23},[%1]\n\t"
		:
		: "r"(s), "r"(d)
		: "memory");
	return;
}

static inline void neon_fill_load(uint32_t *s)
{
	__asm volatile(
		"vld4.8    {d20-d23},[%0]\n\t" // Load foreground
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
    if(g->clip.w > 0 && g->clip.h > 0)
        grect_insect(&g->clip, &r);

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
			graph_pixel_argb_neon(dst, dx, dy, &src->buffer[offset + sx], MIN(ex-sx, 8), alpha);	
		}
	}
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

static inline float get_weight(float x) {
    int index = (int)((x + 3.0f) / 6.0f * (WEIGHT_TABLE_SIZE - 1));
    index = CLAMP(index, 0, WEIGHT_TABLE_SIZE - 1);
    return weight_table[index];
}

static inline float get_weight_fast(float x) {
    float normalized = (x + 3.0f) * (1.0f / 6.0f);
    int index = (int)(normalized * (WEIGHT_TABLE_SIZE - 1));
    if(index < 0) index = 0;
    if(index >= WEIGHT_TABLE_SIZE) index = WEIGHT_TABLE_SIZE - 1;
    return weight_table[index];
}

static inline float get_weight_bilinear(float x) {
    x = fabsf(x);
    if(x >= 1.0f) return 0.0f;
    return 1.0f - x;
}

static inline float get_weight_bicubic_fast(float x) {
    x = fabsf(x);
    if(x >= 2.0f) return 0.0f;
    if(x < 1.0f) {
        return 1.0f - 2.0f*x*x + x*x*x;
    }
    float t = 2.0f - x;
    return t*t*t * 0.5f;
}

static inline uint32_t get_pixel_clamped_bsp(graph_t* g, int x, int y) {
    x = CLAMP(x, 0, g->w - 1);
    y = CLAMP(y, 0, g->h - 1);
    return g->buffer[y * g->w + x];
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

                    sum_r[0] = vmlaq_f32(sum_r[0], r_f, weight0);
                    sum_r[1] = vmlaq_f32(sum_r[1], r_f, weight1);
                    sum_g[0] = vmlaq_f32(sum_g[0], g_f, weight0);
                    sum_g[1] = vmlaq_f32(sum_g[1], g_f, weight1);
                    sum_b[0] = vmlaq_f32(sum_b[0], b_f, weight0);
                    sum_b[1] = vmlaq_f32(sum_b[1], b_f, weight1);
                    sum_a[0] = vmlaq_f32(sum_a[0], a_f, weight0);
                    sum_a[1] = vmlaq_f32(sum_a[1], a_f, weight1);
                    sum_weight[0] = vmlaq_f32(sum_weight[0], vdupq_n_f32(1.0f), weight0);
                    sum_weight[1] = vmlaq_f32(sum_weight[1], vdupq_n_f32(1.0f), weight1);
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

void graph_scale_tof_fast_bsp(graph_t* g, graph_t* dst, double scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;

    #define FIXED_SHIFT 16
    #define FIXED_SCALE (1 << FIXED_SHIFT)
    #define FIXED_MASK (FIXED_SCALE - 1)

    int hmax = g->h - 1;
    int wmax = g->w - 1;
    uint32_t inv_scale = (uint32_t)((1.0f / scale) * FIXED_SCALE);

    for(int i = 0; i < dst->h; i++) {
        uint32_t gi = (i * inv_scale) >> FIXED_SHIFT;
        int gi0 = (int)gi;
        uint32_t gi_frac = (i * inv_scale) & FIXED_MASK;
        int gi1 = gi0 + 1;

        if(gi0 < 0) { gi0 = 0; gi1 = 0; gi_frac = 0; }
        else if(gi0 >= hmax) { gi0 = hmax; gi1 = hmax; gi_frac = 0; }
        if(gi1 > hmax) gi1 = hmax;

        int gi0w = gi0 * g->w;
        int gi1w = gi1 * g->w;

        int j = 0;
        for(; j <= dst->w - 4; j += 4) {
            uint32_t gj[4];
            int gj0[4];
            int gj1[4];
            uint32_t gj_frac[4];

            for(int k = 0; k < 4; k++) {
                gj[k] = (j + k) * inv_scale;
                gj0[k] = (int)(gj[k] >> FIXED_SHIFT);
                gj_frac[k] = gj[k] & FIXED_MASK;
                gj1[k] = gj0[k] + 1;
                if(gj0[k] < 0) { gj0[k] = 0; gj1[k] = 0; gj_frac[k] = 0; }
                else if(gj0[k] > wmax) { gj0[k] = wmax; gj1[k] = wmax; gj_frac[k] = 0; }
                if(gj1[k] > wmax) gj1[k] = wmax;
            }

            uint32_t result[4];
            for(int k = 0; k < 4; k++) {
                uint32_t p00 = g->buffer[gi0w + gj0[k]];
                uint32_t p01 = g->buffer[gi0w + gj1[k]];
                uint32_t p10 = g->buffer[gi1w + gj0[k]];
                uint32_t p11 = g->buffer[gi1w + gj1[k]];

                if(p00 == p01 && p00 == p10 && p00 == p11) {
                    result[k] = p00;
                    continue;
                }

                uint32_t one_minus_fx = FIXED_SCALE - gj_frac[k];
                uint32_t one_minus_fy = FIXED_SCALE - gi_frac;

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
                                 (uint64_t)gj_frac[k] * one_minus_fy * r01 +
                                 (uint64_t)one_minus_fx * gi_frac * r10 +
                                 (uint64_t)gj_frac[k] * gi_frac * r11;
                uint64_t tmp_g = (uint64_t)one_minus_fx * one_minus_fy * g00 +
                                 (uint64_t)gj_frac[k] * one_minus_fy * g01 +
                                 (uint64_t)one_minus_fx * gi_frac * g10 +
                                 (uint64_t)gj_frac[k] * gi_frac * g11;
                uint64_t tmp_b = (uint64_t)one_minus_fx * one_minus_fy * b00 +
                                 (uint64_t)gj_frac[k] * one_minus_fy * b01 +
                                 (uint64_t)one_minus_fx * gi_frac * b10 +
                                 (uint64_t)gj_frac[k] * gi_frac * b11;
                uint64_t tmp_a = (uint64_t)one_minus_fx * one_minus_fy * a00 +
                                 (uint64_t)gj_frac[k] * one_minus_fy * a01 +
                                 (uint64_t)one_minus_fx * gi_frac * a10 +
                                 (uint64_t)gj_frac[k] * gi_frac * a11;

                uint32_t r = tmp_r >> (2 * FIXED_SHIFT);
                uint32_t g_val = tmp_g >> (2 * FIXED_SHIFT);
                uint32_t b = tmp_b >> (2 * FIXED_SHIFT);
                uint32_t a = tmp_a >> (2 * FIXED_SHIFT);

                result[k] = (a << 24) | (r << 16) | (g_val << 8) | b;
            }
            dst->buffer[i * dst->w + j] = result[0];
            dst->buffer[i * dst->w + j + 1] = result[1];
            dst->buffer[i * dst->w + j + 2] = result[2];
            dst->buffer[i * dst->w + j + 3] = result[3];
        }

        for(; j < dst->w; j++) {
            uint32_t gj = j * inv_scale;
            int gj0 = (int)(gj >> FIXED_SHIFT);
            uint32_t gj_frac = gj & FIXED_MASK;
            int gj1 = gj0 + 1;

            if(gj0 < 0) { gj0 = 0; gj1 = 0; gj_frac = 0; }
            else if(gj0 >= wmax) { gj0 = wmax; gj1 = wmax; gj_frac = 0; }
            if(gj1 > wmax) gj1 = wmax;

            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + gj1];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + gj1];

            if(p00 == p01 && p00 == p10 && p00 == p11) {
                dst->buffer[i * dst->w + j] = p00;
                continue;
            }

            uint32_t one_minus_fx = FIXED_SCALE - gj_frac;
            uint32_t one_minus_fy = FIXED_SCALE - gi_frac;

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
                             (uint64_t)gj_frac * one_minus_fy * r01 +
                             (uint64_t)one_minus_fx * gi_frac * r10 +
                             (uint64_t)gj_frac * gi_frac * r11;
            uint64_t tmp_g = (uint64_t)one_minus_fx * one_minus_fy * g00 +
                             (uint64_t)gj_frac * one_minus_fy * g01 +
                             (uint64_t)one_minus_fx * gi_frac * g10 +
                             (uint64_t)gj_frac * gi_frac * g11;
            uint64_t tmp_b = (uint64_t)one_minus_fx * one_minus_fy * b00 +
                             (uint64_t)gj_frac * one_minus_fy * b01 +
                             (uint64_t)one_minus_fx * gi_frac * b10 +
                             (uint64_t)gj_frac * gi_frac * b11;
            uint64_t tmp_a = (uint64_t)one_minus_fx * one_minus_fy * a00 +
                             (uint64_t)gj_frac * one_minus_fy * a01 +
                             (uint64_t)one_minus_fx * gi_frac * a10 +
                             (uint64_t)gj_frac * gi_frac * a11;

            uint32_t r = tmp_r >> (2 * FIXED_SHIFT);
            uint32_t g_val = tmp_g >> (2 * FIXED_SHIFT);
            uint32_t b = tmp_b >> (2 * FIXED_SHIFT);
            uint32_t a = tmp_a >> (2 * FIXED_SHIFT);

            dst->buffer[i * dst->w + j] = (a << 24) | (r << 16) | (g_val << 8) | b;
        }
    }
    #undef FIXED_SHIFT
    #undef FIXED_SCALE
    #undef FIXED_MASK
}

#endif

#ifdef __cplusplus
}
#endif
