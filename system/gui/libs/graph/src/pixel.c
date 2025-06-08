#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

inline void graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	g->buffer[y * g->w + x] = color;
}

inline uint32_t graph_get_pixel(graph_t* g, int32_t x, int32_t y) {
	return g->buffer[y * g->w + x];
}

inline void graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	if(g == NULL)
		return;
	if(g->clip.w == 0 || g->clip.h == 0) {
		if(x < 0 || x >= g->w || y < 0 || y >= g->h)
			return;
	}
	else {
		if(x < g->clip.x || x >= (g->clip.x + g->clip.w) ||
				y < g->clip.y || y >= (g->clip.y + g->clip.h))
			return;
	}
	graph_pixel(g, x, y, color);
}

inline void graph_pixel_argb(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	if(a == 0)
		return;
	if(a == 0xff) {
		graph->buffer[y * graph->w + x] = argb(a, r, g, b);
		return;
	}
	
	register uint32_t oc = graph->buffer[y * graph->w + x];
	register uint8_t oa = (oc >> 24) & 0xff;
	register uint8_t or = (oc >> 16) & 0xff;
	register uint8_t og = (oc >> 8)  & 0xff;
	register uint8_t ob = oc & 0xff;

	/*oa = oa + (255 - oa) * a / 255;
	or = r*a/255 + or*(255-a)/255;
	og = g*a/255 + og*(255-a)/255;
	ob = b*a/255 + ob*(255-a)/255;
	*/
	oa = oa + ((255 - oa) * a >> 8);
	or = ((r*a)>>8) + (or*(255-a)>>8);
	og = ((g*a)>>8) + (og*(255-a)>>8);
	ob = ((b*a)>>8) + (ob*(255-a)>>8);

	graph->buffer[y * graph->w + x] = argb(oa, or, og, ob);
}

inline void graph_pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	if(graph == NULL || a == 0)
		return;
	if(graph->clip.w == 0 || graph->clip.h == 0) {
		if(x < 0 || x >= graph->w || y < 0 || y >= graph->h)
			return;
	}
	else {
		if(x < graph->clip.x || x >= (graph->clip.x + graph->clip.w) ||
				y < graph->clip.y || y >= (graph->clip.y + graph->clip.h))
			return;
	}
	graph_pixel_argb(graph, x, y, a, r, g, b);
}

#ifdef __cplusplus
}
#endif
