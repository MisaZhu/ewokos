#ifndef KALLOC_H
#define KALLOC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct PageList {
	struct PageList *next;
} page_list_t;

typedef struct {
	int32_t* refs;
	uint32_t max;
	uint32_t phy_base;
} pages_ref_t;

extern pages_ref_t _pages_ref;
uint32_t page_ref_index(uint32_t paddr);

/* exported function declarations */
uint32_t kalloc_init(uint32_t start, uint32_t end);
void *kalloc4k(void);
void kfree4k(void *page);
void *kalloc1k(void);
void kfree1k(void *page);
uint32_t get_free_mem_size(void);

#endif
