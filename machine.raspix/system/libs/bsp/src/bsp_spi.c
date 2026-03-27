/*----------------------------------------------------------------------------*/
/**
 * - for accessing spi
 *   = this is using spi0 (spi-specific interface)
 *   = bcm2835 has 2 more mini-spi interfaces (spi1/spi2)
 *     $ part of auxiliary peripheral (along with mini-uart)
 *     $ NOT using these
 **/
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>
#include <ewoksys/mmio.h>

void bsp_spi_set_div(int32_t clk_divide) {
	bcm283x_spi_set_div(clk_divide);
}

void bsp_spi_init(void) {
	bcm283x_spi_init();
}

void bsp_spi_select(uint32_t which) {
	bcm283x_spi_select(which);
}

void bsp_spi_activate(uint8_t enable) {
	bcm283x_spi_activate(enable);
}

void bsp_spi_send_recv(const uint8_t* send, uint8_t* recv, uint32_t size) {
	bcm283x_spi_send_recv(send, recv, size);
}

uint8_t bsp_spi_transfer(uint8_t data) {
	return bcm283x_spi_transfer(data);
}
