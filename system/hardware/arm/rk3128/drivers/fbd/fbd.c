#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <upng/upng.h>
#include <arch/rk3128/framebuffer.h>

int argv2rgb(uint8_t  *out,  uint32_t *in , int w, int h)
{
	for(int i = 0; i < w*h; i++){
		 register uint32_t color = *in++;
		 *out++=color ;
		 *out++=color >> 8;
		 *out++=color >> 16;
	}	

	return 0;
}

/*static void splash(graph_t* g) {
	int y, h, l;
	uint32_t c, bc;

	l = g->h/8;
	h = (g->h / l);
	h = (h==0 ? 1:h); 

	bc = 0xff / l;
	bc = (bc==0 ? 1:bc); 
	for(y=0; y<l; y++) {
		c = (l-1-y) * bc;
		graph_fill(g, 0, y*h, g->w, h, (c | c<<8 | c<<16 | 0xff000000));
	}

#ifdef GRAPH_2D_BOOST
	graph_t* logo = png_image_new("/data/images/vadar.png");
#else
	graph_t* logo = png_image_new("/data/images/ewok.png");
#endif
	graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
			g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
	graph_free(logo);
}
*/

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	(void)size;
	(void)rotate;

	//return argv2rgb((uint8_t*)fbinfo->pointer, (uint32_t*)buf, fbinfo->width, fbinfo->height);
	return memcpy((uint8_t*)fbinfo->pointer, (uint32_t*)buf, fbinfo->width * fbinfo->height * 4);
}

static fbinfo_t* get_info(void) {
	return rk3128_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return rk3128_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	return fbd_run(&fbd, mnt_point, 640, 480, G_ROTATE_NONE);
}
