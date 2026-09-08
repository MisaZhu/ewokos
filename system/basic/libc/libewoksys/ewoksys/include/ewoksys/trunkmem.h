#ifndef TRKMALLOC_H
#define TRKMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <ewokos_config.h>

typedef struct mem_block {
	struct mem_block* next;
	struct mem_block* prev;

	/*
	 * Payload size and used flag share one address-width storage word: the
	 * top bit is the flag, the remaining bits are the size. This keeps the
	 * original compact bitfield design (32-byte header on 64-bit) while the
	 * size field scales with the address width (63 bits on aarch64, 31 bits
	 * on arm32 - the original 2GB single-block cap only existed because the
	 * base type was uint32_t).
	 */
	ewokos_addr_t size : sizeof(ewokos_addr_t) * 8 - 1;
	ewokos_addr_t used : 1;
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
	pthread_mutex_t lock;
	uint32_t lock_inited;
} malloc_t;

char* trunk_malloc(malloc_t* m, ewokos_addr_t size);
void  trunk_free(malloc_t* m, char* p);
ewokos_addr_t  trunk_msize(malloc_t* m, char* p);

#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#ifdef __cplusplus
}
#endif

#endif
