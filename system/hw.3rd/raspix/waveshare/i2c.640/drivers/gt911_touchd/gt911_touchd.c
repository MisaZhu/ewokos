#include <ewoksys/vdevice.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/mmio.h>

#include "gt911.h"
static bool press = false;
static	TouchCordinate_t cordinate[5];
static 	uint8_t  number_of_cordinate = 0;
static 	uint64_t last_ts = 0;	

static int tp_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;
	(void)size;

	uint16_t* d = (uint16_t*)buf;
	number_of_cordinate = 0;
	GT911_Status_t ret = GT911_ReadTouch(cordinate, &number_of_cordinate);
	if (ret != GT911_OK || !number_of_cordinate)
	{
		if (press && (kernel_tic_ms(0) - last_ts) > 100)
		{
			press = false;
			d[0] = press;
			d[1] = cordinate[0].x;
			d[2] = cordinate[0].y;
			return 6;
		}else{
			return 0;
		}
	}
	last_ts = kernel_tic_ms(0);
	press = true;
	d[0] = press;
	d[1] = cordinate[0].x;
	d[2] = cordinate[0].y;	

	return 6;	
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/touch0";
	_mmio_base = mmio_map();

    GT911_Init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "gt911");
	dev.read = tp_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
