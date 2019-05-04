#ifndef MAILBOX_H
#define MAILBOX_H

void mailboxInit(void);
unsigned int mailboxRead(unsigned int channel);
void mailboxWrite(unsigned int channel,unsigned int value);

typedef struct {
	unsigned int tags_id;
	unsigned int vbuf_size; /* usually 8-bytes */
	unsigned int req_res; /* req:0x00000000, res: 0x80000000 | value length */
	unsigned int vbuffer[2]; /* most tag response is 8-bytes long */
} TagsHeadT;

typedef struct {
	unsigned int info_status;
	unsigned int test, temp;
	volatile unsigned int *buff;
	/* hardware/board info */
	unsigned int vc_revision;
	unsigned int board_model;
	unsigned int board_revision;
	unsigned int board_mac_addrl; /* 6-bytes actually! */
	unsigned int board_mac_addrh;
	unsigned int board_serial_l;
	unsigned int board_serial_h;
	unsigned int memory_arm_base;
	unsigned int memory_arm_size;
	unsigned int memory_vc_base;
	unsigned int memory_vc_size;
	/* framebuffer info */
	unsigned int fb_width, fb_height;
	unsigned int fb_vwidth, fb_vheight;
	unsigned int fb_depth, fb_pixel_order;
	unsigned int fb_alpha_mode, fb_pitch;
	unsigned int fb_vx_offset, fb_vy_offset;
} TagsInfoT;

unsigned int* mailboxGetBoardInfo(TagsInfoT* info);
unsigned int* mailboxGetVideoInfo(TagsInfoT* info);

#endif
