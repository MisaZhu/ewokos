#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct queue_item_st {
	void* data;
	struct queue_item_st* prev;
	struct queue_item_st* next;
} queue_item_t;

typedef void (*free_func_t)(void* p);

typedef struct {
	queue_item_t* head;	
	queue_item_t* tail;	
}	queue_t;

void  queue_init(queue_t* q);
void  queue_push(queue_t* q, void* data);
void  queue_push_head(queue_t* q, void* data);
void* queue_pop(queue_t* q);
void  queue_clear(queue_t* q, free_func_t fr);
bool  queue_in(queue_t* q, void* data);

#endif
