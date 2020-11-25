#include <kernel/core.h>
#include <kernel/system.h>

#ifdef KERNEL_SMP

core_t _cpu_cores[CPU_MAX_CORES];

static uint32_t _multi_core_flag = 0x0;
inline void start_multi_cores(uint32_t cores) {
	_multi_core_flag = 0x12345678;
	uint32_t i;
	for(i=0; i<CPU_MAX_CORES; i++) {
		_cpu_cores[i].actived = 0;
	}
	_cpu_cores[0].actived = 1;
}

inline int32_t multi_cores_ready(void) {
	if(_multi_core_flag != 0x12345678)
		return -1;

	int32_t i = get_cpu_id();
	if(i < CPU_MAX_CORES)
		_cpu_cores[i].actived = 1;
	return 0;
}

#endif

