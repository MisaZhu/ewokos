#include <mm/kmalloc.h>
#include <mm/trunkmalloc.h>
#include <mm/mmu.h>
#include <kernel.h>
#include <printk.h>

static malloc_t _kmalloc;
static uint32_t _kmalloc_mem_tail;

static void km_shrink(void* arg, int32_t pages) {
	(void)arg;
	_kmalloc_mem_tail -= pages * PAGE_SIZE;	
}

static bool km_expand(void* arg, int32_t pages) {
	(void)arg;
	uint32_t to = _kmalloc_mem_tail + (pages * PAGE_SIZE);
	if(to > (KMALLOC_BASE + KMALLOC_SIZE))
		return false;

	_kmalloc_mem_tail = to;
	return true;
}

static void* km_get_mem_tail(void* arg) {
	(void)arg;
	return (void*)_kmalloc_mem_tail;
}

void km_init() {
	_kmalloc_mem_tail = KMALLOC_BASE;
	_kmalloc.expand = km_expand;
	_kmalloc.shrink = km_shrink;
	_kmalloc.get_mem_tail = km_get_mem_tail;
}

void *km_alloc(uint32_t size) {
	void *ret = trunk_malloc(&_kmalloc, size);
	if(ret == NULL) {
		printk("Panic: km_alloc failed!\n");
	}
	return ret;
}

void km_free(void* p) {
	if(p == NULL)
		return;
	trunk_free(&_kmalloc, p);
}
