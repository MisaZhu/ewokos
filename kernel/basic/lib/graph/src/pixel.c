#include <graph/graph.h>
#include <kstring.h>

inline void graph_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	g->buffer[y * g->w + x] = color;
}

inline void graph_pixel_safe(graph_t* g, int32_t x, int32_t y, uint32_t color) {
	if(g == NULL)
		return;
	if(x < 0 || x >= g->w || y < 0 || y >= g->h)
		return;
	graph_pixel(g, x, y, color);
}

inline void graph_pixel_argb(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	register uint32_t oc = graph->buffer[y * graph->w + x];
	register uint8_t oa = (oc >> 24) & 0xff;
	register uint8_t or = (oc >> 16) & 0xff;
	register uint8_t og = (oc >> 8)  & 0xff;
	register uint8_t ob = oc & 0xff;

	oa = oa + (255 - oa) * a / 255;
	or = r*a/255 + or*(255-a)/255;
	og = g*a/255 + og*(255-a)/255;
	ob = b*a/255 + ob*(255-a)/255;

	graph->buffer[y * graph->w + x] = argb(oa, or, og, ob);
}

inline void graph_pixel_argb_safe(graph_t* graph, int32_t x, int32_t y,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
	if(graph == NULL)
		return;
	if(x < 0 || x >= graph->w || y < 0 || y >= graph->h)
		return;
	graph_pixel_argb(graph, x, y, a, r, g, b);
}

