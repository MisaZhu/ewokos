#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/keydef.h>
#include <arch/bcm283x/gpio.h>

static uint32_t KEY_X_PIN = 13;
static uint32_t KEY_B_PIN = 22;
static uint32_t KEY_Y_PIN = 16;
static uint32_t KEY_A_PIN = 21;
static uint32_t KEY_START_PIN = 27;
static uint32_t KEY_L1_PIN = 24;
static uint32_t KEY_R1_PIN = 15;


static uint32_t KEY_UP_PIN = 6;
static uint32_t KEY_DOWN_PIN = 7;
static uint32_t KEY_LEFT_PIN = 17;
static uint32_t KEY_RIGHT_PIN = 14;
static uint32_t KEY_SELECT_PIN = 20;

static int joykeyb_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;

	char* rd = (char*)buf;
	*rd = 0;
	int i;

	for(i = 0; i < size; i++){
		if(bcm283x_gpio_read(KEY_X_PIN) == 0)
			*rd = KEY_BUTTON_X;
		else if(bcm283x_gpio_read(KEY_B_PIN) == 0)
			*rd = KEY_BUTTON_B;
		else if(bcm283x_gpio_read(KEY_A_PIN) == 0)
			*rd = KEY_BUTTON_A;
		else if(bcm283x_gpio_read(KEY_Y_PIN) == 0)
			*rd = KEY_BUTTON_Y;
		else if(bcm283x_gpio_read(KEY_START_PIN) == 0)
			*rd = KEY_BUTTON_START;
		else if(bcm283x_gpio_read(KEY_L1_PIN) == 0)
			*rd = KEY_BUTTON_L1;
		else if(bcm283x_gpio_read(KEY_R1_PIN) == 0)
			*rd = KEY_BUTTON_R1;
		else if(bcm283x_gpio_read(KEY_UP_PIN) == 0)
			*rd = KEY_UP;
		else if(bcm283x_gpio_read(KEY_DOWN_PIN) == 0)
			*rd = KEY_DOWN;
		else if(bcm283x_gpio_read(KEY_LEFT_PIN) == 0)
			*rd = KEY_LEFT;
		else if(bcm283x_gpio_read(KEY_RIGHT_PIN) == 0)
			*rd = KEY_RIGHT;
		else if(bcm283x_gpio_read(KEY_SELECT_PIN) == 0)
			*rd = KEY_BUTTON_SELECT;
		else
			break;
		rd++;
	}
	return (i>0)?i:ERR_RETRY_NON_BLOCK;
}

static void init_gpio(void) {
	bcm283x_gpio_config(KEY_X_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_X_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_B_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_B_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_Y_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_Y_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_A_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_A_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_START_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_START_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_L1_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_L1_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_R1_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_R1_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_UP_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_UP_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_DOWN_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_DOWN_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_LEFT_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_LEFT_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_RIGHT_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_RIGHT_PIN, GPIO_PULL_UP); //pull up

	bcm283x_gpio_config(KEY_SELECT_PIN, GPIO_INPUT);//input	
	bcm283x_gpio_pull(KEY_SELECT_PIN, GPIO_PULL_UP); //pull up
}

int main(int argc, char** argv) {
	bcm283x_gpio_init();
	KEY_X_PIN = 12;
	KEY_B_PIN = 21;
	KEY_Y_PIN = 15;
	KEY_A_PIN = 20;
	KEY_START_PIN = 26;
	KEY_L1_PIN = 23;
	KEY_R1_PIN = 14;

	KEY_UP_PIN = 5;
	KEY_DOWN_PIN = 6;
	KEY_LEFT_PIN = 16;
	KEY_RIGHT_PIN = 13;
	KEY_SELECT_PIN = 19;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";
	init_gpio();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "joykeyb");
	dev.read = joykeyb_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
