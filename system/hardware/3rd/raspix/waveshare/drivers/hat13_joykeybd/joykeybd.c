#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/keydef.h>
#include <arch/bcm283x/gpio.h>

#define KEY_UP_PIN      6
#define KEY_DOWN_PIN    19
#define KEY_LEFT_PIN    5
#define KEY_RIGHT_PIN   26
#define KEY_PRESS_PIN   13
#define KEY1_PIN        21
#define KEY2_PIN        20
#define KEY3_PIN        16

static bool _j_x_rev = false;
static bool _j_y_rev = false;

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

	if(bcm283x_gpio_read(KEY_UP_PIN) == 0) {
		data[cnt] = _j_y_rev ? KEY_DOWN:KEY_UP;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY_DOWN_PIN) == 0) {
		data[cnt] = _j_y_rev ? KEY_UP:KEY_DOWN;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY_LEFT_PIN) == 0) {
		data[cnt] = _j_x_rev ? KEY_RIGHT:KEY_LEFT;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY_RIGHT_PIN) == 0) {
		data[cnt] = _j_x_rev ? KEY_LEFT:KEY_RIGHT;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY_PRESS_PIN) == 0) {
		data[cnt] = KEY_BUTTON_A;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY1_PIN) == 0) {
		data[cnt] = KEY_BUTTON_START;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY2_PIN) == 0) {
		data[cnt] = KEY_BUTTON_SELECT;
		if((cnt++) >= size)
			return cnt;
	}

	if(bcm283x_gpio_read(KEY3_PIN) == 0) {
		data[cnt] = KEY_HOME;
		cnt++;
	}	

	return cnt == 0 ? -1 : cnt;
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

	bcm283x_gpio_config(KEY1_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY1_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY2_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY2_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY3_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY3_PIN, GPIO_PULL_UP); //pull up
}

int main(int argc, char** argv) {
	bcm283x_gpio_init();
	init_gpio();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/joykeyb";

	_j_x_rev = false;
	_j_y_rev = false;
	if(argc > 2 && strstr(argv[2], "rev") != NULL) {
		if(strchr(argv[2], 'x') != NULL)
			_j_x_rev = true;
		if(strchr(argv[2], 'y') != NULL)
			_j_y_rev = true;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joystick_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
