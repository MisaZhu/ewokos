#ifndef KERNEL_DEV_H
#define KERNEL_DEV_H

#include <mm/mmu.h>

extern uint32_t _mmio_base;
void enable_vmmio_base(void);

void dev_init(void);

#endif
