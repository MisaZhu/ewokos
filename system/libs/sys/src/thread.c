#include <sys/thread.h>
#include <stdlib.h>
#include <sys/syscall.h>

static void thread_entry(thread_func_t func, void* p) {
	func(p);
	exit(0);
}

int thread_create(thread_func_t func, void* p) {
	return syscall3(SYS_THREAD, (int32_t)thread_entry, (int32_t)func, (int32_t)p);
}
