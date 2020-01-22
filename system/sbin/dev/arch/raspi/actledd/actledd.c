#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <dev/device.h>
#include "../lib/gpio_arch.h"

static int actled_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;

	if(((const char*)buf)[0] == 0)
		gpio_arch_write(47, 1); //1 for off
	else
		gpio_arch_write(47, 0); //o for on
	return 1;
}

int main(int argc, char** argv) {
	gpio_arch_init();
	gpio_arch_config(47, 1);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/actled";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.write = actled_write;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
