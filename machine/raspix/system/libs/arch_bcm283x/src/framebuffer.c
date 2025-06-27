#include <string.h>
#include <arch/bcm283x/mailbox.h>
#include <arch/bcm283x/framebuffer.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>
#include <ewoksys/mmio.h>
#include <ewoksys/dma.h>

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

extern void test(void);

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);

	bcm283x_mailbox_init();

	fb_init_t* fbinit = (fb_init_t*)(dma_alloc(0, sizeof(fb_init_t)));
	
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	memset(fbinit, 0, sizeof(fb_init_t));
	fbinit->width = w;
	fbinit->height = h;
	fbinit->vwidth = fbinit->width;
	fbinit->vheight = fbinit->height;
	fbinit->depth = dep;

	msg.data = (dma_phy_addr(0, (ewokos_addr_t)fbinit) | 0xC0000000) >> 4; // ARM addr to GPU addr
	bcm283x_mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	bcm283x_mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

	if(fbinit->size == 0)
		fbinit->size = w * h * 4;

	_fb_info.width = fbinit->width;
	_fb_info.height = fbinit->height;
	_fb_info.vwidth = fbinit->width;
	_fb_info.vheight = fbinit->height;
	_fb_info.depth = fbinit->depth;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.phy_base = ((uint32_t)fbinit->pointer) & 0x3fffffff; //GPU addr to ARM addr
	_fb_info.pointer = sysinfo.gpu.v_base;
	_fb_info.size = fbinit->size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.size_max = sysinfo.gpu.max_size;
	syscall3(SYS_MEM_MAP,(ewokos_addr_t) _fb_info.pointer, (ewokos_addr_t)_fb_info.phy_base, (ewokos_addr_t)_fb_info.size_max);
	_fb_info.dma_id = dma_set(_fb_info.phy_base+_fb_info.size, _fb_info.size_max - _fb_info.size, true);

	dma_free(0, (ewokos_addr_t)fbinit);
	return 0;
}

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
