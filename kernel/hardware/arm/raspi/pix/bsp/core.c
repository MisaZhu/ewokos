#include <kernel/core.h>

#ifdef KERNEL_SMP

#ifdef PI4
void cpu_core_ready(uint32_t core_id) {
	gic_init(MMIO_BASE + 0x1840000);
    ipi_enable(core_id);
    __irq_enable();
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}

#else

void cpu_core_ready(uint32_t core_id) {
	(void)core_id;
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}
#endif

#endif
