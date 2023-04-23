#include <kernel/smp/ipi.h>
#include <kernel/hw_info.h>
#include <gic.h>

static inline void dsb(void){
    	__asm("dsb");
}


void ipi_enable(uint32_t core_id) {
	gic_irq_enable(core_id, 0);
	dsb();
}

void ipi_send(uint32_t core_id) {
	//if(core_id)
	//	printf("%s %d\n", __func__, core_id);
	gic_gen_soft_irq(core_id, 0);
}

void ipi_clear(uint32_t core_id) {
}