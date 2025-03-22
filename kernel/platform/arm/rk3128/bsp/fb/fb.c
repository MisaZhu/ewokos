#include <dev/fb.h>
#include <graph/graph.h>
#include <kernel/hw_info.h>
#include <kstring.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static void* _fb_pointer = NULL;
int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	(void)w;
	(void)h;

	fbinfo->width = 1024;
	fbinfo->height = 600;
	fbinfo->vwidth = 1024;
	fbinfo->vheight = 600;
	fbinfo->depth = 32;
	fbinfo->pointer = _sys_info.fb.v_base;
	fbinfo->size = 1024 * 600 * 4;

	_fb_pointer = (void*)fbinfo->pointer;
	return 0;
}

void fb_flush_graph_bsp(graph_t* g) {
	if(_fb_pointer != g->buffer)
		memcpy(_fb_pointer, g->buffer, g->w*g->h*4);
}

graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h) {
	return graph_new(_fb_pointer, w, h);
}