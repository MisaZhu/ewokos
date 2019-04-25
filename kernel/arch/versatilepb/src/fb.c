#include "mm/mmu.h"
#include "dev/fb.h"
#include "hardware.h"
#include "kstring.h"

int32_t videoInit(fb_info_t *fb_info) {
	/*
	//640x480
	*((uint32_t*)(MMIO_BASE | 0x1c)) = 0x2c77;
	*((uint32_t*)(MMIO_BASE | 0x00120000)) = 0x3f1f3f9c; 
	*((uint32_t*)(MMIO_BASE | 0x00120004)) = 0x090b61df; 
	*((uint32_t*)(MMIO_BASE | 0x00120008)) = 0x067f1800; 
	//800x600
	*((uint32_t*)(MMIO_BASE | 0x1c)) = 0x2cac;
	*((uint32_t*)(MMIO_BASE | 0x00120000)) = 0x1313a4c4;
	*((uint32_t*)(MMIO_BASE | 0x00120004)) = 0x0505f6f7; 
	*((uint32_t*)(MMIO_BASE | 0x00120008)) = 0x071f1800; 
	*/
	//1024x768
	*((uint32_t*)(MMIO_BASE | 0x00120000)) = 0x3F << 2;
	*((uint32_t*)(MMIO_BASE | 0x00120004)) = 767; 
	
	*((uint32_t*)(MMIO_BASE | 0x00120010)) = fb_info->pointer; 
	*((uint32_t*)(MMIO_BASE | 0x00120018)) = 0x082b;
	return 0;
}

static fb_info_t _fb_info __attribute__((aligned(16)));

bool fb_init() {
	// initialize fbinfo 640x480
	/*
	_fb_info.height = 480;
	_fb_info.width = 640;
	_fb_info.vheight = 480;
	_fb_info.vwidth = 640;
	*/

	// initialize fbinfo 800x600
	/*
	_fb_info.height = 600;
	_fb_info.width = 800;
	_fb_info.vheight = 600;
	_fb_info.vwidth = 800;
	*/

	// initialize fbinfo 1024x768
	_fb_info.height = 768;
	_fb_info.width = 1024;
	_fb_info.vheight = 768;
	_fb_info.vwidth = 1024;

	_fb_info.pitch = 0;
	_fb_info.depth = 32;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.pointer = V2P(_fb_start);
	_fb_info.size = 0;

	if(videoInit(&_fb_info) == 0)
		return true;
	return false;
}

int32_t dev_fb_info(int16_t id, void* info) {
	(void)id;

	fb_info_t* fb_info = (fb_info_t*)info;
	memcpy(fb_info, &_fb_info, sizeof(fb_info_t));
	return 0;
}

int32_t dev_fb_write(int16_t id, void* buf, uint32_t size) {
	(void)id;
	uint32_t sz = (_fb_info.depth/8) * _fb_info.width * _fb_info.height;
	if(size > sz)
		size = sz;
	memcpy((void*)_fb_info.pointer, buf, size);
	return (int32_t)size;
}

