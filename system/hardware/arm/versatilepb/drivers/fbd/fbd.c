#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <upng/upng.h>
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

static void splash(graph_t* g) {
	int sz = 2; 
	int x = 0;
	int y = 0;
	uint32_t c1;
	uint32_t c2;
	for(int i=0; ;i++) {
		if((i%2) == 0) {
			c1 = 0xffdddddd;
			c2 = 0xff555555;
		}
		else {
			c2 = 0xffdddddd;
			c1 = 0xff555555;
		}

		for(int j=0; ;j++) {
			graph_fill(g, x, y, sz, sz, (j%2)==0? c1:c2);
			x += sz;
			if(x >= g->w)
				break;
		}
		x = 0;
		y += sz;
		if(y >= g->h)
			break;
	}

	graph_t* logo = png_image_new("/data/icons/mac1984/mac_face.png");
	graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
			g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
	graph_free(logo);
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

	fbd.splash = splash;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	int res = fbd_run(&fbd, mnt_point, w, h, rotate);
	return res;
}
