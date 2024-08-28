#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/keydef.h>
#include <arch/bcm283x/gpio.h>

/*default gpio for waveshare GamePi15 Hat*/
#define KEY_BUTTON_X_PIN  		12
#define KEY_BUTTON_B_PIN  		21
#define KEY_BUTTON_Y_PIN  		15
#define KEY_BUTTON_A_PIN  		20
#define KEY_BUTTON_START_PIN  	26
#define KEY_BUTTON_L1_PIN  		23
#define KEY_BUTTON_R1_PIN  		14
#define KEY_UP_PIN  			5
#define KEY_DOWN_PIN  			6
#define KEY_LEFT_PIN  			16
#define KEY_RIGHT_PIN 			13
#define KEY_BUTTON_SELECT_PIN  	19

#define GPIO_HIGH				1
#define GPIO_LOW				0

#define DECLARE_GPIO_KEY(name, level)	{name, name##_PIN, level, !level}

struct gpio_pins{
	int key;
	int pin;
	int active;
	int status;
}_pins[] = {
	DECLARE_GPIO_KEY(KEY_UP, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_DOWN, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_LEFT, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_RIGHT, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_A, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_B, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_X, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_Y, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_SELECT, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_START, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_L1, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R1, GPIO_LOW),
};

static int joykeyb_read(int fd, int from_pid, fsinfo_t* node,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)size;
	(void)p;

	char* rd = (char*)buf;
	*rd = 0;
	int cnt = 0;

	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		if(bcm283x_gpio_read(_pins[i].pin) == _pins[i].active){
			*rd = _pins[i].key;
			rd++;
			cnt++;
			if(cnt >= size)
				break;
		}
	}
	return (cnt>0)?cnt:VFS_ERR_RETRY;
}

static void init_gpio(void) {
	for(int i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		bcm283x_gpio_config(_pins[i].pin, GPIO_INPUT); 
		if(_pins[i].active == GPIO_LOW)
			bcm283x_gpio_pull(_pins[i].pin, GPIO_PULL_UP);
		else
			bcm283x_gpio_pull(_pins[i].pin, GPIO_PULL_DOWN);
	}
}

int main(int argc, char** argv) {
	bcm283x_gpio_init();
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";
	init_gpio();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joykeyb_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
