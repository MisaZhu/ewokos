#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

inline uint32_t color_gray(uint32_t oc) {
	uint8_t oa = (oc >> 24) & 0xff;
	uint8_t or = (oc >> 16) & 0xff;
	uint8_t og = (oc >> 8)  & 0xff;
	uint8_t ob = oc & 0xff;
	or = (or + og + ob) / 3;
	return argb(oa, or, or, or);
}

inline uint32_t color_reverse(uint32_t oc) {
	uint8_t oa = (oc >> 24) & 0xff;
	uint8_t or = 0xff - ((oc >> 16) & 0xff);
	uint8_t og = 0xff - ((oc >> 8)  & 0xff);
	uint8_t ob = 0xff - (oc & 0xff);
	return argb(oa, or, og, ob);
}

inline uint32_t color_reverse_rgb(uint32_t oc) {
	uint8_t oa = (oc >> 24) & 0xff;
	uint8_t or = ((oc) & 0xff);
	uint8_t og = ((oc >> 8)  & 0xff);
	uint8_t ob = ((oc >> 16)  & 0xff);
	return argb(oa, or, og, ob);
}

static void* aligned_malloc(uint32_t size, uint32_t alignment) {
    // 检查对齐值是否为 2 的幂
    if ((alignment & (alignment - 1)) != 0) {
        return NULL; // 对齐值必须是 2 的幂
    }
    // 计算额外需要的空间（对齐偏移 + 存储原始指针）
    uint32_t extra = alignment - 1 + sizeof(void*);
    void* raw_ptr = malloc(size + extra);
    if (!raw_ptr) return NULL;
    // 计算对齐后的地址
    uintptr_t aligned_addr = (uintptr_t)raw_ptr + sizeof(void*);
    aligned_addr = (aligned_addr + alignment - 1) & ~(alignment - 1);
    // 保存原始指针供 free 使用
    *((void**)aligned_addr - 1) = raw_ptr;
    return (void*)aligned_addr;
}

// 对应的释放函数
static void aligned_free(void* ptr) {
    if (ptr) {
        // 获取原始指针并释放
        void* raw_ptr = *((void**)ptr - 1);
        free(raw_ptr);
    }
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
		g->buffer = (uint32_t*)aligned_malloc(w*h*4, 32);
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
	ret->buffer = (uint32_t*)aligned_malloc(g->w*g->h*4, 32);
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
		aligned_free(g->buffer);
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
	for(int32_t i=0; i < g->w*g->h; i++) {
		g->buffer[i] = color_reverse(g->buffer[i]);
	}
}

void graph_reverse_rgb(graph_t* g) {
	if(g == NULL)
		return;
	for(int32_t i=0; i < g->w*g->h; i++) {
		g->buffer[i] = color_reverse_rgb(g->buffer[i]);
	}
}

void graph_gray(graph_t* g) {
	if(g == NULL)
		return;
	for(int32_t i=0; i < g->w*g->h; i++) {
		g->buffer[i] = color_gray(g->buffer[i]);
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
	else if(rot == G_ROTATE_270) {
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

	if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
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

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;
    
    // 使用双线性插值算法添加抗锯齿平滑
	int hmin = g->h-2;
	int wmin = g->w-2;
    for(int i=0; i<dst->h; i++) {
        float gi = (float)i / scale; // 浮点坐标
        int gi0 = (int)gi;           // 整数部分
        float gi_frac = gi - gi0;    // 小数部分
        
        // 确保不会越界
		gi0 = CLAMP(gi0, 0, hmin);
        
		int gi0w = gi0 * g->w;
		int gi1w = (gi0+1) * g->w;
        for(int j=0; j<dst->w; j++) {
            float gj = (float)j / scale; // 浮点坐标
            int gj0 = (int)gj;           // 整数部分
            float gj_frac = gj - gj0;    // 小数部分
            
            // 确保不会越界
			gj0 = CLAMP(gj0, 0, wmin);
            
            // 获取四个相邻像素的颜色
            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + (gj0+1)];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + (gj0+1)];
			if(p00 == p01 && p00 == p10 && p00 == p11) {
            	dst->buffer[i*dst->w + j] = p00;
				continue;
			}
            
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

inline void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
//TODO: 优化下，bsp效率不高
/*
#if BSP_BOOST
    graph_scale_tof_bsp(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
*/
    graph_scale_tof_cpu(g, dst, scale);
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

#ifdef __cplusplus
}
#endif
