#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <ewoksys/syscall.h>
#include <arch/bcm283x/gpio.h>

/*default gpio for waveshare GamePi15 Hat*/
static uint32_t KEY_UP_PIN = 5;
static uint32_t KEY_DOWN_PIN = 6;
static uint32_t KEY_LEFT_PIN = 16;
static uint32_t KEY_RIGHT_PIN = 13;
static uint32_t KEY_PRESS_PIN = 19;

static int joystick_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)size;
	(void)p;

	char* rd = (char*)buf;
	*rd = 0;

	if(bcm283x_gpio_read(KEY_UP_PIN) == 0)
		*rd |= JOYSTICK_UP;
	if(bcm283x_gpio_read(KEY_DOWN_PIN) == 0)
		*rd |= JOYSTICK_DOWN;
	if(bcm283x_gpio_read(KEY_LEFT_PIN) == 0)
		*rd |= JOYSTICK_LEFT;
	if(bcm283x_gpio_read(KEY_RIGHT_PIN) == 0)
		*rd |= JOYSTICK_RIGHT;
	if(bcm283x_gpio_read(KEY_PRESS_PIN) == 0)
		*rd |= JOYSTICK_PRESS;
	return 1;
}

static void init_gpio(void) {
	bcm283x_gpio_config(KEY_UP_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_UP_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_DOWN_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_DOWN_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_LEFT_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_LEFT_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_RIGHT_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_RIGHT_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_PRESS_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_PRESS_PIN, GPIO_PULL_UP); //pull up
}

int main(int argc, char** argv) {
	bcm283x_gpio_init();
	KEY_UP_PIN = 5;
	KEY_DOWN_PIN = 6;
	KEY_LEFT_PIN = 16;
	KEY_RIGHT_PIN = 13;
	KEY_PRESS_PIN = 19;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick";
	if(argc >= 7) {
		KEY_UP_PIN = atoi(argv[2]);
		KEY_DOWN_PIN = atoi(argv[3]);
		KEY_LEFT_PIN = atoi(argv[4]);
		KEY_RIGHT_PIN = atoi(argv[5]);
		KEY_PRESS_PIN = atoi(argv[6]);
	}
	init_gpio();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0664);
	return 0;
}
