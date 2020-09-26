#include <stdint.h>
#include <kernel/hw_info.h>

#ifdef SDC
int32_t load_init_sdc(void);
int32_t load_init_proc(void) {
	_sys_info.kfs = 0;
	return load_init_sdc();
}
#else
int32_t load_init_kfs(void);
int32_t load_init_proc(void) {
	_sys_info.kfs = 1;
	return load_init_kfs();
}
#endif
