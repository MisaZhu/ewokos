#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>
#include <gic.h>

void ipi_enable(uint32_t core_id) {
	gic_irq_enable(core_id, 0);
}

void ipi_send(uint32_t core_id) {
	gic_gen_soft_irq(core_id, 0);
}

void ipi_clear(uint32_t core_id) {
}