#ifndef KALLOC_H
#define KALLOC_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

typedef struct PageList {
	struct PageList *next;
} page_list_t;

typedef uint32_t page_ref_t;

typedef struct {
	page_ref_t* refs;
	uint32_t max;
	ewokos_addr_t phy_base;
} pages_ref_t;

extern pages_ref_t _pages_ref;
uint32_t page_ref_index(ewokos_addr_t paddr);

void kalloc_reset(void);
/* exported function declarations */
uint32_t kalloc_append(ewokos_addr_t start, ewokos_addr_t end);
void *kalloc4k(void);
void kfree4k(void *page);
void *kalloc1k(void);
void kfree1k(void *page);
uint32_t get_free_mem_size(void);

#endif
