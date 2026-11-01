#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
int32_t map_pages(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t pstart, ewokos_addr_t pend, uint32_t permissions, uint32_t pte_attr) {
    ewokos_addr_t physical_current = 0;
    ewokos_addr_t virtual_current = 0;

    ewokos_addr_t virtual_start = ALIGN_DOWN(vaddr, PAGE_SIZE);
    ewokos_addr_t physical_start = ALIGN_DOWN(pstart, PAGE_SIZE);
    ewokos_addr_t physical_end = ALIGN_UP( pend, PAGE_SIZE);

    /* iterate over pages and map each page */
    virtual_current = virtual_start;
    for (physical_current = physical_start; 
            physical_current < physical_end;
            physical_current += PAGE_SIZE) {
        if(map_page(vm, virtual_current, physical_current,
                    permissions, pte_attr) != 0)
            return -1;
        virtual_current += PAGE_SIZE;
    }
    return 0;
}

int32_t map_pages_size(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t pstart, uint32_t size, uint32_t permissions, uint32_t pte_attr) {
    return map_pages(vm, vaddr, pstart, pstart + size,
            permissions, pte_attr);
}

void unmap_pages(page_dir_entry_t *vm, ewokos_addr_t virtual_addr, uint32_t pages) {
    uint32_t i;
    for(i=0; i<pages; i++) {
        unmap_page(vm, virtual_addr + PAGE_SIZE*i);
    }
}

inline ewokos_addr_t resolve_kernel_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
    ewokos_addr_t phy = resolve_phy_address(vm, virtual);
    return P2V(phy);
}

inline int32_t map_page_ref(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t paddr, uint32_t permissions, uint32_t pte_attr) {
    if(map_page(vm, vaddr, paddr, permissions, pte_attr) != 0)
        return -1;
    uint32_t i = page_ref_index(paddr);
    if(i < _pages_ref.max)
        _pages_ref.refs[i]++;
    return 0;
}

inline void unmap_page_ref(page_dir_entry_t *vm, ewokos_addr_t virtual_addr) {
    ewokos_addr_t paddr = resolve_phy_address(vm, virtual_addr);
    uint32_t i = page_ref_index(paddr);
    if(i < _pages_ref.max) {
        if(_pages_ref.refs[i] > 0)
            _pages_ref.refs[i]--;
        if(_pages_ref.refs[i] == 0)
            kfree_page((void*)P2V(paddr));
    }
    unmap_page(vm, virtual_addr);
}

/* ------------------------------------------------------------------ */
/* anonymous user-mapping window (see DL_WINDOW_* in <mm/mmu.h>)        */
/* ------------------------------------------------------------------ */

#if DL_WINDOW_SLOTS > 0

/* The slot granularity is fixed by the user-visible ABI while PAGE_SIZE is a
   per-target kernel build choice, so check the one against the other here
   instead of letting a hypothetical large-page build divide a slot down to
   zero pages. File scope keeps it out of -Wunused-local-typedefs. */
typedef char dl_window_slot_page_check[
        (DL_WINDOW_SLOT % PAGE_SIZE == 0 && DL_WINDOW_PAGES_PER_SLOT > 0) ? 1 : -1];

static inline int dl_slot_used(const proc_space_t* space, uint32_t slot) {
    return (int)((space->dl_slot_used[slot >> 3] >> (slot & 7)) & 1);
}

static inline void dl_slot_mark(proc_space_t* space, uint32_t slot, int used) {
    if(used)
        space->dl_slot_used[slot >> 3] |= (uint8_t)(1u << (slot & 7));
    else
        space->dl_slot_used[slot >> 3] &= (uint8_t)~(1u << (slot & 7));
}

/* EWOK_PROT_* -> the AP[2:1] field. dl_prot_ok() rejects PROT_WRITE without
   PROT_READ first, so a writable mapping is always readable too - AArch64 has
   no write-only translation anyway. PROT_NONE keeps EL1's access, so the
   kernel's own P2V alias of the page is unaffected and only EL0 is locked
   out. */
static uint32_t dl_prot_to_ap(uint32_t prot) {
    if((prot & EWOK_PROT_WRITE) != 0)
        return AP_RW_RW;
    if((prot & EWOK_PROT_READ) != 0)
        return AP_RW_R;
    return AP_RW_D;
}

static int dl_prot_ok(uint32_t prot) {
    if((prot & ~(uint32_t)(EWOK_PROT_READ | EWOK_PROT_WRITE | EWOK_PROT_EXEC)) != 0)
        return 0;
    if((prot & EWOK_PROT_WRITE) != 0 && (prot & EWOK_PROT_READ) == 0)
        return 0;
    return 1;
}

/* [vaddr, vaddr+size) must be page-aligned and lie wholly inside the window.
   That range check is deliberately stricter than user_ptr_ok(): the window is
   a specific reservation, and accepting any user VA here would let a caller
   re-permission its own heap or stack. */
