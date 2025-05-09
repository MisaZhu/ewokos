#include <ewoksys/mmio.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _mmio_base = 0;

uint32_t mmio_map_offset(uint32_t offset, uint32_t size) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	if(size == 0 || (offset+size) > (sysinfo.mmio.phy_base+sysinfo.mmio.size))
		return 0;

	_mmio_base = sysinfo.mmio.v_base;
	syscall3(SYS_MEM_MAP,
			(ewokos_addr_t)sysinfo.mmio.v_base+offset,
			(ewokos_addr_t)sysinfo.mmio.phy_base+offset,
			(ewokos_addr_t)size);
	return sysinfo.mmio.v_base+offset;
}

uint32_t mmio_map(void) {
	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	_mmio_base = mmio_map_offset(0, sysinfo.mmio.size);
	return _mmio_base;
}

#ifdef __cplusplus
}
#endif

