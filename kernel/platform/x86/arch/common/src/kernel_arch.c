/*
 * x86_64 kernel arch hooks.
 *
 * Strong override of the weak no-op default in the common
 * kernel/kernel/src/kernel.c (declared in <kernel/kernel.h>). It reproduces the
 * `__x86_64__` block that used to sit at the top of _kernel_entry_c(): mask
 * interrupts before touching anything else. Keeping it here lets the common
 * kernel.c stay free of architecture #if macros.
 */
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>

void arch_kernel_entry_early(void) {
    __asm__ volatile("cli");
}

static inline page_table_entry_t* entry_to_table_local(page_table_entry_t* entry) {
    return (page_table_entry_t*)P2V((ewokos_addr_t)(entry->Address << 12));
}

/*
 * Strong override of the weak default in the common kernel.c: give each process
 * a private user half but share the kernel high-half PDP entries. Reproduces
 * the `__x86_64__` clone_kernel_vm() that used to live inline in kernel.c.
 */
void arch_set_proc_vm(page_dir_entry_t* vm) {
    page_table_entry_t* kernel_pdpt;
    page_table_entry_t* proc_pdpt;

    memset(vm, 0, PAGE_DIR_SIZE);
    flush_dcache();

    proc_pdpt = kalloc_page();
    if (proc_pdpt == NULL) {
        return;
    }
    memset(proc_pdpt, 0, PAGE_TABLE_SIZE);

    vm[0].value = 0;
    vm[0].present = 1;
    vm[0].rw = 1;
    vm[0].us = 1;
    vm[0].Address = (uint64_t)V2P(proc_pdpt) >> 12;

    kernel_pdpt = entry_to_table_local(&_kernel_info.kernel_vm[0]);
    proc_pdpt[2] = kernel_pdpt[2];
    proc_pdpt[3] = kernel_pdpt[3];

    // Keep the low DMA identity window available while leaving the rest of
    // the user half private for per-process mappings.
    map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base,
            _sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_NOCACHE);
    flush_tlb();
}

/*
 * The x86 AP trampoline passes the target logical core id explicitly in the
 * first argument register. Prefer it over the APIC-id based lookup so each AP
 * binds to the correct per-core idle slot during early bring-up.
 */
uint32_t arch_slave_resolve_core(uint32_t boot_core_id, uint32_t detected_cid) {
    if (boot_core_id < _sys_info.cores) {
        return boot_core_id;
    }
    return detected_cid;
}
