#include "mm/mmu.h"
#include "string.h"
#include "dev/fbinfo.h"
#include "dev/framebuffer.h"

static fbinfo_t _fbinfo __attribute__((aligned(16)));

int32_t fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fbinfo, 0, sizeof(fbinfo_t));
	dep = 32;
	_fbinfo.width = w;
	_fbinfo.height = h;
	_fbinfo.vwidth = w;
	_fbinfo.vheight = h;
	_fbinfo.depth = dep;
	_fbinfo.pitch = 0;
	_fbinfo.xoffset = 0;
	_fbinfo.yoffset = 0;
	_fbinfo.pointer = (uint32_t)_framebuffer_base_raw;

	if(w == 640 && h == 480) {
		put32((_mmio_base | 0x1c), 0x2c77);
		put32((_mmio_base | 0x00120000), 0x3f1f3f9c);
		put32((_mmio_base | 0x00120004), 0x090b61df); 
		put32((_mmio_base | 0x00120008), 0x067f1800); 
	}
	else if(w == 800 && h == 600) {
		put32((_mmio_base | 0x1c), 0x2cac);
		put32((_mmio_base | 0x00120000), 0x1313a4c4);
		put32((_mmio_base | 0x00120004), 0x0505f6f7);
		put32((_mmio_base | 0x00120008), 0x071f1800); 
	}
	else {
		//1024x768
		w = 1024;
		h = 768;
		_fbinfo.width = w;
		_fbinfo.height = h;
		_fbinfo.vwidth = w;
		_fbinfo.vheight = h;
		put32((_mmio_base | 0x00120000), 0x3F << 2);
		put32((_mmio_base | 0x00120004), 767);
	}	
	put32((_mmio_base | 0x00120010), V2P(_fbinfo.pointer));
	put32((_mmio_base | 0x00120018), 0x082b);
	
	_fbinfo.size = _fbinfo.width * _fbinfo.height * (_fbinfo.depth/8);
	return 0;
}

inline fbinfo_t* fb_get_info(void) {
	return &_fbinfo;
}

int32_t fb_dev_write(dev_t* dev, const void* buf, uint32_t size) {
	(void)dev;
	uint32_t sz = (_fbinfo.depth/8) * _fbinfo.width * _fbinfo.height;
	if(size > sz)
		size = sz;
	memcpy((void*)_fbinfo.pointer, buf, size);
	return (int32_t)size;
}
