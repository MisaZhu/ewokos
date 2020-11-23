#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <arch/bcm283x/gpio.h>

#define KEY_UP_PIN      5
#define KEY_DOWN_PIN    6
#define KEY_LEFT_PIN    16
#define KEY_RIGHT_PIN   13
#define KEY_PRESS_PIN   19

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10

static int joystick_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;


	char* rd = (char*)buf;
	*rd = 0;

	if(bcm283x_gpio_read(KEY_UP_PIN) == 0)
		*rd |= KEY_V_UP;
	if(bcm283x_gpio_read(KEY_DOWN_PIN) == 0)
		*rd |= KEY_V_DOWN;
	if(bcm283x_gpio_read(KEY_LEFT_PIN) == 0)
		*rd |= KEY_V_LEFT;
	if(bcm283x_gpio_read(KEY_RIGHT_PIN) == 0)
		*rd |= KEY_V_RIGHT;
	if(bcm283x_gpio_read(KEY_PRESS_PIN) == 0)
		*rd |= KEY_V_PRESS;
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
	init_gpio();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
