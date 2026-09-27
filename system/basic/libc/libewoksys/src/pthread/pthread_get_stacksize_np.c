#include <pthread.h>
#include <unistd.h>
#include <stddef.h>

/*
 * pthread_get_stacksize_np() is a BSD/Darwin extension - the _np suffix marks
 * it non-portable - that returns the stack size of a thread. Ported code calls
 * it with pthread_self(); e.g. the ewebview wasm runtime uses it to derive a
 * native recursion floor for its JIT'd frames.
 *
 * EwokOS implements pthreads as thread-like child tasks (SYS_THREAD). The
 * kernel hands every such task a fixed-size stack of THREAD_STACK_PAGES pages
 * (see kernel/kernel/include/kernel/proc.h) and neither exposes a per-thread
 * stack size to userspace nor honours pthread_attr_setstacksize() -
 * pthread_create() ignores its attr argument entirely. So the size is the same
 * for every pthread and the `thread` argument is advisory only.
 *
 * getpagesize() supplies the page size (4096 on every current EwokOS target)
 * so the reported value tracks the kernel's page granularity instead of
 * hard-coding a byte count. EWOK_THREAD_STACK_PAGES mirrors the kernel's
 * THREAD_STACK_PAGES; keep the two in sync if the kernel stack ever changes.
 */
#define EWOK_THREAD_STACK_PAGES 64

size_t pthread_get_stacksize_np(pthread_t thread) {
	(void)thread;
	return (size_t)EWOK_THREAD_STACK_PAGES * (size_t)getpagesize();
}
