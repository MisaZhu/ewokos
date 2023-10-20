#include <dev/fb.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

#define FB_DEF_W 1600
#define FB_DEF_H 600

static fbinfo_t _fb_info;
int32_t vpb_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));

	dep = 32;
	_fb_info.width = w;
	_fb_info.height = h;
	_fb_info.vwidth = w;
	_fb_info.vheight = h;
	_fb_info.depth = dep;
	_fb_info.pitch = 0;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.pointer = _sys_info.fb.v_base;

	if(w == 640 && h == 480) {
		put32((_sys_info.mmio.v_base | 0x1c), 0x2c77);
		put32((_sys_info.mmio.v_base | 0x00120000), 0x3f1f3f9c);
		put32((_sys_info.mmio.v_base | 0x00120004), 0x090b61df);
		put32((_sys_info.mmio.v_base | 0x00120008), 0x067f1800);
	}
	else if(w == 800 && h == 600) {
		put32((_sys_info.mmio.v_base | 0x1c), 0x2cac);
		put32((_sys_info.mmio.v_base | 0x00120000), 0x1313a4c4);
		put32((_sys_info.mmio.v_base | 0x00120004), 0x0505f6f7);
		put32((_sys_info.mmio.v_base | 0x00120008), 0x071f1800);
	}
	else {
		//1024x768
		w = 1024;
		h = 768;
		_fb_info.width = w;
		_fb_info.height = h;
		_fb_info.vwidth = w;
		_fb_info.vheight = h;
		put32((_sys_info.mmio.v_base | 0x00120000), 0x3F << 2);
		put32((_sys_info.mmio.v_base | 0x00120004), 767);
	}
	put32((_sys_info.mmio.v_base | 0x00120010), _fb_info.pointer - _sys_info.kernel_base);
	put32((_sys_info.mmio.v_base | 0x00120018), 0x092b);

	_fb_info.size = _fb_info.width * _fb_info.height * (_fb_info.depth/8);
	_fb_info.size_max = 4*1024*768;
	return 0;
}

int32_t fb_init(uint32_t w, uint32_t h, fbinfo_t* fbinfo) {
	if(vpb_fb_init(w, h, 32) != 0)
		return -1;
	memcpy(fbinfo, &_fb_info, sizeof(fbinfo_t));
	return 0;
}

void fb_flush32(uint32_t* g32, uint32_t w, uint32_t h) {
	(void)g32;
	(void)w;
	(void)h;
}
