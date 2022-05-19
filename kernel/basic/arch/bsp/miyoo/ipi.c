#include <kernel/smp/ipi.h>
#include <bcm283x/ipi.h>

#ifdef KERNEL_SMP

void ipi_enable(uint32_t core_id) {
	bcm283x_ipi_enable(core_id);
}

void ipi_clear(uint32_t core_id) {
	bcm283x_ipi_clear(core_id);
}

void ipi_send(uint32_t core_id) {
	bcm283x_ipi_send(core_id);
}

#endif
