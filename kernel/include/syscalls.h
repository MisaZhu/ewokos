#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "types.h"

int32_t handle_syscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2);

#endif
