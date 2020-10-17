#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/schedule.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <kernel/kevqueue.h>
#include <kstring.h>
#include <kprintf.h>
#include <queue.h>
#include <kernel/elf.h>
#include <stddef.h>

static proc_t _proc_table[PROC_MAX];
__attribute__((__aligned__(PAGE_DIR_SIZE))) 
static page_dir_entry_t _proc_vm[PROC_MAX][PAGE_DIR_NUM];
static queue_t _ready_queue;

proc_t* _current_proc = NULL;
bool _core_ready = false;
int32_t _core_pid = -1;

/* proc_init initializes the process sub-system. */
void procs_init(void) {
	_core_ready = false;
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		_proc_table[i].info.state = UNUSED;
		_proc_table[i].info.wait_for = -1;
	}
	_current_proc = NULL;
	_core_pid = -1;
	queue_init(&_ready_queue);
}

proc_t* proc_get(int32_t pid) {
	if(pid < 0 || pid >= PROC_MAX ||
			_proc_table[pid].info.state == UNUSED ||
			_proc_table[pid].info.state == ZOMBIE)
		return NULL;
	return &_proc_table[pid];
}

static inline uint32_t proc_get_user_stack_pages(proc_t* proc) {
	return proc->info.type == PROC_TYPE_PROC ? STACK_PAGES: THREAD_STACK_PAGES;
}

static inline  uint32_t proc_get_user_stack_base(proc_t* proc) {
	(void)proc;
  uint32_t proc_stack_base = USER_STACK_TOP - STACK_PAGES*PAGE_SIZE;
	if(proc->info.type == PROC_TYPE_PROC)
		return proc_stack_base;
  return proc_stack_base - proc->info.pid*THREAD_STACK_PAGES*PAGE_SIZE;
}

static void* proc_get_mem_tail(void* p) {
	proc_t* proc = (proc_t*)p;
	return (void*)proc->space->heap_size;
}

static int32_t proc_expand(void* p, int32_t page_num) {
	return proc_expand_mem((proc_t*)p, page_num);
}

static void proc_shrink(void* p, int32_t page_num) {
	proc_shrink_mem((proc_t*)p, page_num);
}

static void proc_init_space(proc_t* proc) {
	page_dir_entry_t *vm = _proc_vm[proc->info.pid];
	set_kernel_vm(vm);
	proc->space = (proc_space_t*)kmalloc(sizeof(proc_space_t));
	memset(proc->space, 0, sizeof(proc_space_t));

	proc->space->vm = vm;
	proc->space->heap_size = 0;
	proc->space->malloc_man.arg = (void*)proc;
	proc->space->malloc_man.head = NULL;
	proc->space->malloc_man.tail = NULL;
	proc->space->malloc_man.expand = proc_expand;
	proc->space->malloc_man.shrink = proc_shrink;
	proc->space->malloc_man.get_mem_tail = proc_get_mem_tail;
}

void proc_switch(context_t* ctx, proc_t* to, bool quick){
	if(to == NULL || to == _current_proc)
		return;
	
	if(_current_proc != NULL && _current_proc->info.state != UNUSED) {
		memcpy(&_current_proc->ctx, ctx, sizeof(context_t));
		if(_current_proc->info.state == RUNNING) {
			_current_proc->info.state = READY;
			if(quick)
				queue_push_head(&_ready_queue, _current_proc);
			else
				queue_push(&_ready_queue, _current_proc);
		}	
	}

	memcpy(ctx, &to->ctx, sizeof(context_t));

	if(_current_proc != to) {
		page_dir_entry_t *vm = to->space->vm;
		set_translation_table_base((uint32_t) V2P(vm));
		_current_proc = to;
	}
}

