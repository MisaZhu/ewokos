#include "mm/mmu.h"
#include "mm/kmalloc.h"
#include "mm/kalloc.h"
#include "kstring.h"
#include <kernel/system.h>
#include "raspberrypi-firmware.h"
#include "mailbox.h"

#define VCMSG_ID_ARM_CLOCK 0x000000003		/* Clock/Voltage ID's */
#define RPI_FIRMWARE_V3D_CLK_ID     0x00000005

int firmware_property(uint32_t tag, uint32_t id, uint32_t *value) {
	/*message head + tag head + property*/
	size_t size = 12 + 12 + 8;
	uint32_t buf[8] __attribute__((aligned(16)));
	mail_message_t msg;

	/*message head*/
	buf[0] = size;
	buf[1] = RPI_FIRMWARE_STATUS_REQUEST;
	/*tag head*/
	buf[2] = tag;								/*tag*/
	buf[3] = 8;									/*buffer size*/
	buf[4] = 0;									/*respons size*/
	/*property package*/
	buf[5] =  id;				/*property id*/
	buf[6] =  *value;								/*property value*/
	/*message end*/
	buf[7] = RPI_FIRMWARE_PROPERTY_END;
	
	msg.data = ((uint32_t)buf + 0x40000000) >> 4;	
	mailbox_send(PROPERTY_CHANNEL, &msg);
	mailbox_read(PROPERTY_CHANNEL, &msg);
	*value = buf[6];
	//printf("value = %u\n", *value);
	return 0;

}

void cpu_freq_init(void) {
	uint32_t voltage, freq;
	/*adjuest cpu voltage*/
	 firmware_property(RPI_FIRMWARE_GET_MAX_VOLTAGE, VCMSG_ID_ARM_CLOCK, &voltage);	
	 firmware_property(RPI_FIRMWARE_SET_VOLTAGE, VCMSG_ID_ARM_CLOCK, &voltage);	
	 /*wait voltage stable*/
	 _delay_usec(20000);
	
	 firmware_property(RPI_FIRMWARE_GET_MAX_CLOCK_RATE, VCMSG_ID_ARM_CLOCK, &freq);	
	 firmware_property(RPI_FIRMWARE_SET_CLOCK_RATE, VCMSG_ID_ARM_CLOCK, &freq);	
}

