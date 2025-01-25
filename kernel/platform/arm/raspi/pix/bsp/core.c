#include <kernel/core.h>
#include "hw_arch.h"

#ifdef KERNEL_SMP

void cpu_core_ready(uint32_t core_id) {
	if(_pi4) {
		gic_init(MMIO_BASE + 0x1840000);
		ipi_enable(core_id);
		__irq_enable();
	}
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}

#endif