static int dl_range_ok(ewokos_addr_t vaddr, uint32_t size, ewokos_addr_t* end_out) {
    ewokos_addr_t end;
    if(size == 0)
        return 0;
    if((vaddr & (PAGE_SIZE - 1)) != 0)
        return 0;
    end = vaddr + size;
    if(end < vaddr)                       /* wrapped */
        return 0;
    if(vaddr < DL_WINDOW_BASE || end > DL_WINDOW_END)
        return 0;
    *end_out = end;
    return 1;
}

/*
 * Map one page the caller has just kalloc'd. map_page() resets PXN/UXN (see
 * set_pte_flags: a reused slot would otherwise keep the execute-never bit of
 * whatever was mapped there last), so the execute-never half of W^X has to be
 * applied after it - the same ordering the user-stack path relies on.
 */
static int32_t dl_map_page(page_dir_entry_t* vm, ewokos_addr_t vaddr,
                void* page, uint32_t prot) {
    if(map_page_ref(vm, vaddr, V2P(page), dl_prot_to_ap(prot), PTE_ATTR_WRBACK) != 0)
        return -1;
    if((prot & EWOK_PROT_EXEC) == 0)
        arch_mark_pte_noexec(vm, vaddr);
    return 0;
}

ewokos_addr_t dl_window_alloc(proc_space_t* space, uint32_t size, uint32_t prot) {
    uint32_t slots, pages, i, p, start;
    ewokos_addr_t vaddr;

    if(space == NULL || space->vm == NULL)
        return 0;
    if(!dl_prot_ok(prot))
        return 0;
    /* Also guards the round-up below against overflowing a uint32_t. */
    if(size == 0 || size > DL_WINDOW_SIZE)
        return 0;

    slots = (size + DL_WINDOW_SLOT - 1) / DL_WINDOW_SLOT;

    /* first fit over the slot bitmap; on a miss skip past the slot that broke
       the run rather than restarting the inner scan one slot later */
    start = DL_WINDOW_SLOTS;              /* DL_WINDOW_SLOTS means "none found" */
    for(i = 0; i + slots <= DL_WINDOW_SLOTS; i++) {
        uint32_t k = 0;
        while(k < slots && !dl_slot_used(space, i + k))
            k++;
        if(k == slots) {
            start = i;
            break;
        }
        i += k;
    }
    if(start >= DL_WINDOW_SLOTS)
        return 0;

    pages = slots * DL_WINDOW_PAGES_PER_SLOT;
    vaddr = DL_WINDOW_BASE + (ewokos_addr_t)start * DL_WINDOW_SLOT;

    for(p = 0; p < pages; p++) {
        void* page = kalloc_page();
        if(page == NULL)
            goto failed;
        /* POSIX: a fresh anonymous mapping reads as zero, and kalloc_page()
           hands back whatever the previous owner left. Zero through the
           kernel alias so it works even when the caller asked for PROT_NONE.
           Do it before mapping: the PTE may be read-only or execute-never. */
        memset(page, 0, PAGE_SIZE);
        if(dl_map_page(space->vm, vaddr + (ewokos_addr_t)p * PAGE_SIZE,
                    page, prot) != 0) {
            kfree_page(page);
            goto failed;
        }
    }

    for(i = 0; i < slots; i++)
        dl_slot_mark(space, start + i, 1);
    /* New mappings only: no stale TLB entry can exist for a slot the bitmap
       says was free, and dl_window_free() flushed when it released it. */
    flush_tlb_nosweep();
    return vaddr;

failed:
    for(i = 0; i < p; i++)
        unmap_page_ref(space->vm, vaddr + (ewokos_addr_t)i * PAGE_SIZE);
    flush_tlb();
    return 0;
}

int32_t dl_window_protect(proc_space_t* space, ewokos_addr_t vaddr,
                uint32_t size, uint32_t prot) {
    ewokos_addr_t v, end;

    if(space == NULL || space->vm == NULL)
        return -1;
    if(!dl_prot_ok(prot))
        return -1;
    if(!dl_range_ok(vaddr, size, &end))
        return -1;

    /* Validate the whole range before touching any PTE: a caller that gets
       -1 must not be left with half its mapping re-permissioned. */
    for(v = vaddr; v < end; v += PAGE_SIZE) {
        if(resolve_phy_address(space->vm, v) == 0)
            return -1;
    }

    for(v = vaddr; v < end; v += PAGE_SIZE) {
        ewokos_addr_t phy = resolve_phy_address(space->vm, v);
        /* map_page, NOT map_page_ref: the page is already counted once for
           this mapping and re-permissioning must not inflate its refcount. */
        map_page(space->vm, v, phy, dl_prot_to_ap(prot), PTE_ATTR_WRBACK);
        if((prot & EWOK_PROT_EXEC) == 0)
            arch_mark_pte_noexec(space->vm, v);
    }
    /*
     * A permission change on a live PTE does not publish itself. User-half
     * entries are ASID-tagged, so an address-space switch will not drop the
     * stale translation - the same reason proc_load_elf's RW->RO downgrade of
     * the process image ends in a full flush_tlb(). Without it a downgrade to
     * read-only can be ignored for as long as the entry stays cached, which is
     * the whole guarantee W^X is here to provide.
     */
    flush_tlb();
    return 0;
}

