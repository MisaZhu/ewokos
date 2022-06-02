#include <kernel/core.h>
#include <gic.h>

void cpu_core_ready(uint32_t core_id) {
	gic_init();
	gic_irq_enable(core_id, 0);
	__irq_enable();
}
