/*
 * AArch64 kernel arch hooks.
 *
 * Strong override of the weak default in the common kernel/kernel/src/kernel.c
 * (declared in <kernel/kernel.h>). It reproduces the `__aarch64__` branch that
 * used to live inline in the common show_config(): aarch64 addresses are
 * 64-bit, so each is printed as a hi/lo word pair, and the extra
 * sys_shm_contig_base line is shown. Keeping it here lets the common kernel.c
 * stay free of architecture #if macros.
 */
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <kernel/boot.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kprintf.h>
#include <sysinfo.h>

#define SPLIT_ADDR(x) ((uint32_t)(((uint64_t)(x)) >> 32)), ((uint32_t)(x))

void arch_show_config(void) {
    printf("\n"
          "  machine              %s\n" 
          "  arch                 %s-%dk\n"
          "  cores                %d\n"
          "  kernel_timer_freq    %d\n"
          "  mem_offset           0x%08x%08x\n"
          "  phy mem size         %d MB\n"
          "  usable mem size      %d MB\n"
          "  mmio_base            Phy:0x%08x%08x V:0x%08x%08x (%d MB)\n"
          "  kernel image         Phy:0x%08x%08x ~ 0x%08x%08x (%d KB)\n"
          "  vsyscall info        Phy:0x%08x%08x ~ 0x%08x%08x (%d KB)\n"
          "  kernel page dir      Phy:0x%08x%08x ~ 0x%08x%08x (%d KB)\n"
          "  allocable page dir   Phy:0x%08x%08x ~ 0x%08x%08x (%d MB)\n"
          "  kmalloc              Phy:0x%08x%08x ~ 0x%08x%08x (%d MB)\n"
          "  sys_dma_base         Phy:0x%08x%08x ~ 0x%08x%08x (%d MB)\n"
          "  sys_shm_contig_base  Phy:0x%08x%08x ~ 0x%08x%08x (%d MB)\n"
          "  allocable mem info   Phy:0x%08x%08x ~ 0x%08x%08x (%d MB)\n"
          "  max proc num         %d\n"
          "  max task total       %d\n"
          "  max task per proc    %d\n"
          "-----------------------------------------------------\n",
            _sys_info.machine,
            _sys_info.arch, PAGE_SIZE/1024,
            _kernel_config.cores,
            _kernel_config.timer_freq,
            SPLIT_ADDR(_sys_info.phy_offset),
            _sys_info.total_phy_mem_size/(1*MB),
            _sys_info.total_usable_mem_size / (1*MB),
            SPLIT_ADDR(_sys_info.mmio.phy_base), SPLIT_ADDR(_sys_info.mmio.v_base), _sys_info.mmio.size/(1*MB),
            SPLIT_ADDR(V2P(_kernel_start)), SPLIT_ADDR(V2P(_kernel_end)), (_kernel_end - _kernel_start) / (1*KB),
            SPLIT_ADDR(V2P(KERNEL_VSYSCALL_INFO_BASE)), SPLIT_ADDR(V2P(KERNEL_VSYSCALL_INFO_END)), KERNEL_VSYSCALL_INFO_SIZE / (1*KB),
            SPLIT_ADDR(V2P(KERNEL_PAGE_DIR_BASE)), SPLIT_ADDR(V2P(KERNEL_PAGE_DIR_END)), KERNEL_PAGE_DIR_SIZE / (1*KB),
            SPLIT_ADDR(V2P(ALLOCABLE_PAGE_DIR_BASE)), SPLIT_ADDR(V2P(ALLOCABLE_PAGE_DIR_END)), ALLOCABLE_PAGE_DIR_SIZE / (1*MB),
            SPLIT_ADDR(V2P(KMALLOC_BASE)), SPLIT_ADDR(V2P(KMALLOC_END)), _sys_info.kmalloc_size / (1*MB),
            SPLIT_ADDR(_sys_info.sys_dma.phy_base), SPLIT_ADDR(_sys_info.sys_dma.phy_base+_sys_info.sys_dma.size), _sys_info.sys_dma.size/(1*MB),
            SPLIT_ADDR(_sys_info.shm_contig.phy_base), SPLIT_ADDR(_sys_info.shm_contig.phy_base+_sys_info.shm_contig.size), _sys_info.shm_contig.size/(1*MB),
            SPLIT_ADDR(_sys_info.allocable_phy_mem_base), SPLIT_ADDR(_sys_info.allocable_phy_mem_top), (uint32_t)(get_free_mem_size() / (1*MB)),
            _kernel_config.max_proc_num,
            _kernel_config.max_task_num,
            _kernel_config.max_task_per_proc);
}

/*
 * Strong override of the weak default in the common kernel.c: share the kernel
 * high-half tables directly so each process does not rebuild the full
 * allocable-RAM direct map, and keep only the user half private. Reproduces the
 * `__aarch64__` clone_kernel_vm() that used to live inline in kernel.c.
 */
void arch_set_proc_vm(page_dir_entry_t* vm) {
    uint32_t kernel_l1_base = PAGE_ROOT_INDEX(KERNEL_BASE);

    memset(vm, 0, PAGE_DIR_SIZE);

    for(uint32_t i = kernel_l1_base; i < PAGE_DIR_NUM; i++) {
        vm[i] = _kernel_info.kernel_vm[i];
    }

    /*
     * Keep the common low DMA identity window available in the per-process
     * user half. Boards that need additional private mappings can extend this
     * through arch_clone_proc_vm().
     */
    map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base,
            _sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_SYS_DMA);
    if(arch_clone_proc_vm(vm, _kernel_info.kernel_vm) != 0)
        return;
    flush_dcache();
    flush_tlb();
}
