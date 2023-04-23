#include <kernel/core.h>
#include <kernel/system.h>

core_t _cpu_cores[CPU_MAX_CORES];

#ifdef KERNEL_SMP

extern uint32_t __core_id(void);
extern uint32_t __cpu_cores(void);

inline uint32_t get_core_id(void) {
	return __core_id();
}

extern uint32_t __smp_lock(int32_t* v);
extern uint32_t __smp_unlock(int32_t* v);

inline void mcore_lock(int32_t* v) {
	__smp_lock(v);
}

inline void mcore_unlock(int32_t* v) {
	__smp_unlock(v);
}

#endif