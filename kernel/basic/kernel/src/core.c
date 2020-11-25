#include <kernel/core.h>

#ifdef KERNEL_SMP
static uint32_t _slave_core_flag = 0x0;
inline void start_slave_cores(void) {
	_slave_core_flag = 0x12345678;
}

inline int32_t slave_cores_ready(void) {
	if(_slave_core_flag == 0x12345678)
		return 0;
	return -1;
}

#endif

