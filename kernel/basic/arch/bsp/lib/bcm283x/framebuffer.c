#include "mm/kalloc.h"
#include "kstring.h"
#include "mailbox.h"
#include "framebuffer.h"
#include <kstring.h>
#include <kernel/system.h>
#include <kernel/kernel.h>

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

static fb_init_t fbinit __attribute__((aligned(16)));
static fbinfo_t _fb_info;

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	memset(&fbinit, 0, sizeof(fb_init_t));
	fbinit.width = w;
	fbinit.height = h;
	fbinit.vwidth = fbinit.width;
	fbinit.vheight = fbinit.height;
	fbinit.depth = dep;

	msg.data = ((uint32_t)&fbinit + 0x40000000) >> 4;//gpu address add 0x40000000 with l2 cache enabled.
	//msg.data = ((uint32_t)&fbinit + 0xC0000000) >> 4;//gpu address add 0x40000000 with l2 cache disabled.
	mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

	_fb_info.width = fbinit.width;
	_fb_info.height = fbinit.height;
	_fb_info.vwidth = fbinit.width;
	_fb_info.vheight = fbinit.height;
	_fb_info.depth = fbinit.depth;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = (uint32_t)fbinit.pointer - 0x40000000; //gpu address sub 0x40000000 with l2 cache enabled
	//_fb_info.pointer = (uint32_t)fbinit.pointer - 0xc0000000; //gpu address sub 0xc0000000 with l2 cache disabled
	_fb_info.size = fbinit.size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	if(_fb_info.pointer < KERNEL_BASE) {
		_fb_info.pointer = P2V(_fb_info.pointer);
	}

	map_pages(_kernel_vm, _fb_info.pointer, V2P(_fb_info.pointer), V2P(_fb_info.pointer)+_fb_info.size, AP_RW_D, 0);
	flush_tlb();
	kmake_hole(_fb_info.pointer, _fb_info.pointer+_fb_info.size);
	return 0;
}

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