int32_t dl_window_free(proc_space_t* space, ewokos_addr_t vaddr, uint32_t size) {
    ewokos_addr_t v, end;
    uint32_t s;

    if(space == NULL || space->vm == NULL)
        return -1;
    if(!dl_range_ok(vaddr, size, &end))
        return -1;

    for(v = vaddr; v < end; v += PAGE_SIZE) {
        /* unmap_page_ref frees the backing page once its refcount reaches 0,
           so this returns the memory to kalloc as well as the VA. Skip holes:
           page_ref_index(0) would take a reference off physical page 0. */
        if(resolve_phy_address(space->vm, v) != 0)
            unmap_page_ref(space->vm, v);
    }

    /* Release only the slots the range covers whole. libc rounds an mmap()
       length up to a slot and records that rounded length, so in practice
       this is every slot touched; a partial release would otherwise leave the
       rest of a slot unallocatable with no way to find it again. */
    for(s = (uint32_t)((vaddr - DL_WINDOW_BASE) / DL_WINDOW_SLOT);
            s < DL_WINDOW_SLOTS &&
                DL_WINDOW_BASE + (ewokos_addr_t)s * DL_WINDOW_SLOT < end;
            s++) {
        ewokos_addr_t slot_lo = DL_WINDOW_BASE + (ewokos_addr_t)s * DL_WINDOW_SLOT;
        if(slot_lo >= vaddr && slot_lo + DL_WINDOW_SLOT <= end)
            dl_slot_mark(space, s, 0);
    }
    flush_tlb();
    return 0;
}

void dl_window_release(proc_space_t* space) {
    uint32_t b, k;

    if(space == NULL || space->vm == NULL)
        return;

    /* Byte-at-a-time so a space that never used the window costs one pass
       over 512 zero bytes and no page-table walk at all. */
    for(b = 0; b < (uint32_t)sizeof(space->dl_slot_used); b++) {
        if(space->dl_slot_used[b] == 0)
            continue;
        for(k = 0; k < 8; k++) {
            uint32_t slot = b * 8 + k;
            uint32_t p;
            if(slot >= DL_WINDOW_SLOTS)
                return;
            if(((space->dl_slot_used[b] >> k) & 1) == 0)
                continue;
            for(p = 0; p < DL_WINDOW_PAGES_PER_SLOT; p++) {
                ewokos_addr_t v = DL_WINDOW_BASE +
                        (ewokos_addr_t)slot * DL_WINDOW_SLOT +
                        (ewokos_addr_t)p * PAGE_SIZE;
                if(resolve_phy_address(space->vm, v) != 0)
                    unmap_page_ref(space->vm, v);
            }
            space->dl_slot_used[b] &= (uint8_t)~(1u << k);
        }
    }
    flush_tlb_nosweep();
}

int32_t dl_window_clone(proc_space_t* to, proc_space_t* from) {
    uint32_t b, k;

    if(to == NULL || from == NULL || to->vm == NULL || from->vm == NULL)
        return -1;

    /*
     * Deep copy, deliberately NOT the copy-on-write sharing proc_clone() uses
     * for the heap. copy_on_write() in irq.c only repairs faults inside
     * [rw_heap_base, heap_size) and the user stack, so a window page shared
     * read-only would take a write fault the kernel refuses to fix up and the
     * child would die; and repairing it by remapping AP_RW_RW would make a
     * page the loader had marked read-only-and-executable writable again,
     * undoing the W^X the window exists for. Fresh pages preserve each
     * mapping's own permissions exactly.
     */
    for(b = 0; b < (uint32_t)sizeof(from->dl_slot_used); b++) {
        if(from->dl_slot_used[b] == 0)
            continue;
        for(k = 0; k < 8; k++) {
            uint32_t slot = b * 8 + k;
            uint32_t p;
            if(slot >= DL_WINDOW_SLOTS)
                return 0;
            if(((from->dl_slot_used[b] >> k) & 1) == 0)
                continue;
            for(p = 0; p < DL_WINDOW_PAGES_PER_SLOT; p++) {
                ewokos_addr_t v = DL_WINDOW_BASE +
                        (ewokos_addr_t)slot * DL_WINDOW_SLOT +
                        (ewokos_addr_t)p * PAGE_SIZE;
                ewokos_addr_t phy = resolve_phy_address(from->vm, v);
                void* page;
                if(phy == 0)
                    continue;
                page = kalloc_page();
                if(page == NULL) {
                    dl_window_release(to);
                    return -1;
                }
                memcpy(page, (void*)P2V(phy), PAGE_SIZE);
                if(arch_clone_pte_prot(to->vm, v, page, from->vm) != 0) {
                    /* the hook maps the page itself, so on failure the page is
                       still ours to release */
                    kfree_page(page);
                    dl_window_release(to);
                    return -1;
                }
            }
            to->dl_slot_used[b] |= (uint8_t)(1u << k);
        }
    }
    flush_tlb_nosweep();
    return 0;
}

#endif /* DL_WINDOW_SLOTS > 0 */
