#include <dev/fb.h>
#include <graph/graph.h>
#include <kernel/kernel.h>
#include <kstring.h>
#include <stdbool.h>
#include "fb.h"

typedef struct
{
    bool bValid;
    DispDeviceTiming  timingType;
    DispTimingConfig timing;
}PanelConfig;



PanelConfig _panelConfig;

void panel_init(void){

	_panelConfig.bValid = true;

	_panelConfig.timing.Fps = 60;

	_panelConfig.timingType = DISP_OUTPUT_USER;
	_panelConfig.timing.Hstart = 48;
	_panelConfig.timing.HsyncWidth = 4;
	_panelConfig.timing.HsyncBackPorch = 48;
	_panelConfig.timing.Hactive = 640;
	_panelConfig.timing.Htotal = 740;

	_panelConfig.timing.Vstart = 10;
	_panelConfig.timing.VsyncBackPorch = 10;
	_panelConfig.timing.VsyncWidth = 4;
	_panelConfig.timing.Vactive = 480;
	_panelConfig.timing.Vtotal = 504;
}

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
	uint32_t b = rgb[0];
	uint32_t g = rgb[1];
	uint32_t r = rgb[2];
	return (306*r + 601*g + 117*b) >> 10;
}

static inline void rgb2uv(uint8_t *rgb, uint8_t *uv){
	uint32_t b = rgb[0];
	uint32_t g = rgb[1];
	uint32_t r = rgb[2];
	uv[0] = ((446*b - 150*r - 296*g) /1024) + 0x7F;
	uv[1] = ((630*r - 527*g - 102*b) /1024) + 0x7F;
}

int rgb2nv12(uint8_t  *out,  uint32_t *in , int w, int h)
{
	uint8_t *y = out;
	uint8_t *uv = y + w * h;
	uint32_t *rgb = (in + w * h);

	for(int i = 0; i < w; i++){
		for( int j = 0; j < h; j++){
			*y = rgb2y((uint8_t*)rgb); 
			y++;
			rgb--;
		}
	}	

	return 0;
}


int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	panel_init();
	fbinfo->width = 640;
	fbinfo->height = 480;
	fbinfo->vwidth = 640;
	fbinfo->vheight = 480;
	fbinfo->depth = 16;
	fbinfo->pointer = 0x87c00000;
	fbinfo->size = 640 * 480 * 4;
	memset(0x87c00000, 0x7F , 480*640*3/2);
	return 0;
}

void fb_flush32_bsp(uint32_t* g32, uint32_t w, uint32_t h) {
	if(strcmp(_sys_info.machine, "miyoo-plus") == 0)
		rgb2nv12(0x87c00000 + 626 , g32, w, h);	
	else
		rgb2nv12(0x87c00000, g32, w, h);	
}


void fb_flush_graph_bsp(graph_t* g) {
	void* p = (void*)(0x87c00000);
	if(strcmp(_sys_info.machine, "miyoo-plus") == 0)
		p = (void*)(0x87c00000 + 626);
	rgb2nv12(p, g->buffer, g->w, g->h);	
}

graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h) {
	return graph_new(NULL, w, h);
}
