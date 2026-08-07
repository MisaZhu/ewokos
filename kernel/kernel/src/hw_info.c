#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

static uint32_t get_kmalloc_size(void) {
    uint32_t ret = 8*MB;

    if(_sys_info.total_phy_mem_size >= 8ull*GB)
        ret = 64*MB;
    else if(_sys_info.total_phy_mem_size >= 4ull*GB)
        ret = 32*MB;
    else if(_sys_info.total_phy_mem_size >= 2ull*GB)
        ret = 16*MB;
    return ret;
}

static uint32_t get_dma_size(void) {
    uint32_t ret = 16*MB;

    if(_sys_info.total_phy_mem_size >= 8ull*GB)
        ret = 128*MB;
    else if(_sys_info.total_phy_mem_size >= 4ull*GB)
        ret = 64*MB;
    else if(_sys_info.total_phy_mem_size >= 2ull*GB)
        ret = 32*MB;
    return ret;
}

void sys_info_init(void) {
    sys_info_init_arch();

    _sys_info.kernel_base = KERNEL_BASE;

	_sys_info.mmio.v_base = MMIO_BASE;
    if(_sys_info.mmio.size > MMIO_MAX_SIZE)
        _sys_info.mmio.size = MMIO_MAX_SIZE;

    if(_sys_info.kmalloc_size == 0)
        _sys_info.kmalloc_size = get_kmalloc_size();

    _sys_info.allocable_phy_mem_base = V2P(KMALLOC_END);

    _sys_info.sys_dma.v_base = DMA_V_BASE;
	_sys_info.sys_dma.phy_base = _sys_info.allocable_phy_mem_base;
    if(_sys_info.sys_dma.size == 0)
    	_sys_info.sys_dma.size = get_dma_size();
	_sys_info.allocable_phy_mem_base += _sys_info.sys_dma.size;

    _sys_info.page_size = PAGE_SIZE;
}