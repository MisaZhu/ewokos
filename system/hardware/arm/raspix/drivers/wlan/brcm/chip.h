// SPDX-License-Identifier: ISC
/*
 * Copyright (c) 2014 Broadcom Corporation
 */
#ifndef BRCMF_CHIP_H
#define BRCMF_CHIP_H

#include <types.h>
#include "chipcommon.h"

#define CORE_CC_REG(base, field) \
		(base + offsetof(struct chipcregs, field))

/**
 * struct brcmf_chip - chip level information.
 *
 * @chip: chip identifier.
 * @chiprev: chip revision.
 * @cc_caps: chipcommon core capabilities.
 * @cc_caps_ext: chipcommon core extended capabilities.
 * @pmucaps: PMU capabilities.
 * @pmurev: PMU revision.
 * @rambase: RAM base address (only applicable for ARM CR4 chips).
 * @ramsize: amount of RAM on chip including retention.
 * @srsize: amount of retention RAM on chip.
 * @name: string representation of the chip identifier.
 */
struct brcmf_chip {
	uint32_t chip;
	uint32_t chiprev;
	uint32_t cc_caps;
	uint32_t cc_caps_ext;
	uint32_t pmucaps;
	uint32_t pmurev;
	uint32_t rambase;
	uint32_t ramsize;
	uint32_t srsize;
	char name[12];
};

/**
 * struct brcmf_core - core related information.
 *
 * @id: core identifier.
 * @rev: core revision.
 * @base: base address of core register space.
 */
struct brcmf_core {
	u16 id;
	u16 rev;
	uint32_t base;
};

/**
 * struct brcmf_buscore_ops - buscore specific callbacks.
 *
 * @read32: read 32-bit value over bus.
 * @write32: write 32-bit value over bus.
 * @prepare: prepare bus for core configuration.
 * @setup: bus-specific core setup.
 * @active: chip becomes active.
 *	The callback should use the provided @rstvec when non-zero.
 */
struct brcmf_buscore_ops {
	uint32_t (*read32)(void *ctx, uint32_t addr);
	void (*write32)(void *ctx, uint32_t addr, uint32_t value);
	int (*prepare)(void *ctx);
	int (*reset)(void *ctx, struct brcmf_chip *chip);
	int (*setup)(void *ctx, struct brcmf_chip *chip);
	void (*activate)(void *ctx, struct brcmf_chip *chip, uint32_t rstvec);
};

struct brcmf_core *brcmf_chip_get_pmu(void);
bool brcmf_chip_set_active(uint32_t rstvec);
char *brcmf_chip_name(uint32_t id, uint32_t rev, char *buf, uint32_t len);
struct brcmf_chip *brcmf_chip_attach(void);
struct brcmf_core *brcmf_chip_get_core(u16 coreid);
#endif /* BRCMF_AXIDMP_H */
