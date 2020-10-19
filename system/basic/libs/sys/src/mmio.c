#include <sys/mmu.h>
#include <sys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _mmio_base = 0;

uint32_t mmio_map(bool cache) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	uint32_t size = sysinfo.mmio.size;
	if(cache) {
		size |= 0x80000000;
	}

	_mmio_base = syscall3(SYS_MEM_MAP,
			sysinfo.mmio.v_base,
			sysinfo.mmio.phy_base,
			size);
	return _mmio_base;
}

#ifdef __cplusplus
}
#endif

