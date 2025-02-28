// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016
 * Author: Chen-Yu Tsai <wens@csie.org>
 *
 * Based on assembly code by Marc Zyngier <marc.zyngier@arm.com>,
 * which was based on code by Carl van Schaik <carl@ok-labs.com>.
 */

#include <gic.h>
#include <kernel/hw_info.h>

#define __irq		__attribute__ ((interrupt ("IRQ")))

#define SUNXI_GIC400_BASE	(MMIO_BASE + 0x3020000)
#define	GICD_BASE	(SUNXI_GIC400_BASE + 0x1000)
#define	GICC_BASE	(SUNXI_GIC400_BASE + 0x2000)
#define GICD_IPRIORITYRn	0x0400
#define GICD_IGROUPRn		0x0080
#define SUNXI_CPUCFG_BASE	0
/*
 * R40 is different from other single cluster SoCs.
 *
 * The power clamps are located in the unused space after the per-core
 * reset controls for core 3. The secondary core entry address register
 * is in the SRAM controller address range.
 */
#define SUN8I_R40_PWROFF			(0x110)
#define SUN8I_R40_PWR_CLAMP(cpu)		(0x120 + (cpu) * 0x4)
#define SUN8I_R40_SRAMC_SOFT_ENTRY_REG0		(0xbc)
#define true	1
#define false	0

struct __attribute__((packed)) sunxi_cpucfg_reg {
    uint8_t res0[0x40];      /* 0x000 */
    struct sunxi_cpucfg_cpu cpu[4];     /* 0x040 */
    uint8_t res1[0x44];      /* 0x140 */
    uint32_t gen_ctrl;       /* 0x184 */
    uint32_t l2_status;      /* 0x188 */
    uint8_t res2[0x4];       /* 0x18c */
    uint32_t event_in;       /* 0x190 */
    uint8_t res3[0xc];       /* 0x194 */
    uint32_t super_standy_flag;  /* 0x1a0 */
    uint32_t priv0;      /* 0x1a4 */
    uint32_t priv1;      /* 0x1a8 */
    uint8_t res4[0x4];       /* 0x1ac */
    uint32_t cpu1_pwr_clamp; /* 0x1b0 sun7i only */
    uint32_t cpu1_pwroff;    /* 0x1b4 sun7i only */
    uint8_t res5[0x2c];      /* 0x1b8 */
    uint32_t dbg_ctrl1;      /* 0x1e4 */
    uint8_t res6[0x18];      /* 0x1e8 */
    uint32_t idle_cnt0_low;  /* 0x200 */
    uint32_t idle_cnt0_high; /* 0x204 */
    uint32_t idle_cnt0_ctrl; /* 0x208 */
    uint8_t res8[0x4];       /* 0x20c */
    uint32_t idle_cnt1_low;  /* 0x210 */
    uint32_t idle_cnt1_high; /* 0x214 */
    uint32_t idle_cnt1_ctrl; /* 0x218 */
    uint8_t res9[0x4];       /* 0x21c */
    uint32_t idle_cnt2_low;  /* 0x220 */
    uint32_t idle_cnt2_high; /* 0x224 */
    uint32_t idle_cnt2_ctrl; /* 0x228 */
    uint8_t res10[0x4];      /* 0x22c */
    uint32_t idle_cnt3_low;  /* 0x230 */
    uint32_t idle_cnt3_high; /* 0x234 */
    uint32_t idle_cnt3_ctrl; /* 0x238 */
    uint8_t res11[0x4];      /* 0x23c */
    uint32_t idle_cnt4_low;  /* 0x240 */
    uint32_t idle_cnt4_high; /* 0x244 */
    uint32_t idle_cnt4_ctrl; /* 0x248 */
    uint8_t res12[0x34];     /* 0x24c */
    uint32_t cnt64_ctrl;     /* 0x280 */
    uint32_t cnt64_low;      /* 0x284 */
    uint32_t cnt64_high;     /* 0x288 */
};


static void  cp15_write_cntp_tval(uint32_t tval)
{
	__asm volatile ("mcr p15, 0, %0, c14, c2, 0" : : "r" (tval));
}

static void  cp15_write_cntp_ctl(uint32_t val)
{
	__asm volatile ("mcr p15, 0, %0, c14, c2, 1" : : "r" (val));
}

