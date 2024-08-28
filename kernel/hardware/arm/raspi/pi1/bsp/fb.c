#include <dev/fb.h>
#include <bcm283x/framebuffer.h>
#include <kstring.h>
#include <graph/graph.h>
#include <stddef.h>

static uint16_t* _g16 = NULL;
static uint32_t* _g32 = NULL;
int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	_g16 = NULL;
	if(bcm283x_fb_init(w, h, dep) != 0)
		return -1;
	memcpy(fbinfo, bcm283x_get_fbinfo(), sizeof(fbinfo_t));
	if(fbinfo->depth == 16)
		_g16 = (uint16_t*)fbinfo->pointer;
	else
		_g32 = (uint32_t*)fbinfo->pointer;
	return 0;
}

void fb_flush_graph_bsp(graph_t* g) {
	if(_g16 != NULL)
		blt16(g->buffer, _g16, g->w, g->h);
	else if(g->buffer != _g32)
		memcpy(_g32, g->buffer, g->w*g->h*4);
}

graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h) {
	if(_g16 != NULL)
		return graph_new(NULL, w, h);
	return graph_new(_g32, w, h);
}