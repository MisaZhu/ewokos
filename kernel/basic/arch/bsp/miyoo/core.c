#include <kernel/core.h>
#include <gic.h>

void cpu_core_ready(uint32_t core_id) {
	gic_init();
	gic_irq_enable(core_id, 0);
	__irq_enable();
}

/*
extern uint32_t __smp_lock(int32_t* v);
extern uint32_t __smp_unlock(int32_t* v);
*/

void mcore_lock(int32_t* v) {
	//__smp_lock(v);
	while((*v) != 0) {
		_delay(1);
		__asm__ volatile("WFENE");
	}
	*v = 1;
	__asm__ volatile("DSB");
}

void mcore_unlock(int32_t* v) {
	//__smp_unlock(v);
	*v = 0;
	__asm__ volatile("DSB");
	__asm__ volatile("SEV");
}