#include <mm/mmu.h>
#include <dev/actled.h>
#include "bcm283x/mailbox.h"

void actled(bool on) {
	mail_message_t msg;
	/*message head + tag head + property*/
	uint32_t size = 12 + 12 + 8;
	uint32_t buf[8] __attribute__((aligned(16)));

	/*message head*/
	buf[0] = size;
	buf[1] = 0;	//RPI_FIRMWARE_STATUS_REQUEST;
	/*tag head*/
	buf[2] = 0x00038041;								/*tag*/
	buf[3] = 8;									/*buffer size*/
	buf[4] = 0;									/*respons size*/
	/*property package*/
	buf[5] =  130;				/*actled pin number*/
	buf[6] =  on ? 1: 0;								/*property value*/
	/*message end*/
	buf[7] = 0;
	
	msg.data = ((uint32_t)buf + 0x40000000) >> 4;	
	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);
}
