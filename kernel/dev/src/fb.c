#include <dev/fb.h>
#include <graph/graph.h>

static graph_t* _fb_g = NULL;
void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h, uint8_t rotate) {
	graph_t g;
	graph_init(&g, g32, w, h);

	if(rotate == G_ROTATE_N90 || rotate == G_ROTATE_90) {
		if(_fb_g == NULL)
			_fb_g = fb_fetch_graph_bsp(h, w);
		graph_rotate_to(&g, _fb_g, rotate);
	}
	else if(rotate == G_ROTATE_180) {
		if(_fb_g == NULL)
			_fb_g = fb_fetch_graph_bsp(w, h);
		graph_rotate_to(&g, _fb_g, rotate);
	}

	if(_fb_g != NULL)
		fb_flush_graph_bsp(_fb_g);
	else
		fb_flush_graph_bsp(&g);
}

int32_t fb_init(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	_fb_g = NULL;
	return fb_init_bsp(w, h, dep, fbinfo);
}

void fb_close(void) {
	if(_fb_g == NULL)
		return;
	graph_free(_fb_g);
	_fb_g = NULL;
}
