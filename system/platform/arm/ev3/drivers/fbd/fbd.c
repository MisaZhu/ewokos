#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <upng/upng.h>
#include <bsp/bsp_fb.h>
#include <ewoksys/mmio.h>
#include "st7586.h"

static void argb32_to_gray(uint32_t *argb, uint8_t *gray, int size){
    uint8_t *p = (uint8_t*)argb;
    for(int i = 0; i < size; i++){
        uint8_t b = *p++;
        uint8_t g = *p++;
        uint8_t r = *p++;
        p++;
        gray[i] = (r+g+b)/3;
    }
}

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	argb32_to_gray(g->buffer, (uint8_t*)fbinfo->pointer, g->w * g->h);
	st7586_update((uint8_t*)fbinfo->pointer, g->w , g->h);
	return 4 * g->w * g->h;
}

static fbinfo_t* get_info(void) {
	return bsp_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return bsp_fb_init(w, h, dep);
}

static void splash(graph_t* g, const char* logo_fname) {
	graph_clear(g, 0xffffffff);
	graph_t* logo = png_image_new(logo_fname);
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
}

int main(int argc, char** argv) {
	fbd_t fbd;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";
	_mmio_base = mmio_map();
	st7586_init();

	fbd.splash = splash;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, mnt_point, 178, 128, G_ROTATE_NONE);
}
