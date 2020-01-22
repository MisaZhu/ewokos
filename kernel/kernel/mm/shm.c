#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kstring.h>
#include <kprintf.h>

static uint32_t shmem_tail = 0;
static uint32_t shmem_count = 1; //id start with 1 not 0

typedef struct share_mem {
	uint32_t id; //uniqe accumulating counter
	uint32_t addr; //memory block base address
	uint32_t pages; //memory pages
	uint32_t used; //used or free
	int32_t flag; 
	int32_t owner; //process id which alloced this shm
	int32_t refs;
	struct share_mem* next;
	struct share_mem* prev;
} share_mem_t;

static share_mem_t* _shm_head = NULL;
static share_mem_t* _shm_tail = NULL;

void shm_init() {
	//share memory base address at virtual address 1GB
	shmem_tail = (uint32_t)ALIGN_UP(1*GB, PAGE_SIZE);
	_shm_head = NULL;
	_shm_tail = NULL;
	shmem_count = 1;
}

static share_mem_t* shm_new(void) {
	share_mem_t* ret = (share_mem_t*)kmalloc(sizeof(share_mem_t));
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
		uint32_t physical_addr = resolve_phy_address(_kernel_vm, addr);

		//get the kernel address for kalloc/kfree
		uint32_t kernel_addr = P2V(physical_addr);
		unmap_page(_kernel_vm, addr);
		kfree((void *) kernel_addr);
		addr += PAGE_SIZE;
	}
	_flush_tlb();
}

static int32_t shm_map_pages(uint32_t addr, uint32_t pages) {
	uint32_t old_addr = addr;
	uint32_t i;
	for (i = 0; i < pages; i++) {
		char *page = kalloc4k();
		if(page == NULL) {
			printf("shm_map: kalloc failed!\n", (uint32_t)page);
			shm_unmap_pages(old_addr, i);
			return 0;
		}
		memset(page, 0, PAGE_SIZE);

		map_page(_kernel_vm,
				addr,
				V2P(page),
				AP_RW_D);
		addr += PAGE_SIZE;
	}
	return 1;
}

int32_t shm_alloc(uint32_t size, int32_t flag) {
	uint32_t addr = shmem_tail;
	uint32_t pages = (size / PAGE_SIZE);
	if((size % PAGE_SIZE) != 0)
		pages++;
	
	share_mem_t* i = _shm_head;
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
				if(i == _shm_tail)
					_shm_tail = tmp;
			}
		}
	}
	else { // not found, need to expand pages for new block.
		i = shm_new();
		if(i == NULL) {
			return -1;
		}
		i->addr = addr;
		if(_shm_head == NULL) {
			_shm_head = _shm_tail = i;
		}
		else  {
			_shm_tail->next = i;
			i->prev = _shm_tail;
			_shm_tail = i;
		}
	}		

	if(i->pages == 0) { // map pages expanded new block
		if(!shm_map_pages(addr, pages)) {
			return -1;
		}
		i->pages = pages;
	}	

	if(addr == shmem_tail)
		shmem_tail += pages * PAGE_SIZE;

	i->used = 1;
	i->owner = _current_proc->pid;
	i->flag = flag;

	return i->id;
}

static share_mem_t* shm_item(int32_t id) { //get shm item by id.
	share_mem_t* i = _shm_head;
	while(i != NULL) {
		if(i->used && i->id == (uint32_t)id)
			return i;
		i = i->next;
	}
	return NULL;
}

int32_t shm_alloced_size(void) {
	int32_t ret = 0;
	share_mem_t* i = _shm_head;
	while(i != NULL) {
		if(i->used) {
			ret += i->pages * PAGE_SIZE;
		}
		i = i->next;
	}
	return ret;
}


static share_mem_t* free_item(share_mem_t* it) {
	//shm_unmap_pages(it->addr, it->pages);
	it->used = 0;
	if(it->next != NULL && !it->next->used) { //merge right free items
		share_mem_t* p = it->next;
		it->next = p->next;
		if(p->next != NULL)
			p->next->prev = it;
		else //tail
			_shm_tail = it;
		it->pages += p->pages;
		kfree(p);
	}

	if(it->prev != NULL && !it->prev->used) { //merge left free items
		share_mem_t* p = it->prev;
		p->next = it->next;
		if(it->next != NULL)
			it->next->prev = p;
		else
			_shm_tail = p;
		p->pages += it->pages;
		kfree(it);
		it = p;
	}
	return it->next;
}

void* shm_raw(int32_t id) {
	share_mem_t* it = shm_item(id);

	if(it == NULL)
		return NULL;
	return (void*)it->addr;
}

static int32_t check_owner(proc_t* proc, share_mem_t* it) {
	if(it->flag != 0)  //1 for public, 0 for family only
		return 0;

	while(proc != NULL) {
		if(proc->pid == it->owner)
			return 0; //passed
		proc = proc_get(proc->father_pid);
	}
	return -1; //not passed!
}
		
/*map share memory to process*/
int32_t shm_proc_ref(int32_t pid, int32_t id) {
	share_mem_t* it = shm_item(id);
	proc_t* proc = proc_get(pid);
	if(it == NULL || proc == NULL)
		return -1;

	if(check_owner(proc, it) != 0)
		return -1;
	return it->refs;
}

	
/*map share memory to process*/
void* shm_proc_map(int32_t pid, int32_t id) {
	share_mem_t* it = shm_item(id);
	proc_t* proc = proc_get(pid);
	if(it == NULL || proc == NULL)
		return NULL;

	if(check_owner(proc, it) != 0)
		return NULL;

	uint32_t i;
	for (i = 0; i < SHM_MAX; i++) {
		if(proc->space->shms[i] == 0) {
			proc->space->shms[i] = id;
			break;
		}
	}
	if(i >= SHM_MAX)
		return NULL;

	uint32_t addr = it->addr;
	for (i = 0; i < it->pages; i++) {
		uint32_t physical_addr = resolve_phy_address(_kernel_vm, addr);
		map_page(proc->space->vm,
				addr,
				physical_addr,
				AP_RW_RW);
		addr += PAGE_SIZE;
	}
	it->refs++;
	return (void*)it->addr;
}

/*unmap share memory of process*/
int32_t shm_proc_unmap(int32_t pid, int32_t id) {
	proc_t* proc = proc_get(pid);
	share_mem_t* it = shm_item(id);
	if(it == NULL || proc == NULL)
		return -1;

	/*if(check_owner(proc, it) != 0)
		return -1;
	*/

	uint32_t i;
	for (i = 0; i < SHM_MAX; i++) {
		if(proc->space->shms[i] == id) {
			proc->space->shms[i] = 0;
			break;
		}
	}
	if(i >= SHM_MAX)
		return -1;

	uint32_t addr = it->addr;
	for (i = 0; i < it->pages; i++) {
		unmap_page(proc->space->vm, addr);
		addr += PAGE_SIZE;
	}

	it->refs--;
	if(it->refs <= 0) {
		free_item(it);
	}
	_flush_tlb();
	return 0;
}

