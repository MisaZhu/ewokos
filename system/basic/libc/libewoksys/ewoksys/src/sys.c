#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info) {
    return syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)info);
}

#ifdef __cplusplus
}
#endif
