#include <kstring.h>
#include <bcm283x/mailbox.h>
#include <bcm283x/framebuffer.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <bcm283x/gpio.h>

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
	//fb_init_t* fbinit = (fb_init_t*)kalloc4k();
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	memset(fbinit, 0, sizeof(fb_init_t));
	
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	fbinit->width = w;
	fbinit->height = h;
	fbinit->vwidth = fbinit->width;
	fbinit->vheight = fbinit->height;
	fbinit->depth = dep;

	msg.data = (V2P((uint32_t)fbinit + 0xC0000000)) >> 4; // DMA offset
	mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
	mailbox_read(FRAMEBUFFER_CHANNEL, &msg);
	if(fbinit->pointer == 0) {
		kfree4k(fbinit);
		return -1;
	}

	_fb_info.width = fbinit->width;
	_fb_info.height = fbinit->height;
	_fb_info.vwidth = fbinit->width;
	_fb_info.vheight = fbinit->height;
	_fb_info.depth = fbinit->depth;
	_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

	_fb_info.pointer = P2V((uint32_t)fbinit->pointer) - 0xc0000000; //DMA offset
	_fb_info.size = fbinit->size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.size_max = _sys_info.phy_mem_size - (_fb_info.pointer-_sys_info.kernel_base);

	//kfree4k(fbinit);
	map_pages_size(_kernel_vm, 
		_fb_info.pointer,
		V2P(_fb_info.pointer),
		_fb_info.size_max,
		AP_RW_D, 0);
	flush_tlb();

#ifdef ENABLE_DPI
	const char pins[] = {0,1,2,3,4,5,6,7,8,9,10,11,
                    	12, 13, 14, 15, 16, 17, 18, 19, 20,
                    	21, 22, 23, 24, 25, 26, 27};


	for(int i = 0; i < sizeof(pins); i++){
		printf("set pin %d to %d\n", pins[i], GPIO_ALTF2);
		gpio_config(pins[i], GPIO_ALTF2);
	}
#endif

	return 0;
}

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
