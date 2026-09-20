#include <ewoksys/thread.h>
#include <stdlib.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>

#ifdef __cplusplus
extern "C" {
#endif

static void thread_entry(thread_func_t func, void* p) {
    /*
     * Cache this task's kernel thread id in its TLS block before any user code
     * runs. This is the single entry every thread_create()/pthread_create()
     * thread reaches (the kernel ipc pool workers are cached separately, in the
     * ipc handler). Once stored, thread_get_id()/pthread_self() - taken from
     * the heap lock on every malloc/free - is a TPIDR_EL0 register read, not a
     * syscall. __ewok_tls_init_tid() may malloc the block, and that malloc's
     * own thread_get_id() falls back to one syscall because the block is not
     * published to TPIDR_EL0 until emutls_self() returns - so it cannot recurse.
     */
    __ewok_tls_init_tid((int32_t)syscall0(SYS_GET_THREAD_ID));
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
    /*
     * Fast path: the id cached in this task's TLS block, read straight from
     * TPIDR_EL0 with no trap and no malloc. Slow path (no block yet - very
     * early, or a task that never went through an entry point that caches it):
     * one syscall, stored back only if a block already exists. The block is
     * never created here: thread_get_id() runs inside the heap lock, so a malloc
     * to create it would re-enter that lock and recurse.
     */
    int32_t tid = __ewok_tls_get_tid();
    if(tid >= 0)
        return tid;
    tid = (int32_t)syscall0(SYS_GET_THREAD_ID);
    __ewok_tls_cache_tid(tid);
    return tid;
}

#ifdef __cplusplus
}
#endif
