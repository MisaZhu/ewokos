#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <arch/bcm283x/framebuffer.h>

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	uint32_t sz = 4 * fbinfo->width * fbinfo->height;
	if(size < sz || fbinfo->depth != 32)
		return -1;

	graph_t g;
	if(rotate == G_ROTATE_N90 || rotate == G_ROTATE_90)
		graph_init(&g, buf, fbinfo->height, fbinfo->width);
	else
		graph_init(&g, buf, fbinfo->width, fbinfo->height);

	graph_t *rg = graph_rotate(&g, rotate);
	if(rg != NULL) {
		memcpy((void*)fbinfo->pointer, rg->buffer, sz);
		graph_free(rg);
	}
	else  {
		memcpy((void*)fbinfo->pointer, buf, sz);
	}
	return size;
}

static fbinfo_t* get_info(void) {
	return bcm283x_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return bcm283x_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, argc, argv);
}
