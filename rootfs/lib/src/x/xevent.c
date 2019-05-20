#include <x/xevent.h>
#include <kstring.h>

void x_ev_queue_init(x_ev_queue_t *q) {
	memset(q, 0, sizeof(x_ev_queue_t));
}

int32_t x_ev_queue_push(x_ev_queue_t *queue, x_ev_t* ev, bool loop) {
	uint32_t at;
	if(queue->size == X_EV_MAX) {
		if(!loop)
			return -1;
		queue->start++;
		if(queue->start == X_EV_MAX)
			queue->start = 0;
		at = (queue->start + queue->size)-1;
	}
	else
		at = (queue->start + queue->size);

	if(at >= X_EV_MAX) {
		at -= X_EV_MAX;
	}
	memcpy(&queue->items[at], ev, sizeof(x_ev_t));
	if(queue->size < X_EV_MAX)
		queue->size++;
	return 0;
}

int32_t x_ev_queue_pop(x_ev_queue_t *queue, x_ev_t* ev) {
	if(queue->size == 0)
		return -1;

	memcpy(ev, &queue->items[queue->start], sizeof(x_ev_t));
	queue->start++;
	if(queue->start == X_EV_MAX)
		queue->start = 0;
	queue->size--;
	return 0;
}

