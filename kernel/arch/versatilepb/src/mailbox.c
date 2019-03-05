#include "mailbox.h"
#include "mm/mmu.h"
#include "hardware.h"

#define MAILBOX_OFFSET 0x0000B880
#define MAILBOX_BASE (MMIO_BASE | MAILBOX_OFFSET)

#define MAIL0_BASE    (MAILBOX_BASE+0x00)
#define MAIL0_READ    (MAILBOX_BASE+0x00)
#define MAIL0_POLL    (MAILBOX_BASE+0x10)
#define MAIL0_SEND_ID (MAILBOX_BASE+0x14)
#define MAIL0_STATUS  (MAILBOX_BASE+0x18)
#define MAIL0_CONFIG  (MAILBOX_BASE+0x1C)
#define MAIL0_WRITE   (MAILBOX_BASE+0x20)
/* MAIL0_WRITE IS ACTUALLY MAIL1_BASE/MAIL1_READ? */
#define MAIL1_BASE    (MAILBOX_BASE+0x20)
#define MAIL1_READ    (MAILBOX_BASE+0x20)
#define MAIL1_POLL    (MAILBOX_BASE+0x30)
#define MAIL1_SEND_ID (MAILBOX_BASE+0x34)
#define MAIL1_STATUS  (MAILBOX_BASE+0x38)
#define MAIL1_CONFIG  (MAILBOX_BASE+0x3C)

#define MAIL_CHANNEL_MASK 0x0000000F
#define MAIL_CH_POWER 0x00000000
#define MAIL_CH_FBUFF 0x00000001
#define MAIL_CH_VUART 0x00000002
#define MAIL_CH_VCHIQ 0x00000003
#define MAIL_CH_LEDS  0x00000004
#define MAIL_CH_BUTTS 0x00000005
#define MAIL_CH_TOUCH 0x00000006
#define MAIL_CH_NOUSE 0x00000007
#define MAIL_CH_TAGAV 0x00000008
#define MAIL_CH_TAGVA 0x00000009
/**
0: Power management
1: Framebuffer
2: Virtual UART
3: VCHIQ
4: LEDs
5: Buttons
6: Touch screen
7: <NOT USED?>
8: Property tags (ARM -> VC)
9: Property tags (VC -> ARM)
**/

#define TAGS_STATUS_REQUEST 0x00000000
#define TAGS_STATUS_SUCCESS 0x80000000
#define TAGS_STATUS_FAILURE 0x80000001

#define TAGS_END 				0x00000000
#define TAGS_MASK_SET			0x00008000
#define TAGS_REQUESTS			0x00000000
#define TAGS_RESPONSE			0x80000000

/** number of 32-bit buffer allocated for tags response */
#define TAGS_RESPONSE_SIZE		2

/** VideoCore */
#define TAGS_FIRMWARE_REVISION	0x00000001

#define TAGS_SET_CURSOR_INFO	(0x00000010|TAGS_MASK_SET)
#define TAGS_SET_CURSOR_STATE	(0x00000011|TAGS_MASK_SET)

/** Hardware */
#define TAGS_BOARD_MODEL		0x00010001
#define TAGS_BOARD_REVISION		0x00010002
#define TAGS_BOARD_MAC_ADDR		0x00010003
#define TAGS_BOARD_SERIAL		0x00010004
#define TAGS_ARM_MEMORY			0x00010005
#define TAGS_VC_MEMORY			0x00010006
#define TAGS_CLOCKS				0x00010007

#define TAGS_POWER_STATE		0x00020001
#define TAGS_TIMING				0x00020002

#define TAGS_SET_POWER_STATE	(TAGS_POWER_STATE|TAGS_MASK_SET)

#define TAGS_CLOCK_STATE		0x00030001
#define TAGS_CLOCK_RATE			0x00030002
#define TAGS_VOLTAGE			0x00030003
#define TAGS_MAX_CLOCK_RATE		0x00030004
#define TAGS_MAX_VOLTAGE		0x00030005
#define TAGS_TEMPERATURE		0x00030006
#define TAGS_MIN_CLOCK_RATE		0x00030007
#define TAGS_MIN_VOLTAGE		0x00030008
#define TAGS_TURBO				0x00030009
#define TAGS_MAX_TEMPERATURE	0x0003000a
#define TAGS_STC				0x0003000b
#define TAGS_ALLOCATE_MEMORY	0x0003000c
#define TAGS_LOCK_MEMORY		0x0003000d
#define TAGS_UNLOCK_MEMORY		0x0003000e
#define TAGS_RELEASE_MEMORY		0x0003000f
#define TAGS_EXECUTE_CODE		0x00030010
#define TAGS_EXECUTE_QPU		0x00030011
#define TAGS_ENABLE_QPU			0x00030012
#define TAGS_X_RES_MEM_HANDLE	0x00030014
#define TAGS_EDID_BLOCK			0x00030020
#define TAGS_CUSTOMER_OTP		0x00030021
#define TAGS_DOMAIN_STATE		0x00030030
#define TAGS_GPIO_STATE			0x00030041
#define TAGS_GPIO_CONFIG		0x00030043

