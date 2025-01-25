#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>
#include <upng/upng.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>

static fbinfo_t _fb_info;

static int32_t clockwork_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	memset(&_fb_info, 0, sizeof(fbinfo_t));
	_fb_info.width = w;
	_fb_info.height = h;
	_fb_info.vwidth = w;
	_fb_info.vheight = h;
	_fb_info.depth = dep;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = syscall1(SYS_P2V, 0xC00000); //GPU addr to ARM addr
	_fb_info.size = w*h*(dep/8);
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.size_max = w*h*(dep/8);
	syscall3(SYS_MEM_MAP, _fb_info.pointer, _fb_info.pointer-sysinfo.kernel_base, _fb_info.size_max);
	return 0;
}

static graph_t* _g = NULL;

static void blt16(uint32_t* src, uint16_t* dst, uint32_t w, uint32_t h) {
	uint32_t sz = w * h;
	uint32_t i;

	for (i = 0; i < sz; i++) {
		uint32_t s = src[i];
		uint8_t r = (s >> 16) & 0xff;
		uint8_t g = (s >> 8)  & 0xff;
		uint8_t b = s & 0xff;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	if(fbinfo->depth != 32 && fbinfo->depth != 16)
		return 0;

	uint32_t sz = 4 * g->w * g->h;
	if(fbinfo->depth == 16)
		blt16(g->buffer, fbinfo->pointer, fbinfo->width, fbinfo->height);
	else if(fbinfo->pointer != g->buffer)
		memcpy((void*)fbinfo->pointer, g->buffer, sz);
	return sz;
}

static fbinfo_t* get_info(void) {
	return &_fb_info;
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	return clockwork_fb_init(w, h, dep);
}

int main(int argc, char** argv) {
	fbd_t fbd;
	_g = NULL;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;

	int res = fbd_run(&fbd, mnt_point, 640, 480, G_ROTATE_NONE);
	if(_g != NULL)
		graph_free(_g);
	return res;
}
