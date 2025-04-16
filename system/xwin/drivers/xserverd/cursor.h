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
	gpos_t offset_normal;
	gpos_t offset_busy;

	gsize_t size;
	graph_t* saved;

	graph_t* img;
	graph_t* img_busy;

	uint32_t type;
	bool down;
	bool drop;
} cursor_t;

void cursor_init(const char* theme, cursor_t* cursor);

void draw_cursor(graph_t* g, cursor_t* cursor, int mx, int my, bool busy);

#endif