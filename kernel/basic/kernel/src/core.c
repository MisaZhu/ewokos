#include <kernel/core.h>
#include <kernel/system.h>

#ifdef KERNEL_SMP
extern uint32_t __core_id(void);
extern uint32_t __cpu_cores(void);

uint32_t get_core_id(void) {
	return __core_id();
}

uint32_t get_cpu_cores(void) {
	return __cpu_cores();
}

core_t _cpu_cores[CPU_MAX_CORES];

static uint32_t _multi_core_flag = 0x0;
inline void start_multi_cores(uint32_t cores) {
	(void)cores;

	uint32_t i;
	for(i=0; i<CPU_MAX_CORES; i++) {
		_cpu_cores[i].actived = 0;
	}
	_cpu_cores[0].actived = 1;
	_multi_core_flag = 0x12345678;
}

inline int32_t multi_cores_ready(void) {
	if(_multi_core_flag != 0x12345678)
		return -1;

	int32_t i = get_core_id();
	if(i < CPU_MAX_CORES)
		_cpu_cores[i].actived = 1;
	return 0;
}

extern uint32_t __smp_lock(int32_t* v);
extern uint32_t __smp_unlock(int32_t* v);

void mcore_lock(int32_t* v) {
	__smp_lock(v);
}

void mcore_unlock(int32_t* v) {
	__smp_unlock(v);
}

#else

void mcore_lock(int32_t* v) {
	(void)v;
}

void mcore_unlock(int32_t* v) {
	(void)v;
}

uint32_t get_core_id(void) {
	return 0;
}

uint32_t get_cpu_cores(void) {
	return 1;
}

#endif

