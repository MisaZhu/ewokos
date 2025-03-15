// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2009 Texas Instruments Incorporated - https://www.ti.com/
 *
 * Driver for SPI controller on DaVinci. Based on atmel_spi.c
 * by Atmel Corporation
 *
 * Copyright (C) 2007 Atmel Corporation
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <kernel/system.h>

#include "spi.h"

#define BIT(x)	(0x1<<(x))

/* SPIGCR0 */
#define SPIGCR0_SPIENA_MASK	0x1
#define SPIGCR0_SPIRST_MASK	0x0

/* SPIGCR0 */
#define SPIGCR1_CLKMOD_MASK	BIT(1)
#define SPIGCR1_MASTER_MASK	BIT(0)
#define SPIGCR1_SPIENA_MASK	BIT(24)

/* SPIPC0 */
#define SPIPC0_DIFUN_MASK	BIT(11)		/* SIMO */
#define SPIPC0_DOFUN_MASK	BIT(10)		/* SOMI */
#define SPIPC0_CLKFUN_MASK	BIT(9)		/* CLK */
#define SPIPC0_EN0FUN_MASK	BIT(0)

/* SPIFMT0 */
#define SPIFMT_SHIFTDIR_SHIFT	20
#define SPIFMT_POLARITY_SHIFT	17
#define SPIFMT_PHASE_SHIFT	16
#define SPIFMT_PRESCALE_SHIFT	8

/* SPIDAT1 */
#define SPIDAT1_CSHOLD_SHIFT	28
#define SPIDAT1_CSNR_SHIFT	16

/* SPIDELAY */
#define SPI_C2TDELAY_SHIFT	24
#define SPI_T2CDELAY_SHIFT	16

/* SPIBUF */
#define SPIBUF_RXEMPTY_MASK	BIT(31)
#define SPIBUF_TXFULL_MASK	BIT(29)

/* SPIDEF */
#define SPIDEF_CSDEF0_MASK	(0xFF)

#define writel(val, reg)    (*(volatile uint32_t*)(reg) = (val))
#define readl(reg)          (*(volatile uint32_t*)(reg))

/* davinci spi register set */
struct davinci_spi_regs {
	volatile uint32_t	gcr0;		/* 0x00 */
	volatile uint32_t	gcr1;		/* 0x04 */
	volatile uint32_t	int0;		/* 0x08 */
	volatile uint32_t	lvl;		/* 0x0c */
	volatile uint32_t	flg;		/* 0x10 */
	volatile uint32_t	pc0;		/* 0x14 */
	volatile uint32_t	pc1;		/* 0x18 */
	volatile uint32_t	pc2;		/* 0x1c */
	volatile uint32_t	pc3;		/* 0x20 */
	volatile uint32_t	pc4;		/* 0x24 */
	volatile uint32_t	pc5;		/* 0x28 */
	volatile uint32_t	rsvd[3];
	volatile uint32_t	dat0;		/* 0x38 */
	volatile uint32_t	dat1;		/* 0x3c */
	volatile uint32_t	buf;		/* 0x40 */
	volatile uint32_t	emu;		/* 0x44 */
	volatile uint32_t	delay;		/* 0x48 */
	volatile uint32_t	def;		/* 0x4c */
	volatile uint32_t	fmt0;		/* 0x50 */
	volatile uint32_t	fmt1;		/* 0x54 */
	volatile uint32_t	fmt2;		/* 0x58 */
	volatile uint32_t	fmt3;		/* 0x5c */
	volatile uint32_t	intvec0;	/* 0x60 */
	volatile uint32_t	intvec1;	/* 0x64 */
};

/* davinci spi slave */
struct davinci_spi_slave {
	struct davinci_spi_regs *regs;
	unsigned int freq; /* current SPI bus frequency */
	unsigned int mode; /* current SPI mode used */
	uint8_t num_cs;	   /* total no. of CS available */
	uint8_t cur_cs;	   /* CS of current slave */
	bool half_duplex;  /* true, if master is half-duplex only */
};

static struct davinci_spi_slave _spi = {
	.regs = (struct davinci_spi_regs*)SPI1_BASE,
	.freq = 1*1024*1024,
	.mode = 0,
	.cur_cs = 0,
	.num_cs = 1,
	.half_duplex = false,
};

static inline uint32_t davinci_spi_xfer_data(uint32_t data)
{
	uint32_t buf_reg_val;

	/* send out data */
	writel(data, &_spi.regs->dat1);

	/* wait for the data to clock in/out */
	while ((buf_reg_val = readl(&_spi.regs->buf)) & SPIBUF_RXEMPTY_MASK)
		;

	return buf_reg_val;
}

