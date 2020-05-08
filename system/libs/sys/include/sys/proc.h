#ifndef PROC_H
#define PROC_H

#include <stdint.h>

int proc_ping(int pid);
void proc_ready_ping(void);
void proc_wait_ready(int pid);

#endif
