#include <dev/fb.h>
#include <graph/graph.h>

void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h, uint8_t rotate) {
    graph_t *g = NULL, *fb_g = NULL;

	if(rotate == G_ROTATE_N90 || rotate == G_ROTATE_90) {
		g = graph_new(g32, w, h);
		fb_g = graph_new(NULL, h, w);
		graph_rotate_to(g, fb_g, rotate);
	}
	else if(rotate == G_ROTATE_180) {
		g = graph_new(g32, w, h);
		fb_g = graph_new(NULL, w, h);
		graph_rotate_to(g, fb_g, rotate);
	}

    if(fb_g != NULL) {
        fb_flush32_bsp(fb_g->buffer, w, h);
        graph_free(g);
        graph_free(fb_g);
    }
    else
        fb_flush32_bsp(g32, w, h);
}

