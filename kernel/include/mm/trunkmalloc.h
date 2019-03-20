#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#include <types.h>

typedef struct mem_block {
	struct mem_block* next;
	struct mem_block* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} mem_block_t;

typedef struct {
	void* arg;

	bool (*expand)(void* arg, int pages);
	void (*shrink)(void* arg, int pages);
	void* (*getMemTail)(void*);

	mem_block_t* mHead;
	mem_block_t* mTail;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
void trunk_free(malloc_t* m, char* p);

#endif
