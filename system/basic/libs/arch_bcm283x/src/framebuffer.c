#include <string.h>
#include <arch/bcm283x/mailbox.h>
#include <arch/bcm283x/framebuffer.h>
#include <sys/syscall.h>
#include <sysinfo.h>
#include <sys/mmio.h>

#define KERNEL_BASE 0x80000000

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

static fbinfo_t _fb_info;

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	bcm283x_mailbox_init();
	if(_bcm283x_mailbox_addr == 0) //mailbox not inited!
		return -1;

	fb_init_t* fbinit = (fb_init_t*)_bcm283x_mailbox_addr;
	
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	memset(fbinit, 0, sizeof(fb_init_t));
	fbinit->width = w;
	fbinit->height = h;
	fbinit->vwidth = fbinit->width;
	fbinit->vheight = fbinit->height;
	fbinit->depth = dep;

	msg.data = ((uint32_t)fbinit + 0x40000000) >> 4;//gpu address add 0x40000000 with l2 cache enabled.
	//msg.data = ((uint32_t)&fbinit + 0xC0000000) >> 4;//gpu address add 0x40000000 with l2 cache disabled.
	bcm283x_mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	bcm283x_mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

	_fb_info.width = fbinit->width;
	_fb_info.height = fbinit->height;
	_fb_info.vwidth = fbinit->width;
	_fb_info.vheight = fbinit->height;
	_fb_info.depth = fbinit->depth;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = (uint32_t)fbinit->pointer - 0x40000000; //gpu address sub 0x40000000 with l2 cache enabled
	//_fb_info.pointer = (uint32_t)fbinit.pointer - 0xc0000000; //gpu address sub 0xc0000000 with l2 cache disabled
	_fb_info.size = fbinit->size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	if(_fb_info.pointer < KERNEL_BASE) {
		_fb_info.pointer += KERNEL_BASE;
	}

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	_fb_info.size_max = sysinfo.phy_mem_size - (_fb_info.pointer-KERNEL_BASE);
	
	syscall3(SYS_MEM_MAP, _fb_info.pointer, _fb_info.pointer-KERNEL_BASE, _fb_info.size_max);
	return 0;
}

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
