#include <kernel/kernel.h>
#include <kernel/hw_info.h>
sys_info_t _sys_info;

void sys_info_init(void) {
    sys_info_init_arch();

    _sys_info.kernel_base = KERNEL_BASE;

	_sys_info.mmio.v_base = MMIO_BASE;
    if(_sys_info.mmio.size > MMIO_MAX_SIZE)
        _sys_info.mmio.size = MMIO_MAX_SIZE;

    _sys_info.kmalloc_size = KMALLOC_END - KMALLOC_BASE;
}