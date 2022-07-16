#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/keydef.h>
#include <sys/ipc.h>
#include <sys/mmio.h>

#define GPIO_PIN(bank, pin)		((bank << 5) | (pin & 0x1F))

#define GPIO_HIGH				1
#define GPIO_LOW				0

#define  KEY_UP_PIN				GPIO_PIN(1,12)
#define  KEY_DOWN_PIN			GPIO_PIN(1,13)
#define  KEY_LEFT_PIN			GPIO_PIN(1,14)
#define  KEY_RIGHT_PIN			GPIO_PIN(1,15)
#define  KEY_ENTER_PIN			GPIO_PIN(1,2)
#define  KEY_BACK_PIN			GPIO_PIN(1,5)
#define  KEY_HOME_PIN			GPIO_PIN(2,4)
#define  KEY_BUTTON_A_PIN		GPIO_PIN(1,2)
#define  KEY_BUTTON_B_PIN		GPIO_PIN(1,5)
#define  KEY_BUTTON_X_PIN		GPIO_PIN(1,7)
#define  KEY_BUTTON_Y_PIN		GPIO_PIN(1,6)
#define  KEY_BUTTON_SELECT_PIN	GPIO_PIN(3,9)
#define  KEY_BUTTON_START_PIN	GPIO_PIN(3,12)
#define  KEY_BUTTON_L1_PIN		GPIO_PIN(2,6)
#define  KEY_BUTTON_L2_PIN		GPIO_PIN(3,10)
#define  KEY_BUTTON_R1_PIN		GPIO_PIN(2,7)
#define  KEY_BUTTON_R2_PIN		GPIO_PIN(3,15)

#define DECLARE_GPIO_KEY(name, level)	{#name, name, name##_PIN, level, !level}

struct gpio_pins{
	char* name;
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
	DECLARE_GPIO_KEY(KEY_BUTTON_L2, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R1, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R2, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_ENTER, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_HOME, GPIO_LOW),
};

#define GPIO_NUMBER                         91
#define REG8(addr)                 (*(volatile uint8_t*)(_mmio_base + (((addr) & ~1)<<1) + (addr & 1)))

struct rockchip_gpio_regs {
    uint32_t swport_dr;
    uint32_t swport_ddr;
    uint32_t reserved0[(0x30 - 0x08) / 4];
    uint32_t inten;
    uint32_t intmask;
    uint32_t inttype_level;
    uint32_t int_polarity;
    uint32_t int_status;
    uint32_t int_rawstatus;
    uint32_t debounce;
    uint32_t porta_eoi;
    uint32_t ext_port;
    uint32_t reserved1[(0x60 - 0x54) / 4];
    uint32_t ls_sync;
};

struct rockchip_gpio_regs *gpio[4];



static int rockchip_gpio_get_value(struct gpio_pins* pin)
{
	int bank = pin->pin>>5;
	if(bank > 3)
		return 0;
	uint32_t  pin_mask = 0x1 << (pin->pin & 0x1f);
    return (!!(gpio[bank]->ext_port & pin_mask) == pin->active) ? 1 : 0;
}

static int joystick_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)size;
	(void)p;

	char* keys = (char*)buf;
	int key_cnt = 0;

	for(uint32_t i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		if(rockchip_gpio_get_value(&_pins[i])){
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
	gpio[0] = (struct rockchip_gpio_regs *)(_mmio_base + 0x040000);
	gpio[1] = (struct rockchip_gpio_regs *)(_mmio_base + 0x250000);
	gpio[2] = (struct rockchip_gpio_regs *)(_mmio_base + 0x260000);
	gpio[3] = (struct rockchip_gpio_regs *)(_mmio_base + 0x270000);
}

static int power_button(void* p) {
	(void)p;
	usleep(1000000);
	return 0;
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
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
