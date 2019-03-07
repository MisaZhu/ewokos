#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <kernel.h>
#include <hardware.h>
#include <proc.h>

static uint32_t _shMemTail = 0;
static uint32_t _shmCount = 0;

typedef struct ShareMem {
	uint32_t id;
	uint32_t addr;
	uint32_t pages;
	uint32_t used;
	uint32_t flags;
	int32_t owner;
	struct ShareMem* next;
} ShareMemT;

static ShareMemT* _shmHead = NULL;

void shmInit() {
	_shMemTail = (uint32_t)ALIGN_UP(getPhyRamSize(), PAGE_SIZE);
	_shmHead = NULL;
	_shmCount = 0;
}

ShareMemT* shmNew() {
	ShareMemT* ret = (ShareMemT*)kmalloc(sizeof(ShareMemT));
	if(ret == NULL)
		return NULL;

	memset(ret, 0, sizeof(ShareMemT));
	ret->owner = -1;
	ret->id = _shmCount++;
	return ret;
}

static void shmUnmapPages(uint32_t addr, uint32_t pages) {
	for (uint32_t i = 0; i < pages; i++) {
		uint32_t physicalAddr = resolvePhyAddress(_kernelVM, addr);

		//get the kernel address for kalloc/kfree
		uint32_t kernelAddr = P2V(physicalAddr);
		unmapPage(_kernelVM, addr);
		kfree((void *) kernelAddr);
		addr += PAGE_SIZE;
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

int32_t shmalloc(uint32_t size) {
	uint32_t addr = _shMemTail;
	uint32_t pages = (size / PAGE_SIZE);
	if((size % PAGE_SIZE) != 0)
		pages++;
	
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
			tmp = shmNew();
			if(tmp != NULL) {
				tmp->pages = i->pages -  pages;
				tmp->addr = i->addr + (pages * PAGE_SIZE);
				i->pages = pages;
			}
		}
	}
	else { //need to expand.
		tmp = shmNew();
		if(tmp == NULL)
			return -1;
		tmp->pages = pages;
		tmp->addr = addr;
		i = tmp;
	}

	if(tmp != NULL) {
		if(_shmHead == NULL) {
			_shmHead = tmp;
		}
		else  {
			tmp->next = _shmHead;
			_shmHead = tmp;
		}
	}

	if(!shmMapPages(addr, pages))
		return -1;

	if(addr == _shMemTail)
		_shMemTail += pages * PAGE_SIZE;

	i->used = true;
	i->owner = _currentProcess->pid;
	return i->id;
}

static ShareMemT* shmItem(int32_t id) {
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->id == (uint32_t)id)
			return i;
		i = i->next;
	}
	return NULL;
}

void shmfree(int32_t id) {
	ShareMemT* i = shmItem(id);
	if(i == NULL || i->owner != _currentProcess->pid)
		return;
	shmUnmapPages(i->addr, i->pages);
	i->used = false;
}

void* shmProcMap(int32_t id) {
	ShareMemT* it = shmItem(id);
	if(it == NULL)
		return NULL;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		uint32_t physicalAddr = resolvePhyAddress(_kernelVM, addr);
		mapPage(_currentProcess->vm,
				addr,
				physicalAddr,
				AP_RW_RW);
		addr += PAGE_SIZE;
	}
	return (void*)it->addr;
}

int32_t shmProcUnmap(int32_t id) {
	ShareMemT* it = shmItem(id);
	if(it == NULL)
		return -1;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		unmapPage(_currentProcess->vm, addr);
		addr += PAGE_SIZE;
	}
	return 0;
}

void shmProcFree(int32_t pid) {
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->owner == pid) {
			shmUnmapPages(i->addr, i->pages);
			i->used = false;
		}
		i = i->next;
	}
}

