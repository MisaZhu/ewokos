#include <arch/arm/bcm283x/gpio.h>
#include <sys/vdevice.h>
#include <string.h>

#define DS   6
#define SHCP 19
#define STCP 13

static void init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(DS, GPIO_OUTPUT);
	bcm283x_gpio_config(SHCP, GPIO_OUTPUT);
	bcm283x_gpio_config(STCP, GPIO_OUTPUT);

	bcm283x_gpio_write(DS, 0);
	bcm283x_gpio_write(SHCP, 0);
	bcm283x_gpio_write(STCP, 0);
}

void leds(int v) {
	int i;
	for(i=0; i<8; i++) {
		bcm283x_gpio_write(DS, (v >> i) & 0x1);
		bcm283x_gpio_write(SHCP, 0);
		bcm283x_gpio_write(SHCP, 1);
	}
	bcm283x_gpio_write(STCP, 0);
	bcm283x_gpio_write(STCP, 1);
}

static int leds_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size == 0) {
		leds(0);
	}
	else {
		leds(((char*)buf)[0]);
	}
	return 1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/leds";
	init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "nxez_leds");
	dev.write = leds_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
