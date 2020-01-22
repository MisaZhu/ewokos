#ifndef HW_INFO_H
#define HW_INFO_H

#include <types.h>
#include <mm/mmu.h>

typedef struct {
	char machine[32];
	uint32_t phy_mem_size;
	uint32_t phy_mmio_base;
	uint32_t mmio_size;
} hw_info_t;

extern void hw_info_init(void);
extern hw_info_t*  get_hw_info(void);
extern void arch_vm(page_dir_entry_t* vm);
extern void hw_optimise(void);

#endif