#define TAGS_SET_CLOCK_STATE	(TAGS_CLOCK_STATE|TAGS_MASK_SET)
#define TAGS_SET_CLOCK_RATE		(TAGS_CLOCK_RATE|TAGS_MASK_SET)
#define TAGS_SET_VOLTAGE		(TAGS_VOLTAGE|TAGS_MASK_SET)
#define TAGS_SET_TURBO			(TAGS_TURBO|TAGS_MASK_SET)
#define TAGS_SET_CUSTOMER_OTP	(TAGS_CUSTOMER_OTP|TAGS_MASK_SET)
#define TAGS_SET_DOMAIN_STATE	(TAGS_DOMAIN_STATE|TAGS_MASK_SET)
#define TAGS_SET_GPIO_STATE		(TAGS_GPIO_STATE|TAGS_MASK_SET)
#define TAGS_SET_SDHOST_CLOCK	(0x00038042|TAGS_MASK_SET)
#define TAGS_SET_GPIO_CONFIG	(TAGS_GPIO_CONFIG|TAGS_MASK_SET)

#define TAGS_FB_ALLOCATE		0x00040001
#define TAGS_FB_BLANK			0x00040002
#define TAGS_FB_PHYS_DIMS		0x00040003
#define TAGS_FB_VIRT_DIMS		0x00040004
#define TAGS_FB_DEPTH			0x00040005
#define TAGS_FB_PIXEL_ORDER		0x00040006
#define TAGS_FB_ALPHA_MODE		0x00040007
#define TAGS_FB_PITCH			0x00040008
#define TAGS_FB_VIROFFSET		0x00040009
#define TAGS_FB_OVERSCAN		0x0004000a
#define TAGS_FB_PALETTE			0x0004000b
#define TAGS_FB_TOUCHBUF		0x0004000f
#define TAGS_FB_GPIOVIRTBUF		0x00040010

#define TAGS_FBT_PHYS_DIMS		0x00044003
#define TAGS_FBT_VIRT_DIMS		0x00044004
#define TAGS_FBT_DEPTH			0x00044005
#define TAGS_FBT_PIXEL_ORDER	0x00044006
#define TAGS_FBT_ALPHA_MODE		0x00044007
#define TAGS_FBT_VIROFFSET		0x00044009
#define TAGS_FBT_OVERSCAN		0x0004400a
#define TAGS_FBT_PALETTE		0x0004400b
#define TAGS_FBT_VSYNC			0x0004400e

#define TAGS_FB_RELEASE			(TAGS_FB_ALLOCATE|TAGS_MASK_SET)
#define TAGS_FB_SET_PHYS_DIMS	(TAGS_FB_PHYS_DIMS|TAGS_MASK_SET)
#define TAGS_FB_SET_VIRT_DIMS	(TAGS_FB_VIRT_DIMS|TAGS_MASK_SET)
#define TAGS_FB_SET_DEPTH		(TAGS_FB_DEPTH|TAGS_MASK_SET)
#define TAGS_FB_SET_PIXEL_ORDER	(TAGS_FB_PIXEL_ORDER|TAGS_MASK_SET)
#define TAGS_FB_SET_ALPHA_MODE	(TAGS_FBT_ALPHA_MODE|TAGS_MASK_SET)
#define TAGS_FB_SET_VIROFFSET	(TAGS_FB_VIROFFSET|TAGS_MASK_SET)
#define TAGS_FB_SET_OVERSCAN	(TAGS_FB_OVERSCAN|TAGS_MASK_SET)
#define TAGS_FB_SET_PALETTE		(TAGS_FB_PALETTE|TAGS_MASK_SET)
#define TAGS_FB_SET_TOUCHBUF	(0x0004801f|TAGS_MASK_SET)
#define TAGS_FB_SET_GPIOVIRTBUF	(0x00048020|TAGS_MASK_SET)
#define TAGS_FB_SET_VSYNC		(0x0004800e|TAGS_MASK_SET)
#define TAGS_FB_SET_BACKLIGHT	(0x0004800f|TAGS_MASK_SET)
#define TAGS_VCHIQ_INIT			(0x00048010|TAGS_MASK_SET)

