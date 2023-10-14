#include <kernel/kevqueue.h>
#include <queue.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>
#include <stddef.h>

static queue_t  _kev_queue;

void kev_init(void) {
	queue_init(&_kev_queue);
}

kevent_t* kev_push(uint32_t type, uint32_t arg0, uint32_t arg1, uint32_t arg2) {
	kevent_t* kev = (kevent_t*)kmalloc(sizeof(kevent_t));
	kev->type = type;
	kev->data[0] = arg0;
	kev->data[1] = arg1;
	kev->data[2] = arg2;
	queue_push(&_kev_queue, kev);
	proc_wakeup(-1, (uint32_t)kev_init);
	return kev;
}

kevent_t* kev_pop(void) {
	proc_t* cproc = get_current_proc();
	if(cproc->info.pid != _core_proc_pid)	 //only core proc access allowed.
		return NULL;

	kevent_t* kev = queue_pop(&_kev_queue);
	if(kev == NULL) {
		return NULL;
	}

	kevent_t* ret = (kevent_t*)proc_malloc(cproc, sizeof(kevent_t));
	ret->type = kev->type;
	ret->data[0] = kev->data[0];
	ret->data[1] = kev->data[1];
	ret->data[2] = kev->data[2];
	kfree(kev);
	return ret;
}
