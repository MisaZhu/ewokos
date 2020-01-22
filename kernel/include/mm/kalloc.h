#ifndef KALLOC_H
#define KALLOC_H

#include <types.h>

typedef struct PageList {
	struct PageList *next;
} page_list_t;

/* exported function declarations */
void kalloc_init(uint32_t start, uint32_t end, bool skip_hole);
void *kalloc4k(void);
void kfree4k(void *page);
void *kalloc1k(void);
void kfree1k(void *page);
uint32_t get_free_mem_size(void);

typedef struct {
	uint32_t base;	
	uint32_t end;	
} ram_hole_t;

#define RAM_HOLE_MAX 4
extern ram_hole_t _ram_holes[RAM_HOLE_MAX];

void kmake_hole(uint32_t base, uint32_t end);

#endif