#define TAGS_COMMAND_LINE		0x00050001
#define TAGS_DMA_CHANNELS		0x00060001

#define INFO_STATUS_OK 0
#define INFO_STATUS_UNKNOWN_ERROR_ 1
#define INFO_STATUS_INVALID_BUFFER 2
#define INFO_STATUS_REQUEST_FAILED 3
#define INFO_STATUS_REQUEST_ERROR_ 4
#define INFO_STATUS_READING_TAGS 5

void __memBarrier();

static inline unsigned int get32(unsigned int addr) {
	return *(unsigned int*)addr;
}

static inline void put32(unsigned int addr, unsigned int value) {
	*((unsigned int*)addr) = value;
}

/**
 * Mailbox (MB) 0: VC -> ARM, MB 1: ARM->VC
 * - write to MB1, read from MB0!
**/
#define MAIL_STATUS_FULL  0x80000000
#define MAIL_STATUS_EMPTY 0x40000000

void mailboxInit(void) {
	/* nothing to do! */
}

unsigned int mailboxRead(unsigned int channel) {
	unsigned int value;
	while(true) {
		/* wait if mailbox is empty */
		while(get32(MAIL0_STATUS) & MAIL_STATUS_EMPTY);
		/* get value@channel */
		value = get32(MAIL0_BASE);
		/* check if the expected channel */
		if ((value & MAIL_CHANNEL_MASK)==channel) break;
		/* if not, data lost? should we store it somewhere? */
	}
	/* return address only */
	value &= ~MAIL_CHANNEL_MASK;
	return value;
}

void mailboxWrite(unsigned int channel,unsigned int value) {
	/* merge value/channel data */
	value &= ~MAIL_CHANNEL_MASK;
	value |= (channel & MAIL_CHANNEL_MASK);
	/* wait if mailbox is full */
	while (get32(MAIL1_STATUS) & MAIL_STATUS_FULL);
	/* read-write barrier */
	__memBarrier();
	/* send it to MB1! */
	put32(MAIL1_BASE, value);
}

/* for a 1kb mailbox buffer size */
#define MAIL_BUFFER_SIZE 256
volatile unsigned int mbbuff[MAIL_BUFFER_SIZE] __attribute__((aligned(16)));

int tags_init(volatile unsigned int* buff) {
	int size = 1;
	buff[size++] = TAGS_STATUS_REQUEST;
	buff[size++] = TAGS_END;
	buff[0] = size*sizeof(unsigned int);
	return size;
}

int tags_insert(volatile unsigned int* buff, int size, unsigned int tags_id, int tags_buff_count) {
	size--; /* override previous tags_end! */
	buff[size++] = tags_id;
	buff[size++] = tags_buff_count*sizeof(unsigned int);
	buff[size++] = TAGS_REQUESTS;
	while (tags_buff_count>0) {
		buff[size++] = 0x00000000;
		tags_buff_count--;
	}
	/* place terminating tags */
	buff[size++] = TAGS_END;
	/* update size */
	buff[0] = size*sizeof(unsigned int);
	return size;
}

int tags_isinfo(volatile unsigned int* buff, int size, unsigned int tags_id, int tags_needed, volatile TagsHeadT** pptag) {
	TagsHeadT* ptag = (TagsHeadT*)&buff[size];
	unsigned int test = ptag->req_res&TAGS_RESPONSE;
	unsigned int temp = ptag->req_res&~TAGS_RESPONSE;
	unsigned int vbufsize = tags_needed*sizeof(unsigned int);
	size += (ptag->vbuf_size/sizeof(unsigned int))+3;
	if (ptag->tags_id!=tags_id||ptag->vbuf_size<vbufsize||!test||temp>vbufsize)
		*pptag = 0x0;
	else
		*pptag = ptag;
	return size;
}

