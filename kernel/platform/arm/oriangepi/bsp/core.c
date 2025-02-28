#include <kernel/core.h>

#ifdef KERNEL_SMP

void cpu_core_ready(uint32_t core_id) {
    gic_init(MMIO_BASE + 0x3020000);
    gic_irq_enable(core_id, 0);
    __irq_enable();
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}

#endif
