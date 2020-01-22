/*----------------------------------------------------------------------------*/
/**
 * - for accessing spi
 *   = this is using spi0 (spi-specific interface)
 *   = bcm2835 has 2 more mini-spi interfaces (spi1/spi2)
 *     $ part of auxiliary peripheral (along with mini-uart)
 *     $ NOT using these
 **/
#include "gpio_arch.h"
#include "spi_arch.h"
#include <sys/mmio.h>


static uint32_t _mmio_base = 0;
static uint32_t spi_which = SPI_SELECT_DEFAULT;

void spi_arch_init(int32_t clk_divide) {
	_mmio_base = mmio_map();

	uint32_t data = SPI_CNTL_CLMASK; /* clear both rx/tx fifo */
	/* clear spi fifo */
	put32(SPI_CS_REG,data);
	/* set largest clock divider */
	clk_divide &= SPI_CLK_DIVIDE_MASK; /* 16-bit value */
	put32(SPI_CLK_REG,clk_divide); /** 0=65536, power of 2, rounded down */
	/* setup spi pins (ALTF0) */
	gpio_arch_config(SPI_SCLK, GPIO_ALTF0);
	gpio_arch_config(SPI_MOSI, GPIO_ALTF0);
	gpio_arch_config(SPI_MISO, GPIO_ALTF0);
	gpio_arch_config(SPI_CE0N, GPIO_ALTF0);
	gpio_arch_config(SPI_CE1N, GPIO_ALTF0);
}

void spi_arch_select(uint32_t which) {
	switch (which) {
		case SPI_SELECT_0:
		case SPI_SELECT_1:
			spi_which = which;
			break;
		default:
			spi_which = SPI_SELECT_DEFAULT;
	}
}

inline void spi_arch_activate(uint32_t enable) {
	uint32_t data = SPI_CNTL_TRXACT;
	if (enable) {
		/* activate transfer on selected channel 0 or 1 */
		put32(SPI_CS_REG,data|(spi_which>>1));
	}
	else {
		/* de-activate transfer */
		put32(SPI_CS_REG,get32(SPI_CS_REG)&~SPI_CNTL_TRXACT);
	}
}

inline void spi_arch_write(uint32_t data) {
	/* wait if fifo is full */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	/* write a byte */
	put32(SPI_FIFO_REG,data&0xff);
	/* wait until done */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
}

inline uint32_t spi_arch_transfer(uint32_t data) {
	/* wait if fifo is full */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	/* write a byte */
	put32(SPI_FIFO_REG,data&0xff);
	/* wait until done */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
	/* should get a byte? */
	while (!(get32(SPI_CS_REG)&SPI_STAT_RXDATA));
	/* read a byte */
	return get32(SPI_FIFO_REG)&0xff;
}
