#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <mm/mmudef.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>

#define 	IPC_PRIVATE 0
#define 	IPC_CREAT   00001000 /* create if key is nonexistent */
#define 	IPC_EXCL    00002000 /* fail if key exists */

static ewokos_addr_t shmem_tail = 0;
static int32_t id_counter = 1;

typedef struct share_mem {
	int32_t id;
	int32_t key;
	ewokos_addr_t addr; //memory block base address
	uint32_t pages; //memory pages
	uint32_t used; //used or free
	int32_t flag; 
	int32_t owner_pid; //process id which alloced this shm
	int32_t refs;
	struct share_mem* next;
	struct share_mem* prev;
} share_mem_t;

static share_mem_t* _shm_head = NULL;
static share_mem_t* _shm_tail = NULL;

void shm_init() {
	//share memory base address at virtual address 1GB
	shmem_tail = (uint32_t)ALIGN_UP(SHM_BASE, PAGE_SIZE);
	_shm_head = NULL;
	_shm_tail = NULL;
	id_counter = 1;
}

static share_mem_t* shm_new(void) {
	share_mem_t* ret = (share_mem_t*)kmalloc(sizeof(share_mem_t));
	if(ret == NULL)
		return NULL;

	memset(ret, 0, sizeof(share_mem_t));
	ret->owner_pid = -1;
	return ret;
}

static void shm_unmap_pages(uint32_t addr, uint32_t pages) {
	uint32_t i;
	for (i = 0; i < pages; i++) {
		uint32_t physical_addr = resolve_phy_address(_kernel_vm, addr);

		//get the kernel address for kalloc/kfree
		uint32_t kernel_addr = P2V(physical_addr);
		kfree4k((void *) kernel_addr);
		unmap_page(_kernel_vm, addr);
		addr += PAGE_SIZE;
	}
	flush_tlb();
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
				AP_RW_D, PTE_ATTR_NOCACHE);
		addr += PAGE_SIZE;
	}
	flush_tlb();
	return 1;
}

static int32_t shm_alloc(int32_t key, uint32_t size, int32_t flag) {
	size = ALIGN_UP(size, 32);
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
		if((shmem_tail + size) >= (SHM_BASE + SHM_MAX_SIZE))
			return -1;

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
	i->key = key;
	i->id = id_counter++;
	i->owner_pid = get_current_proc()->info.pid;
	i->flag = flag;

	return i->id;
}

static share_mem_t* shm_item_by_key(int32_t key) { //get shm item by key.
	share_mem_t* i = _shm_head;
	while(i != NULL) {
		if(i->used && i->key == key)
			return i;
		i = i->next;
	}
	return NULL;
}

int32_t shm_get(int32_t key, uint32_t size, int32_t flag) {
	if(key != IPC_PRIVATE) {
		share_mem_t* it = shm_item_by_key(key);
		if(it != NULL) {
			if((flag & IPC_EXCL) != 0)
				return -1;
			return it->id;
		}
		else if((flag & IPC_CREAT) == 0)
			return -1;
	}
	else {
		flag = 0666;
	}

	return shm_alloc(key, size, (flag & 0666));
}

static share_mem_t* shm_item_by_id(int32_t id) { //get shm item by id.
	share_mem_t* i = _shm_head;
	while(i != NULL) {
		if(i->used && i->id == id)
			return i;
		i = i->next;
	}
	return NULL;
}

static share_mem_t* shm_item_by_addr(void* addr) { //get shm item by addr.
	share_mem_t* i = _shm_head;
	while(i != NULL) {
		if(i->used && i->addr == addr) 
			return i;
		i = i->next;
	}
	return NULL;
}

