#include <arch/bcm283x/mailbox.h>
#include <ewoksys/dma.h>

void bcm283x_mbox_actled(bool on) {
	mail_message_t msg;
	/*message head + tag head + property*/
	uint32_t size = 12 + 12 + 8;
	//uint32_t* buf = (uint32_t*)dma_phy_addr(0, dma_alloc(0, size));
	uint32_t* buf = (uint32_t*)(dma_alloc(0, size));

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
	msg.channel = PROPERTY_CHANNEL;
	bcm283x_mailbox_call(&msg);
}