unsigned int* mailboxGetBoardInfo(TagsInfoT* info) {
	volatile TagsHeadT *ptag;
	unsigned int temp = (unsigned int)mbbuff, test;
	int size, read;
	/* for DEBUG! */
	info->buff = mbbuff;
	/* configure buffer for request */
	size = tags_init(mbbuff);
	/* tag to request firmware revision */
	size = tags_insert(mbbuff,size,TAGS_FIRMWARE_REVISION,TAGS_RESPONSE_SIZE);
	/* tag to request board model */
	size = tags_insert(mbbuff,size,TAGS_BOARD_MODEL,TAGS_RESPONSE_SIZE);
	/* tag to request board revision */
	size = tags_insert(mbbuff,size,TAGS_BOARD_REVISION,TAGS_RESPONSE_SIZE);
	/* tag to request board mac addr */
	size = tags_insert(mbbuff,size,TAGS_BOARD_MAC_ADDR,TAGS_RESPONSE_SIZE);
	/* tag to request board serial num */
	size = tags_insert(mbbuff,size,TAGS_BOARD_SERIAL,TAGS_RESPONSE_SIZE);
	/* tag to request arm memory */
	size = tags_insert(mbbuff,size,TAGS_ARM_MEMORY,TAGS_RESPONSE_SIZE);
	/* tag to request videocore memory */
	size = tags_insert(mbbuff,size,TAGS_VC_MEMORY,TAGS_RESPONSE_SIZE);
	/* prepare address */
	temp = P2V(temp);
	/* mail it! */
	__memBarrier();
	mailboxWrite(MAIL_CH_TAGAV, temp);
	__memBarrier();
	test = mailboxRead(MAIL_CH_TAGAV);
	__memBarrier();
	/* validate response */
	if (test!=temp) {
		info->test = test;
		info->temp = temp;
		info->info_status = INFO_STATUS_INVALID_BUFFER;
		return 0x0;
	}
	/* DEBUG */
	info->test = mbbuff[0];
	info->temp = mbbuff[1];
	read = 2;
	/* get request status */
	switch(info->temp) {
		case TAGS_STATUS_SUCCESS:
			info->info_status = INFO_STATUS_READING_TAGS;
			break;
		case TAGS_STATUS_FAILURE:
			info->info_status = INFO_STATUS_REQUEST_FAILED;
			return 0x0;
		default:
			info->info_status = INFO_STATUS_REQUEST_ERROR_;
			return 0x0;
	}
	/* get firmware revision */
	read = tags_isinfo(mbbuff,read,TAGS_FIRMWARE_REVISION,1,&ptag);
	if (!ptag) return 0x0;
	info->vc_revision = ptag->vbuffer[0];
	/* get board model */
	read = tags_isinfo(mbbuff,read,TAGS_BOARD_MODEL,1,&ptag);
	if (!ptag) return 0x0;
	info->board_model = ptag->vbuffer[0];
	/* get board revision */
	read = tags_isinfo(mbbuff,read,TAGS_BOARD_REVISION,1,&ptag);
	if (!ptag) return 0x0;
	info->board_revision = ptag->vbuffer[0];
	/* get board mac addr */
	read = tags_isinfo(mbbuff,read,TAGS_BOARD_MAC_ADDR,2,&ptag);
	if (!ptag) return 0x0;
	/** need to convert network byte order to little endian! */
	info->board_mac_addrh = ptag->vbuffer[0];
	info->board_mac_addrl = ptag->vbuffer[1];
	/* get board serial num */
	read = tags_isinfo(mbbuff,read,TAGS_BOARD_SERIAL,2,&ptag);
	if (!ptag) return 0x0;
	info->board_serial_l = ptag->vbuffer[0];
	info->board_serial_h = ptag->vbuffer[1];
	/* get arm memory */
	read = tags_isinfo(mbbuff,read,TAGS_ARM_MEMORY,2,&ptag);
	if (!ptag) return 0x0;
	info->memory_arm_base = ptag->vbuffer[0];
	info->memory_arm_size = ptag->vbuffer[1];
	/* get vc memory */
	read = tags_isinfo(mbbuff,read,TAGS_VC_MEMORY,2,&ptag);
	if (!ptag) return 0x0;
	info->memory_vc_base = ptag->vbuffer[0];
	info->memory_vc_size = ptag->vbuffer[1];
	/* return pointer to buffer on success */
	info->info_status = INFO_STATUS_OK;
	return (unsigned int*)mbbuff;
}

