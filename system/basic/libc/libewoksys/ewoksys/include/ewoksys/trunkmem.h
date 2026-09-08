#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>

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
	/* Size of the largest free block currently in the chain (0 = none).
	 * Lets trunk_malloc skip the O(block_count) first-fit walk when the
	 * request cannot possibly be satisfied without expanding, which is the
	 * common case for allocation-heavy workloads on a nearly-full heap.
	 * May transiently overestimate (corrected by a rescan when the walk
	 * comes up empty); it must never underestimate below the true max
	 * without a rescan, or allocations would expand the heap needlessly. */
	uint32_t free_max;
	pthread_mutex_t lock;
	uint32_t lock_inited;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
void  trunk_free(malloc_t* m, char* p);
uint32_t  trunk_msize(malloc_t* m, char* p);

/* Walk the chain once and report heap shape. O(block_count), so this is for
 * diagnostics only -- never call it on an allocation path. Any of the output
 * pointers may be NULL. */
void  trunk_stat(malloc_t* m, uint32_t* blocks, uint32_t* free_blocks,
        uint32_t* used_bytes, uint32_t* free_bytes);

#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#ifdef __cplusplus
}
#endif

#endif
