#include <sys/mmio.h>
#include <sys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _mmio_base = 0;

uint32_t mmio_map(void) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);
	_mmio_base = syscall3(SYS_MEM_MAP,
			sysinfo.mmio.v_base,
			sysinfo.mmio.phy_base,
			sysinfo.mmio.size);
	return _mmio_base;
}

#ifdef __cplusplus
}
#endif

