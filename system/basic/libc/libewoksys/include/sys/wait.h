#ifndef EWOKOS_LIBC_SYS_WAIT_H
#define EWOKOS_LIBC_SYS_WAIT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WNOHANG 1
#define WUNTRACED 2

#define WIFEXITED(status) (1)
#define WIFSIGNALED(status) (0)
#define WIFSTOPPED(status) (0)
#define WEXITSTATUS(status) ((status) & 0xff)
#define WTERMSIG(status) (0)
#define WSTOPSIG(status) (0)

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#ifdef __cplusplus
}
#endif

#endif
