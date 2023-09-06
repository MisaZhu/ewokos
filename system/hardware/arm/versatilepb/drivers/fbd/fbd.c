#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <arch/vpb/framebuffer.h>

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	(void)rotate;
	uint32_t sz = 4 * fbinfo->width * fbinfo->height;
	if(size < sz || fbinfo->depth != 32)
		return -1;

	memcpy((void*)fbinfo->pointer, buf, sz);
	return size;
}

static fbinfo_t* get_info(void) {
	return vpb_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return vpb_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";
	uint32_t rotate = argc > 4 ? atoi(argv[4]): G_ROTATE_NONE;
	uint32_t w = 640;
	uint32_t h = 480;

	if(argc > 3) {
		w = atoi(argv[2]);
		h = atoi(argv[3]);
	}

	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	int res = fbd_run(&fbd, mnt_point, w, h, rotate);
	return res;
}
