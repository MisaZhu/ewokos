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

#define SPI0_OFFSET 0x00204000
#define SPI1_OFFSET 0x00215080
#define SPI2_OFFSET 0x002150C0

#define SPI_BASE (_mmio_base | SPI0_OFFSET)

#define SPI_CS_REG   (SPI_BASE+0x00)
#define SPI_FIFO_REG (SPI_BASE+0x04)
#define SPI_CLK_REG  (SPI_BASE+0x08)
#define SPI_DLEN_REG (SPI_BASE+0x0C)
#define SPI_LTOH_REG (SPI_BASE+0x10)
#define SPI_DC_REG   (SPI_BASE+0x14)

/* control status bits */
#define SPI_CNTL_CSPOL2 0x00800000
#define SPI_CNTL_CSPOL1 0x00400000
#define SPI_CNTL_CSPOL0 0x00200000
#define SPI_STAT_RXFULL 0x00100000
#define SPI_STAT_RXREAD 0x00080000
#define SPI_STAT_TXDATA 0x00040000
#define SPI_STAT_RXDATA 0x00020000
#define SPI_STAT_TXDONE 0x00010000
#define SPI_CNTL_READEN 0x00001000
#define SPI_CNTL_ADCS   0x00000800
#define SPI_CNTL_INTRXR 0x00000400
#define SPI_CNTL_INTRDN 0x00000200
#define SPI_CNTL_DMA_EN 0x00000100
#define SPI_CNTL_TRXACT 0x00000080
#define SPI_CNTL_CSPOL  0x00000040
#define SPI_CNTL_CLMASK 0x00000030
#define SPI_CNTL_CLR_RX 0x00000020
#define SPI_CNTL_CLR_TX 0x00000010
#define SPI_CNTL_CPOL   0x00000008
#define SPI_CNTL_CPHA   0x00000004
#define SPI_CNTL_CSMASK 0x00000003
#define SPI_CNTL_CS0    0x00000000
#define SPI_CNTL_CS1    0x00000001
#define SPI_CNTL_CS2    0x00000002

#define SPI0_SCLK 11
#define SPI0_MOSI 10
#define SPI0_MISO 9
#define SPI0_CE0N 8
#define SPI0_CE1N 7
#define SPI_SCLK SPI0_SCLK
#define SPI_MOSI SPI0_MOSI
#define SPI_MISO SPI0_MISO
#define SPI_CE0N SPI0_CE0N
#define SPI_CE1N SPI0_CE1N

#define SPI_CLK_DIVIDE_MASK 0xFFFF
#define SPI_CLK_DIVIDE_DEFAULT 0

#define SPI_ACTIVATE 1
#define SPI_DEACTIVATE 0

#define SPI0_CS_CPOL                 0x00000008 ///< Clock Polarity
#define SPI0_CS_CPHA                 0x00000004 ///< Clock Phase


static uint32_t bcm283x_spi_which = SPI_SELECT_DEFAULT;

void bcm283x_spi_set_div(int32_t clk_divide) {
	/* set largest clock divider */
	clk_divide &= SPI_CLK_DIVIDE_MASK; /* 16-bit value */
	put32(SPI_CLK_REG,clk_divide); /** 0=65536, power of 2, rounded down */
}

void bcm283x_spi_init(void) {
	bcm283x_gpio_init();

	uint32_t data = SPI_CNTL_CLMASK; /* clear both rx/tx fifo */
	/* clear spi fifo */
	put32(SPI_CS_REG,data);
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

struct SPI0_regs {
    volatile uint32_t cs;
    volatile uint32_t fifo;
    volatile uint32_t clock;
    volatile uint32_t size;
};
#define _spi0_regs ((struct SPI0_regs *)(SPI_BASE))

inline void bcm283x_spi_activate(uint8_t enable) {
	//uint32_t data = SPI_CNTL_TRXACT;
	if (enable) {
		/*
		_spi0_regs->cs = _spi0_regs->cs |
				 SPI_CNTL_TRXACT |
				 SPI_CNTL_CLR_RX |
				 SPI_CNTL_CLR_TX |
				(bcm283x_spi_which>>1);
				*/
		_spi0_regs->cs = SPI_CNTL_TRXACT | (bcm283x_spi_which>>1);
	}
	else {
		_spi0_regs->cs = (_spi0_regs->cs & ~SPI_CNTL_TRXACT);
	}
}

inline void bcm283x_spi_send_recv(const uint8_t* send, uint8_t* recv, uint32_t size) {
	_spi0_regs->size = size;
	uint32_t read_count = 0;
	uint32_t write_count = 0;

	while(read_count < size || write_count < size) {
		while(write_count < size && _spi0_regs->cs & SPI_STAT_TXDATA) {
			if (send)
				_spi0_regs->fifo = *send++;
			else
				_spi0_regs->fifo = 0;
			write_count++;
		}

		while(read_count < size && _spi0_regs->cs & SPI_STAT_RXDATA) {
			uint32_t data = _spi0_regs->fifo;
			if (recv) 
				*recv++ = data;
			read_count++;
		}
	}
}

inline uint8_t bcm283x_spi_transfer(uint8_t data) {
	//write
	while (!(_spi0_regs->cs & SPI_STAT_TXDATA));
	_spi0_regs->fifo = data;

	//read
	while (!(_spi0_regs->cs & SPI_STAT_RXDATA));
	data  = _spi0_regs->fifo;
	return data;
}
