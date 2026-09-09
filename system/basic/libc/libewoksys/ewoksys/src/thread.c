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
     *
     * The task's thread_locals are released here rather than in pthread_exit:
     * this is the one teardown every thread reaches, and it runs after the
     * pthread_key destructors a pthread trampoline already fired - those may
     * read thread_locals themselves, so the block has to outlive them.
     */
    __ewok_emutls_thread_exit();
    proc_exit();
    syscall1(SYS_EXIT, 0);
}

int thread_create(thread_func_t func, void* p) {
    /*
     * Allocate the malloc-guard mutex before we flip on locking, while this
     * process is still single-threaded. Doing it here (not lazily in
     * proc_global_lock) avoids a startup race between the first two threads.
     */
    proc_malloc_lock_prepare();
    _proc_global_need_lock = true;
    return syscall3(SYS_THREAD, (ewokos_addr_t)thread_entry, (ewokos_addr_t)func, (ewokos_addr_t)p);
}

int thread_get_id(void) {
    return syscall0(SYS_GET_THREAD_ID);
}

#ifdef __cplusplus
}
#endif
