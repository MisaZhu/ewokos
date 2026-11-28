#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <mm/dma.h>
#include <mm/shm.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <sysinfo.h>

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
    /* Unmapped hole (e.g. a heap page released by SYS_MALLOC_RELEASE and
     * later swept by a tail shrink): nothing to deref. Without this guard
     * the refcount of physical page 0 would be decremented instead. */
    if(paddr == 0) {
        unmap_page(vm, virtual_addr);
        return;
    }
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
/* physical page migration (page_move)                                  */
/* ------------------------------------------------------------------ */

/*
 * Arch hook: walk every leaf PTE of vm and count the ones mapping phy_from;
 * with apply != 0 also retarget them to phy_to, keeping the virtual address
 * and every attribute bit intact. Two classes of mapping are deliberately
 * left alone because they are physical-address windows, not references to
 * the page's content:
 *   - identity mappings (VA == phy_from), e.g. the low sys_dma window;
 *   - the kernel direct-map alias (VA == P2V(phy_from)), which must keep
 *     resolving to phy_from so P2V() stays a fixed physical window after
 *     the source page is recycled.
 * Returns the number of matching leaf PTEs, or -1 when a mapping cannot be
 * retargeted page-exactly (a block descriptor covering phy_from). The weak
 * default means "arch unsupported"; aarch64 overrides it in mmu_arch.c.
 */
__attribute__((weak)) int32_t arch_retarget_ptes(page_dir_entry_t* vm,
        ewokos_addr_t phy_from, ewokos_addr_t phy_to, int apply) {
    (void)vm; (void)phy_from; (void)phy_to; (void)apply;
    return -1;
}

/* Weak no-op: arches without a leaf-PTE revoke walk report "nothing revoked"
 * (0), so SYS_FREE_PHY_CONTIG still returns the run but the caller must drop
 * its own SYS_MEM_MAP_NOCACHE mapping first. aarch64 overrides it to invalidate user
 * PTEs by physical range. */
__attribute__((weak)) int32_t arch_unmap_ptes_phy(page_dir_entry_t* vm,
        ewokos_addr_t phy_base, ewokos_addr_t phy_size) {
    (void)vm; (void)phy_base; (void)phy_size;
    return 0;
}

/* Weak no-op: arches whose PTE_ATTR_SYS_DMA equals WRBACK (or that lack a
 * per-page direct-map attribute hook) need no flip. aarch64 overrides it. */
__attribute__((weak)) void arch_set_direct_map_attr(ewokos_addr_t paddr,
        uint32_t pte_attr) {
    (void)paddr; (void)pte_attr;
}

/* Walk the kernel vm plus every process address space. Threads share their
 * owner's space, so visiting TASK_TYPE_PROC entries only covers each space
 * exactly once. */
static int32_t page_move_walk(ewokos_addr_t phy_from, ewokos_addr_t phy_to,
        int apply) {
    int32_t total = arch_retarget_ptes(_kernel_info.kernel_vm,
            phy_from, phy_to, apply);
    if(total < 0)
        return -1;
    for(int32_t pid = 0; pid < (int32_t)_kernel_config.max_task_num; pid++) {
        proc_t* p = proc_get(pid);
        if(p == NULL || p->info.type != TASK_TYPE_PROC ||
                p->space == NULL || p->space->vm == NULL ||
                p->space->vm == _kernel_info.kernel_vm)
            continue;
        int32_t n = arch_retarget_ptes(p->space->vm, phy_from, phy_to, apply);
        if(n < 0)
            return -1;
        total += n;
    }
    return total;
}

/*
 * Move one physical page from phy_from to phy_to:
 *   1. copy the content through the kernel direct map;
 *   2. retarget EVERY PTE mapping phy_from (kernel vm plus every process
 *      address space) to phy_to - all virtual addresses stay unchanged;
 *   3. transfer the page refcount and return phy_from to the allocator.
 * phy_to must be a fresh page the caller owns (e.g. kalloc_page()); it is
 * consumed by this call. Returns the number of retargeted PTEs (>= 0), or
 * -1 on bad arguments / unsupported arch, in which case nothing is changed
 * and both pages stay with the caller.
 *
 * Caller contract: kernel lock held and the page quiescent - until the final
 * flush_tlb() completes, another core may still translate through a stale
 * TLB entry for phy_from, so concurrent stores to the page would be lost.
 * DMA-owned/pinned pages and live shm runs are refused: their
 * physical runs are tracked by the DMA/shm allocators, which a PTE retarget
 * alone cannot update.
 */
int32_t page_move(ewokos_addr_t phy_from, ewokos_addr_t phy_to) {
    if((phy_from & (PAGE_SIZE - 1)) != 0 || (phy_to & (PAGE_SIZE - 1)) != 0)
        return -1;
    if(phy_from == phy_to || _pages_ref.refs == NULL)
        return -1;
    uint32_t fi = page_ref_index(phy_from);
    uint32_t ti = page_ref_index(phy_to);
    if(fi >= _pages_ref.max || ti >= _pages_ref.max)
        return -1;
    /* DMA-owned or device-pinned storage cannot be retargeted by PTE alone */
    if(!dma_page_movable(phy_from))
        return -1;
    /* nor can a run backing a live shm segment */
    if(!shm_page_movable(phy_from))
        return -1;

    /* Validate every address space before touching a single PTE so a refusal
       can never leave half the mappings retargeted. */
    int32_t total = page_move_walk(phy_from, phy_to, 0);
    if(total < 0)
        return -1;

    memcpy((void*)P2V(phy_to), (void*)P2V(phy_from), PAGE_SIZE);

    /* Apply pass: the validation above ran under the same kernel lock with
       no mapping changes in between, so it cannot fail here. */
    page_move_walk(phy_from, phy_to, 1);

    /* The mappings now reference phy_to: move the refcount with them. */
    _pages_ref.refs[ti] += _pages_ref.refs[fi];
    _pages_ref.refs[fi] = 0;

    /* Publish to every core before phy_from can be recycled. User-half
       entries are ASID-tagged, so an address-space switch alone would not
       drop the stale translations. */
    flush_tlb();
    kfree_page((void*)P2V(phy_from));
    return total;
}

/* ------------------------------------------------------------------ */
/* physically-contiguous allocation with compaction (mm_alloc_contig)   */
/* ------------------------------------------------------------------ */

/*
 * Contiguous physical run inside [phy_base, phy_base + phy_size), with a
 * page_move compaction fallback when no free run exists (the same organizing
 * dma_alloc uses). phy_size == 0 leaves the upper end open. All page-aligned:
 * size rounds up, phy_base rounds up, the grant is a whole-page run. Returns
 * the physical base, or 0. Caller holds the kernel lock, pages quiescent.
 */
ewokos_addr_t mm_alloc_contig(uint32_t size, ewokos_addr_t phy_base,
        ewokos_addr_t phy_size) {
    size = ALIGN_UP(size, PAGE_SIZE);
    if(size == 0)
        return 0;
    phy_base = ALIGN_UP(phy_base, PAGE_SIZE);
    ewokos_addr_t phy_hi = 0;
    if(phy_size != 0) {
        phy_hi = phy_base + phy_size;
        if(phy_hi < phy_base) /* wrap-around */
            return 0;
    }
    void* base = kalloc_contig_compact(size / PAGE_SIZE, phy_base, phy_hi);
    if(base == NULL)
        return 0;
    return V2P((ewokos_addr_t)base);
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
