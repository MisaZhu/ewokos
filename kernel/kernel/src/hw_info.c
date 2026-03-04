#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

#define KMALLOC_SIZE_DEF      (8*MB)
#define DMA_SIZE_DEF          (16*MB)

void sys_info_init(void) {
    sys_info_init_arch();

    _sys_info.kernel_base = KERNEL_BASE;

	_sys_info.mmio.v_base = MMIO_BASE;
    if(_sys_info.mmio.size > MMIO_MAX_SIZE)
        _sys_info.mmio.size = MMIO_MAX_SIZE;

    if(_sys_info.kmalloc_size == 0)
        _sys_info.kmalloc_size = KMALLOC_SIZE_DEF;

    _sys_info.allocable_phy_mem_base = V2P(KMALLOC_END);

    _sys_info.sys_dma.v_base = DMA_V_BASE;
	_sys_info.sys_dma.phy_base = _sys_info.allocable_phy_mem_base;
    if(_sys_info.sys_dma.size == 0)
    	_sys_info.sys_dma.size = DMA_SIZE_DEF;
	_sys_info.allocable_phy_mem_base += _sys_info.sys_dma.size;
}