/* proc_exapnad_memory expands the heap size of the given process. */
int32_t proc_expand_mem(proc_t *proc, int32_t page_num) {
	int32_t i;
	int32_t res = 0;

	for (i = 0; i < page_num; i++) {
		char *page = kalloc4k();
		if(page == NULL) {
			printf("proc expand failed!! free mem size: (%x), pid:%d, pages ask:%d\n", get_free_mem_size(), proc->info.pid, page_num);
			proc_shrink_mem(proc, i);
			res = -1;
			break;
		}
		memset(page, 0, PAGE_SIZE);
		map_page(proc->space->vm,
				proc->space->heap_size,
				V2P(page),
				AP_RW_RW, 0);
		proc->space->heap_size += PAGE_SIZE;
	}
	vm_flush_tlb(proc->space->vm);
	return res;
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void proc_shrink_mem(proc_t* proc, int32_t page_num) {
	if(page_num <= 0)
		return;

	int32_t i;
	for (i = 0; i < page_num; i++) {
		uint32_t virtual_addr = proc->space->heap_size - PAGE_SIZE;
		uint32_t kernel_addr = resolve_kernel_address(proc->space->vm, virtual_addr);
		//get the kernel address for kalloc4k/kfree4k
		kfree4k((void *) kernel_addr);

		unmap_page(proc->space->vm, virtual_addr);
		proc->space->heap_size -= PAGE_SIZE;
		if (proc->space->heap_size == 0)
			break;
	}
	vm_flush_tlb(proc->space->vm);
}

static void proc_unmap_shms(proc_t *proc) {
	int32_t i;
	for(i=0; i<SHM_MAX; i++) {
		int32_t shm = proc->space->shms[i];
		if(shm > 0)
			shm_proc_unmap(proc->info.pid, shm);
	}
}

static void proc_free_space(proc_t *proc) {
	if(proc->info.type != PROC_TYPE_PROC)
		return;

	/*unmap share mems*/
	proc_unmap_shms(proc);

	/*free file info*/
	proc_shrink_mem(proc, proc->space->heap_size / PAGE_SIZE);

	free_page_tables(proc->space->vm);
	kfree(proc->space);
}

void proc_ready(proc_t* proc) {
	if(proc == NULL)
		return;

	proc->info.state = READY;
	if(queue_in(&_ready_queue, proc) == NULL)
		queue_push_head(&_ready_queue, proc);
}

proc_t* proc_get_next_ready(void) {
	proc_t* next = queue_pop(&_ready_queue);
	while(next != NULL && next->info.state != READY)
		next = queue_pop(&_ready_queue);

	if(next == NULL && _core_ready) {
		next = &_proc_table[_core_pid];
		if(next->info.state == UNUSED || next->info.state == ZOMBIE || next->info.state == CREATED)
			return NULL;
		proc_ready(next);
	}
	return next;
}

static void proc_unready(context_t* ctx, proc_t* proc, int32_t state) {
	queue_item_t* it = queue_in(&_ready_queue, proc);	
	if(it != NULL)
		queue_remove(&_ready_queue, it);

	proc->info.state = state;
	if(_current_proc == proc) {
		schedule(ctx);
	}
	else {
		memcpy(&proc->ctx, ctx, sizeof(context_t));
	}
}

static void proc_wakeup_waiting(int32_t pid) {
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		proc_t *proc = &_proc_table[i];
		if (proc->info.state == WAIT && proc->info.wait_for == pid) {
			proc->info.wait_for = -1;
			proc_ready(proc);
		}
	}
}

static void proc_terminate(context_t* ctx, proc_t* proc) {
	if(proc->info.state == ZOMBIE || proc->info.state == UNUSED)
		return;

	if(proc->info.type == PROC_TYPE_PROC) {
		kevent_t* kev = kev_push(KEV_PROC_EXIT, NULL);
		PF->addi(kev->data, proc->info.pid);
	}

	proc_unready(ctx, proc, ZOMBIE);
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		proc_t *p = &_proc_table[i];
		/*terminate forked from this proc*/
		if(p->info.father_pid == proc->info.pid) { //terminate forked children, skip reloaded ones
			proc_exit(ctx, p, 0);
		}
	}

	proc_wakeup_waiting(proc->info.pid);
}

static inline void proc_init_user_stack(proc_t* proc) {
	uint32_t i;
	uint32_t user_stack_base =  proc_get_user_stack_base(proc);
	uint32_t pages = proc_get_user_stack_pages(proc);
	for(i=0; i<pages; i++) {
		proc->user_stack[i] = kalloc4k();
		map_page(proc->space->vm,
			user_stack_base + PAGE_SIZE*i,
			V2P(proc->user_stack[i]),
			AP_RW_RW, 0);
	}
	vm_flush_tlb(proc->space->vm);
	proc->ctx.sp = user_stack_base + pages*PAGE_SIZE;
}

