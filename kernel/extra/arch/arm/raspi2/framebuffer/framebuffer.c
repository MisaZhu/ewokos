#include "mm/mmu.h"
#include "mm/kmalloc.h"
#include "mm/kalloc.h"
#include "kstring.h"
#include "dev/framebuffer.h"
#include "mailbox.h"
#include "kstring.h"
#include <kernel/system.h>
#include <kernel/kernel.h>

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

static fb_init_t fbinit __attribute__((aligned(16)));
//int32_t __attribute__((optimize("O0"))) fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
int32_t fb_dev_init(uint32_t w, uint32_t h, uint32_t dep) {
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
	kmake_hole(_fb_info.pointer, _fb_info.pointer+_fb_info.size);
	return 0;
}

inline fbinfo_t* fb_get_info(void) {
	return &_fb_info;
}

static inline void argb2abgr(uint32_t* dst, const uint32_t* src, uint32_t size) {
	while(size > 0) {
		register uint32_t c = src[size-1];
		uint8_t a = c >> 24;
		uint8_t r = c >> 16;
		uint8_t g = c >> 8;
		uint8_t b = c & 0xff;

		dst[size-1] = a << 24 | b << 16 | g << 8 | r;
		size--;
	}
}

static inline void dup16(uint16_t* dst, uint32_t* src, uint32_t w, uint32_t h) {
	register int32_t i, size;
	size = w * h;
	for(i=0; i < size; i++) {
		register uint32_t s = src[i];
		register uint8_t r = (s >> 16) & 0xff;
		register uint8_t g = (s >> 8)  & 0xff;
		register uint8_t b = s & 0xff;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}
int32_t fb_dev_write(const void* buf, uint32_t size) {
	uint32_t sz = (_fb_info.depth/8) * _fb_info.width * _fb_info.height;
	if(size > sz)
		size = sz;
	if(_fb_info.depth == 32) 
		argb2abgr((uint32_t*)_fb_info.pointer, (const uint32_t*)buf, size/4);
	else if(_fb_info.depth == 16) 
		dup16((uint16_t*)_fb_info.pointer, (uint32_t*)buf,  _fb_info.width, _fb_info.height);
	return (int32_t)size;
}
