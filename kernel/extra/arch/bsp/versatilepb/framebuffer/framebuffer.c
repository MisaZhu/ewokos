#include "dev/dev.h"
#include "string.h"
#include "dev/framebuffer.h"
#include "graph.h"

extern char _framebuffer_base_raw[];
extern char _framebuffer_end_raw[];

static fbinfo_t _fb_info;
int32_t fb_dev_init(void) {
	uint32_t w=1024, h=768, dep=32;

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
	_fb_info.pointer = (uint32_t)_framebuffer_base_raw;

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
		_fb_info.width = w;
		_fb_info.height = h;
		_fb_info.vwidth = w;
		_fb_info.vheight = h;
		put32((_mmio_base | 0x00120000), 0x3F << 2);
		put32((_mmio_base | 0x00120004), 767);
	}	
	put32((_mmio_base | 0x00120010), V2P(_fb_info.pointer));
	put32((_mmio_base | 0x00120018), 0x082b);
	
	_fb_info.size = _fb_info.width * _fb_info.height * (_fb_info.depth/8);
	return 0;
}

inline fbinfo_t* fb_get_info(void) {
	return &_fb_info;
}

static inline void argb2abgr(uint32_t* dst, const uint32_t* src, uint32_t size) {
	while(size > 0) {
		register uint32_t c = src[size-1];
		uint8_t a = c >> 24;
		uint8_t r = c >> 16;
		uint8_t g = c >> 8;
		uint8_t b = c & 0xff;

		dst[size-1] = a << 24 | b << 16 | g << 8 | r;
		size--;
	}
}

static inline void dup16(uint16_t* dst, uint32_t* src, uint32_t w, uint32_t h) {
	register int32_t i, size;
	size = w * h;
	for(i=0; i < size; i++) {
		register uint32_t s = src[i];
		register uint8_t r = (s >> 16) & 0xff;
		register uint8_t g = (s >> 8)  & 0xff;
		register uint8_t b = s & 0xff;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}
int32_t fb_dev_write(const void* buf, uint32_t size) {
	uint32_t sz = (_fb_info.depth/8) * _fb_info.width * _fb_info.height;
	if(size > sz)
		size = sz;
	if(_fb_info.depth == 32) 
		argb2abgr((uint32_t*)_fb_info.pointer, (const uint32_t*)buf, size/4);
	else if(_fb_info.depth == 16) 
		dup16((uint16_t*)_fb_info.pointer, (uint32_t*)buf,  _fb_info.width, _fb_info.height);
	return (int32_t)size;
}

