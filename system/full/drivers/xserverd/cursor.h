#ifndef CURSOR_H
#define CURSOR_H

#include <graph/graph.h>

enum {
	CURSOR_MOUSE=0,
	CURSOR_TOUCH
};

typedef struct {
	gpos_t old_pos;
	gpos_t cpos;
	gpos_t offset;
	gsize_t size;
	graph_t* g;
	uint32_t type;
	bool drop;
} cursor_t;

void draw_cursor(graph_t* g, int mx, int my, int mw, int mh, int type);

#endif