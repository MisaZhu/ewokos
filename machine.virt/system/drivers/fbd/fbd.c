#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <graph/graph_png.h>
#include <bsp/bsp_fb.h>

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	uint32_t sz = 4 * g->w * g->h;
	if(fbinfo->pointer != g->buffer)
		memcpy((void*)fbinfo->pointer, g->buffer, sz);
	return sz;
}

static fbinfo_t* get_info(void) {
	return bsp_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return bsp_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	int res = fbd_run(&fbd, mnt_point, 640, 480, "");
	return res;
}
