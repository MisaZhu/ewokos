#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>
#include <ewoksys/vdevice.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vfs.h>

//GPIO ports for waveshare 3.5 inch
static int TP_CS = 7;
static int TP_IRQ = 25;
static int SPI_DIV = 64;

static bool _down = false;
static int32_t _x, _y;

static void TP_init(void) {
	_down = false;
	_x = _y = 0;
	//klog("tp_init\n");
	bcm283x_spi_set_div(SPI_DIV);
	bcm283x_spi_select(SPI_SELECT_0);

	bcm283x_gpio_config(TP_CS, GPIO_OUTPUT);
	bcm283x_gpio_config(TP_IRQ, GPIO_INPUT);
	bcm283x_gpio_pull(TP_IRQ, GPIO_PULL_UP);
	bcm283x_gpio_write(TP_CS, 1); // prevent blockage of the SPI bus
	//klog("tp_init done\n");
}

static uint32_t cmd(uint8_t cmd) {
	uint16_t get_val;

	bcm283x_spi_transfer(cmd);
	get_val = bcm283x_spi_transfer16(0);

	uint32_t ret = get_val >> 4;
	return ret;
}

static bool do_read(uint16_t* x, uint16_t* y){
	uint16_t tx=0, ty=0;
	uint16_t i=0;

	bcm283x_spi_set_div(SPI_DIV);
	bcm283x_gpio_write(TP_CS, 0);
	bcm283x_spi_activate(1);


	for(i=0; i<4; i++){
		tx += cmd(0x90); //x
		ty += cmd(0xD0);  //y
	}

	bcm283x_spi_activate(0);
	bcm283x_gpio_write(TP_CS, 1);


	*x = tx/4;
	*y = ty/4;
	return true;
}

int xpt2046_read(uint16_t* press,  uint16_t* x, uint16_t* y) {
	if(press == NULL || x == NULL || y == NULL)
		return -1;

	uint32_t t = bcm283x_gpio_read(TP_IRQ);
	if(t == 1 && !_down)
 	   return -1;

	if(t == 0) { //press down
		_down = true;
		*press = 1;

		do_read(x, y);
		_x = *x;
		_y = *y;
		klog("tp: x=%d, y=%d\n", _x, _y);
	}
	else {  //release
		_down = false;
		*press = 0;
		*x = _x;
		*y = _y;
	}
	return 0;	
}

void xpt2046_init(int pin_cs, int pin_irq, int cdiv) {
	TP_CS = pin_cs;
	TP_IRQ = pin_irq;
	if(cdiv > 0)
		SPI_DIV = cdiv;
	TP_init();
}

