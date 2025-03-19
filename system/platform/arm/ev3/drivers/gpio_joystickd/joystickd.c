#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ewoksys/core.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/keydef.h>
#include <ewoksys/ipc.h>
#include <ewoksys/mmio.h>

#include <bsp/bsp_gpio.h>

#define GPIO_HIGH				1
#define GPIO_LOW				0

#define  KEY_UP_PIN				127
#define  KEY_DOWN_PIN			126
#define  KEY_LEFT_PIN			102
#define  KEY_RIGHT_PIN			124
#define  KEY_HOME_PIN			106
#define  KEY_ENTER_PIN			29

#define  KEY_POWER_PIN			29
#define	 POWER_OFF_PIN			107

#define DECLARE_GPIO_KEY(name, level)	{name, name##_PIN, level, !level}

struct gpio_pins{
	int key;
	int pin;
	int active;
	int status;
}_pins[] = {
	DECLARE_GPIO_KEY(KEY_UP, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_DOWN, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_LEFT, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_RIGHT, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_ENTER, GPIO_HIGH),
	DECLARE_GPIO_KEY(KEY_HOME, GPIO_HIGH),
};

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
	memset(keys, 0, size);
	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		if(bsp_gpio_read(_pins[i].pin) == _pins[i].active){
			*keys = _pins[i].key;
			keys++;
			key_cnt++;
			if(key_cnt >= size)
				break;
		}
	}
	if(key_cnt <= 0)
		return 0;
	return key_cnt;
}

static void init_gpio(void) {
	bsp_gpio_init();
	
	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		bsp_gpio_pull(_pins[i].pin, GPIO_PULL_NONE);
		bsp_gpio_config(_pins[i].pin, GPIO_INPUT);
	}
}

static void check_power(void) {
	static int count = 0;
	if(bsp_gpio_read(KEY_POWER_PIN) != 0)
		count++;
	else
		count = 0;

	if(count >= 10){
		bsp_gpio_write(POWER_OFF_PIN, 0);
	}
}

static int power_button(void* p) {
	(void)p;
	ipc_disable();
	check_power();
	ipc_enable();
	proc_usleep(200000);
}

int main(int argc, char** argv) {
	 _mmio_base = mmio_map();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joykeyb";
	init_gpio();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joystick_read;
	dev.loop_step = power_button;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
