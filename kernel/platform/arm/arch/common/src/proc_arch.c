/*
 * ARM (32-bit) process arch hooks.
 *
 * Strong override of the weak default in the common kernel/kernel/src/proc.c
 * (declared in <kernel/proc.h>). Shared by every ARM version (v5/v6/v7).
 *
 * Only arch_clone_pte_prot is provided. The anonymous-mapping (DL) window is
 * enabled on every architecture, so fork's deep copy of a parent's window pages
 * must re-create each page with the same access permission the parent entry
 * already had. Without this override the weak default returns -1 and any fork
 * of a process that mapped the window (via dlopen or plain mmap) fails.
 *
 * Execute-never (XN) is deliberately NOT implemented here: on arm32 the window
 * only guarantees text=read-only / data=read-write. arch_mark_pte_noexec stays
 * the weak no-op (ARMv5 has no XN bit at all, and v6/v7 are left executable too
 * for uniformity), so a page's execute permission is whatever map_page leaves.
 * Read-only text is still enforced because map_page writes the ap field and the
 * window lies outside the range copy_on_write() repairs, so a store into it is a
 * genuine fault.
 */
#include <kernel/proc.h>
#include <mm/mmu_arch.h>

int32_t arch_clone_pte_prot(page_dir_entry_t* to_vm, ewokos_addr_t vaddr,
                void* page, page_dir_entry_t* from_vm) {
    page_table_entry_t* src = get_page_table_entry(from_vm, vaddr);
    uint32_t ap = AP_RW_RW;

    /*
     * arm's map_page stores the AP_RW_* value verbatim in the PTE's ap field,
     * so the parent's permission is read straight back out - no bit inversion
     * needed. The caller resolved a physical address through the source entry,
     * so it is present; defaulting to writable on a surprise can never lose the
     * read-only bit of a mapping that was actually read-only.
     */
    if(src != NULL)
        ap = src->ap;

    /* map_page_ref, not map_page: this is a fresh page the child owns alone, so
       it needs its own reference for unmap_page_ref to release later. */
    if(map_page_ref(to_vm, vaddr, V2P(page), ap, PTE_ATTR_WRBACK) != 0)
        return -1;
    return 0;
}
