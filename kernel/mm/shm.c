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

static uint32_t shmem_tail = 0;
static uint32_t shmem_count = 0;

typedef struct share_mem {
	uint32_t id; //uniqe accumulating counter
	uint32_t addr; //memory block base address
	uint32_t pages; //memory pages
	uint32_t used; //used or free
	uint32_t flags; 
	int32_t owner; //process id which alloced this shm
	struct share_mem* next;
	struct share_mem* prev;
} share_mem_t;

static share_mem_t* _shmHead = NULL;
static share_mem_t* _shmTail = NULL;

void shm_init() {
	//share memory base address at virtual address phy_mem_size + 256MB
	shmem_tail = (uint32_t)ALIGN_UP(get_phy_ram_size()+256*MB, PAGE_SIZE);
	_shmHead = NULL;
	_shmTail = NULL;
	shmem_count = 0;
}

static share_mem_t* shm_new(void) {
	share_mem_t* ret = (share_mem_t*)km_alloc(sizeof(share_mem_t));
	if(ret == NULL)
		return NULL;

	memset(ret, 0, sizeof(share_mem_t));
	ret->owner = -1;
	ret->id = shmem_count++;
	return ret;
}

static void shm_unmap_pages(uint32_t addr, uint32_t pages) {
	uint32_t i;
	for (i = 0; i < pages; i++) {
		uint32_t physicalAddr = resolve_phy_address(_kernel_vm, addr);

		//get the kernel address for kalloc/kfree
		uint32_t kernelAddr = P2V(physicalAddr);
		unmap_page(_kernel_vm, addr);
		kfree((void *) kernelAddr);
		addr += PAGE_SIZE;
	}
}

static bool shm_map_pages(uint32_t addr, uint32_t pages) {
	uint32_t oldAddr = addr;
	uint32_t i;
	for (i = 0; i < pages; i++) {
		char *page = kalloc();
		if(page == NULL) {
			printk("shm_map: kalloc failed!\n", (uint32_t)page);
			shm_unmap_pages(oldAddr, i);
			return false;
		}
		memset(page, 0, PAGE_SIZE);

		map_page(_kernel_vm,
				addr,
				V2P(page),
				AP_RW_D);
		addr += PAGE_SIZE;
	}
	return true;
}

static int32_t _p_lock = 0;

int32_t shm_alloc(uint32_t size) {
	CRIT_IN(_p_lock)

	uint32_t addr = shmem_tail;
	uint32_t pages = (size / PAGE_SIZE);
	if((size % PAGE_SIZE) != 0)
		pages++;
	
	share_mem_t* i = _shmHead;
	while(i != NULL) { //search for available memory block
		if(!i->used && i->pages >= pages)
			break;
		i = i->next;
	}

	share_mem_t* tmp = NULL;
	if(i != NULL) { //avaible item found.
		addr =  i->addr;
		if(i->pages > pages) { //try split one item to two;
			tmp = shm_new();
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
		i = shm_new();
		if(i == NULL) {
			CRIT_OUT(_p_lock)
			return -1;
		}
		i->addr = addr;
		if(_shmHead == NULL) {
			_shmHead = _shmTail = i;
		}
		else  {
			_shmTail->next = i;
			i->prev = _shmTail;
			_shmTail = i;
		}
	}		

	if(i->pages == 0) { // map pages expanded new block
		if(!shm_map_pages(addr, pages)) {
			CRIT_OUT(_p_lock)
			return -1;
		}
		i->pages = pages;
	}	

	if(addr == shmem_tail)
		shmem_tail += pages * PAGE_SIZE;

	i->used = true;
	i->owner = _current_proc->pid;

	CRIT_OUT(_p_lock)
	return i->id;
}

static share_mem_t* shm_item(int32_t id) { //get shm item by id.
	share_mem_t* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->id == (uint32_t)id)
			return i;
		i = i->next;
	}
	return NULL;
}

static share_mem_t* free_item(share_mem_t* it) {
	//shm_unmap_pages(it->addr, it->pages);
	it->used = false;
	if(it->next != NULL && !it->next->used) { //merge right free items
		share_mem_t* p = it->next;
		it->next = p->next;
		if(p->next != NULL)
			p->next->prev = it;
		else //tail
			_shmTail = it;
		it->pages += p->pages;
		km_free(p);
	}

	if(it->prev != NULL && !it->prev->used) { //merge left free items
		share_mem_t* p = it->prev;
		p->next = it->next;
		if(it->next != NULL)
			it->next->prev = p;
		else
			_shmTail = p;
		p->pages += it->pages;
		km_free(it);
		it = p;
	}
	return it->next;
}

void shm_free(int32_t id) {
	CRIT_IN(_p_lock)
	share_mem_t* i = shm_item(id);
	if(i == NULL || i->owner != _current_proc->pid) {
		CRIT_OUT(_p_lock)
		return;
	}
	free_item(i);
	CRIT_OUT(_p_lock)
}

void* shm_raw(int32_t id) {
	CRIT_IN(_p_lock)
	share_mem_t* it = shm_item(id);
	CRIT_OUT(_p_lock)

	if(it == NULL)
		return NULL;
	return (void*)it->addr;
}
	
/*map share memory to process*/
void* shm_proc_map(int32_t pid, int32_t id) {
	CRIT_IN(_p_lock)
	share_mem_t* it = shm_item(id);
	process_t* proc = proc_get(pid);
	if(it == NULL || proc == NULL) {
		CRIT_OUT(_p_lock)
		return NULL;
	}

	uint32_t addr = it->addr;
	uint32_t i;
	for (i = 0; i < it->pages; i++) {
		uint32_t physicalAddr = resolve_phy_address(_kernel_vm, addr);
		map_page(proc->space->vm,
				addr,
				physicalAddr,
				AP_RW_RW);
		addr += PAGE_SIZE;
	}
	CRIT_OUT(_p_lock)
	return (void*)it->addr;
}

/*unmap share memory of process*/
int32_t shm_proc_unmap(int32_t pid, int32_t id) {
	CRIT_IN(_p_lock)
	process_t* proc = proc_get(pid);
	share_mem_t* it = shm_item(id);
	if(it == NULL || proc == NULL) {
		CRIT_OUT(_p_lock)
		return -1;
	}

	uint32_t addr = it->addr;
	uint32_t i;
	for (i = 0; i < it->pages; i++) {
		unmap_page(proc->space->vm, addr);
		addr += PAGE_SIZE;
	}
	CRIT_OUT(_p_lock)
	return 0;
}

/*free all share memory used by the process*/
void shm_proc_free(int32_t pid) {
	CRIT_IN(_p_lock)
	share_mem_t* i = _shmHead;
	while(i != NULL) {
		if(i->used && i->owner == pid) {
			i = free_item(i);
		}
		else 
			i = i->next;
	}
	CRIT_OUT(_p_lock)
}