unsigned int* mailboxGetVideoInfo(TagsInfoT* info) {
	volatile TagsHeadT *ptag;
	unsigned int temp = (unsigned int)mbbuff, test;
	int size, read;
	/* for DEBUG! */
	info->buff = mbbuff;
	/* configure buffer for request */
	size = tags_init(mbbuff);
	/* tag to request physical display dimension */
	size = tags_insert(mbbuff,size,TAGS_FB_PHYS_DIMS,TAGS_RESPONSE_SIZE);
	/* tag to request virtual display dimension */
	size = tags_insert(mbbuff,size,TAGS_FB_VIRT_DIMS,TAGS_RESPONSE_SIZE);
	/* tag to request depth (bits-per-pixel) */
	size = tags_insert(mbbuff,size,TAGS_FB_DEPTH,TAGS_RESPONSE_SIZE);
	/* tag to request pixel order */
	size = tags_insert(mbbuff,size,TAGS_FB_PIXEL_ORDER,TAGS_RESPONSE_SIZE);
	/* tag to request alpha mode */
	size = tags_insert(mbbuff,size,TAGS_FB_ALPHA_MODE,TAGS_RESPONSE_SIZE);
	/* tag to request pitch */
	size = tags_insert(mbbuff,size,TAGS_FB_PITCH,TAGS_RESPONSE_SIZE);
	/* tag to request virtual offset */
	size = tags_insert(mbbuff,size,TAGS_FB_VIROFFSET,TAGS_RESPONSE_SIZE);
	/* prepare address */
	temp = P2V(temp);
	/* mail it! */
	__memBarrier();
	mailboxWrite(MAIL_CH_TAGAV, temp);
	__memBarrier();
	test = mailboxRead(MAIL_CH_TAGAV);
	__memBarrier();
	/* validate response */
	if (test!=temp) {
		info->test = test;
		info->temp = temp;
		info->info_status = INFO_STATUS_INVALID_BUFFER;
		return 0x0;
	}
	/* DEBUG */
	info->test = mbbuff[0];
	info->temp = mbbuff[1];
	read = 2;
	/* get request status */
	switch(info->temp) {
		case TAGS_STATUS_SUCCESS:
			info->info_status = INFO_STATUS_READING_TAGS;
			break;
		case TAGS_STATUS_FAILURE:
			info->info_status = INFO_STATUS_REQUEST_FAILED;
			return 0x0;
		default:
			info->info_status = INFO_STATUS_REQUEST_ERROR_;
			return 0x0;
	}
	/* get physical dimension */
	read = tags_isinfo(mbbuff,read,TAGS_FB_PHYS_DIMS,2,&ptag);
	if (!ptag) return 0x0;
	info->fb_width = ptag->vbuffer[0];
	info->fb_height = ptag->vbuffer[1];
	/* get virtual dimension */
	read = tags_isinfo(mbbuff,read,TAGS_FB_VIRT_DIMS,2,&ptag);
	if (!ptag) return 0x0;
	info->fb_vwidth = ptag->vbuffer[0];
	info->fb_vheight = ptag->vbuffer[1];
	/* get depth */
	read = tags_isinfo(mbbuff,read,TAGS_FB_DEPTH,1,&ptag);
	if (!ptag) return 0x0;
	info->fb_depth = ptag->vbuffer[0];
	/* get pixel order */
	read = tags_isinfo(mbbuff,read,TAGS_FB_PIXEL_ORDER,1,&ptag);
	if (!ptag) return 0x0;
	info->fb_pixel_order = ptag->vbuffer[0]; /** 0x0:BGR , 0x1:RGB */
	/* get apha mode */
	read = tags_isinfo(mbbuff,read,TAGS_FB_ALPHA_MODE,1,&ptag);
	if (!ptag) return 0x0;
	/** 0x0:enabled(0=opaque) , 0x1:reversed(0=transparent), 0x2:ignored */
	info->fb_alpha_mode = ptag->vbuffer[0];
	/* get pitch */
	read = tags_isinfo(mbbuff,read,TAGS_FB_PITCH,1,&ptag);
	if (!ptag) return 0x0;
	info->fb_pitch = ptag->vbuffer[0];
	/* get virtual offsets */
	read = tags_isinfo(mbbuff,read,TAGS_FB_VIROFFSET,2,&ptag);
	if (!ptag) return 0x0;
	info->fb_vx_offset = ptag->vbuffer[0];
	info->fb_vy_offset = ptag->vbuffer[1];
	/* return pointer to buffer on success */
	info->info_status = INFO_STATUS_OK;
	return (unsigned int*)mbbuff;
}
