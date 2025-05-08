#include <mm/kmalloc_vm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <mm/mmudef.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>


static ewokos_addr_t km_vm_mem_tail = 0;

typedef struct km_vm {
	ewokos_addr_t addr; //memory block base address
	uint32_t pages; //memory pages
	uint32_t used; //used or free
	struct km_vm* next;
	struct km_vm* prev;
} km_vm_t;

static km_vm_t* _km_vm_head = NULL;
static km_vm_t* _km_vm_tail = NULL;

void kmalloc_vm_init() {
	//share memory base address at virtual address 1GB
	km_vm_mem_tail = (uint32_t)ALIGN_UP(KMALLOC_VM_BASE, PAGE_SIZE);
	_km_vm_head = NULL;
	_km_vm_tail = NULL;
}

static km_vm_t* km_vm_new(void) {
	km_vm_t* ret = (km_vm_t*)kmalloc(sizeof(km_vm_t));
	if(ret == NULL)
		return NULL;

	memset(ret, 0, sizeof(km_vm_t));
	return ret;
}

static void km_vm_unmap_pages(page_dir_entry_t *vm, void* addr, uint32_t pages) {
	uint32_t i;
	for (i = 0; i < pages; i++) {
		ewokos_addr_t physical_addr = resolve_phy_address(vm, (ewokos_addr_t)addr);

		//get the kernel address for kalloc/kfree
		ewokos_addr_t kernel_addr = P2V(physical_addr);
		kfree4k((void *)kernel_addr);
		unmap_page(vm, addr);
		addr += PAGE_SIZE;
	}
	flush_tlb();
}

static int32_t km_vm_map_pages(page_dir_entry_t *vm, ewokos_addr_t addr, uint32_t pages) {
	ewokos_addr_t old_addr = addr;
	uint32_t i;
	for (i = 0; i < pages; i++) {
		void *page = kalloc4k();
		if(page == NULL) {
			printf("km_vm_map: kalloc failed!\n", (ewokos_addr_t)page);
			km_vm_unmap_pages(vm, old_addr, i);
			return -1;
		}
		memset(page, 0, PAGE_SIZE);

		map_page(vm,
				addr,
				V2P(page),
				AP_RW_RW, PTE_ATTR_WRBACK);
		addr += PAGE_SIZE;
	}
	flush_tlb();
	return 0;
}

void* kmalloc_vm(page_dir_entry_t *vm, uint32_t size) {
	size = ALIGN_UP(size, PAGE_SIZE);
	ewokos_addr_t addr = km_vm_mem_tail;
	uint32_t pages = (size / PAGE_SIZE)+1;
	
	km_vm_t* i = _km_vm_head;
	while(i != NULL) { //search for available memory block
		if(!i->used && i->pages >= pages)
			break;
		i = i->next;
	}

	if(i != NULL) { //avaible item found.
		addr =  i->addr;
		if(i->pages > pages) { //try split one item to two;
			km_vm_t* tmp = km_vm_new();
			if(tmp != NULL) {
				tmp->pages = i->pages -  pages;
				tmp->addr = i->addr + (pages * PAGE_SIZE);
				i->pages = pages;
				tmp->next = i->next;
				tmp->prev = i;
				if(i->next != NULL)
					i->next->prev = tmp;
				i->next = tmp;
				if(i == _km_vm_tail)
					_km_vm_tail = tmp;
			}
		}
	}
	else { // not found, need to expand pages for new block.
		if((km_vm_mem_tail + size) >= (KMALLOC_VM_BASE + KMALLOC_VM_SIZE)) {
			return NULL;
		}

		i = km_vm_new();
		if(i == NULL) {
			return NULL;
		}
		i->addr = addr;
		if(_km_vm_head == NULL) {
			_km_vm_head = _km_vm_tail = i;
		}
		else  {
			_km_vm_tail->next = i;
			i->prev = _km_vm_tail;
			_km_vm_tail = i;
		}
		i->pages = pages;
	}		

	if(km_vm_map_pages(vm, i->addr, i->pages) != 0) {
		return NULL;
	}

	if(i->addr == km_vm_mem_tail)
		km_vm_mem_tail += i->pages * PAGE_SIZE;

	i->used = 1;
	return (void*)i->addr;
}

static km_vm_t* km_vm_item_by_addr(void* addr) { //get km_vm item by addr.
	km_vm_t* i = _km_vm_head;
	while(i != NULL) {
		if(i->used && i->addr == (ewokos_addr_t)addr) 
			return i;
		i = i->next;
	}
	return NULL;
}

static void free_item(km_vm_t* it) {
	it->used = 0;
	if(it->next != NULL && !it->next->used) { //merge right free items
		km_vm_t* p = it->next;
		it->next = p->next;
		if(p->next != NULL)
			p->next->prev = it;
		else //tail
			_km_vm_tail = it;
		it->pages += p->pages;
		kfree(p);
	}

	if(it->prev != NULL && !it->prev->used) { //merge left free items
		km_vm_t* p = it->prev;
		p->next = it->next;
		if(it->next != NULL)
			it->next->prev = p;
		else
			_km_vm_tail = p;
		p->pages += it->pages;
		kfree(it);
		it = p;
	}
}

static void km_vm_unmap_it(page_dir_entry_t *vm, km_vm_t* it) {
	km_vm_unmap_pages(vm, it->addr, it->pages);
	flush_tlb();
	free_item(it);
}

void kfree_vm(page_dir_entry_t *vm, void* p) {
	km_vm_t* it = km_vm_item_by_addr(p);
	if(it == NULL)
		return;
	km_vm_unmap_it(vm, it);
}