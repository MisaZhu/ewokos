#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/system.h>
#include <kernel/kernel.h>

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 * Notice: virtual and physical address inputed must be all aliend by PAGE_SIZE !
 */
int32_t map_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr,
             ewokos_addr_t physical, uint32_t permissions, uint32_t pte_attr) {
    page_table_entry_t *page_table = 0;

    ewokos_addr_t page_dir_index = PAGE_DIR_INDEX(virtual_addr);
    ewokos_addr_t page_index = PAGE_INDEX(virtual_addr);

    bool new_table = (vm[page_dir_index].type == 0);
    /* if this page_dirEntry is not mapped before, map it to a new page table */
    if (new_table) {
        page_table = kalloc1k();
        if(page_table == NULL)
            return -1;

        memset(page_table, 0, PAGE_TABLE_SIZE);
#ifndef ARM_V7
        vm[page_dir_index].type = PAGE_DIR_2LEVEL_TYPE;
        vm[page_dir_index].sbz= 0;
        vm[page_dir_index].domain = 0;
        vm[page_dir_index].base = PAGE_TABLE_TO_BASE(V2P(page_table));
#endif
    }
    /* otherwise use the previously allocated page table */
    else {
        page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
    }

#ifdef ARM_V7
    /* Build the whole descriptor on the stack first, then publish it with a
     * single aligned 32-bit store. A concurrent walk must never see an entry
     * whose type is already valid while base/TEX/AP are not yet updated. */
    page_table_entry_t entry = {0};
    entry.type = SMALL_PAGE_TYPE;
    entry.base = PAGE_TO_BASE(physical);
    entry.ap = permissions;
    /* User-half entries are ASID-tagged (nG) so an address-space switch needs
     * no TLBI; the shared kernel half (VA >= KERNEL_BASE) stays global and
     * identical in every space. This nG split is the invariant that makes the
     * CONTEXTIDR/ASID switch in __set_translation_table_base_asid safe. */
    entry.ng = (virtual_addr < KERNEL_BASE) ? 1 : 0;
    set_pte_flags(&entry, pte_attr);

    page_table_entry_t old = page_table[page_index];
    if(old.type != 0 && (old.base != entry.base || old.tex != entry.tex ||
            old.cacheable != entry.cacheable || old.writeback != entry.writeback ||
            old.sharable != entry.sharable)) {
        /* When the physical page or memory type changes, revoke the old
         * mapping and finish the cross-core invalidate before installing
         * the new one. */
        page_table_entry_t invalid = {0};
        page_table[page_index] = invalid;
        dcache_flush_range(&page_table[page_index], sizeof(entry));
        __flush_tlb();
    }
    page_table[page_index] = entry;
    if(new_table) {
        /* Including the remaining invalid entries: publish the whole L2
         * table before pointing L1 at it. */
        dcache_flush_range(page_table, PAGE_TABLE_SIZE);
        page_dir_entry_t dir = {0};
        dir.type = PAGE_DIR_2LEVEL_TYPE;
        dir.base = PAGE_TABLE_TO_BASE(V2P(page_table));
        vm[page_dir_index] = dir;
        dcache_flush_range(&vm[page_dir_index], sizeof(dir));
    }
    else {
        dcache_flush_range(&page_table[page_index], sizeof(entry));
    }
#else
    /* map the virtual page to physical page in page table */
    page_table[page_index].type = SMALL_PAGE_TYPE;
    page_table[page_index].base = PAGE_TO_BASE(physical);
    page_table[page_index].ap = permissions;
    set_pte_flags(&page_table[page_index], pte_attr);
#endif
    return 0;
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr) {
    page_table_entry_t *page_table = 0;
    ewokos_addr_t page_dir_index = PAGE_DIR_INDEX(virtual_addr);
    ewokos_addr_t page_index = PAGE_INDEX(virtual_addr);
    if(vm[page_dir_index].type == 0)
        return;
    page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
#ifdef ARM_V7
    page_table_entry_t invalid = {0};
    page_table[page_index] = invalid;
    dcache_flush_range(&page_table[page_index], sizeof(invalid));
#else
    page_table[page_index].type = 0;
#endif
}

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    page_dir_entry_t *pdir = 0;
    page_table_entry_t *page = 0;
    ewokos_addr_t result = 0;
    ewokos_addr_t base_address = 0;

    pdir = (page_dir_entry_t*)((ewokos_addr_t) vm | ((virtual >> 20) << 2));
    if(pdir->type == 0)
        return 0;
    base_address = pdir->base << 10;
    page = (page_table_entry_t*)((ewokos_addr_t) base_address | ((virtual >> 10) & 0x3fc));
    page = (page_table_entry_t*)P2V(page);
    if(page->type == 0)
        return 0;
    result = (page->base << 12) | (virtual & 0xfff);
    return result;
}

/*
get page entry(virtual addr) by virtual address
*/
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    page_dir_entry_t *pdir = 0;
    page_table_entry_t *page = 0;
    ewokos_addr_t base_address = 0;

    pdir = (void *) ((ewokos_addr_t) vm | ((virtual >> 20) << 2));
    if(pdir->type == 0)
        return NULL;
    base_address = pdir->base << 10;
    page = (page_table_entry_t*)((ewokos_addr_t) base_address | ((virtual >> 10) & 0x3fc));
    page = (page_table_entry_t*)P2V(page);
    if(page->type == 0)
        return NULL;
    return page;
}


void free_page_tables(page_dir_entry_t *vm) {
    uint32_t kernel_dir_base = PAGE_DIR_INDEX(KERNEL_BASE);
    for (uint32_t i = 0; i < kernel_dir_base; i++) {
        if (vm[i].type != 0) {
            void *page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[i].base));
            if(page_table != NULL)
                kfree1k(page_table);
        }
    }
}
