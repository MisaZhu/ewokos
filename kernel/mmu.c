#include <mmu.h>
#include <kalloc.h>
#include <types.h>
#include <lib/string.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
void mapPages(PageDirEntryT *vm, MemoryMapT mapping)
{
	uint32_t physicalCurrent = 0;
	uint32_t virtualCurrent = 0;

	uint32_t virtualStart = ALIGN_DOWN(mapping.vAddr, PAGE_SIZE);
	uint32_t physicalStart = ALIGN_DOWN(mapping.pStart, PAGE_SIZE);
	uint32_t physicalEnd = ALIGN_UP(mapping.pEnd, PAGE_SIZE);

	/* iterate over pages and map each page */
	virtualCurrent = virtualStart;
	for (physicalCurrent = physicalStart; physicalCurrent != physicalEnd;
	     physicalCurrent += PAGE_SIZE) {
		mapPage(vm, physicalCurrent, virtualCurrent,
			 mapping.permissions);
		virtualCurrent += PAGE_SIZE;
	}
}

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 */
void mapPage(PageDirEntryT *vm, uint32_t physical,
		     uint32_t virtualAddr, int permissions)
{
	PageTableEntryT *pageTable = NULL;

	uint32_t pageDirIndex = PAGE_DIR_INDEX(virtualAddr);
	uint32_t pageIndex = PAGE_INDEX(virtualAddr);

	/* if this pageDir is not mapped before, map it to a new page table */
	if (vm[pageDirIndex].type == 0) {
		pageTable = kalloc1k();
		memset(pageTable, 0, PAGE_TABLE_SIZE);
		
		vm[pageDirIndex].base = PAGE_TABLE_TO_BASE(V2P(pageTable));
		vm[pageDirIndex].type = PAGE_DIR_TYPE;
		vm[pageDirIndex].domain = 0;
	}
	/* otherwise use the previously allocated page table */
	else {
		pageTable = (void *) P2V(BASE_TO_PAGE_TABLE(vm[pageDirIndex].base));
	}

	/* map the virtual page to physical page in page table */
	pageTable[pageIndex].type = PAGE_TYPE,
	pageTable[pageIndex].bufferable = 0;
	pageTable[pageIndex].cacheable = 0;
	pageTable[pageIndex].permissions = permissions;
	pageTable[pageIndex].base = PAGE_TO_BASE(physical);
}

/* unmap_page clears the mapping for the given virtual address */
void unmapPage(PageDirEntryT *vm, uint32_t virtualAddr)
{
	PageTableEntryT *pageTable = NULL;

	uint32_t pageDirIndex = PAGE_DIR_INDEX(virtualAddr);
	uint32_t pageIndex = PAGE_INDEX(virtualAddr);

	pageTable = (void *) P2V(BASE_TO_PAGE_TABLE(vm[pageDirIndex].base));
	pageTable[pageIndex].type = 0;
}

