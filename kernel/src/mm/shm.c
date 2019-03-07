#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <kernel.h>
#include <proc.h>

static uint32_t _shMemTail = 0;

typedef struct ShareMem {
	uint32_t addr;
	uint32_t pages;
	uint32_t used;
	uint32_t flags;
	uint32_t refs;
	struct ShareMem* next;
} ShareMemT;

static ShareMemT* _shmHead = NULL;

void shmInit() {
	_shMemTail = _kernelShareMemBase;
	_shmHead = NULL;
}

static void shmUnmapPages(uint32_t addr, uint32_t pages) {
	for (uint32_t i = 0; i < pages; i++) {
		uint32_t physicalAddr = resolvePhyAddress(_kernelVM, addr);

		//get the kernel address for kalloc/kfree
		uint32_t kernelAddr = P2V(physicalAddr);
		kfree((void *) kernelAddr);

		unmapPage(_kernelVM, addr);
	}
}

static bool shmMapPages(uint32_t addr, uint32_t pages) {
	for (uint32_t i = 0; i < pages; i++) {
		char *page = kalloc();
		if(page == NULL) {
			shmUnmapPages(addr, i);
			return false;
		}
		memset(page, 0, PAGE_SIZE);

		mapPage(_kernelVM,
				addr,
				V2P(page),
				AP_RW_D);
		addr += PAGE_SIZE;
	}
	return true;
}

void* shmalloc(uint32_t size) {
	uint32_t addr = _shMemTail;
	uint32_t pages = (size / PAGE_SIZE) + 1;
	
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(!i->used && i->pages >= pages)
			break;
		i = i->next;
	}

	ShareMemT* tmp = NULL;
	if(i != NULL) { //avaible item found.
		addr =  i->addr;
		if(i->pages > pages) { //try break one item to two;
			tmp = (ShareMemT*)kmalloc(sizeof(ShareMemT));
			if(tmp != NULL) {
				memset(tmp, 0, sizeof(ShareMemT));
				tmp->pages = i->pages -  pages;
				tmp->addr = i->addr + (pages * PAGE_SIZE);
				i->pages = pages;
			}
		}
	}
	else { //need to expand.
		tmp = (ShareMemT*)kmalloc(sizeof(ShareMemT));
		if(tmp == NULL)
			return NULL;
		memset(tmp, 0, sizeof(ShareMemT));
		tmp->pages = pages;
		tmp->addr = addr;
	}

	if(_shmHead == NULL) {
		_shmHead = tmp;
	}
	else  {
		tmp->next = _shmHead;
		_shmHead = tmp;
	}

	if(!shmMapPages(addr, pages))
		return NULL;

	if(addr == _shMemTail)
		_shMemTail += pages * PAGE_SIZE;

	tmp->used = true;
	return (void*)addr;
}

void shmfree(void* p) {
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->addr == (uint32_t)p) {
			break;
		}
		i = i->next;
	}
	if(i == NULL)
		return;
	shmUnmapPages(i->addr, i->pages);
	i->used = false;
}

static ShareMemT* shmItem(void* p) {
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->addr == (uint32_t)p)
			return i;
		i = i->next;
	}
	return NULL;
}

int32_t shmProcMap(void* p) {
	ShareMemT* it = shmItem(p);
	if(it == NULL)
		return -1;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		uint32_t physicalAddr = resolvePhyAddress(_kernelVM, addr);
		mapPage(_currentProcess->vm,
				addr,
				physicalAddr,
				AP_RW_RW);
		addr += PAGE_SIZE;
	}
	it->refs++;
	return 0;
}

int32_t shmProcUnmap(void* p) {
	ShareMemT* it = shmItem(p);
	if(it == NULL)
		return -1;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		unmapPage(_currentProcess->vm, addr);
		addr += PAGE_SIZE;
	}

	it->refs--;
	if(it->refs == 0)
		shmfree(p);
	return 0;
}
