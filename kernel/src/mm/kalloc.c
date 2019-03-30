#include <mm/kalloc.h>
#include <mm/mmu.h>
#include <types.h>
#include <system.h>

/*physical memory split to pages for paging mmu, managed by kalloc/kfree, phymem page state must be occupied or free*/

static page_list_t *page_list_prepend(page_list_t *page_list,
					  char *page_address);

/*
 * _list of pages that are free to be allocated by kalloc. Each of the
 * list nodes are stored in the beginning of the actual page, because
 * the page is free and we can use it for our purposes.
 */
static page_list_t *_free_list4k = NULL;
static page_list_t *_free_list1k = NULL;

/* kalloc_init adds the given address range to the free list. */
void kalloc_init(uint32_t start, uint32_t end) {
	char *start_address = (char *) ALIGN_UP(start, PAGE_SIZE);
	char *end_address = (char *) ALIGN_DOWN(end, PAGE_SIZE);
	char *current_page = NULL;
	_free_list4k = NULL;
	_free_list1k = NULL;

	/* add each of the pages to the free list */
	for (current_page = start_address; current_page != end_address;
	     current_page += PAGE_SIZE) {
		_free_list4k = page_list_prepend(_free_list4k, current_page);
	}
}

/* kalloc allocates and returns a single available page. and removed from free list*/
void *kalloc() {
	//cli();

	void *result = NULL;
	if (_free_list4k != NULL) {
		result = _free_list4k;
		_free_list4k = _free_list4k->next;
	}

	//sti();
	return result;
}

/* kfree adds the given page to the 4k free list. */
void kfree(void *page) {
	//cli();
	_free_list4k = page_list_prepend(_free_list4k, page);
	//sti();
}

/* kalloc1k allocates 1k sized and aligned chuncks of memory. */
void *kalloc1k() {
	void *result = NULL;

	/*
	 * if we don't have any free 1k chunks, convert a 4k page into four
	 * 1k chunks.
	 */
	if (_free_list1k == NULL) {
		char *page = kalloc();
		if (page != NULL) {
			kfree1k(page);
			kfree1k(page + 1*KB);
			kfree1k(page + 2*KB);
			kfree1k(page + 3*KB);
		}
	}

	if (_free_list1k != NULL) {
		result = _free_list1k;
		_free_list1k = _free_list1k->next;
	}

	return result;
}

/* kfree1k adds the given chunk to the 1k free list. */
void kfree1k(void *mem) {
	//cli();
	_free_list1k = page_list_prepend(_free_list1k, mem);
	//sti();
}

/*
 * get_free_memory_size returns total amount of free memory that can be allocated
 * by kalloc and kalloc1k.
 */
uint32_t get_free_mem_size(void) {
	uint32_t result = 0;
	page_list_t *current_page = NULL;

	/* iterate over free 1k pages */
	current_page = _free_list1k;
	while (current_page != NULL) {
		result += 1024;
		current_page = current_page->next;
	}

	/* iterate over free 4k pages */
	current_page = _free_list4k;
	while (current_page != NULL) {
		result += 4096;
		current_page = current_page->next;
	}

	return result;
}

/*
 * page_list_prepend adds the given page to the beginning of the page list
 * and returns the address of the new page list.
 */
static page_list_t *page_list_prepend(page_list_t *page_list,
					  char *page_address) {
	page_list_t *page = (page_list_t *) page_address;
	page->next = page_list;
	return page;
}
