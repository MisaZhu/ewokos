#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <sys/syscall.h>
#include <sysinfo.h>

static fbinfo_t _fb_info;

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	memcpy(fbinfo->pointer, buf, fbinfo->width * fbinfo->height * 4);
}

static fbinfo_t* get_info(void) {

	return &_fb_info;
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	_fb_info.width = 1920;
	_fb_info.height = 1080;
	_fb_info.vwidth = 1920;
	_fb_info.vheight = 1080;
	_fb_info.depth = 32;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = syscall1(SYS_P2V, 0x7eb10000); //GPU addr to ARM addr
	_fb_info.size = _fb_info.width*_fb_info.height*4;
	_fb_info.size_max = (_fb_info.size + 4095)&(~4095);
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	syscall3(SYS_MEM_MAP, _fb_info.pointer, 0x7eb10000, _fb_info.size_max);
	printf("%08x\n", _fb_info.pointer);

	return 0;
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

	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	return fbd_run(&fbd, mnt_point, w, h, rotate);
}
