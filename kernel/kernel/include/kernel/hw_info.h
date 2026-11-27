#ifndef HW_INFO_H
#define HW_INFO_H

#include <stdint.h>
#include <sysinfo.h>
#include <mm/mmu.h>

extern sys_info_t _sys_info;
extern ewokos_addr_t   _core_base_offset;

extern void       sys_info_init_arch(void);
extern void       sys_info_init(void);
extern int32_t    arch_relocate_dma_high(void);
/* BSP hook: report the physical address range [base, top) that DMA masters
 * can reach (page-aligned by the caller). DMA allocations are constrained to
 * this range but are NOT carved out of general RAM. Weak default covers the
 * whole allocable span (coherent / full-range DMA, e.g. QEMU virt). Boards
 * with limited DMA masters (raspi5 VPU first 1GB, orin 32-bit DMA) override. */
extern void       arch_dma_range(ewokos_addr_t* base, ewokos_addr_t* top);
/* Return nonzero after reserving firmware-safe DMA/SHM pools. The BSP must
 * exclude them from kalloc_arch(); zero retains the legacy layout. */
extern int32_t    arch_reserve_memory_pools(void);
extern void       arch_vm(page_dir_entry_t* vm);
extern int32_t    arch_clone_proc_vm(page_dir_entry_t* vm, page_dir_entry_t* kernel_vm);
extern void       kalloc_arch(void);
extern int32_t    mem_map_is_normal_ram_arch(ewokos_addr_t phy_base, uint32_t size);
extern void       sys_info_config(void);
#endif
