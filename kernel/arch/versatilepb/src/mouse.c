#include <types.h>
#include <mm/mmu.h>
#include <printk.h>
#include <system.h>
#include <proc.h>
#include <dev/basic_dev.h>

#define MOUSE_CR 0x00
#define MOUSE_STAT 0x04
#define MOUSE_DATA 0x08
#define MOUSE_CLKDIV  0x0C
#define MOUSE_IIR 0x10

#define MOUSE_CR_TYPE	 		(1 << 5)
#define MOUSE_CR_RXINTREN		(1 << 4)
#define MOUSE_CR_TXINTREN		(1 << 3)
#define MOUSE_CR_EN			(1 << 2)
#define MOUSE_CR_FD			(1 << 1)
#define MOUSE_CR_FC			(1 << 0)

#define MOUSE_STAT_TXEMPTY		(1 << 6)
#define MOUSE_STAT_TXBUSY		(1 << 5)
#define MOUSE_STAT_RXFULL		(1 << 4)
#define MOUSE_STAT_RXBUSY		(1 << 3)
#define MOUSE_STAT_RXPARITY	(1 << 2)
#define MOUSE_STAT_IC			(1 << 1)
#define MOUSE_STAT_ID			(1 << 0)

#define MOUSE_IIR_TXINTR		(1 << 1)
#define MOUSE_IIR_RXINTR		(1 << 0)

#define MOUSE_BASE (MMIO_BASE+0x7000)

static inline bool kmi_write(uint8_t data) {
	int32_t timeout = 1000;

	while((get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_TXEMPTY) == 0 && timeout--);
	if(timeout) {
		put8(MOUSE_BASE+MOUSE_DATA, data);
		while((get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_RXFULL) == 0);
		if(get8(MOUSE_BASE+MOUSE_DATA) == 0xfa)
			return true;
		else
			return false;
	}

	return false;
}

static inline bool kmi_read(uint8_t * data) {
	if( (get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_RXFULL)) {
		*data = get8(MOUSE_BASE+MOUSE_DATA);
		return true;
	}
	return false;
}

bool mouse_init() {
	uint8_t data;

  put8(MOUSE_BASE+MOUSE_CR, MOUSE_CR_EN);
	//reset mouse, and wait ack and pass/fail code
	if(! kmi_write(0xff) )
		return false;
	if(! kmi_read(&data))
		return false;
	if(data != 0xaa)
		return false;

	// enable scroll wheel
	kmi_write(0xf3);
	kmi_write(200);

	kmi_write(0xf3);
	kmi_write(100);

	kmi_write(0xf3);
	kmi_write(80);

	kmi_write(0xf2);
	kmi_read(&data);
	kmi_read(&data);

	// set sample rate, 100 samples/sec 
	kmi_write(0xf3);
	kmi_write(100);

	// set resolution, 4 counts per mm, 1:1 scaling
	kmi_write(0xe8);
	kmi_write(0x02);
	kmi_write(0xe6);

	//enable data reporting
	kmi_write(0xf4);

	// clear a receive buffer
	kmi_read(&data);
	kmi_read(&data);
	kmi_read(&data);
	kmi_read(&data);

	/* re-enables mouse */
  put8(MOUSE_BASE+MOUSE_CR, MOUSE_CR_EN|MOUSE_CR_RXINTREN); 
	return true;
}

#define MOUSE_BUF_SIZE 16*4

static char _buffer_data[MOUSE_BUF_SIZE];
static dev_buffer_t _mouse_buffer = { _buffer_data, MOUSE_BUF_SIZE, 0, 0 };
static int32_t _mouse_lock = 0;

void mouse_handle() {
	uint8_t packet[4], index = 0;
	static uint8_t btn_old = 0;
	int32_t dx, dy, dz;
	uint8_t btndown, btnup, btn;
	uint8_t status;

	status = get8(MOUSE_BASE + MOUSE_IIR);
	while(status & MOUSE_IIR_RXINTR) {
		packet[index] = get8(MOUSE_BASE + MOUSE_DATA);
		index = (index + 1) & 0x3;

		if(index == 0) {
			btn = packet[0] & 0x7;

			btndown = (btn ^ btn_old) & btn;
			btnup = (btn ^ btn_old) & btn_old;
			btn_old = btn;

			if(packet[0] & 0x10)
				dx = 0xffffff00 | packet[1]; //nagtive
			else
				dx = packet[1];

			if(packet[0] & 0x20)
				dy = 0xffffff00 | packet[2]; //nagtive
			else
				dy = packet[2];

			dz = packet[3] & 0xf;
			if(dz == 0xf)
				dz = -1;
			printk("mouse: down:%d, up:%d, x:%d, y:%d, z:%d\n", btndown, btnup, dx, dy, dz);
		}
		status = get8(MOUSE_BASE + MOUSE_IIR);
	}
	get8(MOUSE_BASE + MOUSE_STAT);
}
