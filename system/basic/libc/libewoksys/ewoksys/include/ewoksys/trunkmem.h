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

/* Size-class segregated free lists.
 *
 * A single LIFO free list is not enough: a long-running, allocation-heavy
 * process fragments the heap into a huge number of small holes (xBrowser on a
 * CSS-heavy page reached ~1.3M blocks with ~240k of them free, the largest
 * hole only ~2KB). Searching such a list for a block that does not exist costs
 * O(free_count) per allocation, which is exactly the stall we are removing.
 *
 * Class c holds blocks whose size is in [2^(c+TRUNK_FREE_MIN_BITS),
 * 2^(c+TRUNK_FREE_MIN_BITS+1)-1]; class 0 absorbs everything smaller and the
 * last class is open-ended. Taking the head of the first class whose minimum
 * is >= the request therefore needs no size check at all, which makes
 * allocation O(1) instead of O(free_count). */
#define TRUNK_FREE_CLASSES 24
#define TRUNK_FREE_MIN_BITS 4

typedef struct {
	uint32_t seg_size;
	void* arg;

	int32_t (*expand)(void* arg, int32_t pages);
	void (*shrink)(void* arg, int32_t pages);
	void* (*get_mem_tail)(void*);

	mem_block_t* head;
	mem_block_t* tail;
	mem_block_t* start;
	/* Heads of the size-class free lists. Each free block is chained through
	 * two pointers stored in its own (dead) payload, so no extra per-block
	 * header space is needed. A block is always unlinked before its size
	 * changes (allocate / merge / shrink), so its class stays stable while it
	 * is on a list. */
	mem_block_t* free_class[TRUNK_FREE_CLASSES];
	pthread_mutex_t lock;
	uint32_t lock_inited;
} malloc_t;

char* trunk_malloc(malloc_t* m, uint32_t size);
void  trunk_free(malloc_t* m, char* p);
uint32_t  trunk_msize(malloc_t* m, char* p);

/* Walk the chain once and report heap shape. O(block_count), so this is for
 * diagnostics only -- never call it on an allocation path. Any of the output
 * pointers may be NULL. free_list_len counts the blocks reachable from the
 * size-class lists and must agree with free_blocks; a mismatch means a class
 * list drifted out of sync with the physical chain. free_list_max is the size
 * of the largest hole. */
void  trunk_stat(malloc_t* m, uint32_t* blocks, uint32_t* free_blocks,
        uint32_t* used_bytes, uint32_t* free_bytes,
        uint32_t* free_list_len, uint32_t* free_list_max);

#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#ifdef __cplusplus
}
#endif

#endif
