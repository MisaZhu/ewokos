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
	uint32_t display_index;
	bool down;
	bool drop;
} cursor_t;

void cursor_init(cursor_t* cursor);

void draw_cursor(graph_t* g, cursor_t* cursor, int mx, int my);

#endif