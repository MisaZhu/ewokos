#ifndef HW_INFO_H
#define HW_INFO_H

#include <stdint.h>
#include <sysinfo.h>
#include <mm/mmu.h>

extern sys_info_t _sys_info;
extern uint32_t   _allocatable_phy_mem_top;
extern uint32_t   _allocatable_phy_mem_base;

extern void       sys_info_init(void);
extern void       arch_vm(page_dir_entry_t* vm);

#endif
