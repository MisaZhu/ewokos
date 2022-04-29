#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>
#include <sys/vdevice.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>

static int TP_CS = 7;
static int TP_IRQ = 25;

#define SPI_CLK_DIVIDE_TEST 256

static bool _down = false;
static int32_t _x, _y;

static void TP_init(void) {
	_down = false;
	_x = _y = 0;
	bcm283x_spi_init(SPI_CLK_DIVIDE_TEST);
	bcm283x_spi_select(SPI_SELECT_0);

	bcm283x_gpio_config(TP_CS, GPIO_OUTPUT);
	bcm283x_gpio_config(TP_IRQ, GPIO_INPUT);
	bcm283x_gpio_pull(TP_IRQ, GPIO_PULL_UP);
	bcm283x_gpio_write(TP_CS, 1); // prevent blockage of the SPI bus
}

static uint32_t cmd(uint8_t cmd) {
	uint16_t get_val;

	bcm283x_gpio_write(TP_CS, 0);
	bcm283x_spi_activate(1);

	bcm283x_spi_transfer(cmd);
	get_val = bcm283x_spi_transfer16(0);

	bcm283x_spi_activate(0);
	bcm283x_gpio_write(TP_CS, 1);
	uint32_t ret = get_val >> 4;
	return ret;
}

static bool do_read(uint16_t* x, uint16_t* y){
	uint16_t tx=0, ty=0;
	uint16_t i=0;
	for(i=0; i<4; i++){
		tx += cmd(0x90); //x
		ty += cmd(0xD0);  //y
	}
	*x = tx/4;
	*y = ty/4;
	return true;
}

int xpt2046_read(uint16_t* press,  uint16_t* x, uint16_t* y) {
	if(press == NULL || x == NULL || y == NULL)
		return -1;

	bcm283x_gpio_write(TP_CS, 0);
	uint32_t t = bcm283x_gpio_read(TP_IRQ);
	//klog("t: pin:%d, %d\n", TP_IRQ, t);
	if(t == 1 && !_down)
    return -1;

	if(t == 0) { //press down
		_down = true;
		*press = 1;

		do_read(x, y);
		_x = *x;
		_y = *y;
	}
	else {  //release
		_down = false;
		*press = 0;
		*x = _x;
		*y = _y;
	}
	return 0;	
}

void xpt2046_init(int pin_cs, int pin_irq) {
	TP_CS = pin_cs;
	TP_IRQ = pin_irq;
	TP_init();
}

