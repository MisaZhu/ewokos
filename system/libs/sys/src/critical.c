#include <sys/critical.h>
#include <sys/syscall.h>

inline void critical_enter(void) {
	syscall0(SYS_PROC_CRITICAL_ENTER);
}

inline void critical_quit(void) {
	syscall0(SYS_PROC_CRITICAL_QUIT);
}


