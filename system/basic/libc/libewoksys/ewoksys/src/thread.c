#include <ewoksys/thread.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>

#ifdef __cplusplus
extern "C" {
#endif

static void thread_entry(thread_func_t func, void* p) {
	func(p);
	/*
	 * Ewok pthreads are implemented as thread-like child tasks. They must not
	 * run process atexit handlers on thread completion, otherwise libraries
	 * such as SDL will execute global shutdown paths from the worker task.
	 */
	proc_exit();
	syscall1(SYS_EXIT, 0);
}

int thread_create(thread_func_t func, void* p) {
	_proc_global_need_lock = true;
	return syscall3(SYS_THREAD, (ewokos_addr_t)thread_entry, (ewokos_addr_t)func, (ewokos_addr_t)p);
}

int thread_get_id(void) {
	return syscall0(SYS_GET_THREAD_ID);
}

#ifdef __cplusplus
}
#endif
