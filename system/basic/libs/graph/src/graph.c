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

inline int32_t has_alpha(uint32_t c) {
	if(((c >> 24) & 0xff) != 0xff)
		return 1;
	return 0;
}

inline void graph_init(graph_t* g, uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return;

	g->w = w;
	g->h = h;
	if(buffer != NULL) {
		g->buffer = buffer;
		g->need_free = false;
	}
	else {
		g->buffer = (uint32_t*)malloc(w*h*4);
		g->need_free = true;
	}
}

graph_t* graph_new(uint32_t* buffer, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return NULL;

	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	if(ret != NULL)
		graph_init(ret, buffer, w, h);
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

graph_t* graph_rotate(graph_t* g, int rot) {
	if(g == NULL)
		return NULL;

	if(rot == G_ROTATE_90) {
		graph_t* ret = graph_new(NULL, g->h, g->w);
		for(int i=0; i<g->w; i++) {
			for(int j=0; j<g->h; j++) {
				ret->buffer[(ret->h-i)*ret->w + (ret->w-j)] = g->buffer[j*g->w + (g->w-i)];
			}
		}
		return ret;
	}
	else if(rot == G_ROTATE_N90) {
		graph_t* ret = graph_new(NULL, g->h, g->w);
		for(int i=0; i<g->w; i++) {
			for(int j=0; j<g->h; j++) {
				ret->buffer[i*ret->w + j] = g->buffer[j*g->w + (g->w-i)];
			}
		}
		return ret;
	}
	else if(rot == G_ROTATE_180) {
		graph_t* ret = graph_new(NULL, g->w, g->h);
		for(int i=0; i<g->h; i++) {
			for(int j=0; j<g->w; j++) {
				ret->buffer[i*g->w + j] = g->buffer[(g->h-i)*g->w + (g->w-j)];
			}
		}
		return ret;
	}
	return NULL;
}

#ifdef __cplusplus
}
#endif
