#include "arch/bcm283x/gpio.h"
#include "arch/bcm283x/spi.h"
#include <unistd.h>

#define TP_CS 7 //GPIO 7
#define TP_IRQ 25 //GPIO 25

#define SPI_CLK_DIVIDE_TEST 16384

void TP_init(void) {
	spi_init(SPI_CLK_DIVIDE_TEST);
	spi_select(SPI_SELECT_0);

	gpio_config(TP_CS, GPIO_OUTPUT);
	gpio_write(TP_CS, 0); // prevent blockage of the SPI bus
	gpio_config(TP_IRQ, GPIO_INPUT);
}

uint32_t TP_Send(uint8_t set_val){
	uint16_t get_val;
	uint8_t* p = &get_val;
	spi_activate(1);
	gpio_write(TP_CS, 0);
	spi_write(set_val);
	p[1] = spi_transfer(0);
	p[0] = spi_transfer(0);
	gpio_write(TP_CS, 1);
	spi_activate(0);
	uint32_t ret = get_val >> 4;
	return ret;
}

bool TP_Read(uint32_t* x, uint32_t* y){
	uint32_t tx[3];
	uint32_t ty[3];
	uint32_t i;
	if(gpio_read(TP_IRQ) != 0)
		return false;
	for(i=0; i<3; i++){
		tx[i] = TP_Send(0xD0);  //x
		ty[i] =  TP_Send(0x90); //y
	}
	*x = (tx[0] + tx[1] + tx[2])/3;
	*y = (ty[0] + ty[1] + ty[2])/3;
	return true;
}

static inline void wait_msec(uint32_t n) {
	usleep(n*1000);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	TP_init();

	while(true) {
		uint32_t x, y;
		uint32_t t = gpio_read(TP_IRQ);
		if(t == 0){
			TP_Read(&x, &y);
			kprintf(false, "touch: %d, %d\n", x, y);
		}
		wait_msec(10);
	}

	return 0;
}
