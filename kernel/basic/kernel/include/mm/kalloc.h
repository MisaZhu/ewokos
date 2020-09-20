#ifndef KALLOC_H
#define KALLOC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct PageList {
	struct PageList *next;
} page_list_t;

/* exported function declarations */
void kalloc_init(uint32_t start, uint32_t end);
void *kalloc4k(void);
void kfree4k(void *page);
void *kalloc1k(void);
void kfree1k(void *page);
uint32_t get_free_mem_size(void);

extern uint32_t _allocatable_mem_size;

#endif
