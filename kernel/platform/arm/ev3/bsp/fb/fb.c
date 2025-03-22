#include <dev/fb.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

#include "../st7586.h"

#define FB_DEF_W 178
#define FB_DEF_H 128
#define FB_DEP 4

static uint8_t _framebuffer[FB_DEF_W*FB_DEF_H*FB_DEP];
static fbinfo_t _fb_info;

int32_t ev3_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	(void)w;
	(void)h;
	(void)dep;
	dep = 32;
	_fb_info.width = FB_DEF_W;
	_fb_info.height = FB_DEF_H;
	_fb_info.vwidth = FB_DEF_W;
	_fb_info.vheight = FB_DEF_H;
	_fb_info.depth = FB_DEP;
	_fb_info.pitch = 0;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.pointer = (uint32_t)_framebuffer;
	_fb_info.size = _fb_info.width * _fb_info.height * (_fb_info.depth/8);
	_fb_info.size_max = _fb_info.size;
	return 0;
}

int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	if(ev3_fb_init(w, h, dep) != 0)
		return -1;
	memcpy(fbinfo, &_fb_info, sizeof(fbinfo_t));

	st7586_init();

	return 0;
}

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

void fb_flush_graph_bsp(graph_t* g) {
	(void)g;
	argb32_to_gray(g->buffer, _framebuffer,  FB_DEF_W*FB_DEF_H);
	st7586_update(_framebuffer,  FB_DEF_W, FB_DEF_H);
}

graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h) {
	return graph_new((uint32_t*)_fb_info.pointer, w, h);
}
