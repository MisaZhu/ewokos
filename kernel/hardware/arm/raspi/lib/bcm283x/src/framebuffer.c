#include <kstring.h>
#include <bcm283x/mailbox.h>
#include <bcm283x/framebuffer.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <bcm283x/gpio.h>

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

static __attribute__((__aligned__(PAGE_SIZE))) fb_init_t _fbinit;
int32_t fb_init_raw(uint32_t w, uint32_t h, uint32_t dep) {
	if(strstr(_sys_info.machine, "clockwork") != 0) {
		//for clockwork
		_fb_info.width = w;
		_fb_info.height = h;
		_fb_info.vwidth = w;
		_fb_info.vheight = h;
		_fb_info.depth = dep;
		_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

		_fb_info.pointer = P2V(0xC00000); //GPU addr to ARM addr
		_fb_info.size = w*h*(dep/8);
		_fb_info.xoffset = 0;
		_fb_info.yoffset = 0;
		_fb_info.size_max = w*h*(dep/8);
	}
	else {
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

		msg.data = (V2P((uint32_t)fbinit) | 0xC0000000) >> 4; // ARM addr to GPU addr
		flush_dcache();

		mailbox_send(FRAMEBUFFER_CHANNEL, &msg);
		mailbox_read(FRAMEBUFFER_CHANNEL, &msg);

		if(fbinit->pointer == 0)
			return -1;

		_fb_info.width = fbinit->width;
		_fb_info.height = fbinit->height;
		_fb_info.vwidth = fbinit->width;
		_fb_info.vheight = fbinit->height;
		_fb_info.depth = fbinit->depth;
		_fb_info.pitch = _fb_info.width*(_fb_info.depth/8);

		_fb_info.pointer = P2V(((uint32_t)fbinit->pointer) & 0x3fffffff); //GPU addr to ARM addr
		_fb_info.size = fbinit->size;
		_fb_info.xoffset = 0;
		_fb_info.yoffset = 0;
		_fb_info.size_max = _sys_info.phy_mem_size - (_fb_info.pointer-_sys_info.kernel_base);
	}
	//kfree4k(fbinit);
	map_pages_size(_kernel_vm, 
		_fb_info.pointer,
		V2P(_fb_info.pointer),
		_fb_info.size_max,
		AP_RW_D, PTE_ATTR_DEV);
	flush_tlb();
	
	return 0;
}

/*
volatile uint32_t  __attribute__((aligned(16))) mbox[36];
int32_t fb_init_raw(uint32_t w, uint32_t h, uint32_t dep) {
    mbox[0] = 35*4;
    mbox[1] = 0; //request

    mbox[2] = 0x48003;  //set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = w;
    mbox[6] = h;

    mbox[7] = 0x48004;  //set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = w;
    mbox[11] = h;

    mbox[12] = 0x48009; //set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0;       //FrameBufferInfo.x_offset
    mbox[16] = 0;       //FrameBufferInfo.y.offset

    mbox[17] = 0x48005; //set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = dep;      //FrameBufferInfo.depth

    mbox[21] = 0x48006; //set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;      //RGB, not BGR preferably

    mbox[25] = 0x40001; //get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;    //FrameBufferInfo.pointer
    mbox[29] = 0;       //FrameBufferInfo.size

    mbox[30] = 0x40008; //get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;       //FrameBufferInfo.pitch

    mbox[34] = 0;

	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));
	msg.data = (V2P((uint32_t)&mbox | 0xC0000000)) >> 4; // ARM addr to GPU addr
	flush_dcache();

	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);

    if(mbox[28] == 0 || mbox[29] == 0)
		return -1;
		
	_fb_info.pointer = P2V(mbox[28] & 0x3fffffff); //GPU addr to ARM addr
	_fb_info.width = mbox[5];
	_fb_info.height = mbox[6];
	_fb_info.pitch = mbox[33];
	_fb_info.depth = mbox[20];
	_fb_info.size = mbox[29];
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;
	_fb_info.size_max = _sys_info.phy_mem_size - (_fb_info.pointer-_sys_info.kernel_base);
 	
	 map_pages_size(_kernel_vm, 
		_fb_info.pointer,
		V2P(_fb_info.pointer),
		_fb_info.size_max,
		AP_RW_D, 0);
	flush_tlb();

	return 0;
}
*/

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	if(fb_init_raw(w, h, dep) != 0)
		return -1;

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
