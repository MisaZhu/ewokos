#include <kernel/core.h>
#include <kernel/system.h>
#include <stddef.h>

core_t _cpu_cores[CPU_MAX_CORES];

#ifdef KERNEL_SMP

extern uint32_t __core_id(void);
extern uint32_t __cpu_cores(void);
extern uint32_t x86_apic_id_to_core_id(uint32_t apic_id) __attribute__((weak));

inline uint32_t get_core_id(void) {
	uint32_t core_id = __core_id();
#ifdef __x86_64__
	if (x86_apic_id_to_core_id != NULL) {
		core_id = x86_apic_id_to_core_id(core_id);
	}
#endif
	return core_id;
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
