#include <kernel/core.h>
#include <kernel/system.h>

#ifdef KERNEL_SMP

void cpu_core_ready(uint32_t core_id) {
	gic_init(MMIO_BASE, MMIO_BASE + 0x10000);
	set_vector_table(&interrupt_table_start);
	ipi_enable(core_id);
	__irq_enable();
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}

#endif
