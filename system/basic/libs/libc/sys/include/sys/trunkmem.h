#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct mem_block {
	struct mem_block* next;
	struct mem_block* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} mem_block_t;

typedef struct {
	uint32_t seg_size;
	void* arg;

	int32_t (*expand)(void* arg, int32_t pages);
	void (*shrink)(void* arg, int32_t pages);
	void* (*get_mem_tail)(void*);

	mem_block_t* head;
	mem_block_t* tail;
	mem_block_t* start;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
void  trunk_free(malloc_t* m, char* p);
uint32_t  trunk_msize(malloc_t* m, char* p);

#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#ifdef __cplusplus
}
#endif

#endif
