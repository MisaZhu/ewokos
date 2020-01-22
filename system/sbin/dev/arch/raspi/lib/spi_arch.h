#ifndef SPI_ARCH_H
#define SPI_ARCH_H

#include <types.h>

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

#define SPI_SELECT_0 0x01
#define SPI_SELECT_1 0x02
#define SPI_SELECT_DEFAULT SPI_SELECT_0

#define SPI_ACTIVATE 1
#define SPI_DEACTIVATE 0

#define SPI0_CS_CPOL                 0x00000008 ///< Clock Polarity
#define SPI0_CS_CPHA                 0x00000004 ///< Clock Phase

void spi_arch_init(int32_t clk_divide);
uint32_t spi_arch_transfer(uint32_t data);
void spi_arch_write(uint32_t data);
void spi_arch_activate(uint32_t enable);
void spi_arch_select(uint32_t which); 

#endif
