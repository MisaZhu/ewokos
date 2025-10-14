#include <string.h>
#include <arch/virt/framebuffer.h>
#include <arch/virt/fw_cfg.h>
#include <arch/virt/dma.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>

#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

static fbinfo_t _fb_info;

struct fb_cfg {
    uint64_t address;
    uint32_t fourcc; 
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} __attribute__((packed));

int32_t virt_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));

	_mmio_base = mmio_map();
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);

	memset(&_fb_info, 0, sizeof(fbinfo_t));
	_fb_info.width = w;
  	_fb_info.height = h;
  	_fb_info.vwidth = w;
  	_fb_info.vheight = h;
  	_fb_info.depth = 32;
  	_fb_info.size = _fb_info.width * _fb_info.height * (_fb_info.depth/8);
  	_fb_info.size_max = ALIGN_UP(_fb_info.size, 4096);

	_fb_info.pointer = dma_user_alloc(_fb_info.size_max);
	uint64_t fb_phy = dma_user_phy(_fb_info.pointer);
	//klog("DMA alloc v:%08x p:%08x size:%d\n",  _fb_info.pointer, fb_phy, _fb_info.size_max);

	struct fb_cfg cfg;
	cfg.address = BE64(fb_phy);
    cfg.fourcc = BE32(0x34325258);
    cfg.flags = BE32(0);
    cfg.width = BE32(w);
    cfg.height = BE32(h);
    cfg.stride = BE32(4 * w);


	fw_init();
	fw_set_cfg("etc/ramfb", &cfg, sizeof(cfg));
  	return 0;
}

fbinfo_t* virt_get_fbinfo(void) {
	return &_fb_info;
}
