#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/mmio.h>
#include <ewoksys/syscall.h>

#include "../include/arch/ev3/gpio.h"

#define GPIO_INPUT  0x00
#define GPIO_OUTPUT 0x01
#define GPIO_ALTF5  0x02
#define GPIO_ALTF4  0x03
#define GPIO_ALTF0  0x04
#define GPIO_ALTF1  0x05
#define GPIO_ALTF2  0x06
#define GPIO_ALTF3  0x07

#define GPIO_PULL_NONE 0x00
#define GPIO_PULL_DOWN 0x01
#define GPIO_PULL_UP   0x02

#define writel(val, reg)    (*(volatile uint32_t*)(reg) = (val))
#define readl(reg)          (*(volatile uint32_t*)(reg))

#define GPIO_MASK(x)	(0x1 << ((x)%32))

#define MAX_GPIO	(144)

struct pinmux{
	uint8_t idx;
	uint8_t shift;
	uint8_t mode;
};

static uint8_t pullgp[] = {
	//BANK 0
	  3,  2,  2,  2,  2,  2,  2,  1,   0,  0,  0,  0,  0,  0,  0,  0,
	//BANK 1
	 13, 13, 12, 12, 11, 11, 10, 10,   7,  5,  5,  5,  5,  5,  5,  4,
	//BANK 2
	 16, 16, 16, 16, 16, 16, 16, 16,  16, 16, 15, 15, 15, 15, 14, 14,
	//BANK 3
	 17, 17, 17, 17, 17, 17, 17, 17,  16, 16, 16, 16, 16, 16, 16, 16,
	//BANK 4
	 18, 18, 18, 18, 18, 18, 18,255,  17, 17, 17, 17, 17, 17, 17, 17,
	//BANK 5
	 20, 20, 20, 20, 20, 20, 20, 20,  19, 19, 19, 19, 19, 19, 19, 19,
	//BANK 6
	 31, 30, 30, 30, 30, 27, 25, 25,  24, 24, 24, 24, 23, 23, 22, 21,
	//BANK 7
	 29, 29, 29, 29, 29, 29, 29, 29,  28, 28, 28, 28, 28, 28, 28, 28,
	//BANK 8
	255,  9,  9,  8,  8,  7,  7,  6,  31, 31, 31, 31, 30, 30, 30, 30,
};

