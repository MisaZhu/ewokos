#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <upng/upng.h>
#include <arch/miyoo/framebuffer.h>


/*
*	Following are the conversion formulas from RGB to YUV 
*	and from YUV to RGB. See YUV and color space conversion.
*	From RGB to YUV Y = 0.299R + 0.587G + 0.114B 
*	U = 0.492 (B-Y) V = 0.877 (R-Y) 
*	It can also be represented as: 
*	Y = 0.299R + 0.587G + 0.114B 
*	U = -0.147R - 0.289G + 0.436B 
*	V = 0.615R - 0.515G - 0.100B
*/
static inline uint8_t rgb2y(uint8_t *rgb){
	//return(((257 * rgb[0])/1000) + ((504 * rgb[1])/1000) + ((98 * rgb[2])/1000) + 16);
	register uint32_t b = rgb[0];
	register uint32_t g = rgb[1];
	register uint32_t r = rgb[2];
	return ((306*r + 601*g + 117*b) >> 10) ;
}

static inline void rgb2uv(uint8_t *rgb, uint8_t *uv){
	register uint32_t b = rgb[0];
	register uint32_t g = rgb[1];
	register uint32_t r = rgb[2];
	uv[0] = ((446*b - 150*r - 296*g) >> 10) + 128;
	uv[1] = ((630*r - 527*g - 102*b) >> 10) + 128;
}

int rgb2nv12(uint8_t  *out,  uint32_t *in , int w, int h)
{
	uint8_t *y = out;
	uint8_t *uv = y + w * h;
	uint32_t *rgb = (in + w * h);

	for(int i = 0; i < h; i+=2){
		for( int j = 0; j < w; j+=2){
			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;

			rgb2uv(rgb, uv);
			uv+=2;		

			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;
		}
		for( int j = 0; j < w; j+=2){
			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;

			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;
		}
	}	

	return 0;
}

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	rgb2nv12(fbinfo->pointer, buf, fbinfo->width, fbinfo->height);
}

static fbinfo_t* get_info(void) {
	return miyoo_get_fbinfo();
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return miyoo_fb_init(w, h, dep);
}

static void splash(graph_t* g) {
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

#ifdef NEON_ENABLE
	graph_t* logo = png_image_new("/data/images/vadar.png");
#else
	graph_t* logo = png_image_new("/data/images/ewok.png");
#endif
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
	return fbd_run(&fbd, mnt_point, w, h, rotate);
}
