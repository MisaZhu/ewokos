#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <dev/device.h>
#include "../lib/gpio_arch.h"

#define KEY_UP_PIN      6
#define KEY_DOWN_PIN    19
#define KEY_LEFT_PIN    5
#define KEY_RIGHT_PIN   26
#define KEY_PRESS_PIN   13
#define KEY1_PIN        21
#define KEY2_PIN        20
#define KEY3_PIN        16

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10
#define KEY_V_1         0x20
#define KEY_V_2         0x40
#define KEY_V_3         0x80

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
/*
	if(gpio_arch_read(KEY_UP_PIN) == 0)
		*rd |= KEY_V_UP;
	if(gpio_arch_read(KEY_DOWN_PIN) == 0)
		*rd |= KEY_V_DOWN;
	if(gpio_arch_read(KEY_LEFT_PIN) == 0)
		*rd |= KEY_V_LEFT;
	if(gpio_arch_read(KEY_RIGHT_PIN) == 0)
		*rd |= KEY_V_RIGHT;
	if(gpio_arch_read(KEY_PRESS_PIN) == 0)
		*rd |= KEY_V_PRESS;
	if(gpio_arch_read(KEY1_PIN) == 0)
		*rd |= KEY_V_1;
	if(gpio_arch_read(KEY2_PIN) == 0)
		*rd |= KEY_V_2;
	if(gpio_arch_read(KEY3_PIN) == 0)
		*rd |= KEY_V_3;
*/
	return 1;
}

static void init_gpio(void) {
	gpio_arch_config(KEY_UP_PIN, 0);//input	
	gpio_arch_pull(KEY_UP_PIN, 2); //pull up

	gpio_arch_config(KEY_DOWN_PIN, 0);//input	
	gpio_arch_pull(KEY_DOWN_PIN, 2); //pull up

	gpio_arch_config(KEY_LEFT_PIN, 0);//input	
	gpio_arch_pull(KEY_LEFT_PIN, 2); //pull up

	gpio_arch_config(KEY_RIGHT_PIN, 0);//input	
	gpio_arch_pull(KEY_RIGHT_PIN, 2); //pull up

	gpio_arch_config(KEY_PRESS_PIN, 0);//input	
	gpio_arch_pull(KEY_PRESS_PIN, 2); //pull up

	gpio_arch_config(KEY1_PIN, 0);//input	
	gpio_arch_pull(KEY1_PIN, 2); //pull up

	gpio_arch_config(KEY2_PIN, 0);//input	
	gpio_arch_pull(KEY2_PIN, 2); //pull up

	gpio_arch_config(KEY3_PIN, 0);//input	
	gpio_arch_pull(KEY3_PIN, 2); //pull up
}

int main(int argc, char** argv) {
	gpio_arch_init();
	init_gpio();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
