#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <kernel.h>
#include <hardware.h>
#include <proc.h>
#include <system.h>
#include <printk.h>

static uint32_t _shMemTail = 0;
static uint32_t _shmCount = 0;

typedef struct ShareMem {
	uint32_t id; //uniqe accumulating counter
	uint32_t addr; //memory block base address
	uint32_t pages; //memory pages
	uint32_t used; //used or free
	uint32_t flags; 
	int32_t owner; //process id which alloced this shm
	struct ShareMem* next;
	struct ShareMem* prev;
} ShareMemT;

static ShareMemT* _shmHead = NULL;
static ShareMemT* _shmTail = NULL;

void shmInit() {
	//share memory base address at virtual address phy_mem_size + 256MB
	_shMemTail = (uint32_t)ALIGN_UP(getPhyRamSize()+256*MB, PAGE_SIZE);
	_shmHead = NULL;
	_shmTail = NULL;
	_shmCount = 0;
}

static ShareMemT* shmNew() {
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
	uint32_t oldAddr = addr;
	for (uint32_t i = 0; i < pages; i++) {
		char *page = kalloc();
		if(page == NULL) {
			printk("shmMap: kalloc failed!\n", (uint32_t)page);
			shmUnmapPages(oldAddr, i);
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
	while(i != NULL) { //search for available memory block
		if(!i->used && i->pages >= pages)
			break;
		i = i->next;
	}

	ShareMemT* tmp = NULL;
	if(i != NULL) { //avaible item found.
		addr =  i->addr;
		if(i->pages > pages) { //try split one item to two;
			tmp = shmNew();
			if(tmp != NULL) {
				tmp->pages = i->pages -  pages;
				tmp->addr = i->addr + (pages * PAGE_SIZE);
				i->pages = pages;
				tmp->next = i->next;
				tmp->prev = i;
				if(i->next != NULL)
					i->next->prev = tmp;
				i->next = tmp;
				if(i == _shmTail)
					_shmTail = tmp;
			}
		}
	}
	else { // not found, need to expand pages for new block.
		i = shmNew();
		if(i == NULL)
			return -1;
		i->addr = addr;
		if(_shmHead == NULL) {
			_shmHead = _shmTail = i;
		}
		else  {
			_shmTail->next = i;
			i->prev = _shmTail;
		}
	}		

	if(i->pages == 0) { // map pages expanded new block
		if(!shmMapPages(addr, pages))
			return -1;
		i->pages = pages;
	}	

	if(addr == _shMemTail)
		_shMemTail += pages * PAGE_SIZE;

	i->used = true;
	i->owner = _currentProcess->pid;
	return i->id;
}

static ShareMemT* shmItem(int32_t id) { //get shm item by id.
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->id == (uint32_t)id)
			return i;
		i = i->next;
	}
	return NULL;
}

static ShareMemT* freeItem(ShareMemT* it) {
	//shmUnmapPages(it->addr, it->pages);
	it->used = false;
	if(it->next != NULL && !it->next->used) { //merge right free items
		ShareMemT* p = it->next;
		it->next = p->next;
		if(p->next != NULL)
			p->next->prev = it;
		else //tail
			_shmTail = it;
		it->pages += p->pages;
		kmfree(p);
	}

	if(it->prev != NULL && !it->prev->used) { //merge left free items
		ShareMemT* p = it->prev;
		p->next = it->next;
		if(it->next != NULL)
			it->next->prev = p;
		else
			_shmTail = p;
		p->pages += it->pages;
		kmfree(it);
		it = p;
	}
	return it->next;
}

void shmfree(int32_t id) {
	ShareMemT* i = shmItem(id);
	if(i == NULL || i->owner != _currentProcess->pid) {
		return;
	}
	freeItem(i);
}

void* shmRaw(int32_t id) {
	ShareMemT* it = shmItem(id);
	if(it == NULL)
		return NULL;
	return (void*)it->addr;
}
	
/*map share memory to process*/
void* shmProcMap(int32_t pid, int32_t id) {
	ShareMemT* it = shmItem(id);
	ProcessT* proc = procGet(pid);
	if(it == NULL || proc == NULL)
		return NULL;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		uint32_t physicalAddr = resolvePhyAddress(_kernelVM, addr);
		mapPage(proc->vm,
				addr,
				physicalAddr,
				AP_RW_RW);
		addr += PAGE_SIZE;
	}
	return (void*)it->addr;
}

/*unmap share memory of process*/
int32_t shmProcUnmap(int32_t pid, int32_t id) {
	ProcessT* proc = procGet(pid);
	ShareMemT* it = shmItem(id);
	if(it == NULL || proc == NULL)
		return -1;

	uint32_t addr = it->addr;
	for (uint32_t i = 0; i < it->pages; i++) {
		unmapPage(proc->vm, addr);
		addr += PAGE_SIZE;
	}
	return 0;
}

/*free all share memory used by the process*/
void shmProcFree(int32_t pid) {
	ShareMemT* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->owner == pid) {
			i = freeItem(i);
		}
		else 
			i = i->next;
	}
}

