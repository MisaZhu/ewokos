#include <pthread.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>

void pthread_exit(void *retval) {
	(void)retval;
	__pthread_tls_thread_exit();
	proc_exit();
	syscall1(SYS_EXIT, 0);
	__builtin_unreachable();
}
