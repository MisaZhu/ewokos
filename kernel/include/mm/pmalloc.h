#ifndef PMALLOC_H
#define PMALLOC_H

#include <types.h>

typedef struct MemBlock {
	struct MemBlock* next;
	struct MemBlock* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} MemBlockT;

typedef struct {
	void* arg;

	bool (*expand)(void* arg, int pages);
	void (*shrink)(void* arg, int pages);
	void* (*getMemTail)(void*);

	MemBlockT* mHead;
	MemBlockT* mTail;
} MallocT;

char* pmalloc(MallocT* m, uint32_t size);
void pfree(MallocT* m, char* p);

#endif
