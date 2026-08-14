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

static int _display_index = 0;
static int doargs(int argc, char* argv[]) {
	int c = 0;

	while(c != -1) {
		c = getopt(argc, argv, "i:");
		if(c == -1)
			break;

		switch(c) {
		case 'i':
			_display_index = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	fbd_t fbd;
	int opti = doargs(argc, argv);
	const char* mnt_point = (opti < argc && opti >= 0) ? argv[opti]: "/dev/fb0";

	memset(&fbd, 0, sizeof(fbd));
	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	fbd_set_flush_rect(fbd_flush_rect_to);

	int res = fbd_run(&fbd, mnt_point, 640, 480, "", _display_index);
	return res;
}
