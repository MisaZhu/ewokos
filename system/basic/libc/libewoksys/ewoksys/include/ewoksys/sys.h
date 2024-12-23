#ifndef SYS_H
#define SYS_H

#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info);
extern vsyscall_info_t* _vsyscall_info;

#ifdef __cplusplus
}
#endif

#endif
