#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

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
    else if(_sys_info.total_phy_mem_size >= 1ull*GB)
        ret = 8*MB;

#if defined(__aarch64__) && defined(PAGE_SIZE_64K)
    /*
     * With 64KB granules each per-process top-level page directory grows to
     * 64KB. The default 128 proc_vm_t entries alone consume about 8MB, so the
     * historic 8MB kmalloc pool on 1GB boards is no longer sufficient.
     */
    if(ret < 16*MB)
        ret = 16*MB;
#endif
    return ret;
}

static uint32_t get_dma_size(void) {
    uint32_t ret = 16*MB;

    if(_kernel_config.dma_size == 0) {
        if(_sys_info.total_phy_mem_size >= 8ull*GB)
            ret = 64*MB;
        else if(_sys_info.total_phy_mem_size >= 4ull*GB)
            ret = 64*MB;
        else if(_sys_info.total_phy_mem_size >= 2ull*GB)
            ret = 32*MB;
    }
    else {
        ret = _kernel_config.dma_size;
    }
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

void sys_info_config(void) {
    _sys_info.kmalloc_size = get_kmalloc_size();
    _sys_info.allocable_phy_mem_base = V2P(KMALLOC_END);

    if(_sys_info.sys_dma.size == 0)
        _sys_info.sys_dma.size = get_dma_size();
    if(_sys_info.shm_contig.size == 0)
        _sys_info.shm_contig.size = get_shm_contig_size();

    _sys_info.sys_dma.v_base = DMA_V_BASE;

#if defined(__aarch64__)
    /*
     * On boards with RAM well above 4GB, carve the identity-mapped
     * shm-contig + sys_dma windows from the TOP of RAM instead of right
     * after the kernel image. The user sbrk heap then grows contiguously
     * from the end of the ELF across the 4GB mark (apps can malloc >4GB)
     * without colliding with those user-half identity mappings, and small
     * processes keep 32-bit-safe low heap pointers.
     */
    if((_sys_info.phy_offset + _sys_info.total_usable_mem_size) >
            (6ull*GB + _sys_info.sys_dma.size + _sys_info.shm_contig.size)) {
        ewokos_addr_t ram_top = _sys_info.phy_offset +
            _sys_info.total_usable_mem_size;
        _sys_info.sys_dma.phy_base = ram_top - _sys_info.sys_dma.size;
        _sys_info.shm_contig.phy_base = _sys_info.sys_dma.phy_base -
            _sys_info.shm_contig.size;
        _sys_info.shm_contig.v_base = 0; //unused.
        /*the whole region between the kernel image and the high carve-outs
          stays allocatable*/
        _sys_info.allocable_phy_mem_top = _sys_info.shm_contig.phy_base;
        return;
    }
#endif

    _sys_info.sys_dma.phy_base = _sys_info.allocable_phy_mem_base;
    _sys_info.allocable_phy_mem_base += _sys_info.sys_dma.size;

    /* physically-contiguous slab for IPC_CONTIG shm segments, carved right
       after the dma window with a default size. kernel.conf "shm_contig_size"
       re-carves it (a fresh reservation, so it still lands after a relocated
       dma window) */
    _sys_info.shm_contig.phy_base = _sys_info.allocable_phy_mem_base;
    _sys_info.shm_contig.v_base = 0; //unused.
    _sys_info.allocable_phy_mem_base += _sys_info.shm_contig.size;
}