#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <graph/graph_png.h>
#include <bsp/bsp_fb.h>

static graph_t* _g = NULL;

static void blt16(uint32_t* src, uint16_t* dst, uint32_t w, uint32_t h) {
	uint32_t sz = w * h;
	uint32_t i;

	for (i = 0; i < sz; i++) {
		uint32_t s = src[i];
		uint8_t r = (s >> 16) & 0xff;
		uint8_t g = (s >> 8)  & 0xff;
		uint8_t b = s & 0xff;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	if(fbinfo->depth != 32 && fbinfo->depth != 16)
		return 0;

	uint32_t sz = 4 * g->w * g->h;
	if(fbinfo->depth == 16)
		blt16(g->buffer, fbinfo->pointer, fbinfo->width, fbinfo->height);
	else if(fbinfo->pointer != g->buffer)
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
	_g = NULL;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	int res = fbd_run(&fbd, mnt_point, 640, 480, G_ROTATE_NONE);
	if(_g != NULL)
		graph_free(_g);
	return res;
}
