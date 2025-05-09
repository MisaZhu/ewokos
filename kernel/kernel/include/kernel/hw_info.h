#ifndef HW_INFO_H
#define HW_INFO_H

#include <stdint.h>
#include <sysinfo.h>
#include <mm/mmu.h>

extern sys_info_t _sys_info;
extern ewokos_addr_t   _allocable_phy_mem_top;
extern ewokos_addr_t   _allocable_phy_mem_base;
extern ewokos_addr_t   _core_base_offset;

extern void       sys_info_init_arch(void);
extern void       sys_info_init(void);
extern void       arch_vm(page_dir_entry_t* vm);
extern void       kalloc_arch(void);
extern int32_t    check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size);

#endif
