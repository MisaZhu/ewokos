#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

/*
 * BSP hook: opt in to placing DMA-shared contiguous runs (shm segments,
 * which the GPU/accelerators read and write) in the HIGH part of RAM on
 * boards well above 4GB, keeping the low 4GB free for one contiguous user
 * heap (apps can malloc >4GB). Only BSPs whose DMA masters are 64-bit clean
 * may return nonzero; the weak default keeps the legacy low layout (32-bit
 * DMA controllers / firmware mailbox bus addresses). The hard reachability
 * limit is always arch_dma_range() - this hook only picks placement inside
 * that range (see shm_init).
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
 * The BSP arch_dma_range() gives the hardware limit. kernel.conf may set the
 * extent: dma_base raises the range start, dma_size sets the span (intersected
 * with the hardware limit, never widened beyond it). With dma_size unset the
 * span defaults to DMA_SHM_RANGE_DEFAULT (1G). The range is a pure address
 * cap: dma_alloc draws runs on demand and sys_dma_alloc maps each buffer into
 * the high DMA_V_BASE window, so the span costs no per-process page tables
 * (the old whole-range per-fork identity pre-map was removed). Small boards
 * are clamped to arch_dma_range() automatically. The floor never drops below
 * the allocable heap base (kernel-core pages must not become DMA targets).
 * Reserved firmware/GPU pools inside the range are already kept out of the
 * free list by kalloc_arch, so contiguous scans skip them naturally.
 */
static ewokos_addr_t get_dma_default_size(void) {
    return DMA_SHM_RANGE_DEFAULT;
}

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
    ewokos_addr_t span = (_kernel_config.dma_size != 0) ?
        _kernel_config.dma_size : get_dma_default_size();
    ewokos_addr_t ct = base + ALIGN_DOWN(span, PAGE_SIZE);
    if(ct < top)
        top = ct;

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

    _sys_info.sys_dma.v_base = DMA_V_BASE;

    if(arch_reserve_memory_pools()) {
        /* BSP fully manages the physical layout (reserved GPU / firmware
           pools) and sets allocable_phy_mem_top itself. sys_dma is still a
           range constraint, not a carve-out: derive it from arch_dma_range
           (narrowed by kernel.conf) unless the BSP already published one. */
        if(_sys_info.sys_dma.size == 0)
            publish_sys_dma_range();
        return;
    }

    /*
     * sys_dma: a DMA-reachable range constraint over general RAM, NOT a
     * reserved pool - normal memory allocation uses these pages too.
     * kernel.conf dma_base/dma_size may narrow the BSP hardware range.
     */
    publish_sys_dma_range();
}
