#include <string.h>
#include <bcm283x/mailbox.h>
#include <bcm283x/framebuffer.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>

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

static __attribute__((__aligned__(PAGE_SIZE))) fb_init_t _fbinit;
static fbinfo_t _fb_info;

int32_t __attribute__((optimize("O0"))) bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	fb_init_t* fbinit = &_fbinit;

	memset(&_fb_info, 0, sizeof(fbinfo_t));
	memset(fbinit, 0, sizeof(fb_init_t));
	
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	fbinit->width = w;
	fbinit->height = h;
	fbinit->vwidth = fbinit->width;
	fbinit->vheight = fbinit->height;
	fbinit->depth = dep;

	msg.data = ((uint32_t)fbinit + 0x40000000) >> 4;//gpu address add 0x40000000 with l2 cache enabled.
	//msg.data = ((uint32_t)&fbinit + 0xC0000000) >> 4;//gpu address add 0xc0000000 with l2 cache disabled.
	mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

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
	_fb_info.size_max = _sys_info.phy_mem_size - (_fb_info.pointer-KERNEL_BASE);
	map_pages(_kernel_vm, 
		_fb_info.pointer,
		V2P(_fb_info.pointer),
		V2P(_fb_info.pointer) + _fb_info.size_max,
		AP_RW_D, 1);
	flush_tlb();
	return 0;
}

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
