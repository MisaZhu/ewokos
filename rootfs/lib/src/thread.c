#include <thread.h>
#include <syscall.h>

int32_t thread_raw(thread_func_t func, void* p) {
	return syscall2(SYSCALL_THREAD, (int32_t)func, (int32_t)p);
}
