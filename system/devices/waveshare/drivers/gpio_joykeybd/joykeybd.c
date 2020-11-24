#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/keydef.h>
#include <arch/bcm283x/gpio.h>

static uint32_t KEY_UP_PIN = 12;
static uint32_t KEY_DOWN_PIN = 21;
static uint32_t KEY_LEFT_PIN = 15;
static uint32_t KEY_RIGHT_PIN = 20;
static uint32_t KEY_PRESS_PIN = 26;

static bool _j_x_rev = false;
static bool _j_y_rev = false;

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
		*rd = _j_y_rev ? KEY_DOWN:KEY_UP;
	else if(bcm283x_gpio_read(KEY_DOWN_PIN) == 0)
		*rd = _j_y_rev ? KEY_UP:KEY_DOWN;
	else if(bcm283x_gpio_read(KEY_RIGHT_PIN) == 0)
		*rd = _j_x_rev ? KEY_LEFT:KEY_RIGHT;
	else if(bcm283x_gpio_read(KEY_PRESS_PIN) == 0)
		*rd = KEY_ENTER;
	else if(bcm283x_gpio_read(KEY_LEFT_PIN) == 0)
		*rd = _j_x_rev ? KEY_RIGHT:KEY_LEFT;
	else
		return ERR_RETRY;
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
	KEY_UP_PIN = 12;
	KEY_DOWN_PIN = 21;
	KEY_LEFT_PIN = 15;
	KEY_RIGHT_PIN = 20;
	KEY_PRESS_PIN = 26;

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
