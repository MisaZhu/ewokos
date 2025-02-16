#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#include <stdint.h>

typedef struct mem_block {
	struct mem_block* next;
	struct mem_block* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} mem_block_t;

typedef struct {
	void* arg;

	int32_t (*expand)(void* arg, int32_t pages);
	void (*shrink)(void* arg, int32_t pages);
	void* (*get_mem_tail)(void*);
	void* (*get_mem_top)(void*);

	mem_block_t* head;
	mem_block_t* tail;
	mem_block_t* start;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
uint32_t trunk_free_size(malloc_t* m);
uint32_t trunk_msize(malloc_t* m, char* p);
void  trunk_free(malloc_t* m, char* p);

#endif
