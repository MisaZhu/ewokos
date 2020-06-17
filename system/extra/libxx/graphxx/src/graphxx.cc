#include <graphxx/graphxx.h>

Graph::Graph(uint32_t* buffer, uint32_t w, uint32_t h) {
	g = graph_new(buffer, w, h);
}

Graph::~Graph(void) {
	if(g != NULL) {
		graph_free(g);
		g = NULL;
	}
}

void Graph::pixel(int32_t x, int32_t y, uint32_t color) {
	if(g == NULL)
		return;
	graph_pixel(g, x, y, color);
}

void Graph::clear(uint32_t color) {
	if(g == NULL)
		return;
	graph_clear(g, color);
}

void Graph::box(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL)
		return;
	graph_box(g, x, y, w, h, color);
}

void Graph::fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL)
		return;
	graph_fill(g, x, y, w, h, color);
}

void Graph::line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if(g == NULL)
		return;
	graph_line(g, x1, y1, x2, y2, color);
}

void Graph::drawChar(int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	graph_draw_char(g, x, y, c, font, color);
}

void Graph::drawText(int32_t x, int32_t y, const char* str, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	graph_draw_text(g, x, y, str, font, color);
}

void Graph::blt(graph_t* src, 
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	if(g == NULL)
		return;
	graph_blt_alpha(src, sx, sy, sw, sh, g, dx, dy, dw, dh, alpha);
}

void Graph::blt(Graph* src,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	this->blt(src->g, sx, sy, sw, sh, dx, dy, dw, dh, alpha);
}

void Graph::blt(graph_t* src,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	if(g == NULL)
		return;
	graph_blt(src, sx, sy, sw, sh, g, dx, dy, dw, dh);
}

void Graph::blt(Graph* src, 
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	this->blt(src->g, sx, sy, sw, sh, dx, dy, dw, dh);
}
