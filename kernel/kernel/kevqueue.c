#include <kernel/kevqueue.h>
#include <queue.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>

static queue_t  _kev_queue;

void kev_init(void) {
	queue_init(&_kev_queue);
}

kevent_t* kev_push(uint32_t type, const proto_t* data) {
	kevent_t* kev = (kevent_t*)kmalloc(sizeof(kevent_t));
	kev->type = type;
	if(data == NULL)
		kev->data = proto_new(NULL, 0);
	else
		kev->data = proto_new(data->data, data->size);
	queue_push(&_kev_queue, kev);
	proc_wakeup(-1, (uint32_t)kev_init);
	return kev;
}

kevent_t*  kev_pop(void) {
	return queue_pop(&_kev_queue);
}