uint32_t shm_alloced_size(void) {
	uint32_t ret = 0;
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

#define SHM_R 0x1
#define SHM_W 0x2
#define SHM_N 0x0

static uint32_t check_access(proc_t* proc, share_mem_t* it) {
	if(proc->info.uid == 0)
		return (SHM_R | SHM_W);
	
	proc_t* owner = proc_get(it->owner_pid);
	owner = proc_get_proc(owner);
	if(owner == NULL)
		return SHM_N;
	
	if(owner == proc)
		return (SHM_R | SHM_W);

	if(it->key == IPC_PRIVATE) { //family only
		if(proc_childof(proc, owner) == 0)
				return (SHM_R | SHM_W); //passed
		return SHM_N;
	}

	int32_t a = 0;
	if(owner->info.uid == proc->info.uid)
		a = (it->flag >> 6) & 0x7;
	else if(owner->info.gid == proc->info.gid)
		a = (it->flag >> 3) & 0x7;
	else
		a = it->flag & 0x7;

	uint32_t res = SHM_N;
	if((a & 0x4) != 0)
		res |= SHM_R;
	if((a & 0x2) != 0)
		res |= SHM_W;
	return res;
}
	
/*map share memory to process*/
void* shm_proc_map(proc_t* proc, int32_t id) {
	share_mem_t* it = shm_item_by_id(id);
	if(it == NULL || proc == NULL) {
		return NULL;
	}

	uint32_t access = check_access(proc, it);
	//uint32_t access = SHM_W | SHM_R;
	if(access == SHM_N)
		return NULL;

	uint32_t i;
	//check if mapped , keep it and return
	for (i = 0; i < SHM_MAX; i++) {
		if(proc->space->shms[i] == id)
			return (void*)it->addr;
	}

	//do real map
	for (i = 0; i < SHM_MAX; i++) {
		if(proc->space->shms[i] == 0) {
			proc->space->shms[i] = id;
			break;
		}
	}
	if(i >= SHM_MAX) {
		return NULL;
	}

	if((access & SHM_W) != 0)
		access = AP_RW_RW;
	else
		access = AP_RW_R;

	uint32_t addr = it->addr;
	for (i = 0; i < it->pages; i++) {
		uint32_t physical_addr = resolve_phy_address(_kernel_vm, addr);
		map_page(proc->space->vm,
				addr,
				physical_addr,
				access, PTE_ATTR_WRBACK);
		addr += PAGE_SIZE;
	}
	flush_tlb();
	proc->info.shm_size += it->pages * PAGE_SIZE;
	it->refs++;
	return (void*)it->addr;
}

/*unmap share memory of process*/
static int32_t shm_proc_unmap_it(proc_t* proc, share_mem_t* it, bool free_it) {
	uint32_t i;
	for (i = 0; i < SHM_MAX; i++) {
		if(proc->space->shms[i] == it->id) {
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
	flush_tlb();
	if(proc->info.shm_size > (it->pages*PAGE_SIZE))
		proc->info.shm_size -= it->pages * PAGE_SIZE;
	else
		proc->info.shm_size = 0;

	it->refs--;
	if(free_it && it->refs <= 0) {
		free_item(it);
	}
	return 0;
}

void*   shm_map(proc_t* proc, int32_t key, uint32_t size, int32_t flag, int32_t* id) {
	*id = -1;
	int32_t sid = shm_get(key, size, flag);
	if(sid <= 0)
		return NULL;
	void* ret = shm_proc_map(proc, sid);
	if(ret == NULL)
		return NULL;
	*id = sid;
	return ret;
}

int32_t shm_proc_unmap_by_id(proc_t* proc, uint32_t id, bool free_it) {
	share_mem_t* it = shm_item_by_id(id);
	if(it == NULL)
		return -1;
	return shm_proc_unmap_it(proc, it, free_it);
}

int32_t shm_set_owner(uint32_t id, int32_t pid) {
	share_mem_t* it = shm_item_by_id(id);
	if(it == NULL || !it->used)
		return -1;
	it->owner_pid = pid;	
}

/*unmap share memory of process*/
int32_t shm_proc_unmap(proc_t* proc, void* p) {
	share_mem_t* it = shm_item_by_addr(p);
	if(it == NULL || proc == NULL)
		return -1;

	return shm_proc_unmap_it(proc, it, true);
}