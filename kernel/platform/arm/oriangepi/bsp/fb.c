#include <dev/fb.h>
#include <kstring.h>
#include <graph/graph.h>
#include <stddef.h>

static uint16_t* _g16 = NULL;
static uint32_t* _g32 = NULL;
static uint8_t _fb[640 * 480 * 4];

int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	fbinfo->width = 640;
    fbinfo->height = 480;
    fbinfo->vwidth = 640;
    fbinfo->vheight = 480;
    fbinfo->depth = 32;
    fbinfo->pointer = _fb;
    fbinfo->size = 640 * 480 * 4;
	return 0;
}

void fb_flush_graph_bsp(graph_t* g) {

}

graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h) {
	if(_g16 != NULL)
		return graph_new(NULL, w, h);
	return graph_new(_g32, w, h);
}
