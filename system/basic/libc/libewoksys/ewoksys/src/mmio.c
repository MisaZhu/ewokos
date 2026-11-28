#include <ewoksys/mmio.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t _mmio_base = 0;

ewokos_addr_t mmio_map_offset(uint32_t offset, uint32_t size) {
    sys_info_t sysinfo;
    sys_get_sys_info(&sysinfo);
    if (size == 0 || offset > sysinfo.mmio.size ||
            size > sysinfo.mmio.size - offset)
        return 0;

    _mmio_base = sysinfo.mmio.v_base;
    if (syscall3(SYS_MEM_MAP,
            (ewokos_addr_t)sysinfo.mmio.v_base+offset,
            (ewokos_addr_t)sysinfo.mmio.phy_base+offset,
            (ewokos_addr_t)size) != sysinfo.mmio.v_base + offset)
        return 0;
    return sysinfo.mmio.v_base+offset;
}

ewokos_addr_t mmio_map(void) {
    sys_info_t sysinfo;
    sys_get_sys_info(&sysinfo);
    _mmio_base = mmio_map_offset(0, sysinfo.mmio.size);
    return _mmio_base;
}

#ifdef __cplusplus
}
#endif
