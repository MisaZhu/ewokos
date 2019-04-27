#ifndef HARDWARE_H
#define HARDWARE_H

#include <types.h>
#include <mm/mmu.h>

extern char _fb_start[];

extern void hw_init();
extern uint32_t get_mmio_base_phy();
extern uint32_t get_mmio_mem_size();
extern uint32_t get_phy_ram_size();

extern void arch_set_kernel_vm(page_dir_entry_t* vm);

#endif
