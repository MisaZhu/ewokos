#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

/*
 * BSP hook: opt in to carving the reserved shm_contig slab from the top of
 * RAM instead of right after the kernel image. That frees the low 4GB for
 * one contiguous user heap (apps can malloc >4GB). sys_dma is not carved
 * here anymore - it is a range constraint over general RAM (arch_dma_range
 * keeps DMA buffers inside their masters' address reach regardless of how
 * far the heap grows). Weak default: keep the legacy low layout.
 */
__attribute__((weak))
int32_t arch_relocate_dma_high(void) {
    return 0;
}

__attribute__((weak))
int32_t arch_reserve_memory_pools(void) {
    return 0;
}

/*
 * DMA-reachable physical range. sys_dma is no longer a reserved carve-out:
 * it is a page-aligned address constraint [base, top) that dynamic DMA
 * allocations must land inside, while the pages themselves stay part of
 * general allocable RAM. The weak default spans the whole allocable range
 * (coherent / 64-bit-clean DMA masters, e.g. QEMU virt). Boards whose DMA
 * masters only reach a subset of RAM override this (raspi5 VPU first 1GB,
 * orin 32-bit DMA below 4GB).
 */
__attribute__((weak))
void arch_dma_range(ewokos_addr_t* base, ewokos_addr_t* top) {
    *base = _sys_info.allocable_phy_mem_base;
    *top = _sys_info.allocable_phy_mem_top;
}

__attribute__((weak))
int32_t arch_clone_proc_vm(page_dir_entry_t* vm, page_dir_entry_t* kernel_vm) {
    (void)vm;
    (void)kernel_vm;
    return 0;
}

static uint32_t get_kmalloc_size(void) {
    if(_kernel_config.kmalloc_size > 0) 
        return _kernel_config.kmalloc_size;
   
    uint32_t ret = 4*MB;

    if(_sys_info.total_phy_mem_size >= 8ull*GB)
        ret = 64*MB;
    else if(_sys_info.total_phy_mem_size >= 4ull*GB)
        ret = 32*MB;
    else if(_sys_info.total_phy_mem_size >= 2ull*GB)
        ret = 16*MB;
    else if(_sys_info.total_phy_mem_size >= 512*MB)
        ret = 8*MB;

#if defined(PAGE_SIZE_64K)
    /*
     * With 64KB granules each per-process top-level page directory grows to
     * 64KB. The default 128 proc_vm_t entries alone consume about 8MB, so the
     * historic 8MB kmalloc pool on 1GB boards is no longer sufficient.
     * PAGE_SIZE_64K is a page-granule config macro (only set by BSPs that build
     * with 64KB pages), not an architecture discriminator.
     */
    if(ret < 16*MB)
        ret = 16*MB;
#endif
    return ret;
}

static uint32_t get_shm_contig_size(void) {
    uint32_t ret = _kernel_config.shm_contig_size;

    /*if(_kernel_config.shm_contig_size == 0) {
        if(_sys_info.total_phy_mem_size >= 8ull*GB)
            ret = 64*MB;
        else if(_sys_info.total_phy_mem_size >= 4ull*GB)
            ret = 16*MB;
        else if(_sys_info.total_phy_mem_size >= 2ull*GB)
            ret = 8*MB;
    }
    */
    return ret;
}

void sys_info_init(void) {
    sys_info_init_arch();

    _sys_info.kernel_base = KERNEL_BASE;

    _sys_info.mmio.v_base = MMIO_BASE;
    if(_sys_info.mmio.size > MMIO_MAX_SIZE)
        _sys_info.mmio.size = MMIO_MAX_SIZE;

    _sys_info.page_size = PAGE_SIZE;

    if(_sys_info.kmalloc_size == 0)
        _sys_info.kmalloc_size = 4*MB; //tmp size for kernel start
}

/*
 * Publish sys_dma: a DMA-reachable physical address range (page-aligned),
 * NOT a reserved pool. Its pages remain general allocable RAM - normal
 * kalloc/kmalloc/user allocation uses them too; dma_alloc merely MUST draw
 * contiguous runs from inside the range (compacting via page_move when
 * fragmented) so limited DMA masters can reach its buffers.
 *
 * The BSP arch_dma_range() gives the hardware limit. kernel.conf may narrow
 * it: dma_base raises the range start, dma_size caps the extent (0 = whole
 * arch range). The configured range is intersected with the hardware limit,
 * never widened beyond it, and the floor never drops below the allocable
 * heap base (kernel-core pages must not become DMA targets). Reserved slabs
 * inside the range (shm_contig etc.) are already kept out of the free list
 * by kalloc_arch, so contiguous scans skip them naturally.
 */
