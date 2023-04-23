#include <kernel/core.h>

void cpu_core_ready(uint32_t core_id) {
	(void)core_id;
}

inline uint32_t get_cpu_cores(void) {
	return 4;
}