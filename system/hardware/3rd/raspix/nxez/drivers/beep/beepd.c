#include <arch/bcm283x/gpio.h>
#include <sys/vdevice.h>
#include <string.h>

#define BEEP   12

static void init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(BEEP, GPIO_OUTPUT);
	bcm283x_gpio_write(BEEP, 1);
}

static int beep_write(int fd, int from_pid, uint32_t node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size == 0) {
		bcm283x_gpio_write(BEEP, 1);
	}
	else {
		if(((char*)buf)[0] == 0)
			bcm283x_gpio_write(BEEP, 1);
		else
			bcm283x_gpio_write(BEEP, 0);
	}
	return 1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/beep";
	init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "nxez_beep");
	dev.write = beep_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
