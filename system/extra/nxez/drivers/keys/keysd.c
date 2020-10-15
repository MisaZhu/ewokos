#include <arch/arm/bcm283x/gpio.h>
#include <sys/vdevice.h>
#include <string.h>

#define YELLOW  20
#define BLUE    16
#define SW1     21
#define SW2     26

static void init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(YELLOW, GPIO_INPUT);
	bcm283x_gpio_config(BLUE, GPIO_INPUT);
	bcm283x_gpio_config(SW1, GPIO_INPUT);
	bcm283x_gpio_config(SW2, GPIO_INPUT);

	bcm283x_gpio_pull(YELLOW, GPIO_PULL_UP);
	bcm283x_gpio_pull(BLUE, GPIO_PULL_UP);
	bcm283x_gpio_pull(SW1, GPIO_PULL_UP);
	bcm283x_gpio_pull(SW2, GPIO_PULL_UP);
}

static int keys_read(int fd,
		int from_pid,
		fsinfo_t* info,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)buf;
	(void)size;
	(void)offset;
	(void)p;

	if(size <= 0)
		return 0;
	//read yellow
	char v = 0;
	if(bcm283x_gpio_read(YELLOW) == 0)
		v |= 0x1;
	if(bcm283x_gpio_read(BLUE) == 0)
		v |= 0x2;
	if(bcm283x_gpio_read(SW1) == 0)
		v |= 0x4;
	if(bcm283x_gpio_read(SW2) == 0)
		v |= 0x8;
	((char*)buf)[0] = v;
	return 1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keys";
	init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "nxez_keys");
	dev.read = keys_read;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
