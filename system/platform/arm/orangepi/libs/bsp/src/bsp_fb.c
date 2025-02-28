#include <bsp/bsp_fb.h>
#include <stdint.h>
#include <sysinfo.h>
#include <string.h>

static fbinfo_t _fb_info;
static char _framebuffer[640*480*4] = {0};

fbinfo_t* bsp_get_fbinfo(void) {
    _fb_info.width = 640;
    _fb_info.height = 480;
    _fb_info.vwidth = 640;
    _fb_info.vheight = 480;
    _fb_info.depth = 32;
    _fb_info.pitch = _fb_info.width*(_fb_info.depth/8);
    _fb_info.pointer = _framebuffer; //GPU addr to ARM addr

    _fb_info.size = _fb_info.width*_fb_info.height*2;
    _fb_info.size_max = _fb_info.size;
    _fb_info.xoffset = 0;
    _fb_info.yoffset = 0;
    return 0;
}

int32_t bsp_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	return 0;
}
