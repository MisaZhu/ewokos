/*
 * RISC-V process arch hooks.
 *
 * Strong override of the weak default in the common kernel/kernel/src/proc.c
 * (declared in <kernel/proc.h>).
 *
 * Only arch_clone_pte_prot is provided. The anonymous-mapping (DL) window is
 * enabled on every architecture, so fork's deep copy of a parent's window pages
 * must re-create each page with the same access permission the parent entry
 * already had. Without this override the weak default returns -1 and any fork
 * of a process that mapped the window (via dlopen or plain mmap) fails.
 *
 * Execute-never is deliberately NOT implemented here: on riscv the window only
 * guarantees text=read-only / data=read-write. arch_mark_pte_noexec stays the
 * weak no-op, so the X bit of the 3-bit permission field is left as map_page
 * set it (both AP_RW_R -> R+X and AP_RW_RW -> RWX keep X). Read-only text is
 * still enforced because map_page writes the permission bits and the window
 * lies outside the range copy_on_write() repairs, so a store into it is a
 * genuine fault.
 */
#include <kernel/proc.h>
#include <mm/mmu_arch.h>

int32_t arch_clone_pte_prot(page_dir_entry_t* to_vm, ewokos_addr_t vaddr,
                void* page, page_dir_entry_t* from_vm) {
    /*
     * riscv's get_page_table_entry returns the base of the leaf (level0) table,
     * not the entry for vaddr, so index it with PAGE_INDEX to reach the PTE.
     */
    page_table_entry_t* table = get_page_table_entry(from_vm, (uint32_t)vaddr);
    page_table_entry_t* src = (table != NULL) ? &table[PAGE_INDEX(vaddr)] : NULL;
    uint32_t ap = AP_RW_RW;

    /*
     * Recover the AP_RW_* the parent entry was mapped with. set_pte_permission
     * encodes it as user = (permissions != AP_RW_D) and the W bit (permission
     * bit1) = (permissions == AP_RW_RW), so invert that here. The caller
     * resolved a physical address through the source entry, so it is valid;
     * defaulting to writable on a surprise can never lose the read-only bit of
     * a mapping that was actually read-only.
     */
    if(src != NULL && src->valid) {
        if(!src->user)
            ap = AP_RW_D;
        else if((src->permission & 2) == 0)
            ap = AP_RW_R;
        else
            ap = AP_RW_RW;
    }

    /* map_page_ref, not map_page: this is a fresh page the child owns alone, so
       it needs its own reference for unmap_page_ref to release later. */
    if(map_page_ref(to_vm, vaddr, V2P(page), ap, PTE_ATTR_WRBACK) != 0)
        return -1;
    return 0;
}
