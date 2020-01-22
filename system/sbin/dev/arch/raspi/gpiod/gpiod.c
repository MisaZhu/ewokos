#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/gpio.h>
#include <sys/vdevice.h>
#include <dev/device.h>
#include "../lib/gpio_arch.h"

static int gpio_fcntl(int fd, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	int32_t gpio_num = proto_read_int(in);
	if(cmd == 0) { //0: config
		int32_t v = proto_read_int(in);
		if(v == GPIO_MODE_INPUT)
			gpio_arch_config(gpio_num, GPIO_INPUT);
		else if(v == GPIO_MODE_OUTPUT)
			gpio_arch_config(gpio_num, GPIO_OUTPUT);
		else
			gpio_arch_config(gpio_num, v);
	}
	else if(cmd == 1) { //1: pull
		int32_t v = proto_read_int(in);
		if(v == GPIO_PULL_DOWN)
			gpio_arch_pull(gpio_num, 1);
		else if(v == GPIO_PULL_UP)
			gpio_arch_pull(gpio_num, 2);
		else
			gpio_arch_pull(gpio_num, v);
	}
	else if(cmd == 2) { //2: write
		int32_t v = proto_read_int(in);
		gpio_arch_write(gpio_num, v);
	}
	else if(cmd == 3) { //3: read
		int32_t v = gpio_arch_read(gpio_num);
		proto_add_int(out, v);
	}
	return 0;
}

int main(int argc, char** argv) {
	gpio_arch_init();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/gpio";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "gpio");
	dev.fcntl = gpio_fcntl;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
