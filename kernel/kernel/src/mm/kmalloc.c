#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/trunkmem.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>

/*
kmalloc manage the memory(from KMALLOC_BASE to KMALLOC_END) for kernel.
*/
static malloc_t _kmalloc;
static ewokos_addr_t _kmalloc_mem_tail;

static void km_shrink(void* arg, int32_t pages) {
	(void)arg;
	_kmalloc_mem_tail -= pages * PAGE_SIZE;
}

static int32_t km_expand(void* arg, int32_t pages) {
	(void)arg;
	ewokos_addr_t to = _kmalloc_mem_tail + (pages * PAGE_SIZE);
	if(to > KMALLOC_END) //over flow
		return -1;

	_kmalloc_mem_tail = to;
	return 0;
}


static void* km_get_mem_tail(void* arg) {
	(void)arg;
	return (void*)_kmalloc_mem_tail;
}

static void* km_get_mem_top(void* arg) {
	(void)arg;
	return (void*)KMALLOC_END;
}

void kmalloc_init() {
	memset(&_kmalloc, 0, sizeof(malloc_t));
	_kmalloc_mem_tail = KMALLOC_BASE;
	_kmalloc.expand = km_expand;
	_kmalloc.shrink = km_shrink;
	_kmalloc.get_mem_tail = km_get_mem_tail;
	_kmalloc.get_mem_top = km_get_mem_top;
	_kmalloc.arg = NULL;
}

void *kmalloc(uint32_t size) {
	void *ret = trunk_malloc(&_kmalloc, size);
	if(ret == 0) {
		printf("Panic: km_alloc failed! (for %d)\n", size);
	}
	return ret;
}

void *kcalloc(uint32_t nmemb, uint32_t size) {
	void* ret = kmalloc(nmemb * size);
	memset(ret, 0, nmemb*size);
	return ret;
}

void kfree(void* p) {
	if(p == 0)
		return;
	trunk_free(&_kmalloc, p);
}

uint32_t  kmalloc_free_size(void) {
	return trunk_free_size(&_kmalloc);
}