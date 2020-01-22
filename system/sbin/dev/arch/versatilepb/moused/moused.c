#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/mmio.h>


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

#define MOUSE_BASE (_mmio_base+0x7000)

static uint32_t _mmio_base = 0;

static inline int32_t kmi_write(uint8_t data) {
	int32_t timeout = 1000;

	while((get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_TXEMPTY) == 0 && timeout--);
	if(timeout) {
		put8(MOUSE_BASE+MOUSE_DATA, data);
		while((get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_RXFULL) == 0);
		if(get8(MOUSE_BASE+MOUSE_DATA) == 0xfa)
			return 1;
		else
			return 0;
	}
	return 0;
}

static inline int32_t kmi_read(uint8_t * data) {
	if( (get8(MOUSE_BASE+MOUSE_STAT) & MOUSE_STAT_RXFULL)) {
		*data = get8(MOUSE_BASE+MOUSE_DATA);
		return 1;
	}
	return 0;
}

int32_t mouse_init(void) {
	_mmio_base = mmio_map();
	uint8_t data;
	uint32_t divisor = 1000;
	put8(MOUSE_CLKDIV, divisor);

	put8(MOUSE_BASE+MOUSE_CR, MOUSE_CR_EN);
	//reset mouse, and wait ack and pass/fail code
	if(! kmi_write(0xff) )
		return -1;
	if(! kmi_read(&data))
		return -1;
	if(data != 0xaa)
		return -1;

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
	return 0;
}

typedef struct {
	int8_t btn;
	int8_t rx;
	int8_t ry;
	int8_t rz;
} mouse_info_t;

int32_t mouse_handler(mouse_info_t *info) {
	static uint8_t packet[4], index = 0;
	static uint8_t btn_old = 0;
	uint8_t btndown, btnup, btn;
	uint8_t status;
	int32_t rx, ry, rz;

	status = get8(MOUSE_BASE + MOUSE_IIR);
	if(status & MOUSE_IIR_RXINTR) {
		packet[index] = get8(MOUSE_BASE + MOUSE_DATA);
		index = (index + 1) & 0x3;
		if(index == 0) {
			btn = packet[0] & 0x7;

			btndown = (btn ^ btn_old) & btn;
			btnup = (btn ^ btn_old) & btn_old;
			btn_old = btn;

			if(packet[0] & 0x10)
				rx = (int8_t)(0xffffff00 | packet[1]); //nagtive
			else
				rx = (int8_t)packet[1];

			if(packet[0] & 0x20)
				ry = -(int8_t)(0xffffff00 | packet[2]); //nagtive
			else
				ry = -(int8_t)packet[2];

			rz = (int8_t)(packet[3] & 0xf);
			if(rz == 0xf)
				rz = -1;
			
			btndown = (btndown << 1 | btnup);

			info->btn = btndown;
			info->rx = rx;
			info->ry = ry;
			info->rz = rz;
			return 0;
		}
	}
	return -1;
}

static int mouse_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)info;

	mouse_info_t minfo;
	if(size < 4 || mouse_handler(&minfo) != 0)
		return ERR_RETRY;

	uint8_t* d = (uint8_t*)buf;
	d[0] = minfo.btn;
	d[1] = minfo.rx;
	d[2] = minfo.ry;
	d[3] = minfo.rz;
	return 4;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/mouse0";

	mouse_init();
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mouse");
	dev.read = mouse_read;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
