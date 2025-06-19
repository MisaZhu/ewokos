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

void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
	if(scale <= 0.0 ||
			dst->w < (int)(g->w*scale) ||
			dst->h < (int)(g->h*scale))
		return;
	
	for(int i=0; i<dst->h; i++) {
		int gi = i / scale;
		for(int j=0; j<dst->w; j++) {
			int gj = j / scale;
			dst->buffer[i*dst->w + j] = g->buffer[(gi*g->w + gj)];
		}
	}
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
