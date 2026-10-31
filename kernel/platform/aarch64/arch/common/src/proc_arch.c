/*
 * AArch64 process arch hooks.
 *
 * Strong overrides of the weak defaults in the common kernel/kernel/src/proc.c
 * (declared in <kernel/proc.h>). They reproduce the `__aarch64__` blocks that
 * used to live inline in the common proc.c so that file stays arch-neutral:
 *
 *   - TPIDR_EL0 is the user thread register libc's emulated TLS hangs its
 *     per-task block off; the context switch carries it explicitly because it
 *     is not part of context_t.
 *   - freshly mapped user pages are marked UXN (user execute-never): the user
 *     stack, and every page of the anonymous-mapping window whose PT_LOAD did
 *     not ask for PROT_EXEC.
 *   - fork's deep copy of that window re-creates each page with the exact
 *     permissions its parent entry already had.
 */
#include <kernel/proc.h>
#include <mm/mmu_arch.h>

ewokos_addr_t arch_proc_tls_base_read(void) {
	ewokos_addr_t v;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(v));
	return v;
}

void arch_proc_tls_base_write(ewokos_addr_t v) {
	__asm__ volatile("msr tpidr_el0, %0" :: "r"(v));
}

void arch_mark_pte_noexec(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    page_table_entry_t* pte = get_page_table_entry(vm, vaddr);
    if(pte != NULL) {
        pte->UXN = 1;
    }
}

void arch_mark_stack_pte_noexec(page_dir_entry_t* vm, ewokos_addr_t vaddr) {
    arch_mark_pte_noexec(vm, vaddr);
}

int32_t arch_clone_pte_prot(page_dir_entry_t* to_vm, ewokos_addr_t vaddr,
                void* page, page_dir_entry_t* from_vm) {
    page_table_entry_t* src = get_page_table_entry(from_vm, vaddr);
    page_table_entry_t* dst;
    uint32_t ap = AP_RW_RW;
    int noexec = 1;
    int knoexec = 0;

    /*
     * The caller resolved a physical address through the source entry, so it
     * is there; defaulting to writable/noexec anyway means a surprise can
     * never produce a page that is both writable and executable.
     */
    if(src != NULL) {
        ap      = (uint32_t)src->S2AP;
        noexec  = (int)src->UXN;
        knoexec = (int)src->PXN;
    }

    /* map_page_ref, not map_page: this is a fresh page the child owns alone,
       so it needs its own reference for unmap_page_ref to release later. */
    if(map_page_ref(to_vm, vaddr, V2P(page), ap, PTE_ATTR_WRBACK) != 0)
        return -1;

    /* set_pte_flags() resets PXN/UXN to 0 on every map, so the execute-never
       half of the source entry has to be re-applied afterwards - the same
       ordering arch_mark_pte_noexec() relies on. */
    dst = get_page_table_entry(to_vm, vaddr);
    if(dst != NULL) {
        if(noexec)
            dst->UXN = 1;
        if(knoexec)
            dst->PXN = 1;
    }
    return 0;
}