static void publish_sys_dma_range(void) {
    ewokos_addr_t db, dt;
    arch_dma_range(&db, &dt);
    ewokos_addr_t base = ALIGN_UP(db, PAGE_SIZE);
    ewokos_addr_t top = ALIGN_DOWN(dt, PAGE_SIZE);

    if(_kernel_config.dma_base != 0) {
        ewokos_addr_t cb = ALIGN_UP(_kernel_config.dma_base, PAGE_SIZE);
        if(cb > base)
            base = cb;
    }
    if(base < _sys_info.allocable_phy_mem_base)
        base = ALIGN_UP(_sys_info.allocable_phy_mem_base, PAGE_SIZE);
    if(top < base)
        top = base;
    if(_kernel_config.dma_size != 0) {
        ewokos_addr_t ct = base +
            ALIGN_DOWN(_kernel_config.dma_size, PAGE_SIZE);
        if(ct < top)
            top = ct;
    }

    _sys_info.sys_dma.phy_base = base;
    _sys_info.sys_dma.size = top - base;
    _sys_info.sys_dma.v_base = DMA_V_BASE;
}

void sys_info_config(void) {
    /* load_kernel_config() has already clamped this to the BSP's hardware
       limit.  Runtime scheduling and secondary-core bring-up use
       _sys_info.cores, so keep it in sync with the configured active count. */
    _sys_info.cores = _kernel_config.cores;

    _sys_info.kmalloc_size = get_kmalloc_size();
    _sys_info.allocable_phy_mem_base = V2P(KMALLOC_END);

    if(_sys_info.shm_contig.size == 0)
        _sys_info.shm_contig.size = get_shm_contig_size();
    /* the contig shm VA window (SHM_CONTIG_BASE..SHM_BASE) is a static
       reservation that also anchors KMALLOC_VM below it; a slab larger than
       that window could never be fully mapped, so cap it here to keep the
       physical carve and the virtual window consistent. */
    if(_sys_info.shm_contig.size > SHM_CONTIG_VA_MAX)
        _sys_info.shm_contig.size = SHM_CONTIG_VA_MAX;

    _sys_info.sys_dma.v_base = DMA_V_BASE;

    if(arch_reserve_memory_pools()) {
        /* BSP fully manages the physical layout (reserved shm_contig / GPU
           pools) and sets allocable_phy_mem_top itself. sys_dma is still a
           range constraint, not a carve-out: derive it from arch_dma_range
           (narrowed by kernel.conf) unless the BSP already published one. */
        if(_sys_info.sys_dma.size == 0)
            publish_sys_dma_range();
        return;
    }

    /*
     * On boards with RAM well above 4GB, optionally carve the reserved
     * shm-contig slab from the TOP of RAM instead of right after the kernel
     * image. The user sbrk heap then grows contiguously from the end of the
     * ELF across the 4GB mark (apps can malloc >4GB). BSPs opt in through the
     * weak arch_relocate_dma_high() hook (default 0 keeps the legacy low
     * layout). sys_dma is no longer carved here - it is a range constraint
     * over general RAM established below.
     */
    if(arch_relocate_dma_high() &&
            (_sys_info.phy_offset + _sys_info.total_usable_mem_size) >
            (6ull*GB + _sys_info.shm_contig.size)) {
        ewokos_addr_t ram_top = _sys_info.phy_offset +
            _sys_info.total_usable_mem_size;
        _sys_info.shm_contig.phy_base = ram_top - _sys_info.shm_contig.size;
        _sys_info.shm_contig.v_base = 0; //unused.
        /*the whole region between the kernel image and the high carve-out
          stays allocatable*/
        _sys_info.allocable_phy_mem_top = _sys_info.shm_contig.phy_base;
    }
    else {
        /* physically-contiguous slab for IPC_CONTIG shm segments, carved
           right after the kernel image. kernel.conf "shm_contig_size"
           re-carves it. */
        _sys_info.shm_contig.phy_base = _sys_info.allocable_phy_mem_base;
        _sys_info.shm_contig.v_base = 0; //unused.
        _sys_info.allocable_phy_mem_base += _sys_info.shm_contig.size;
    }

    /*
     * sys_dma: a DMA-reachable range constraint over general RAM, NOT a
     * reserved pool - normal memory allocation uses these pages too.
     * kernel.conf dma_base/dma_size may narrow the BSP hardware range.
     */
    publish_sys_dma_range();
}
