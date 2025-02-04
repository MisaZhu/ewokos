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
#include <ewoksys/vfs.h>

#define SARADC_CTRL_CHN_MASK        (0x7)
#define SARADC_CTRL_POWER_CTRL      (0x1<<3)
#define SARADC_CTRL_IRQ_ENABLE      (0x1<<5)
#define SARADC_CTRL_IRQ_STATUS      (0x1<<6)

struct rockchip_saradc_regs {
    volatile unsigned int data;
    volatile unsigned int stas;
    volatile unsigned int ctrl;
    volatile unsigned int dly_pu_soc;
};

struct rockchip_saradc_regs* saradc;

#define GPIO_PIN(bank, pin)		((bank << 5) | (pin & 0x1F))

#define GPIO_HIGH				1
#define GPIO_LOW				0

#define  KEY_POWER_PIN			GPIO_PIN(3,17)
#define  KEY_UP_PIN				GPIO_PIN(2,22)
#define  KEY_DOWN_PIN			GPIO_PIN(2,23)
#define  KEY_LEFT_PIN			GPIO_PIN(2,24)
#define  KEY_RIGHT_PIN			GPIO_PIN(2,25)
#define  KEY_ENTER_PIN			GPIO_PIN(0,11)
#define  KEY_BACK_PIN			GPIO_PIN(0,9)
#define  KEY_HOME_PIN			GPIO_PIN(3,21)
#define  KEY_BUTTON_A_PIN		GPIO_PIN(0,11)
#define  KEY_BUTTON_B_PIN		GPIO_PIN(0,9)
#define  KEY_BUTTON_X_PIN		GPIO_PIN(0,13)
#define  KEY_BUTTON_Y_PIN		GPIO_PIN(0,12)
#define  KEY_BUTTON_SELECT_PIN	GPIO_PIN(3,22)
#define  KEY_BUTTON_START_PIN	GPIO_PIN(3,27)
#define  KEY_BUTTON_L1_PIN		GPIO_PIN(0,8)
#define  KEY_BUTTON_R1_PIN		GPIO_PIN(1,9)

#define DECLARE_GPIO_KEY(name, level)	{#name, name, name##_PIN, level, !level}

static struct gpio_pins{
	char* name;
	int key;
	int pin;
	int active;
	int status;
}_pins[] = {
	DECLARE_GPIO_KEY(KEY_POWER, GPIO_LOW),
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
	// DECLARE_GPIO_KEY(KEY_BUTTON_L2, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_BUTTON_R1, GPIO_LOW),
	// DECLARE_GPIO_KEY(KEY_BUTTON_R2, GPIO_LOW),
	//DECLARE_GPIO_KEY(KEY_ENTER, GPIO_LOW),
	DECLARE_GPIO_KEY(KEY_HOME, GPIO_LOW),
};

struct adc_pins{
	char* name;
	int key;
	int ch;
	int min;
	int max;
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

static struct rockchip_gpio_regs *gpio[4];


static uint32_t rockchip_saradc_get_value(int chn){
	uint32_t data;

	saradc->dly_pu_soc = 8;
	 
	     /* Select the channel to be used and trigger conversion */
    saradc->ctrl = (SARADC_CTRL_POWER_CTRL | (chn & SARADC_CTRL_CHN_MASK) |
           SARADC_CTRL_IRQ_ENABLE);

	while((saradc->ctrl & SARADC_CTRL_IRQ_STATUS) != SARADC_CTRL_IRQ_STATUS);
    
	  /* Read value */
    data = saradc->data;
    data &= ((0x1 << 10) - 1);

	saradc->ctrl = 0;

	 return data;
}

static int rockchip_gpio_get(int pin_num){
	int bank = pin_num >> 5;
	uint32_t  pin_mask = 0x1 << (pin_num & 0x1f);

	if(bank > 3)
		return -1;	
	return !!(gpio[bank]->ext_port & pin_mask);
}

static void rockchip_gpio_set(int pin_num, int value){
	int bank = pin_num >> 5;
	uint32_t  pin_mask = 0x1 << (pin_num & 0x1f);
	if(bank > 3)
		return;	
	gpio[bank]->swport_ddr |= pin_mask;
	if(value)
		gpio[bank]->swport_dr |= pin_mask;
	else
		gpio[bank]->swport_dr &= ~pin_mask;
}

static int rockchip_gpio_get_value(struct gpio_pins* pins)
{
	int pin_num = pins->pin;
	int value = rockchip_gpio_get(pin_num);	
    return (value == pins->active) ? 1 : 0;
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

	for(uint32_t i = 0; i < sizeof(_pins)/sizeof(struct gpio_pins);  i++){
		if(rockchip_gpio_get_value(&_pins[i])){
			*keys = _pins[i].key;
			keys++;
			key_cnt++;
			if(key_cnt >= size)
				break;
		}
	}
	//return key_cnt > 0 ? key_cnt : VFS_ERR_RETRY;
	return key_cnt > 0 ? key_cnt : -1;
}

static void init_gpio(void) {
	gpio[0] = (struct rockchip_gpio_regs *)(_mmio_base + 0x7c000);
	gpio[1] = (struct rockchip_gpio_regs *)(_mmio_base + 0x80000);
	gpio[2] = (struct rockchip_gpio_regs *)(_mmio_base + 0x84000);
	gpio[3] = (struct rockchip_gpio_regs *)(_mmio_base + 0x88000);
	saradc = (struct rockchip_saradc_regs *)(_mmio_base +0x6c000);
}

static void check_ux(void) {
	if(rockchip_gpio_get(KEY_BUTTON_L1_PIN) == 0){
		core_prev_ux();
	}
	
	if(rockchip_gpio_get(KEY_BUTTON_R1_PIN) == 0){
		core_next_ux();
	}
}

static int power_button(void* p) {
	(void)p;
	static int count = 0;
	ipc_disable();
	check_ux();
	if(rockchip_gpio_get(113) == 0)
		count++;
	else
		count = 0;

	if(count >= 10){
		//close screnn
		rockchip_gpio_set(44, 1);
		printf("power down!\n");
		proc_usleep(1000);
		rockchip_gpio_set(122, 0);
	}
	ipc_enable();
	proc_usleep(200000);
}


int main(int argc, char** argv) {
	 _mmio_base = mmio_map_offset(0x10000000, 8*1024*1024);

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
