#include <string.h>
#include <arch/ev3/framebuffer.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/mmio.h>

#define FB_DEF_W 178
#define FB_DEF_H 128
#define FB_DEP 32

static uint8_t _framebuffer[FB_DEF_W*FB_DEF_H*FB_DEP/8];
static fbinfo_t _fb_info;

int32_t ev3_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	(void)w;
	(void)h;
	(void)dep;
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

fbinfo_t* ev3_get_fbinfo(void) {
    return &_fb_info;
}
