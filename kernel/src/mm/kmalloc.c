#include <mm/kmalloc.h>
#include <mm/trunkmalloc.h>
#include <mm/mmu.h>
#include <kernel.h>
#include <printk.h>

static MallocT _kMalloc;
static uint32_t _kMallocMemTail;

static void kmShrink(void* arg, int pages) {
	(void)arg;
	_kMallocMemTail -= pages * PAGE_SIZE;	
}

static bool kmExpand(void* arg, int pages) {
	(void)arg;
	uint32_t to = _kMallocMemTail + (pages * PAGE_SIZE);
	if(to > (KMALLOC_BASE + KMALLOC_SIZE))
		return false;

	_kMallocMemTail = to;
	return true;
}

static void* kmGetMemTail(void* arg) {
	(void)arg;
	return (void*)_kMallocMemTail;
}

void kmInit() {
	_kMallocMemTail = KMALLOC_BASE;
	_kMalloc.expand = kmExpand;
	_kMalloc.shrink = kmShrink;
	_kMalloc.getMemTail = kmGetMemTail;
}

void *kmalloc(uint32_t size) {
	void *ret = trunkMalloc(&_kMalloc, size);
	if(ret == NULL) {
		printk("Panic: kmalloc failed!\n");
	}
	return ret;
}

void kmfree(void* p) {
	if(p == NULL)
		return;
	trunkFree(&_kMalloc, p);
}
