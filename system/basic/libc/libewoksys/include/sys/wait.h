#ifndef EWOKOS_LIBC_SYS_WAIT_H
#define EWOKOS_LIBC_SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG 1

pid_t wait(int *status);

#endif
