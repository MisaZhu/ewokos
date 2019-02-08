#ifndef KALLOC_H
#define KALLOC_H

#include <types.h>

typedef struct PageList {
	struct PageList *next;
} PageListT;

/* exported function declarations */
void kallocInit(uint32_t start, uint32_t end);
void *kalloc();
void kfree(void *page);
void *kalloc1k();
void kfree1k(void *page);
uint32_t getFreeMemorySize(void);

#endif
