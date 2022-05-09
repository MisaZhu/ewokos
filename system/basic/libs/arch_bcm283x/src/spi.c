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

static uint32_t bcm283x_spi_which = SPI_SELECT_DEFAULT;

void bcm283x_spi_init(int32_t clk_divide) {
	bcm283x_gpio_init();

	uint32_t data = SPI_CNTL_CLMASK; /* clear both rx/tx fifo */
	/* clear spi fifo */
	put32(SPI_CS_REG,data);
	/* set largest clock divider */
	clk_divide &= SPI_CLK_DIVIDE_MASK; /* 16-bit value */
	put32(SPI_CLK_REG,clk_divide); /** 0=65536, power of 2, rounded down */
	/* setup spi pins (ALTF0) */
	bcm283x_gpio_config(SPI_SCLK, GPIO_ALTF0);
	bcm283x_gpio_config(SPI_MOSI, GPIO_ALTF0);
	bcm283x_gpio_config(SPI_MISO, GPIO_ALTF0);
	bcm283x_gpio_config(SPI_CE0N, GPIO_ALTF0);
	bcm283x_gpio_config(SPI_CE1N, GPIO_ALTF0);
}

void bcm283x_spi_select(uint32_t which) {
	switch (which) {
		case SPI_SELECT_0:
		case SPI_SELECT_1:
			bcm283x_spi_which = which;
			break;
		default:
			bcm283x_spi_which = SPI_SELECT_DEFAULT;
	}
}

inline void bcm283x_spi_activate(uint8_t enable) {
	uint32_t data = SPI_CNTL_TRXACT;
	if (enable) {
		/* activate transfer on selected channel 0 or 1 */
		put32(SPI_CS_REG,data|(bcm283x_spi_which>>1));
	}
	else {
		/* de-activate transfer */
		put32(SPI_CS_REG,get32(SPI_CS_REG)&~SPI_CNTL_TRXACT);
	}
}

inline void bcm283x_spi_write(uint8_t data) {
	/* wait if fifo is full */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	/* write a byte */
	put32(SPI_FIFO_REG,data&0xff);
	/* wait until done */
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
}

inline uint8_t bcm283x_spi_transfer(uint8_t data) {
	// wait if fifo is full
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	// write a byte
	put32(SPI_FIFO_REG,data&0xff);
	// wait until done 
	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDONE));
	// should get a byte? 
	while (!(get32(SPI_CS_REG)&SPI_STAT_RXDATA));
	// read a byte
	return get32(SPI_FIFO_REG)&0xff;
}

inline uint16_t bcm283x_spi_transfer16_fast(uint16_t data) {
	uint8_t hi, low;
	hi = ((data >> 8) & 0xff); 

	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	put32(SPI_FIFO_REG,hi);

	if(get32(SPI_CS_REG)&SPI_STAT_RXDATA)
		hi =  get32(SPI_FIFO_REG); 

	low = data & 0xff; 

	while (!(get32(SPI_CS_REG)&SPI_STAT_TXDATA));
	put32(SPI_FIFO_REG,low);

	if(get32(SPI_CS_REG)&SPI_STAT_RXDATA)
		low =  get32(SPI_FIFO_REG); 	
	return (hi << 8) | low;
} 

inline uint16_t bcm283x_spi_transfer16(uint16_t data) {
	uint8_t hi = bcm283x_spi_transfer((data >> 8) & 0xff);
	uint8_t low = bcm283x_spi_transfer(data & 0xff);
	return (hi << 8) | low;
}
