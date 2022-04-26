#ifndef KERNEL_MMIO_H
#define KERNEL_MMIO_H

#include <mm/mmu.h>

extern uint32_t _mmio_base;
void enable_vmmio_base(void);

#endif
