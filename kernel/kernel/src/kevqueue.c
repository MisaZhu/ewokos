#include <kernel/kevqueue.h>
#include <kernel/core.h>
#include <queue.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>
#include <stddef.h>

static queue_t  _kev_queue;
static int32_t _kev_lock = 0;

void kev_init(void) {
    queue_init(&_kev_queue);
    _kev_lock = 0;
}

kevent_t* kev_push(uint32_t type, uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    kevent_t* kev = (kevent_t*)kmalloc(sizeof(kevent_t));
    kev->type = type;
    kev->data[0] = arg0;
    kev->data[1] = arg1;
    kev->data[2] = arg2;
#ifdef KERNEL_SMP
    mcore_lock(&_kev_lock);
#endif

    queue_push(&_kev_queue, kev);

#ifdef KERNEL_SMP
    mcore_unlock(&_kev_lock);
#endif

    if(_core_proc_pid >= 0) {
        proc_t* core = proc_get(_core_proc_pid);
        if(core != NULL)
            proc_wakeup(core);
    }
    return kev;
}

/*
 * Enqueue a proc exception (core dump) snapshot for the core proc. The crash
 * info is copied into the queued event under the same lock/wake path as
 * kev_push() so the core proc sees a fully-formed record when it pops it.
 */
void kev_push_core_dump(const kev_core_dump_t* dump) {
    kevent_t* kev = (kevent_t*)kmalloc(sizeof(kevent_t));
    kev->type = KEV_PROC_CORE_DUMP;
    kev->data[0] = (uint32_t)dump->pid;
    kev->data[1] = dump->reason;
    kev->data[2] = 0;
    kev->core_dump = *dump;
#ifdef KERNEL_SMP
    mcore_lock(&_kev_lock);
#endif

    queue_push(&_kev_queue, kev);

#ifdef KERNEL_SMP
    mcore_unlock(&_kev_lock);
#endif

    if(_core_proc_pid >= 0) {
        proc_t* core = proc_get(_core_proc_pid);
        if(core != NULL)
            proc_wakeup(core);
    }
}

int32_t kev_pop(kevent_t* ret) {
    proc_t* cproc = get_current_proc();
    if(cproc->info.pid != _core_proc_pid)	 //only core proc access allowed.
        return -1;

#ifdef KERNEL_SMP
    mcore_lock(&_kev_lock);
#endif

    kevent_t* kev = queue_pop(&_kev_queue);

#ifdef KERNEL_SMP
    mcore_unlock(&_kev_lock);
#endif
    if(kev == NULL) {
        return -1;
    }

    ret->type = kev->type;
    ret->data[0] = kev->data[0];
    ret->data[1] = kev->data[1];
    ret->data[2] = kev->data[2];
    ret->core_dump = kev->core_dump;
    kfree(kev);
    return 0;
}
