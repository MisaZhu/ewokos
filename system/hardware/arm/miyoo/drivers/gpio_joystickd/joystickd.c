#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/keydef.h>
#include <ewoksys/ipc.h>
#include <ewoksys/mmio.h>
#include "gpio_table.h"

#define GPIO_HIGH				1
#define GPIO_LOW				0

#define  JOYSTICK_UP_PIN				1
#define  JOYSTICK_DOWN_PIN			69
#define  JOYSTICK_LEFT_PIN			70
#define  JOYSTICK_RIGHT_PIN			5
#define  JOYSTICK_PRESS_PIN		7
#define  KEY_HOME_PIN			12
#define  KEY_BUTTON_B_PIN		6
#define  KEY_BUTTON_X_PIN		9
#define  KEY_BUTTON_Y_PIN		8
#define  KEY_BUTTON_SELECT_PIN	11
#define  KEY_BUTTON_START_PIN	10
#define  KEY_BUTTON_L1_PIN		14
#define  KEY_BUTTON_L2_PIN		13
#define  KEY_BUTTON_R1_PIN		47
#define  KEY_BUTTON_R2_PIN		90
#define  KEY_POWER_PIN			86

#define DECLARE_GPIO_KEY(name, level)	{name, name##_PIN, level, !level}

struct gpio_pins{
	int key;
	int pin;
	int active;
	int status;
}_pins[] = {
	DECLARE_GPIO_KEY(JOYSTICK_UP, GPIO_LOW),
	DECLARE_GPIO_KEY(JOYSTICK_DOWN, GPIO_LOW),
	DECLARE_GPIO_KEY(JOYSTICK_LEFT, GPIO_LOW),
	DECLARE_GPIO_KEY(JOYSTICK_RIGHT, GPIO_LOW),
	DECLARE_GPIO_KEY(JOYSTICK_PRESS, GPIO_LOW)
	/*DECLARE_GPIO_KEY(KEY_BUTTON_B, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_X, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_Y, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_SELECT, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_START, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_L1, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_L2, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R1, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R2, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_POWER, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_HOME, GPIO_LOW),
	*/
};

#define GPIO_NUMBER                         91
#define REG8(addr)                 (*(volatile uint8_t*)(_mmio_base + (((addr) & ~1)<<1) + (addr & 1)))

void miyoo_gpio_set(int pin, int level){
    if (pin >= 0 && pin < GPIO_NUMBER)
    {
		REG8(gpio_table[pin].r_oen) &= (~gpio_table[pin].m_oen);
		if(level){
			REG8(gpio_table[pin].r_out) |= gpio_table[pin].m_out;
		}else{
			REG8(gpio_table[pin].r_out) &= ~gpio_table[pin].m_out;
		}
    }
}

void miyoo_gpio_pull(int  pin, int level)
{
    if (pin >= 0 && pin < GPIO_NUMBER)
    {
		if(level)
        	REG8(gpio_table[pin].r_out) |= gpio_table[pin].m_out;
		else
        	REG8(gpio_table[pin].r_out) &= ~gpio_table[pin].m_out;
    }
}

void miyoo_gpio_init(int pin)
{
    if (pin >= 0 && pin < GPIO_NUMBER)
    {
        REG8(gpio_table[pin].r_oen) |= (gpio_table[pin].m_oen);
    }
}

int miyoo_gpio_read(int pin)
{
    if (pin >= 0 && pin < GPIO_NUMBER)
    {
        return ((REG8(gpio_table[pin].r_in)&gpio_table[pin].m_in)? 1 : 0);
    }
    else
    {
        return 0;
    }
}


static int joystick_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)size;
	(void)p;

	char* keys = (char*)buf;
	int key_cnt = 0;

	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		if(miyoo_gpio_read(_pins[i].pin) == _pins[i].active){
			*keys = _pins[i].key;
			keys++;
			key_cnt++;
			if(key_cnt >= size)
				break;
		}
	}
	return key_cnt > 0 ? key_cnt : ERR_RETRY_NON_BLOCK;
}

static void init_gpio(void) {
	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		miyoo_gpio_init(_pins[i].pin);
		miyoo_gpio_pull(_pins[i].pin, !_pins[i].active);
	}
}

int main(int argc, char** argv) {
	 _mmio_base = mmio_map();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joystick";
	init_gpio();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joystick");
	dev.read = joystick_read;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
