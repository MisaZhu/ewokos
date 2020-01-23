#include <queue.h>
#include <types.h>
#include <mm/kmalloc.h>
#include <kstring.h>

void queue_init(queue_t* q) {
	q->head = q->tail = NULL;
}

void queue_push(queue_t* q, void* data) {
	queue_item_t* it = (queue_item_t*)kmalloc(sizeof(queue_item_t));
	memset(it, 0, sizeof(queue_item_t));
	it->data = data;

	if(q->tail == NULL) {
		q->head = q->tail = it;
	}
	else {
		q->tail->next = it;
		it->prev = q->tail;
		q->tail = it;
	}
}

void* queue_pop(queue_t* q) {
	if(q->head == NULL)
		return NULL;

	queue_item_t* it = q->head;
	q->head = q->head->next;
	if(q->head) 
		q->head->prev = NULL;
	else
		q->tail = NULL;

	void* ret = it->data;
	kfree(it);
	return ret;
}

void queue_clear(queue_t* q, free_func_t fr) {
	queue_item_t* it = q->head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		if(fr != NULL && it->data != NULL)
			fr(it->data);
		kfree(it);
		it = next;
	}
	q->head = q->tail = NULL;
}

