#ifndef KALLOC_H
#define KALLOC_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

typedef struct PageList {
	struct PageList *next;
	struct PageList *prev;
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
void *kalloc_page(void);
void kfree_page(void *page);
void *kalloc1k(void);
void kfree1k(void *page);
ewokos_addr_t get_free_mem_size(void);

/* Allocate the physical-page occupancy bitmap (one bit per allocable page,
 * indexed like _pages_ref). Called once at boot after _pages_ref.max is set. */
void kalloc_busy_init(uint32_t max_pages);
/* Claim npages physically-contiguous free pages below phy_limit (absolute
 * physical address); returns the kernel direct-map VA of the base or NULL. */
void *kalloc_contig(uint32_t npages, ewokos_addr_t phy_limit);
/* kalloc_contig with a page_move compaction fallback; same return contract.
 * Caller holds the kernel lock with pages quiescent. */
void *kalloc_contig_compact(uint32_t npages, ewokos_addr_t phy_limit);
/* Return a kalloc_contig run to the page allocator. */
void kfree_contig(void *base, uint32_t npages);

#endif
