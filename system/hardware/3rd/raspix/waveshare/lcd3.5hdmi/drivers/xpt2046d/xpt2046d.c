#include <xpt2046/xpt2046.h>
#include <ewoksys/vdevice.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>

static int tp_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	if(size < 6)
		return 0;

	uint16_t* d = (uint16_t*)buf;

	if(xpt2046_read(&d[0], &d[1], &d[2]) != 0)
		return 0;
	return 6;	
}

int main(int argc, char** argv) {
	//GPIO ports for waveshare 3.5 inch
	int TP_CS = 7;
	int TP_IRQ = 25;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/touch0";

	if(argc > 3) {
		TP_CS = atoi(argv[2]);
		TP_IRQ = atoi(argv[3]);
	}

	bcm283x_spi_init();
	xpt2046_init(TP_CS, TP_IRQ, 256);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xpt2046");
	dev.read = tp_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
