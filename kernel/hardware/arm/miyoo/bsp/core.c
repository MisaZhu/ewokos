#include <kernel/core.h>
#include <gic.h>

#ifdef KERNEL_SMP
void cpu_core_ready(uint32_t core_id) {
	gic_init(0);
	gic_irq_enable(core_id, 0);
	__irq_enable();
}

inline uint32_t get_cpu_cores(void) {
	return 2;
}
#endif
