#include <mm/kalloc.h>
#include <mm/mmu.h>
#include <types.h>

/*physical memory split to pages for paging mmu, managed by kalloc/kfree, phymem page state must be occupied or free*/

static PageListT *pageListPrepend(PageListT *pageList,
					  char *pageAddress);

/*
 * List of pages that are free to be allocated by kalloc. Each of the
 * list nodes are stored in the beginning of the actual page, because
 * the page is free and we can use it for our purposes.
 */
static PageListT *freeList4k = NULL;
static PageListT *freeList1k = NULL;

/* kalloc_init adds the given address range to the free list. */
void kallocInit(uint32_t start, uint32_t end)
{
	char *startAddress = (char *) ALIGN_UP(start, PAGE_SIZE);
	char *endAddress = (char *) ALIGN_DOWN(end, PAGE_SIZE);
	char *currentPage = NULL;

	//freeList4k = NULL;
	//freeList1k = NULL;

	/* add each of the pages to the free list */
	for (currentPage = startAddress; currentPage != endAddress;
	     currentPage += PAGE_SIZE)
	{
		freeList4k = pageListPrepend(freeList4k, currentPage);
	}
}

/* kalloc allocates and returns a single available page. and removed from free list*/
void *kalloc()
{
	void *result = NULL;

	if (freeList4k != NULL) {
		result = freeList4k;
		freeList4k = freeList4k->next;
	}

	return result;
}

/* kfree adds the given page to the 4k free list. */
void kfree(void *page)
{
	freeList4k = pageListPrepend(freeList4k, page);
}

/* kalloc1k allocates 1k sized and aligned chuncks of memory. */
void *kalloc1k()
{
	void *result = NULL;

	/*
	 * if we don't have any free 1k chunks, convert a 4k page into four
	 * 1k chunks.
	 */
	if (freeList1k == NULL) {
		char *page = kalloc();
		if (page != NULL) {
			kfree1k(page);
			kfree1k(page + 1024);
			kfree1k(page + 2048);
			kfree1k(page + 3072);
		}
	}

	if (freeList1k != NULL) {
		result = freeList1k;
		freeList1k = freeList1k->next;
	}

	return result;
}

/* kfree1k adds the given chunk to the 1k free list. */
void kfree1k(void *page)
{
	freeList1k = pageListPrepend(freeList1k, page);
}

/*
 * get_free_memory_size returns total amount of free memory that can be allocated
 * by kalloc and kalloc1k.
 */
uint32_t getFreeMemorySize(void) {
	uint32_t result = 0;
	PageListT *currentPage = NULL;

	/* iterate over free 1k pages */
	currentPage = freeList1k;
	while (currentPage != NULL) {
		result += 1024;
		currentPage = currentPage->next;
	}

	/* iterate over free 4k pages */
	currentPage = freeList4k;
	while (currentPage != NULL) {
		result += 4096;
		currentPage = currentPage->next;
	}

	return result;
}

/*
 * pageListPrepend adds the given page to the beginning of the page list
 * and returns the address of the new page list.
 */
static PageListT *pageListPrepend(PageListT *pageList,
					  char *pageAddress)
{
	PageListT *page = (PageListT *) pageAddress;
	page->next = pageList;

	return page;
}
