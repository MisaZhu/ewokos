#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>

void ipi_enable_pi4(uint32_t core_id) {
	 gic_irq_enable(core_id, 0);
}

void ipi_send_pi4(uint32_t core_id) {
	gic_gen_soft_irq(core_id, 0);
}

void ipi_clear_pi4(uint32_t core_id) {
}