int davinci_spi_read(unsigned int len,
			    uint8_t *rxp, unsigned long flags)
{
	unsigned int data1_reg_val;

	/* enable CS hold, CS[n] and clear the data bits */
	data1_reg_val = ((1 << SPIDAT1_CSHOLD_SHIFT) |
			 (_spi.cur_cs << SPIDAT1_CSNR_SHIFT));

	/* wait till TXFULL is deasserted */
	while (readl(&_spi.regs->buf) & SPIBUF_TXFULL_MASK)
		;

	/* keep reading 1 byte until only 1 byte left */
	while ((len--) > 1)
		*rxp++ = davinci_spi_xfer_data(data1_reg_val);

	/* clear CS hold when we reach the end */
	if (flags & SPI_XFER_END)
		data1_reg_val &= ~(1 << SPIDAT1_CSHOLD_SHIFT);

	/* read the last byte */
	*rxp = davinci_spi_xfer_data(data1_reg_val);

	return 0;
}

int davinci_spi_write(unsigned int len,
			     const uint8_t *txp, unsigned long flags)
{
	unsigned int data1_reg_val;

	/* enable CS hold and clear the data bits */
	data1_reg_val = ((1 << SPIDAT1_CSHOLD_SHIFT) |
			 (_spi.cur_cs << SPIDAT1_CSNR_SHIFT));

	/* wait till TXFULL is deasserted */
	while (readl(&_spi.regs->buf) & SPIBUF_TXFULL_MASK)
		;

	/* keep writing 1 byte until only 1 byte left */
	while ((len--) > 1)
		davinci_spi_xfer_data(data1_reg_val | *txp++);

	/* clear CS hold when we reach the end */
	if (flags & SPI_XFER_END)
		data1_reg_val &= ~(1 << SPIDAT1_CSHOLD_SHIFT);

	/* write the last byte */
	davinci_spi_xfer_data(data1_reg_val | *txp);

	return 0;
}

int davinci_spi_read_write(unsigned
				  int len, uint8_t *rxp, const uint8_t *txp,
				  unsigned long flags)
{
	unsigned int data1_reg_val;

	/* enable CS hold and clear the data bits */
	data1_reg_val = ((1 << SPIDAT1_CSHOLD_SHIFT) |
			 (_spi.cur_cs << SPIDAT1_CSNR_SHIFT));

	/* wait till TXFULL is deasserted */
	while (readl(&_spi.regs->buf) & SPIBUF_TXFULL_MASK)
		;

	/* keep reading and writing 1 byte until only 1 byte left */
	while ((len--) > 1)
		*rxp++ = davinci_spi_xfer_data(data1_reg_val | *txp++);

	/* clear CS hold when we reach the end */
	if (flags & SPI_XFER_END)
		data1_reg_val &= ~(1 << SPIDAT1_CSHOLD_SHIFT);

	/* read and write the last byte */
	*rxp = davinci_spi_xfer_data(data1_reg_val | *txp);

	return 0;
}

int davinci_spi_claim_bus(int cs)
{
	unsigned int mode = 0, scalar;
	_spi.cur_cs = ~(1 << cs);
	/* Enable the SPI hardware */
	writel(SPIGCR0_SPIRST_MASK, &_spi.regs->gcr0);
	_delay_usec(1000);
	writel(SPIGCR0_SPIENA_MASK, &_spi.regs->gcr0);

	/* Set master mode, powered up and not activated */
	writel(SPIGCR1_MASTER_MASK | SPIGCR1_CLKMOD_MASK, &_spi.regs->gcr1);

	/* CS, CLK, SIMO and SOMI are functional pins */
	writel(((1 << cs) | SPIPC0_CLKFUN_MASK |
		SPIPC0_DOFUN_MASK | SPIPC0_DIFUN_MASK), &_spi.regs->pc0);

	/* setup format */
	scalar = ((CFG_SYS_SPI_CLK / _spi.freq) - 1) & 0xFF;

	/*
	 * Use following format:
	 *   character length = 8,
	 *   MSB shifted out first
	 */
	if (_spi.mode & SPI_CPOL)
		mode |= SPI_CPOL;
	if (!(_spi.mode & SPI_CPHA))
		mode |= SPI_CPHA;
	writel(8 | (scalar << SPIFMT_PRESCALE_SHIFT) |
		(mode << SPIFMT_PHASE_SHIFT), &_spi.regs->fmt0);

	/*
	 * Including a minor delay. No science here. Should be good even with
	 * no delay
	 */
	writel((50 << SPI_C2TDELAY_SHIFT) |
		(50 << SPI_T2CDELAY_SHIFT), &_spi.regs->delay);

	/* default chip select register */
	writel(SPIDEF_CSDEF0_MASK, &_spi.regs->def);

	/* no interrupts */
	writel(0, &_spi.regs->int0);
	writel(0, &_spi.regs->lvl);

	/* enable SPI */
	writel((readl(&_spi.regs->gcr1) | SPIGCR1_SPIENA_MASK), &_spi.regs->gcr1);

	return 0;
}

int davinci_spi_release_bus(void)
{
	/* Disable the SPI hardware */
	writel(SPIGCR0_SPIRST_MASK, &_spi.regs->gcr0);
	return 0;
}
