#include <sys/thread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

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

inline void thread_safe_set(int32_t* to, int32_t v) {
	while(true) {
		if(syscall2(SYS_SAFE_SET, (int32_t)to, v) == 0)
			break;
		//sleep(0);
	}
}

void thread_lock() {
	thread_safe_set(&_thread_lock, 1);
}

void thread_unlock() {
	thread_safe_set(&_thread_lock, 0);
}

#ifdef __cplusplus
}
#endif