static inline void proc_free_user_stack(proc_t* proc) {
	/*free user_stack*/
	uint32_t user_stack_base = proc_get_user_stack_base(proc);
	uint32_t pages = proc_get_user_stack_pages(proc);
	uint32_t i;
	for(i=0; i<pages; i++) {
		unmap_page(proc->space->vm, user_stack_base + PAGE_SIZE*i);
		kfree4k(proc->user_stack[i]);
	}
	vm_flush_tlb(proc->space->vm);
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	proc_terminate(ctx, proc);
	proc->info.state = UNUSED;

	/*free kpage*/
	if(proc->space->kpage != 0) {
		unmap_page(proc->space->vm, proc->space->kpage);
		kfree4k((char*)proc->space->kpage);
		proc->space->kpage = 0;
	}

	/*free user_stack*/
	proc_free_user_stack(proc);
	PF->clear(&proc->ipc_ctx.data);
	proc_free_space(proc);
	memset(proc, 0, sizeof(proc_t));
}

void* proc_malloc(uint32_t size) {
	if(size == 0)
		return NULL;
	return trunk_malloc(&_current_proc->space->malloc_man, size);
}

void* proc_realloc(void* p, uint32_t size) {
	if(size == 0) {
		proc_free(p);
		return NULL;
	}
	return trunk_realloc(&_current_proc->space->malloc_man, p, size);
}

void proc_free(void* p) {
	if(p == NULL)
		return;
	trunk_free(&_current_proc->space->malloc_man, p);
}

/* proc_creates allocates a new process and returns it. */
proc_t *proc_create(int32_t type, proc_t* parent) {
	int32_t index = -1;
	uint32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		if (_proc_table[i].info.state == UNUSED) {
			index = i;
			break;
		}
	}
	if (index < 0)
		return NULL;

	proc_t *proc = &_proc_table[index];
	memset(proc, 0, sizeof(proc_t));
	proc->info.pid = index;
	proc->info.type = type;
	proc->info.father_pid = -1;
	proc->info.state = CREATED;
	if(type == PROC_TYPE_PROC) {
		proc_init_space(proc);
		proc->info.cmd[0] = 0;
	}
	else {
		proc->space = parent->space;
		strcpy(proc->info.cmd, parent->info.cmd);
	}

	proc_init_user_stack(proc);
	proc->ctx.cpsr = 0x50;
	proc->info.start_sec = _kernel_sec;
	return proc;
}

static void proc_free_heap(proc_t* proc) {
	proc_shrink_mem(proc, proc->space->heap_size/PAGE_SIZE);
	proc->space->malloc_man.head = NULL;
	proc->space->malloc_man.tail = NULL;
}

