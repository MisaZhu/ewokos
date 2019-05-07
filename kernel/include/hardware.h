#ifndef HARDWARE_H
#define HARDWARE_H

#include <types.h>
#include <mm/mmu.h>

extern char _fb_start[];

extern void hw_init(void);
extern uint32_t get_mmio_base_phy(void);
extern uint32_t get_mmio_mem_size(void);
extern uint32_t get_phy_ram_size(void);

extern void arch_set_kernel_vm(page_dir_entry_t* vm);

#endif
