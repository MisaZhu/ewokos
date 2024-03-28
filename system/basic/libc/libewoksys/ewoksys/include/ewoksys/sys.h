#ifndef SYS_H
#define SYS_H

#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

int sys_get_sys_info(sys_info_t* info);

#ifdef __cplusplus
}
#endif

#endif