/* proc_load loads the given ELF process image into the given process. */
int32_t proc_load_elf(proc_t *proc, const char *image, uint32_t size) {
	uint32_t prog_header_offset = 0;
	uint32_t prog_header_count = 0;
	uint32_t i = 0;

	char* proc_image = kmalloc(size);
	memcpy(proc_image, image, size);

	if(proc->info.type == PROC_TYPE_VFORK) {
		proc_free_user_stack(proc);
		proc->info.type = PROC_TYPE_PROC;
		proc_init_space(proc);
		page_dir_entry_t *vm = proc->space->vm;
		set_translation_table_base((uint32_t) V2P(vm));
		proc_init_user_stack(proc);

		proc_t* father = proc_get(proc->info.father_pid);
		if(father != NULL &&
				father->info.block_by == proc->info.pid &&
				father->block_event == (uint32_t)proc->info.pid)
			proc_ready(father);
	}
	else {
		proc_free_heap(proc);
	}

	/*read elf format from saved proc image*/
	struct elf_header *header = (struct elf_header *) proc_image;
	if (header->type != ELFTYPE_EXECUTABLE)
		return -1;

	prog_header_offset = header->phoff;
	prog_header_count = header->phnum;

	for (i = 0; i < prog_header_count; i++) {
		uint32_t j = 0;
		struct elf_program_header *header = (void *) (proc_image + prog_header_offset);
		/* make enough room for this section */
		while (proc->space->heap_size < header->vaddr + header->memsz) {
			if(proc_expand_mem(proc, 1) != 0){ 
				kfree(proc_image);
				return -1;
			}
		}
		/* copy the section from kernel to proc mem space*/
		uint32_t hvaddr = header->vaddr;
		uint32_t hoff = header->off;
		for (j = 0; j < header->memsz; j++) {
			uint32_t vaddr = hvaddr + j; /*vaddr in elf (proc vaddr)*/
			uint32_t vkaddr = resolve_kernel_address(proc->space->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/

			uint32_t image_off = hoff + j;
			*(char*)vkaddr = proc_image[image_off];
		}
		prog_header_offset += sizeof(struct elf_program_header);
	}

	uint32_t user_stack_base =  proc_get_user_stack_base(proc);
	uint32_t pages = proc_get_user_stack_pages(proc);
	proc->ctx.sp = user_stack_base + pages*PAGE_SIZE;
	proc->ctx.pc = header->entry;
	proc->ctx.lr = header->entry;
	proc_ready(proc);
	kfree(proc_image);
	return 0;
}

void proc_usleep(context_t* ctx, uint32_t count) {
	if(_current_proc == NULL)
		return;

	_current_proc->sleep_counter = count;
	proc_unready(ctx, _current_proc, SLEEPING);
}

void proc_block_on(context_t* ctx, int32_t pid_by, uint32_t event) {
	if(_current_proc == NULL)
		return;

	_current_proc->block_event = event;
	_current_proc->info.block_by = pid_by;
	proc_unready(ctx, _current_proc, BLOCK);
}

void proc_waitpid(context_t* ctx, int32_t pid) {
	if(_current_proc == NULL || _proc_table[pid].info.state == UNUSED)
		return;

	_current_proc->info.wait_for = pid;
	proc_unready(ctx, _current_proc, WAIT);
}

void proc_wakeup(int32_t pid, uint32_t event) {
	int32_t i = 0;	
	while(1) {
		if(i >= PROC_MAX)
			break;
		proc_t* proc = &_proc_table[i];	
		if(proc->info.state == BLOCK && 
				(proc->block_event == event || event == 0) && 
				proc->info.block_by == pid) {
			proc->block_event = 0;
			proc->info.block_by = -1;
			if(proc->sleep_counter == 0) {
				//proc->space->ipc.ctx.proc_state = READY;
				proc_ready(proc);
			}
			else
				proc->info.state = SLEEPING;
		}
		i++;
	}
}

static void proc_page_clone(proc_t* to, uint32_t to_addr, proc_t* from, uint32_t from_addr) {
	char *to_ptr = (char*)resolve_kernel_address(to->space->vm, to_addr);
	char *from_ptr = (char*)resolve_kernel_address(from->space->vm, from_addr);
	memcpy(to_ptr, from_ptr, PAGE_SIZE);
}

static int32_t proc_clone(proc_t* child, proc_t* parent) {
	uint32_t pages = parent->space->heap_size / PAGE_SIZE;
	if((parent->space->heap_size % PAGE_SIZE) != 0)
		pages++;

	uint32_t p;
	for(p=0; p<pages; ++p) {
		uint32_t v_addr = (p * PAGE_SIZE);
		/*
		page_table_entry_t * pge = get_page_table_entry(parent->space->vm, v_addr);
		if(pge->permissions == AP_RW_R) {
			uint32_t phy_page_addr = resolve_phy_address(parent->space->vm, v_addr);
			map_page(child->space->vm, 
					child->space->heap_size,
					phy_page_addr,
					AP_RW_R, 0);
			child->space->heap_size += PAGE_SIZE;
		}
		*/
		//else {
			if(proc_expand_mem(child, 1) != 0) {
				printf("Panic: kfork expand memory failed!!(%d)\n", parent->info.pid);
				return -1;
			}
			// copy parent's memory to child's memory
			proc_page_clone(child, v_addr, parent, v_addr);
		//}
	}

	/*set father*/
	child->info.father_pid = parent->info.pid;
	/* copy parent's stack to child's stack */
	//proc_page_clone(child, child->user_stack, parent, parent->user_stack);
	int32_t i;
	for(i=0; i<STACK_PAGES; i++) {
		memcpy(child->user_stack[i], parent->user_stack[i], PAGE_SIZE);
	}

	strcpy(child->info.cmd, parent->info.cmd);
	return 0;
}

static inline void proc_vfork_clone(context_t* ctx, proc_t* child, proc_t* parent) {
	uint32_t pages = proc_get_user_stack_pages(child) - 1;
	uint32_t v_addr =  proc_get_user_stack_base(child) + pages*PAGE_SIZE;
	uint32_t stack_top = v_addr + PAGE_SIZE;
	uint32_t ppages = proc_get_user_stack_pages(parent) - 1;
	uint32_t pv_addr =  proc_get_user_stack_base(parent) + ppages*PAGE_SIZE;
	uint32_t pstack_top = pv_addr + PAGE_SIZE;
	uint32_t i;

	for(i=0; i<pages; i++) {
		proc_page_clone(child, v_addr, parent, pv_addr);
		v_addr -= PAGE_SIZE;
		pv_addr -= PAGE_SIZE;
	}
	child->ctx.sp = stack_top - (pstack_top - ctx->sp);
}

proc_t* kfork_raw(context_t* ctx, int32_t type, proc_t* parent) {
	proc_t *child = NULL;
	child = proc_create(type, parent);
	if(child == NULL) {
		printf("panic: kfork create proc failed!!(%d)\n", parent->info.pid);
		return NULL;
	}
	child->info.father_pid = parent->info.pid;
	child->info.owner = parent->info.owner;

	if(type == PROC_TYPE_PROC) {
		if(proc_clone(child, parent) != 0) {
			printf("panic: kfork clone failed!!(%d)\n", parent->info.pid);
			return NULL;
		}
	}
	else {
		proc_vfork_clone(ctx, child, parent);
	}
	return child;
}

proc_t* kfork(context_t* ctx, int32_t type) {
	proc_t* child = kfork_raw(ctx, type, _current_proc);
	if(_core_ready && (child->info.type == PROC_TYPE_PROC || child->info.type == PROC_TYPE_VFORK)) {
		kevent_t* kev = kev_push(KEV_PROC_CREATED, NULL);
		PF->addi(kev->data, _current_proc->info.pid)->
			addi(kev->data, child->info.pid);
	}
	else
		proc_ready(child);
	return child;
}

static int32_t get_procs_num(void) {
	int32_t res = 0;
	int32_t i;
	for(i=0; i<PROC_MAX; i++) {
		if(_proc_table[i].info.state != UNUSED &&
				(_current_proc->info.owner == 0 ||
				 _proc_table[i].info.owner == _current_proc->info.owner)) {
			res++;
		}
	}
	return res;
}

procinfo_t* get_procs(int32_t *num) {
	*num = get_procs_num();
	if(*num == 0)
		return NULL;

	/*need to be freed later used!*/
	procinfo_t* procs = (procinfo_t*)proc_malloc(sizeof(procinfo_t)*(*num));
	if(procs == NULL)
		return NULL;

	int32_t j = 0;
	int32_t i;
	for(i=0; i<PROC_MAX && j<(*num); i++) {
		if(_proc_table[i].info.state != UNUSED && 
				(_current_proc->info.owner == 0 ||
				 _proc_table[i].info.owner == _current_proc->info.owner)) {
			memcpy(&procs[j], &_proc_table[i].info, sizeof(procinfo_t));
			j++;
		}
	}

	*num = j;
	return procs;
}

void renew_sleep_counter(uint32_t usec) {
	int i;
	for(i=0; i<PROC_MAX; i++) {
		proc_t* proc = &_proc_table[i];
		if(proc->info.state == SLEEPING) {
			proc->sleep_counter -= usec;
			if(proc->sleep_counter <= 0) {
				proc->sleep_counter = 0;
				proc_ready(proc);
			}
		}
	}
}

proc_t* proc_get_proc(proc_t* proc) {
	while(proc != NULL) {
		if(proc->info.type == PROC_TYPE_PROC || proc->info.type == PROC_TYPE_VFORK)
			return proc;
		proc = proc_get(proc->info.father_pid);
	}
	return NULL;
}

