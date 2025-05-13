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

int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);

	bcm283x_mailbox_init();
	//fb_init_t* fbinit = (fb_init_t*)dma_phy_addr(dma_map(sizeof(fb_init_t)));
	fb_init_t* fbinit = (fb_init_t*)(dma_map(sizeof(fb_init_t)));
	
	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));

	memset(fbinit, 0, sizeof(fb_init_t));
	fbinit->width = w;
	fbinit->height = h;
	fbinit->vwidth = fbinit->width;
	fbinit->vheight = fbinit->height;
	fbinit->depth = dep;

	//msg.data = (syscall1(SYS_V2P, (uint32_t)fbinit) | 0xC0000000) >> 4; // ARM addr to GPU addr
	msg.data = (dma_phy_addr((ewokos_addr_t)fbinit) | 0xC0000000) >> 4; // ARM addr to GPU addr
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

	uint32_t phy_base = ((uint32_t)fbinit->pointer) & 0x3fffffff; //GPU addr to ARM addr
	_fb_info.pointer = sysinfo.fb.v_base;
	_fb_info.size = fbinit->size;
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	/*if(_fb_info.pointer < sysinfo.kernel_base) {
		_fb_info.pointer += sysinfo.kernel_base;
	}
	*/
	_fb_info.size_max = _fb_info.size;
	syscall3(SYS_MEM_MAP,(ewokos_addr_t) _fb_info.pointer, (ewokos_addr_t)phy_base, (ewokos_addr_t)_fb_info.size_max);
	return 0;
}

/*
int32_t bcm283x_fb_init(uint32_t w, uint32_t h, uint32_t dep) {
	memset(&_fb_info, 0, sizeof(fbinfo_t));
	bcm283x_mailbox_init();

	uint32_t* mbox = (uint32_t*)dma_phy_addr(dma_map(36));
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
    mbox[24] = 0;      //RGB, not BGR preferably

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
	msg.data = (syscall1(SYS_V2P, (ewokos_addr_t)&mbox | 0xC0000000)) >> 4; // ARM addr to GPU addr
	bcm283x_mailbox_send(PROPERTY_CHANNEL, &msg);
	bcm283x_mailbox_read(PROPERTY_CHANNEL, &msg);

	klog("%x, %d\n", mbox[28], mbox[29]);
    if(mbox[28] == 0 || mbox[29] == 0)
		return -1;
		
	_fb_info.pointer = syscall1(SYS_P2V, mbox[28] & 0x3fffffff); //GPU addr to ARM addr
	_fb_info.size = mbox[29];
	_fb_info.width = mbox[5];
	_fb_info.height = mbox[6];
	_fb_info.pitch = mbox[33];
	_fb_info.depth = mbox[20];
	_fb_info.xoffset = 0;
	_fb_info.yoffset = 0;

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);

	_fb_info.size_max = sysinfo.total_usable_mem_size - (_fb_info.pointer-sysinfo.kernel_base);
	syscall3(SYS_MEM_MAP, (ewokos_addr_t)_fb_info.pointer, (ewokos_addr_t)_fb_info.pointer-sysinfo.kernel_base, (ewokos_addr_t)_fb_info.size_max);
	return 0;
}
*/

fbinfo_t* bcm283x_get_fbinfo(void) {
	return &_fb_info;
}