static const struct pinmux PINMUX[MAX_GPIO] = {
	//BANK 0
	{1, 28, 8}, //GP[0]
	{1, 24, 8}, //GP[1]
	{1, 20, 8}, //GP[2]
	{1, 16, 8}, //GP[3]
	{1, 12, 8}, //GP[4]
	{1,  8, 8}, //GP[5]
	{1,  4, 8}, //GP[6]
	{1,  0, 8}, //GP[7]
	{0, 28, 8}, //GP[8]
	{0, 24, 8}, //GP[9]
	{0, 20, 8}, //GP[10]
	{0, 16, 8}, //GP[11]
	{0, 12, 8}, //GP[12]
	{0,  8, 8}, //GP[13]
	{0,  4, 8}, //GP[14]
	{0,  0, 8}, //GP[15]

	//BANK 1
	{4, 28, 8}, //GP[0]
	{4, 24, 8}, //GP[1]
	{4, 20, 8}, //GP[2]
	{4, 16, 8}, //GP[3]
	{4, 12, 8}, //GP[4]
	{4,  8, 8}, //GP[5]
	{4,  4, 4}, //GP[6]
	{4,  0, 4}, //GP[7]
	{3,  0, 4}, //GP[8]
	{2, 24, 4}, //GP[9]
	{2, 20, 4}, //GP[10]
	{2, 16, 4}, //GP[11]
	{2, 12, 4}, //GP[12]
	{2,  8, 4}, //GP[13]
	{2,  4, 4}, //GP[14]
	{2,  0, 8}, //GP[15]

	//BANK 2
	{6, 28, 8}, //GP[0]
	{6, 24, 8}, //GP[1]
	{6, 20, 8}, //GP[2]
	{6, 16, 8}, //GP[3]
	{6, 12, 8}, //GP[4]
	{6,  8, 8}, //GP[5]
	{6,  4, 8}, //GP[6]
	{6,  0, 8}, //GP[7]
	{5, 28, 8}, //GP[8]
	{5, 24, 8}, //GP[9]
	{5, 20, 8}, //GP[10]
	{5, 16, 8}, //GP[11]
	{5, 12, 8}, //GP[12]
	{5,  8, 8}, //GP[13]
	{5,  4, 8}, //GP[14]
	{5,  0, 8}, //GP[15]

	//BANK 3
	{8, 28, 8}, //GP[0]
	{8, 24, 8}, //GP[1]
	{8, 20, 8}, //GP[2]
	{8, 16, 8}, //GP[3]
	{8, 12, 8}, //GP[4]
	{8,  8, 8}, //GP[5]
	{8,  4, 8}, //GP[6]
	{8,  0, 8}, //GP[7]
	{7, 28, 8}, //GP[8]
	{7, 24, 8}, //GP[9]
	{7, 20, 8}, //GP[10]
	{7, 16, 8}, //GP[11]
	{7, 12, 8}, //GP[12]
	{7,  8, 8}, //GP[13]
	{7,  4, 8}, //GP[14]
	{7,  0, 8}, //GP[15]

	//BANK 4
	{10, 28, 8}, //GP[0]
	{10, 24, 8}, //GP[1]
	{10, 20, 8}, //GP[2]
	{10, 16, 8}, //GP[3]
	{10, 12, 8}, //GP[4]
	{10,  8, 8}, //GP[5]
	{10,  4, 8}, //GP[6]
	{10,  0, 8}, //GP[7]
	{9, 28, 8}, //GP[8]
	{9, 24, 8}, //GP[9]
	{9, 20, 8}, //GP[10]
	{9, 16, 8}, //GP[11]
	{9, 12, 8}, //GP[12]
	{9,  8, 8}, //GP[13]
	{9,  4, 8}, //GP[14]
	{9,  0, 8}, //GP[15]

	//BANK 5
	{12, 28, 8}, //GP[0]
	{12, 24, 8}, //GP[1]
	{12, 20, 8}, //GP[2]
	{12, 16, 8}, //GP[3]
	{12, 12, 8}, //GP[4]
	{12,  8, 8}, //GP[5]
	{12,  4, 8}, //GP[6]
	{12,  0, 8}, //GP[7]
	{11, 28, 8}, //GP[8]
	{11, 24, 8}, //GP[9]
	{11, 20, 8}, //GP[10]
	{11, 16, 8}, //GP[11]
	{11, 12, 8}, //GP[12]
	{11,  8, 8}, //GP[13]
	{11,  4, 8}, //GP[14]
	{11,  0, 8}, //GP[15]

	//BANK 6
	{19, 24, 8}, //GP[0]
	{19, 20, 8}, //GP[1]
	{19, 16, 8}, //GP[2]
	{19, 12, 8}, //GP[3]
	{19,  8, 8}, //GP[4]
	{16,  4, 8}, //GP[5]
	{14,  4, 8}, //GP[6]
	{14,  0, 8}, //GP[7]
	{13, 28, 8}, //GP[8]
	{13, 24, 8}, //GP[9]
	{13, 20, 8}, //GP[10]
	{13, 16, 8}, //GP[11]
	{13, 12, 8}, //GP[12]
	{13,  8, 8}, //GP[13]
	{13,  4, 8}, //GP[14]
	{13,  0, 8}, //GP[15]

	//BANK 7
	{18,  4, 8}, //GP[0]
	{18,  0, 8}, //GP[1]
	{17, 28, 8}, //GP[2]
	{17, 24, 8}, //GP[3]
	{17, 20, 8}, //GP[4]
	{17, 16, 8}, //GP[5]
	{17, 12, 8}, //GP[6]
	{17,  8, 8}, //GP[7]
	{17,  4, 8}, //GP[8]
	{17,  0, 8}, //GP[9]
	{16, 28, 8}, //GP[10]
	{16, 24, 8}, //GP[11]
	{16, 20, 8}, //GP[12]
	{16, 16, 8}, //GP[13]
	{16, 12, 8}, //GP[14]
	{16,  8, 8}, //GP[15]

	//BANK 8
	{19, 28, 8}, //GP[0]
	{ 3, 28, 4}, //GP[1]
	{ 3, 24, 4}, //GP[2]
	{ 3, 20, 4}, //GP[3]
	{ 3, 16, 4}, //GP[4]
	{ 3, 12, 4}, //GP[5]
	{ 3,  8, 4}, //GP[6]
	{ 2, 28, 4}, //GP[7]
	{19,  4, 8}, //GP[8]
	{19,  0, 8}, //GP[9]
	{18, 28, 8}, //GP[10]
	{18, 24, 8}, //GP[11]
	{18, 20, 8}, //GP[12]
	{18, 16, 8}, //GP[13]
	{18, 12, 8}, //GP[14]
	{18,  8, 8}, //GP[15]
};

