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

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep, fbinfo_t* fbinfo) {
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

	fbinfo->width = fbinit.width;
	fbinfo->height = fbinit.height;
	fbinfo->vwidth = fbinit.width;
	fbinfo->vheight = fbinit.height;
	fbinfo->depth = fbinit.depth;
	fbinfo->pitch = fbinfo->width*(fbinfo->depth/8);

	fbinfo->pointer = (uint32_t)fbinit.pointer - 0x40000000; //gpu address sub 0x40000000 with l2 cache enabled
	//fbinfo->pointer = (uint32_t)fbinit.pointer - 0xc0000000; //gpu address sub 0xc0000000 with l2 cache disabled
	fbinfo->size = fbinit.size;
	fbinfo->xoffset = 0;
	fbinfo->yoffset = 0;

	if(fbinfo->pointer < KERNEL_BASE) {
		fbinfo->pointer = P2V(fbinfo->pointer);
	}

	map_pages(_kernel_vm, fbinfo->pointer, V2P(fbinfo->pointer), V2P(fbinfo->pointer)+fbinfo->size, AP_RW_D, 0);
	kmake_hole(fbinfo->pointer, fbinfo->pointer+fbinfo->size);
	return 0;
}

