#include <dev/fb.h>
#include <graph/graph.h>

void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h, uint8_t rotate) {
	graph_t* g = graph_new(g32, w, h);
	graph_t *fb_g = NULL;

	if(rotate == G_ROTATE_N90 || rotate == G_ROTATE_90) {
		fb_g = fb_fetch_graph_bsp(h, w);
		graph_rotate_to(g, fb_g, rotate);
	}
	else if(rotate == G_ROTATE_180) {
		g = graph_new(g32, w, h);
		fb_g = fb_fetch_graph_bsp(w, h);
		graph_rotate_to(g, fb_g, rotate);
	}

	if(fb_g != NULL) {
		graph_free(g);
		g = fb_g;
	}

	fb_flush_graph_bsp(g);
	graph_free(g);
}

int32_t fb_init(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	return fb_init_bsp(w, h, dep, fbinfo);
}
