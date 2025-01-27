#include <string.h>
#include <arch/miyoo/framebuffer.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/mmio.h>

static fbinfo_t _fb_info;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t vwidth;
	uint32_t vheight;
	uint32_t bytes;
	uint32_t depth;
	uint32_t ignorex;
	uint32_t ignorey;
	void * pointer;
	uint32_t size;
} fb_init_t;

int32_t miyoo_fb_init(uint32_t w, uint32_t h, uint32_t dep) {

	_fb_info.width = 640;
	_fb_info.height = 480;
	_fb_info.vwidth = 640;
	_fb_info.vheight = 480;
	_fb_info.depth = 16;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	if(strcmp(sysinfo.machine, "miyoo-plus") == 0)
		_fb_info.pointer = syscall1(SYS_P2V, 0x27c00000+628); //GPU addr to ARM addr
	else
		_fb_info.pointer = syscall1(SYS_P2V, 0x27c00000); //GPU addr to ARM addr

	_fb_info.size = _fb_info.width*_fb_info.height*2;
	_fb_info.size_max = _fb_info.size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	if(strcmp(sysinfo.machine, "miyoo-plus") == 0)
		syscall3(SYS_MEM_MAP, _fb_info.pointer, 0x27c00000+628, _fb_info.size);
	else
		syscall3(SYS_MEM_MAP, _fb_info.pointer, 0x27c00000, _fb_info.size);
	return 0;
}

fbinfo_t* miyoo_get_fbinfo(void) {
	return &_fb_info;
}
