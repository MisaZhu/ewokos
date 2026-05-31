#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info) {
    if(info == NULL)
        return -1;

    memset(info, 0, sizeof(sys_info_t));
    int ret = syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)info);
    if(ret < 0)
        return ret;

    info->machine[MACHINE_MAX-1] = 0;
    info->arch[ARCH_MAX-1] = 0;
    if(info->cores > MAX_CORE_NUM)
        info->cores = MAX_CORE_NUM;
    return ret;
}

#ifdef __cplusplus
}
#endif
