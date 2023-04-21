#include <bcm283x/board.h>
#include <kstring.h>
#include <bcm283x/mailbox.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>

#define GET_BOARD_REVISION  0x00010002

static __attribute__((__aligned__(16))) uint32_t data[6];
uint32_t bcm283x_board(void) {
	memset(data, 0, 6*4);
	data[0] = 6*4;
	data[1] = 0;
	data[2] = GET_BOARD_REVISION;
	data[3] = 4;
	data[4] = 4;
	data[5] = 0;

	mail_message_t msg;
	memset(&msg, 0, sizeof(mail_message_t));
	msg.data = ((uint32_t)data + 0x40000000) >> 4;
	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);
	return data[5];
}