struct davinci_gpio_regs {
    volatile uint32_t dir;
    volatile uint32_t out_data;
    volatile uint32_t set_data;
    volatile uint32_t clr_data;
    volatile uint32_t in_data;
    volatile uint32_t set_rising;
    volatile uint32_t clr_rising;
    volatile uint32_t set_falling;
    volatile uint32_t clr_falling;
    volatile uint32_t intstat;
};

static struct davinci_gpio_regs *davinci_gpios;
static volatile uint32_t *pinmux_regs;
static volatile uint32_t *pull_enable;
static volatile uint32_t *pull_up_down;


static uint32_t write_syscfg(uint32_t reg, uint32_t val, uint32_t mask){
	return syscall3(SYS_MMIO_RW, reg, val, mask);
}

void gpio_mux_cfg(int pin, int mode){
	if(mode != MODE_GPIO)
		return;

	int idx = PINMUX[pin].idx;
	int shift = PINMUX[pin].shift;
	mode = PINMUX[pin].mode;

	write_syscfg(&pinmux_regs[idx], mode << shift, 0xf << shift);
}

void ev3_gpio_config(int32_t pin, int32_t mode)
{
    volatile uint32_t temp;
    int bank = pin / 32;
    uint32_t mask = GPIO_MASK(pin);
    struct davinci_gpio_regs *g = &davinci_gpios[bank];

	if(pin >= MAX_GPIO)
		return;

	if(mode == GPIO_OUTPUT || mode == GPIO_INPUT){
		gpio_mux_cfg(pin, MODE_GPIO);
    	temp = readl(&g->dir);
    	if (mode == GPIO_OUTPUT) {
    	    temp &= ~mask;
    	} else {
    	    temp |= mask;
    	}
    	writel(temp, &g->dir);
	}
}

void ev3_gpio_write(int32_t pin, int32_t value){
    int bank = pin / 32;
	struct davinci_gpio_regs *g = &davinci_gpios[bank];

	if(pin >= MAX_GPIO)
		return;

    writel(GPIO_MASK(pin),value ? &g->set_data : &g->clr_data);
}

uint8_t ev3_gpio_read(int32_t pin){
    int bank = pin / 32;
	struct davinci_gpio_regs *g = &davinci_gpios[bank];

	if(pin >= MAX_GPIO)
		return 0;

    return !!(GPIO_MASK(pin) & readl(&g->in_data));
}

void ev3_gpio_pull(int32_t pin, int32_t updown){
	if(pin > 144)
		return;

	int gp = pullgp[pin];

	if(gp > 32)
		return;

	uint32_t mask = 0x1 << gp;
	uint32_t reg = *pull_enable;

	if(updown == GPIO_PULL_NONE){
		write_syscfg(pull_enable, 0, 0x1 << gp);
	}else{
		write_syscfg(pull_enable, 0x1 << gp, 0x1 << gp);
		if(updown == GPIO_PULL_UP)
			write_syscfg(pull_up_down, 0x1 << gp, 0x1 << gp);
		else
			write_syscfg(pull_up_down, 0, 0x1 << gp);
	}
}

void ev3_gpio_init(void){
	_mmio_base = mmio_map();
	davinci_gpios = (struct davinci_gpio_regs*)(_mmio_base + 0x01E26010);
	pinmux_regs = (uint32_t*)(_mmio_base + 0x1c14120);
	pull_enable = (uint32_t*)(_mmio_base + 0x1e2c00C);
	pull_up_down = (uint32_t*)(_mmio_base + 0x1e2c010);
}
