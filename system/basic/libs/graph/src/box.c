#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

void graph_box(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	if(g == NULL || w <= 0 || h <= 0)
		return;

	graph_line(g, x, y, x+w-1, y, color);
	graph_line(g, x, y+1, x, y+h-1, color);
	graph_line(g, x+1, y+h-1, x+w-1, y+h-1, color);
	graph_line(g, x+w-1, y+1, x+w-1, y+h-2, color);
}

#ifdef __cplusplus
}
#endif