static uint32_t  cp15_read_cntp_ctl(void)
{
	uint32_t val;

	__asm volatile ("mrc p15, 0, %0, c14, c2, 1" : "=r" (val));

	return val;
}

#define ONE_MS (COUNTER_FREQUENCY / 1000)

static void  __mdelay(uint32_t ms)
{
	uint32_t reg = ONE_MS * ms;

	cp15_write_cntp_tval(reg);
	isb();
	cp15_write_cntp_ctl(3);

	do {
		isb();
		reg = cp15_read_cntp_ctl();
	} while (!(reg & BIT(2)));

	cp15_write_cntp_ctl(0);
	isb();
}

static void  clamp_release(uint32_t  *clamp)
{
#if defined(CONFIG_MACH_SUN6I) || defined(CONFIG_MACH_SUN7I) || \
	defined(CONFIG_MACH_SUN8I_H3) || \
	defined(CONFIG_MACH_SUN8I_R40)
	uint32_t tmp = 0x1ff;
	do {
		tmp >>= 1;
		writel(tmp, clamp);
	} while (tmp);

	__mdelay(10);
#endif
}

static void  clamp_set(uint32_t __maybe_unused *clamp)
{
#if defined(CONFIG_MACH_SUN6I) || defined(CONFIG_MACH_SUN7I) || \
	defined(CONFIG_MACH_SUN8I_H3) || \
	defined(CONFIG_MACH_SUN8I_R40)
	writel(0xff, clamp);
#endif
}

static void  sunxi_power_switch(uint32_t *clamp, uint32_t *pwroff, int on,
					int cpu)
{
	if (on) {
		/* Release power clamp */
		clamp_release(clamp);

		/* Clear power gating */
		clrbits_le32(pwroff, BIT(cpu));
	} else {
		/* Set power gating */
		setbits_le32(pwroff, BIT(cpu));

		/* Activate power clamp */
		clamp_set(clamp);
	}
}

#ifdef CONFIG_MACH_SUN8I_R40
/* secondary core entry address is programmed differently on R40 */
static void  sunxi_set_entry_address(void *entry)
{
	writel((uint32_t)entry,
	       SUNXI_SRAMC_BASE + SUN8I_R40_SRAMC_SOFT_ENTRY_REG0);
}
#else
static void  sunxi_set_entry_address(void *entry)
{
	struct sunxi_cpucfg_reg *cpucfg =
		(struct sunxi_cpucfg_reg *)SUNXI_CPUCFG_BASE;

	writel((uint32_t)entry, &cpucfg->priv0);
}
#endif

#ifdef CONFIG_MACH_SUN7I
/* sun7i (A20) is different from other single cluster SoCs */
static void  sunxi_cpu_set_power(int __always_unused cpu, int on)
{
	struct sunxi_cpucfg_reg *cpucfg =
		(struct sunxi_cpucfg_reg *)SUNXI_CPUCFG_BASE;

	sunxi_power_switch(&cpucfg->cpu1_pwr_clamp, &cpucfg->cpu1_pwroff,
			   on, 0);
}
#elif defined CONFIG_MACH_SUN8I_R40
static void  sunxi_cpu_set_power(int cpu, int on)
{
	struct sunxi_cpucfg_reg *cpucfg =
		(struct sunxi_cpucfg_reg *)SUNXI_CPUCFG_BASE;

	sunxi_power_switch((void *)cpucfg + SUN8I_R40_PWR_CLAMP(cpu),
			   (void *)cpucfg + SUN8I_R40_PWROFF,
			   on, 0);
}
#else /* ! CONFIG_MACH_SUN7I && ! CONFIG_MACH_SUN8I_R40 */
static void  sunxi_cpu_set_power(int cpu, int on)
{
	struct sunxi_prcm_reg *prcm =
		(struct sunxi_prcm_reg *)SUNXI_PRCM_BASE;

	sunxi_power_switch(&prcm->cpu_pwr_clamp[cpu], &prcm->cpu_pwroff,
			   on, cpu);
}
#endif /* CONFIG_MACH_SUN7I */

