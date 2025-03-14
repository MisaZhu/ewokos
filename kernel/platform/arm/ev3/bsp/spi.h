#ifndef __SPI_DAVINCI_H__
#define __SPI_DAVINCI_H__
#include <mm/mmu.h>

#define SPI0_BASE (MMIO_BASE + 0x01C41000)
#define SPI1_BASE (MMIO_BASE + 0x01F0E000)

#define CFG_SYS_SPI_CLK		(24000000)

/* SPI mode flags */
#define SPI_CPHA    BIT(0)  /* clock phase (1 = SPI_CLOCK_PHASE_SECOND) */
#define SPI_CPOL    BIT(1)  /* clock polarity (1 = SPI_POLARITY_HIGH) */
#define SPI_MODE_0  (0|0)           /* (original MicroWire) */
#define SPI_MODE_1  (0|SPI_CPHA)
#define SPI_MODE_2  (SPI_CPOL|0)
#define SPI_MODE_3  (SPI_CPOL|SPI_CPHA)
#define SPI_CS_HIGH BIT(2)          /* CS active high */
#define SPI_LSB_FIRST   BIT(3)          /* per-word bits-on-wire */
#define SPI_3WIRE   BIT(4)          /* SI/SO signals shared */
#define SPI_LOOP    BIT(5)          /* loopback mode */
#define SPI_SLAVE   BIT(6)          /* slave mode */
#define SPI_PREAMBLE    BIT(7)          /* Skip preamble bytes */
#define SPI_TX_BYTE BIT(8)          /* transmit with 1 wire byte */
#define SPI_TX_DUAL BIT(9)          /* transmit with 2 wires */
#define SPI_TX_QUAD BIT(10)         /* transmit with 4 wires */
#define SPI_RX_SLOW BIT(11)         /* receive with 1 wire slow */
#define SPI_RX_DUAL BIT(12)         /* receive with 2 wires */
#define SPI_RX_QUAD BIT(13)         /* receive with 4 wires */
#define SPI_TX_OCTAL    BIT(14)         /* transmit with 8 wires */
#define SPI_RX_OCTAL    BIT(15)         /* receive with 8 wires */

#define SPI_XFER_BEGIN      BIT(0)  /* Assert CS before transfer */
#define SPI_XFER_END        BIT(1)  /* Deassert CS after transfer */
#define SPI_XFER_ONCE       (SPI_XFER_BEGIN | SPI_XFER_END)
#define SPI_XFER_U_PAGE     BIT(4)
#define SPI_XFER_STACKED    BIT(5)
#define SPI_XFER_LOWER      BIT(6)


#define CS_DEFAULT  0xFF

#define SPIFMT_PHASE_MASK   BIT(16)
#define SPIFMT_POLARITY_MASK    BIT(17)
#define SPIFMT_DISTIMER_MASK    BIT(18)
#define SPIFMT_SHIFTDIR_MASK    BIT(20)
#define SPIFMT_WAITENA_MASK BIT(21)
#define SPIFMT_PARITYENA_MASK   BIT(22)
#define SPIFMT_ODD_PARITY_MASK  BIT(23)
#define SPIFMT_WDELAY_MASK  0x3f000000u
#define SPIFMT_WDELAY_SHIFT 24
#define SPIFMT_PRESCALE_SHIFT   8

/* SPIPC0 */
#define SPIPC0_DIFUN_MASK   BIT(11)     /* MISO */
#define SPIPC0_DOFUN_MASK   BIT(10)     /* MOSI */
#define SPIPC0_CLKFUN_MASK  BIT(9)      /* CLK */
#define SPIPC0_SPIENA_MASK  BIT(8)      /* nREADY */

#define SPIINT_MASKALL      0x0101035F
#define SPIINT_MASKINT      0x0000015F
#define SPI_INTLVL_1        0x000001FF
#define SPI_INTLVL_0        0x00000000

/* SPIDAT1 (upper 16 bit defines) */
#define SPIDAT1_CSHOLD_MASK BIT(12)
#define SPIDAT1_WDEL        BIT(10)

/* SPIGCR1 */
#define SPIGCR1_CLKMOD_MASK BIT(1)
#define SPIGCR1_MASTER_MASK     BIT(0)
#define SPIGCR1_POWERDOWN_MASK  BIT(8)
#define SPIGCR1_LOOPBACK_MASK   BIT(16)
#define SPIGCR1_SPIENA_MASK BIT(24)

/* SPIBUF */
#define SPIBUF_TXFULL_MASK  BIT(29)
#define SPIBUF_RXEMPTY_MASK BIT(31)

/* SPIDELAY */
#define SPIDELAY_C2TDELAY_SHIFT 24
#define SPIDELAY_C2TDELAY_MASK  (0xFF << SPIDELAY_C2TDELAY_SHIFT)
#define SPIDELAY_T2CDELAY_SHIFT 16
#define SPIDELAY_T2CDELAY_MASK  (0xFF << SPIDELAY_T2CDELAY_SHIFT)
#define SPIDELAY_T2EDELAY_SHIFT 8
#define SPIDELAY_T2EDELAY_MASK  (0xFF << SPIDELAY_T2EDELAY_SHIFT)
#define SPIDELAY_C2EDELAY_SHIFT 0
#define SPIDELAY_C2EDELAY_MASK  0xFF

/* Error Masks */
#define SPIFLG_DLEN_ERR_MASK        BIT(0)
#define SPIFLG_TIMEOUT_MASK     BIT(1)
#define SPIFLG_PARERR_MASK      BIT(2)
#define SPIFLG_DESYNC_MASK      BIT(3)
#define SPIFLG_BITERR_MASK      BIT(4)
#define SPIFLG_OVRRUN_MASK      BIT(6)
#define SPIFLG_BUF_INIT_ACTIVE_MASK BIT(24)
#define SPIFLG_ERROR_MASK       (SPIFLG_DLEN_ERR_MASK \
                | SPIFLG_TIMEOUT_MASK | SPIFLG_PARERR_MASK \
                | SPIFLG_DESYNC_MASK | SPIFLG_BITERR_MASK \
                | SPIFLG_OVRRUN_MASK)

#define SPIINT_DMA_REQ_EN   BIT(16)

/* SPI Controller registers */
#define SPIGCR0     0x00
#define SPIGCR1     0x04
#define SPIINT      0x08
#define SPILVL      0x0c
#define SPIFLG      0x10
#define SPIPC0      0x14
#define SPIDAT1     0x3c
#define SPIBUF      0x40
#define SPIDELAY    0x48
#define SPIDEF      0x4c
#define SPIFMT0     0x50

#define DMA_MIN_BYTES   16

int davinci_spi_write(unsigned int len, const uint8_t *txp, unsigned long flags);
int davinci_spi_claim_bus(int cs);
int davinci_spi_release_bus(void);
#endif
