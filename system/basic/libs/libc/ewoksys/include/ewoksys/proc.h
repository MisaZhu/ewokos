#ifndef PROC_H
#define PROC_H

#include <ewoksys/ewokdef.h>
#include <procinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

int      get_vfsd_pid(void);
int      get_cored_pid(void);

void     proc_exec_elf(const char* cmd_line, const char* elf, int32_t size);
int      proc_getpid(int pid);
int      proc_info(int pid, procinfo_t* info);
void     proc_detach(void);

void     proc_block_by(int by_pid, uint32_t evt);
void     proc_wakeup(uint32_t evt);
void     proc_wakeup_pid(int pid, uint32_t evt);
void     proc_init(void);
uint32_t proc_check_uuid(int32_t pid, uint32_t uuid);
uint32_t proc_get_uuid(int32_t pid);

#ifdef __cplusplus
}
#endif

#endif