void  sunxi_cpu_power_off(uint32_t cpuid)
{
	struct sunxi_cpucfg_reg *cpucfg =
		(struct sunxi_cpucfg_reg *)SUNXI_CPUCFG_BASE;
	uint32_t cpu = cpuid & 0x3;

	/* Wait for the core to enter WFI */
	while (1) {
		if (readl(&cpucfg->cpu[cpu].status) & BIT(2))
			break;
		__mdelay(1);
	}

	/* Assert reset on target CPU */
	writel(0, &cpucfg->cpu[cpu].rst);

	/* Lock CPU (Disable external debug access) */
	clrbits_le32(&cpucfg->dbg_ctrl1, BIT(cpu));

	/* Power down CPU */
	sunxi_cpu_set_power(cpuid, false);

	/* Unlock CPU (Disable external debug access) */
	setbits_le32(&cpucfg->dbg_ctrl1, BIT(cpu));
}

static uint32_t  cp15_read_scr(void)
{
	uint32_t scr;

	__asm volatile ("mrc p15, 0, %0, c1, c1, 0" : "=r" (scr));

	return scr;
}

static void  cp15_write_scr(uint32_t scr)
{
	__asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r" (scr));
	isb();
}

/*
 * Although this is an FIQ handler, the FIQ is processed in monitor mode,
 * which means there's no FIQ banked registers. This is the same as IRQ
 * mode, so use the IRQ attribute to ask the compiler to handler entry
 * and return.
 */
void  __irq psci_fiq_enter(void)
{
	uint32_t scr, reg, cpu;

	/* Switch to secure mode */
	scr = cp15_read_scr();
	cp15_write_scr(scr & ~BIT(0));

	/* Validate reason based on IAR and acknowledge */
	reg = readl(GICC_BASE + GICC_IAR);

	/* Skip spurious interrupts 1022 and 1023 */
	if (reg == 1023 || reg == 1022)
		goto out;

	/* End of interrupt */
	writel(reg, GICC_BASE + GICC_EOIR);
	dsb();

	/* Get CPU number */
	cpu = (reg >> 10) & 0x7;

	/* Power off the CPU */
	sunxi_cpu_power_off(cpu);

out:
	/* Restore security level */
	cp15_write_scr(scr);
}

int  psci_cpu_on(uint32_t unused, uint32_t mpidr, uint32_t pc)
{
	struct sunxi_cpucfg_reg *cpucfg =
		(struct sunxi_cpucfg_reg *)SUNXI_CPUCFG_BASE;
	uint32_t cpu = (mpidr & 0x3);

	/* store target PC */
	psci_save_target_pc(cpu, pc);

	/* Set secondary core power on PC */
	sunxi_set_entry_address(0);

	/* Assert reset on target CPU */
	writel(0, &cpucfg->cpu[cpu].rst);

	/* Invalidate L1 cache */
	clrbits_le32(&cpucfg->gen_ctrl, BIT(cpu));

	/* Lock CPU (Disable external debug access) */
	clrbits_le32(&cpucfg->dbg_ctrl1, BIT(cpu));

	/* Power up target CPU */
	sunxi_cpu_set_power(cpu, true);

	/* De-assert reset on target CPU */
	writel(BIT(1) | BIT(0), &cpucfg->cpu[cpu].rst);

	/* Unlock CPU (Disable external debug access) */
	setbits_le32(&cpucfg->dbg_ctrl1, BIT(cpu));

	return 0;
}

void  psci_cpu_off(void)
{
	psci_cpu_off_common();

	/* Ask CPU0 via SGI15 to pull the rug... */
	writel(BIT(16) | 15, GICD_BASE + GICD_SGIR);
	dsb();

	/* Wait to be turned off */
	while (1)
		wfi();
}

void  psci_arch_init(void)
{
	uint32_t reg;

	/* SGI15 as Group-0 */
	clrbits_le32(GICD_BASE + GICD_IGROUPRn, BIT(15));

	/* Set SGI15 priority to 0 */
	writeb(0, GICD_BASE + GICD_IPRIORITYRn + 15);

	/* Be cool with non-secure */
	writel(0xff, GICC_BASE + GICC_PMR);

	/* Switch FIQEn on */
	setbits_le32(GICC_BASE + GICC_CTLR, BIT(3));

	reg = cp15_read_scr();
	reg |= BIT(2);  /* Enable FIQ in monitor mode */
	reg &= ~BIT(0); /* Secure mode */
	cp15_write_scr(reg);
}
