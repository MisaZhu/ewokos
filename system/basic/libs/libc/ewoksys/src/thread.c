#include <ewoksys/thread.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

static void thread_entry(thread_func_t func, void* p) {
	func(p);
	exit(0);
}

static int32_t _thread_lock;

int thread_create(thread_func_t func, void* p) {
	_thread_lock = 0;
	return syscall3(SYS_THREAD, (int32_t)thread_entry, (int32_t)func, (int32_t)p);
}

int thread_get_id(void) {
	return syscall0(SYS_GET_THREAD_ID);
}

#ifdef __cplusplus
}
#endif

