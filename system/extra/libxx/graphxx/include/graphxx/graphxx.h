#ifndef GRAPHXX_H
#define GRAPHXX_H

extern "C" {
	#include <stddef.h>
	#include <graph/graph.h>
}

class Graph {
protected:
	graph_t* g;
public:
	Graph(uint32_t* buffer, uint32_t w, uint32_t h);
	~Graph();

	inline graph_t* getCGraph(void) { return g; }
	inline uint32_t getW(void) { return g->w; }
	inline uint32_t getH(void) { return g->h; }

	void pixel(int32_t x, int32_t y, uint32_t color);

	void clear(uint32_t color);

	void reverse();

	void box(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

	void fill(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

	void line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

	void drawChar(int32_t x, int32_t y, char c, font_t* font, uint32_t color);

	void drawText(int32_t x, int32_t y, const char* str, font_t* font, uint32_t color);

	void blt(Graph* src,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh);

	void blt(graph_t* src,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh);

	void blt(Graph* src,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);

	void blt(graph_t* src,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);

};

#endif
