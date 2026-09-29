/*
 * ARM (32-bit) kernel arch hooks.
 *
 * Strong override of the weak default in the common kernel/kernel/src/kernel.c
 * (declared in <kernel/kernel.h>). It reproduces the `__arm__` clone_kernel_vm()
 * that used to live inline in the common kernel.c, so that file stays free of
 * architecture #if macros. The other kernel.c arch hooks (arch_show_config,
 * arch_kernel_entry_early, arch_slave_resolve_core) keep their weak defaults on
 * ARM.
 */
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kstring.h>

void arch_set_proc_vm(page_dir_entry_t* vm) {
    uint32_t kernel_dir_base = PAGE_DIR_INDEX(KERNEL_BASE);

    memset(vm, 0, PAGE_DIR_SIZE);

    /*
     * Share the kernel high-half L1 entries so processes reuse the same 1KB
     * second-level tables for kernel image, direct-mapped RAM, MMIO and vectors.
     */
    for(uint32_t i = kernel_dir_base; i < PAGE_DIR_NUM; i++) {
        vm[i] = _kernel_info.kernel_vm[i];
    }

    /*
     * Keep the common low DMA identity window private per process. Boards that
     * need additional low-half mappings can extend this through arch_clone_proc_vm().
     */
    map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base,
            _sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_NOCACHE);
    if(arch_clone_proc_vm(vm, _kernel_info.kernel_vm) != 0)
        return;
    flush_dcache();
    flush_tlb();
}
