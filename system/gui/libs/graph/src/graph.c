#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __cplusplus 
extern "C" { 
#endif

inline uint32_t argb(uint32_t a, uint32_t r, uint32_t g, uint32_t b) {
	return a << 24 | r << 16 | g << 8 | b;
}

inline uint8_t color_a(uint32_t c) {
	return (c >> 24) & 0xff;
}

inline uint8_t color_r(uint32_t c) {
	return (c >> 16) & 0xff;
}

inline uint8_t color_g(uint32_t c) {
	return (c >> 8) & 0xff;
}

inline uint8_t color_b(uint32_t c) {
	return c & 0xff;
}

inline void graph_init(graph_t* g, const uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return;

	g->w = w;
	g->h = h;
	if(buffer != NULL) {
		g->buffer = (uint32_t*)buffer;
		g->need_free = false;
	}
	else {
		g->buffer = (uint32_t*)malloc(w*h*4);
		g->need_free = true;
	}
	memset(&g->clip, 0, sizeof(grect_t));
}

void graph_set_clip(graph_t* g, int x, int y, int w, int h) {
	grect_t r = {x, y, w, h};
	g->clip.x = 0;
	g->clip.y = 0;
	g->clip.w = g->w;
	g->clip.h = g->h;
	grect_insect(&r, &g->clip);
}

void graph_unset_clip(graph_t* g) {
	memset(&g->clip, 0, sizeof(grect_t));
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return NULL;

	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	if(ret != NULL)
		graph_init(ret, buffer, w, h);
	return ret;
}

graph_t* graph_dup(graph_t* g) {
	if(g == NULL || g->buffer == NULL)
		return NULL;

	graph_t* ret = graph_new(NULL, g->w, g->h);
	if(ret == NULL)
		return NULL;
	ret->buffer = (uint32_t*)malloc(g->w*g->h*4);
	if(ret->buffer == NULL) {
		graph_free(ret);
		return NULL;
	}
	memcpy(ret->buffer, g->buffer, g->w*g->h*4);
	return ret;
}

void graph_free(graph_t* g) {
	if(g == NULL)
		return;
	if(g->buffer != NULL && g->need_free)
		free(g->buffer);
	free(g);
}

void graph_clear(graph_t* g, uint32_t color) {
	if(g == NULL)
		return;
	if(g->w == 0 || g->w == 0)
		return;
	int32_t i = 0;
	int32_t sz = g->w * 4;
	while(i<g->w) {
		g->buffer[i] = color;
		++i;
	}
	char* p = (char*)g->buffer;
	for(i=1; i<g->h; ++i) {
		memcpy(p+(i*sz), p, sz);
	}
}

void graph_reverse(graph_t* g) {
	if(g == NULL)
		return;
	int32_t i = 0;
	while(i < g->w*g->h) {
		uint32_t oc = g->buffer[i];
		uint8_t oa = (oc >> 24) & 0xff;
		uint8_t or = 0xff - ((oc >> 16) & 0xff);
		uint8_t og = 0xff - ((oc >> 8)  & 0xff);
		uint8_t ob = 0xff - (oc & 0xff);
		g->buffer[i] = argb(oa, or, og, ob);
		i++;
	}
}

void graph_reverse_rgb(graph_t* g) {
	if(g == NULL)
		return;
	int32_t i = 0;
	while(i < g->w*g->h) {
		uint32_t oc = g->buffer[i];
		uint8_t oa = (oc >> 24) & 0xff;
		uint8_t or = ((oc) & 0xff);
		uint8_t og = ((oc >> 8)  & 0xff);
		uint8_t ob = ((oc >> 16)  & 0xff);
		g->buffer[i] = argb(oa, or, og, ob);
		i++;
	}
}

void graph_gray(graph_t* g) {
	if(g == NULL)
		return;
	int32_t i = 0;
	while(i < g->w*g->h) {
		uint32_t oc = g->buffer[i];
		uint8_t oa = (oc >> 24) & 0xff;
		uint8_t or = (oc >> 16) & 0xff;
		uint8_t og = (oc >> 8)  & 0xff;
		uint8_t ob = oc & 0xff;
		or = (or + og + ob) / 3;
		g->buffer[i] = argb(oa, or, or, or);
		i++;
	}
}

static inline void rotate_90_clockwise_cache_optimized(const uint32_t* src, uint32_t* dst, 
                                       int width, int height) {
    for (int y = 0; y < height; y++) {
        const uint32_t* src_row = src + y * width;
        uint32_t* dst_col = dst + (height - 1 - y);
        
        for (int x = 0; x < width; x++) {
            *dst_col = src_row[x];
            dst_col += height;
        }
    }
}

static inline void rotate_90_counter_clockwise_cache_optimized(const uint32_t* src, uint32_t* dst, 
                                       int width, int height) {
    for (int y = 0; y < height; y++) {
        const uint32_t* src_row = src + y * width;
        uint32_t* dst_col = dst + y;
        
        for (int x = 0; x < width; x++) {
            *dst_col = src_row[(width - 1 - x)];
            dst_col += height;
        }
    }
}

void graph_rotate_to(graph_t* g, graph_t* ret, int rot) {
	if(g == NULL || ret == NULL)
		return;

	if(rot == G_ROTATE_90) {
		rotate_90_clockwise_cache_optimized(g->buffer, ret->buffer, g->w, g->h);
	}
	else if(rot == G_ROTATE_N90) {
		rotate_90_counter_clockwise_cache_optimized(g->buffer, ret->buffer, g->w, g->h);
	}
	else if(rot == G_ROTATE_180) {
		int w0 = -(g->w);
		int w1 = ((g->h+1) * g->w) - 1;
		for(int i=0; i<g->h; ++i) {
			w0 += g->w;
			w1 -= g->w;
			for(int j=0; j<g->w; ++j) {
				ret->buffer[w0 + j] = g->buffer[w1 - j];
			}
		}
	}
}

inline graph_t* graph_rotate(graph_t* g, int rot) {
	if(g == NULL)
		return NULL;
	graph_t* ret = NULL;

	if(rot == G_ROTATE_90 || rot == G_ROTATE_N90) {
		ret = graph_new(NULL, g->h, g->w);
	}
	else if(rot == G_ROTATE_180) {
		ret = graph_new(NULL, g->w, g->h);
	}
	else 
		return NULL;
	graph_rotate_to(g, ret, rot);
	return ret;
}

void graph_scale_to(graph_t* g, graph_t* dst, int scale) {
	if(scale == 0 || dst->w < g->w*scale || dst->h < g->h*scale)
		return;
	
	if(scale > 0) { //bigger
		for(int i=0; i<dst->h; i++) {
			int gi = i / scale;
			for(int j=0; j<dst->w; j++) {
				int gj = j / scale;
				dst->buffer[i*dst->w + j] = g->buffer[(gi*g->w + gj)];
			}
		}
		return;
	}
	//smaller
	if(scale < 0)
		scale = -scale;
	for(int i=0; i<dst->h; i++) {
		int gi = i * scale;
		for(int j=0; j<dst->w; j++) {
			int gj = j * scale;
			dst->buffer[i*dst->w + j] = g->buffer[(gi*g->w + gj)];
		}
	}
}

graph_t* graph_scale(graph_t* g, int scale) {
	graph_t* ret = NULL;
	if(scale == 0)
		return NULL;
	
	if(scale > 0) { //bigger
		ret = graph_new(NULL, g->w*scale, g->h*scale);
	}
	if(scale < 0) {//smaller
		int sc = -scale;
		ret = graph_new(NULL, g->w/sc, g->h/sc);
	}
	if(ret == NULL)
		return NULL;
	graph_scale_to(g, ret, scale);
	return ret;
}

void graph_scale_tof_neon(graph_t* g, graph_t* dst, float scale) {
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

void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;
    
    // 使用双线性插值算法添加抗锯齿平滑
    for(int i=0; i<dst->h; i++) {
        float gi = (float)i / scale; // 浮点坐标
        int gi0 = (int)gi;           // 整数部分
        float gi_frac = gi - gi0;    // 小数部分
        
        // 确保不会越界
        if(gi0 >= g->h-1) gi0 = g->h-2;
        if(gi0 < 0) gi0 = 0;
        
        for(int j=0; j<dst->w; j++) {
            float gj = (float)j / scale; // 浮点坐标
            int gj0 = (int)gj;           // 整数部分
            float gj_frac = gj - gj0;    // 小数部分
            
            // 确保不会越界
            if(gj0 >= g->w-1) gj0 = g->w-2;
            if(gj0 < 0) gj0 = 0;
            
            // 获取四个相邻像素的颜色
            uint32_t p00 = g->buffer[gi0*g->w + gj0];
            uint32_t p01 = g->buffer[gi0*g->w + (gj0+1)];
            uint32_t p10 = g->buffer[(gi0+1)*g->w + gj0];
            uint32_t p11 = g->buffer[(gi0+1)*g->w + (gj0+1)];
            
            // 分解颜色通道
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
            
            // 双线性插值计算目标像素颜色
            uint8_t r = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * r00 + gj_frac * r01) + 
                                 gi_frac * ((1 - gj_frac) * r10 + gj_frac * r11));
            uint8_t g_val = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * g00 + gj_frac * g01) + 
                                     gi_frac * ((1 - gj_frac) * g10 + gj_frac * g11));
            uint8_t b = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * b00 + gj_frac * b01) + 
                                 gi_frac * ((1 - gj_frac) * b10 + gj_frac * b11));
            uint8_t a = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * a00 + gj_frac * a01) + 
                                 gi_frac * ((1 - gj_frac) * a10 + gj_frac * a11));
            
            // 重新组合颜色通道
            dst->buffer[i*dst->w + j] = (a << 24) | (r << 16) | (g_val << 8) | b;
        }
    }
}

void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
#ifdef NEON_BOOST
    graph_scale_tof_neon(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

graph_t* graph_scalef(graph_t* g, float scale) {
	graph_t* ret = NULL;
	if(scale <= 0.0)
		return NULL;
	
	ret = graph_new(NULL, g->w*scale, g->h*scale);
	if(ret == NULL)
		return NULL;
	graph_scale_tof(g, ret, scale);
	return ret;
}

bool graph_2d_boosted(void) {
		return graph_2d_boosted_bsp();
}

#ifdef __cplusplus
}
#endif
