#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif


int waitpid(int pid) {
	return syscall1(SYS_WAIT_PID, pid);
}

#ifdef __cplusplus
}
#endif

