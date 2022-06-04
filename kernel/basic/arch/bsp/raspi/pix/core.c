#include <kernel/core.h>

void cpu_core_ready(uint32_t core_id) {
	(void)core_id;
}

extern uint32_t __smp_lock(int32_t* v);
extern uint32_t __smp_unlock(int32_t* v);

void mcore_lock(int32_t* v) {
	__smp_lock(v);
}

void mcore_unlock(int32_t* v) {
	__smp_unlock(v);
}