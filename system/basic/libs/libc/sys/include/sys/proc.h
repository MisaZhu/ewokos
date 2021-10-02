#ifndef PROC_H
#define PROC_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

int  get_vfsd_pid(void);

void proc_exec_elf(const char* cmd_line, const char* elf, int32_t size);
int  proc_getpid(int pid);
void proc_detach(void);
int  proc_ping(int pid);
void proc_ready_ping(void);
void proc_wait_ready(int pid);

void proc_block(int by_pid, uint32_t evt);
void proc_wakeup(uint32_t evt);
void proc_init(void);

#ifdef __cplusplus
}
#endif

#endif
