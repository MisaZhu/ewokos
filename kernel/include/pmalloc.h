#ifndef PMALLOC_H
#define PMALLOC_H

#include <types.h>
#include <proc.h>

typedef struct MemBlock {
	struct MemBlock* next;
	struct MemBlock* prev;
	
	uint32_t size: 31;
	uint32_t used: 1;
	char* mem;
} MemBlockT;

char* pmalloc(ProcessT* proc, uint32_t size);
void pfree(ProcessT* proc, char* p);

#endif
