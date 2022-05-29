#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/keydef.h>
#include <arch/bcm283x/gpio.h>
#include "gpio_table.h"

static uint32_t KEY_UP_PIN;
static uint32_t KEY_DOWN_PIN;
static uint32_t KEY_LEFT_PIN;
static uint32_t KEY_RIGHT_PIN;
static uint32_t KEY_PRESS_PIN;
static uint32_t KEY_BACK_PIN;

static bool _j_x_rev = false;
static bool _j_y_rev = false;

#define GPIO_NUMBER                         80
#define READ_REG8(addr)                 (*(volatile uint8_t*)(_mmio_base + (((addr) & ~1)<<1) + (addr & 1)))

int miyoo_gpio_read(int pin)
{
    if (pin >= 0 && pin < GPIO_NUMBER)
    {
        return ((READ_REG8(gpio_table[pin].r_in)&gpio_table[pin].m_in)? 1 : 0);
    }
    else
    {
        return 0;
    }
}


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

	if(miyoo_gpio_read(KEY_UP_PIN) == 0)
		*rd = _j_y_rev ? KEY_DOWN:KEY_UP;
	else if(miyoo_gpio_read(KEY_DOWN_PIN) == 0)
		*rd = _j_y_rev ? KEY_UP:KEY_DOWN;
	else if(miyoo_gpio_read(KEY_RIGHT_PIN) == 0)
		*rd = _j_x_rev ? KEY_LEFT:KEY_RIGHT;
	else if(miyoo_gpio_read(KEY_LEFT_PIN) == 0)
		*rd = _j_x_rev ? KEY_RIGHT:KEY_LEFT;
	else if(miyoo_gpio_read(KEY_PRESS_PIN) == 0)
		*rd = KEY_ENTER;
	else if(miyoo_gpio_read(KEY_BACK_PIN) == 0)
		*rd = KEY_BACKSPACE;
	else{
		return ERR_RETRY_NON_BLOCK;
	}
	return 1;
}

static void init_gpio(void) {

}

int main(int argc, char** argv) {
	 _mmio_base = mmio_map(false);
	KEY_UP_PIN = 1;
	KEY_DOWN_PIN = 69;
	KEY_LEFT_PIN = 70;
	KEY_RIGHT_PIN = 5;
	KEY_PRESS_PIN = 7;
	KEY_BACK_PIN = 6;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joykeyb";
	if(argc >= 7) {
		KEY_UP_PIN = atoi(argv[2]);
		KEY_DOWN_PIN = atoi(argv[3]);
		KEY_LEFT_PIN = atoi(argv[4]);
		KEY_RIGHT_PIN = atoi(argv[5]);
		KEY_PRESS_PIN = atoi(argv[6]);
	}
	init_gpio();

	_j_x_rev = false;
	_j_y_rev = false;
	if(argc > 7 && strstr(argv[7], "rev") != NULL) {
    if(strchr(argv[7], 'x') != NULL)
      _j_x_rev = true;
    if(strchr(argv[7], 'y') != NULL)
      _j_y_rev = true;
  }

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
