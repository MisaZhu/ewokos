#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct queue_item_st {
	void* data;
	struct queue_item_st* prev;
	struct queue_item_st* next;
} queue_item_t;

typedef void (*free_func_t)(void* p);

typedef struct {
	queue_item_t* head;	
	queue_item_t* tail;	
	uint32_t num;
}	queue_t;

void  queue_init(queue_t* q);
void  queue_push(queue_t* q, void* data);
void  queue_push_head(queue_t* q, void* data);
void* queue_pop(queue_t* q);
void  queue_clear(queue_t* q, free_func_t fr);
queue_item_t* queue_in(queue_t* q, void* data);
void  queue_remove(queue_t* q, queue_item_t* it);
uint32_t queue_num(queue_t* q);
bool  queue_is_empty(queue_t* q);

#endif
