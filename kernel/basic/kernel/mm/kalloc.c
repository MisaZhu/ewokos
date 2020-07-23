#include <mm/kalloc.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kernel/system.h>

ram_hole_t _ram_holes[RAM_HOLE_MAX];
/*physical memory split to pages for paging mmu, managed by kalloc/kfree, phymem page state must be occupied or free*/

/*
 * _list of pages that are free to be allocated by kalloc. Each of the
 * list nodes are stored in the beginning of the actual page, because
 * the page is free and we can use it for our purposes.
 */
static __attribute__((__aligned__(PAGE_DIR_SIZE))) page_list_t *_free_list4k = 0;
static __attribute__((__aligned__(1024))) page_list_t *_free_list1k = 0;

/*
 * page_list_prepend adds the given page to the beginning of the page list
 * and returns the address of the new page list.
 */
static page_list_t *page_list_prepend(page_list_t *page_list, char *page_address) {
	page_list_t *page = (page_list_t *) page_address;
	page->next = page_list;
	return page;
}

void kmake_hole(uint32_t base, uint32_t end) {
	if(base == 0 || base >= end || base <= ALLOCATABLE_MEMORY_START)
		return;

	int32_t i;
	for(i=0; i<RAM_HOLE_MAX; i++) {
		if(_ram_holes[i].base == 0) {
			_ram_holes[i].base = ALIGN_DOWN(base, PAGE_SIZE);
			_ram_holes[i].end = ALIGN_UP(end, PAGE_SIZE);
			return;
		}
	}
}

static inline bool in_hole(uint32_t addr) {
	int32_t i;
	for(i=0; i<RAM_HOLE_MAX; i++) {
		if(_ram_holes[i].base != 0) {
			if(addr >= _ram_holes[i].base && addr <= _ram_holes[i].end)
				return true;
		}
	}
	return false;
}

/* kalloc_init adds the given address range to the free list. */
void kalloc_init(uint32_t start, uint32_t end, bool skip_hole) {
	char *start_address = (char *) ALIGN_UP(start, PAGE_SIZE);
	char *end_address = (char *) ALIGN_DOWN(end, PAGE_SIZE);
	char *current_page = 0;

	_free_list4k = 0;
	_free_list1k = 0;

	/* add each of the pages to the free list */

	if(skip_hole) {
		for (current_page = start_address; current_page != end_address;
				current_page += PAGE_SIZE) {
			if(!in_hole((uint32_t)current_page))
				_free_list4k = page_list_prepend(_free_list4k, current_page);
		}
	}
	else {
		for (current_page = start_address; current_page != end_address;
				current_page += PAGE_SIZE) {
			_free_list4k = page_list_prepend(_free_list4k, current_page);
		}
	}
}

/* kalloc allocates and returns a single available page. and removed from free list*/
inline void* kalloc4k() {
	void *result = 0;
	if (_free_list4k != 0) {
		result = _free_list4k;
		_free_list4k = _free_list4k->next;
	}
	return result;
}

/* kfree adds the given page to the 4k free list. */
inline void kfree4k(void *page) {
	_free_list4k = page_list_prepend(_free_list4k, page);
}

/* kalloc1k allocates 1k sized and aligned chuncks of memory. */
inline void* kalloc1k() {
	void *result = 0;
	/*
	 * if we don't have any free 1k chunks, convert a 4k page into four
	 * 1k chunks.
	 */
	if (_free_list1k == 0) {
		char *page = kalloc4k();
		if (page != 0) {
			kfree1k(page);
			kfree1k(page + 1*KB);
			kfree1k(page + 2*KB);
			kfree1k(page + 3*KB);
		}
	}

	if (_free_list1k != 0) {
		result = _free_list1k;
		_free_list1k = _free_list1k->next;
	}
	return result;
}

/* kfree1k adds the given chunk to the 1k free list. */
inline void kfree1k(void *mem) {
	_free_list1k = page_list_prepend(_free_list1k, mem);
}

/*
 * get_free_memory_size returns total amount of free memory that can be allocated
 * by kalloc and kalloc1k.
 */
uint32_t get_free_mem_size(void) {
	uint32_t result = 0;
	page_list_t *current_page = 0;

	// iterate over free 1k pages
	current_page = _free_list1k;
	while (current_page != 0) {
		result += 1024;
		current_page = current_page->next;
	}

	// iterate over free 4k pages
	current_page = _free_list4k;
	while (current_page != 0) {
		result += 4096;
		current_page = current_page->next;
	}
	return result;
}

