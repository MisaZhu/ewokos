#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <types.h>
#include <string.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
void mapPages(PageDirEntryT *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, int permissions)
{
	uint32_t physicalCurrent = 0;
	uint32_t virtualCurrent = 0;

	uint32_t virtualStart = ALIGN_DOWN(vaddr, PAGE_SIZE);
	uint32_t physicalStart = ALIGN_DOWN(pstart, PAGE_SIZE);
	uint32_t physicalEnd = ALIGN_UP(pend, PAGE_SIZE);

	/* iterate over pages and map each page */
	virtualCurrent = virtualStart;
	for (physicalCurrent = physicalStart; 
			physicalCurrent != physicalEnd;
			physicalCurrent += PAGE_SIZE) {
		mapPage(vm,  virtualCurrent, physicalCurrent, permissions);
		virtualCurrent += PAGE_SIZE;
	}

	virtualStart = 0;
}

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 * Notice: virtual and physical address inputed must be all aliend by PAGE_SIZE !
 */
void mapPage(PageDirEntryT *vm, uint32_t virtualAddr,
		     uint32_t physical, int permissions)
{
	PageTableEntryT *pageTable = NULL;

	uint32_t pageDirIndex = PAGE_DIR_INDEX(virtualAddr);
	uint32_t pageIndex = PAGE_INDEX(virtualAddr);

	/* if this pageDirEntry is not mapped before, map it to a new page table */
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

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
uint32_t resolvePhyAddress(PageDirEntryT *vm, uint32_t virtual)
{
	PageDirEntryT *pdir = NULL;
	PageTableEntryT *page = NULL;
	uint32_t result = 0;
	void *baseAddress = 0;

	pdir = (void *) ((uint32_t) vm | ((virtual >> 20) << 2));
	baseAddress = (void *) (pdir->base << 10);
	page = (void *) ((uint32_t) baseAddress | ((virtual >> 10) & 0x3fc));
	page = (void *) P2V(page);
	result = (page->base << 12) | (virtual & 0xfff);

	return result;
}

void freePageTables(PageDirEntryT *vm)
{
	for (int i = 0; i < PAGE_DIR_NUM; i++) {
		if (vm[i].type != 0) {
			void *pageTable = (void *) P2V(BASE_TO_PAGE_TABLE(vm[i].base));
			kfree1k(pageTable);
		}
	}
}

