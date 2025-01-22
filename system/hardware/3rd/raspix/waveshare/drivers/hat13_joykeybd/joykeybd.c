#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/keydef.h>
#include <arch/bcm283x/gpio.h>

typedef struct st_gpio_key {
	uint32_t pin;
	uint32_t key;
} gpio_key_t;

static gpio_key_t _gpio_keys[] = {
	{6,  KEY_UP},
	{19, KEY_DOWN},
	{5,  KEY_LEFT},
	{26, KEY_RIGHT},
	{13, KEY_BUTTON_A},
	{21, KEY_BUTTON_START},
	{20, KEY_BUTTON_SELECT},
	{16, KEY_HOME},
	{0,  0}
};

static int joystick_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)size;
	(void)p;

	char* data = (char*)buf;
	int cnt = 0;
	int i = 0;
	while(cnt < size) {
		if(_gpio_keys[i].pin == 0)
			break;

		if(bcm283x_gpio_read(_gpio_keys[i].pin) == 0) {
			data[cnt] = _gpio_keys[i].key;
			cnt++;
		}
		i++;
	}

	return cnt == 0 ? -1 : cnt;
}

static void init_gpio(void) {
	int i = 0;
	while(true) {
		if(_gpio_keys[i].key == 0)
			break;

		bcm283x_gpio_config(_gpio_keys[i].pin, GPIO_INPUT);//input	
		bcm283x_gpio_pull(_gpio_keys[i].pin, GPIO_PULL_UP); //pull up
		i++;
	}
}

int main(int argc, char** argv) {
	bcm283x_gpio_init();
	init_gpio();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joykeyb";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
