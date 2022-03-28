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
#include <kernel/hw_info.h>
#include <kernel/core.h>
#include <stddef.h>

static proc_t _proc_table[PROC_MAX];
__attribute__((__aligned__(PAGE_DIR_SIZE))) 
static page_dir_entry_t _proc_vm[PROC_MAX][PAGE_DIR_NUM];
static queue_t _ready_queue[CPU_MAX_CORES];
static proc_t* _current_proc[CPU_MAX_CORES];
static uint32_t _use_core_id = 0;

bool _core_proc_ready = false;
int32_t _core_proc_pid = -1;
uint32_t _ipc_uid = 0;

/* proc_init initializes the process sub-system. */
void procs_init(void) {
	_use_core_id = 0;
	_ipc_uid = 0;
	_core_proc_ready = false;
	int32_t i;
	for (i = 0; i < PROC_MAX; i++) {
		_proc_table[i].info.state = UNUSED;
		_proc_table[i].info.wait_for = -1;
	}
	_core_proc_pid = -1;

	for (i = 0; i < CPU_MAX_CORES; i++) {
		_current_proc[i] = NULL;
		queue_init(&_ready_queue[i]);
	}
}

proc_t* proc_get(int32_t pid) {
	if(pid < 0 || pid >= PROC_MAX ||
			_proc_table[pid].info.state == UNUSED ||
			_proc_table[pid].info.state == ZOMBIE)
		return NULL;
	return &_proc_table[pid];
}

inline proc_t* get_current_proc(void) {
	uint32_t core_id = get_core_id();
	return _current_proc[core_id];
}

inline void set_current_proc(proc_t* proc) {
	uint32_t core_id = get_core_id();
	if(proc == NULL || proc->info.core == core_id)
		_current_proc[core_id] = proc;
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
	proc_t* cproc = get_current_proc();
	if(to == NULL)
		return;

	if(cproc != NULL && cproc->info.state != UNUSED)
		memcpy(&cproc->ctx, ctx, sizeof(context_t));

	if(to->space->ipc_server.ipc != 0) { //have ipc request to handle 
		memcpy(&to->space->ipc_server.ctx, &to->ctx, sizeof(context_t)); //save "to" context to ipc ctx, will restore after ipc done.
		to->ctx.gpr[0] = to->space->ipc_server.ipc;
		to->ctx.gpr[1] = to->space->ipc_server.extra_data;
		to->ctx.pc = to->ctx.lr = to->space->ipc_server.entry;
		to->space->ipc_server.ipc = 0; // clear ipc request mask
	}

	if(cproc != to && cproc != NULL &&
			cproc->info.state != UNUSED && 
			cproc->info.pid != _cpu_cores[cproc->info.core].halt_pid) {
			//halt proc can't be pushed into ready queue, can't be sheduled.
		if(cproc->info.state == RUNNING) {
			cproc->info.state = READY;
			if(quick)
				queue_push_head(&_ready_queue[cproc->info.core], cproc);
			else
				queue_push(&_ready_queue[cproc->info.core], cproc);
		}	
	}

	memcpy(ctx, &to->ctx, sizeof(context_t));
	to->info.state = RUNNING;
	page_dir_entry_t *vm = to->space->vm;
	set_translation_table_base((uint32_t) V2P(vm));
	set_current_proc(to);
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
		map_page_ref(proc->space->vm,
				proc->space->heap_size,
				V2P(page),
				AP_RW_RW);
		proc->space->heap_size += PAGE_SIZE;
	}
	flush_tlb();
	return res;
}

/* proc_shrink_memory shrinks the heap size of the given process. */
void proc_shrink_mem(proc_t* proc, int32_t page_num) {
	if(page_num <= 0)
		return;

	int32_t i;
	for (i = 0; i < page_num; i++) {
		uint32_t virtual_addr = proc->space->heap_size - PAGE_SIZE;
		unmap_page_ref(proc->space->vm, virtual_addr);
		proc->space->heap_size -= PAGE_SIZE;
		if (proc->space->heap_size == 0)
			break;
	}
	flush_tlb();
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
	if(queue_in(&_ready_queue[proc->info.core], proc) == NULL)
		queue_push_head(&_ready_queue[proc->info.core], proc);
}

proc_t* proc_get_core_ready(uint32_t core_id) {
	return (proc_t*)_ready_queue[core_id].head;
}

bool proc_have_ready_task(uint32_t core) {
	return !queue_is_empty(&_ready_queue[core]) ||
			_current_proc[core] != NULL;
}

proc_t* proc_get_next_ready(void) {
	uint32_t core_id = get_core_id();
	proc_t* next = queue_pop(&_ready_queue[core_id]);
	while(next != NULL && next->info.state != READY && next->info.state != RUNNING)
		next = queue_pop(&_ready_queue[core_id]);

	if(next == NULL) {
		proc_t*	cproc = get_current_proc();
		if(cproc != NULL && cproc->info.state == RUNNING &&
				cproc->info.pid != _cpu_cores[cproc->info.core].halt_pid)
				//halt proc can't be sheduled.
			next = cproc;
	}
	return next;
}

static void proc_unready(proc_t* proc, int32_t state) {
	queue_item_t* it = queue_in(&_ready_queue[proc->info.core], proc);	
	if(it != NULL)
		queue_remove(&_ready_queue[proc->info.core], it);
	proc->info.state = state;
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
		kev_push(KEV_PROC_EXIT, proc->info.pid, 0, 0);
	}

	proc_unready(proc, ZOMBIE);
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
	flush_tlb();
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
	flush_tlb();
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	uint32_t core = proc->info.core;
	proc_terminate(ctx, proc);
	proc->info.state = UNUSED;

	/*free kpage*/
	if(proc->space->kpage != 0) {
		unmap_page(proc->space->vm, proc->space->kpage);
		kfree4k((void*)proc->space->kpage);	
		flush_tlb();
		proc->space->kpage = 0;
	}

	/*free user_stack*/
	proc_free_user_stack(proc);

	/*free all ipc context*/
	if(proc->ipc_client != NULL)
		proc_ipc_close(proc->ipc_client);
	proto_clear(&proc->ipc_ctx.data);

	proc_free_space(proc);
	memset(proc, 0, sizeof(proc_t));
	if(_current_proc[core] == proc)
		_current_proc[core] = NULL;
}

void* proc_malloc(uint32_t size) {
	if(size == 0)
		return NULL;
	proc_t* cproc = get_current_proc();
	return trunk_malloc(&cproc->space->malloc_man, size);
}

void* proc_realloc(void* p, uint32_t size) {
	if(size == 0) {
		proc_free(p);
		return NULL;
	}
	proc_t* cproc = get_current_proc();
	return trunk_realloc(&cproc->space->malloc_man, p, size);
}

void proc_free(void* p) {
	if(p == NULL)
		return;
	proc_t* cproc = get_current_proc();
	trunk_free(&cproc->space->malloc_man, p);
}

static inline uint32_t core_fetch(proc_t* proc) {
	uint32_t cores = get_cpu_cores();
	if(cores == 1 || proc->info.owner < 0) {
		return 0;
	}

	_use_core_id++; 
	if(_use_core_id >= cores) 
		_use_core_id = 0;
	return _use_core_id;
}

static inline void core_attach(proc_t* proc) {
	//proc->info.core = 0;
	proc->info.core = core_fetch(proc);
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
	proc_free_heap(proc);

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
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	cproc->sleep_counter = count;
	proc_unready(cproc, SLEEPING);
	schedule(ctx);
}

void proc_block_on(int32_t pid_by, uint32_t event) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	cproc->block_event = event;
	cproc->info.block_by = pid_by;
	proc_unready(cproc, BLOCK);
}

void proc_waitpid(context_t* ctx, int32_t pid) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL || _proc_table[pid].info.state == UNUSED)
		return;

	cproc->info.wait_for = pid;
	proc_unready(cproc, WAIT);
	schedule(ctx);
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

			proc_ready(proc);
			/*
			if(proc->sleep_counter == 0)
				proc_ready(proc);
			else
				proc->info.state = SLEEPING;
				*/
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

	//Copy On Write
	uint32_t p;
	for(p=0; p<pages; ++p) { 
		uint32_t v_addr = (p * PAGE_SIZE);
		uint32_t phy_page_addr = resolve_phy_address(parent->space->vm, v_addr);
		map_page_ref(child->space->vm, 
				v_addr,
				phy_page_addr,
				AP_RW_R); //share page table to child with read only permissions, and ref the page

		map_page(parent->space->vm, 
				v_addr,
				phy_page_addr,
				AP_RW_R, 0); // set parent page table with read only permissions
	}
	flush_tlb();
	child->space->heap_size = pages * PAGE_SIZE;

	//set father
	child->info.father_pid = parent->info.pid;
	// copy parent's stack to child's stack
	int32_t i;
	for(i=0; i<STACK_PAGES; i++) {
		proc_page_clone(child, (uint32_t)child->user_stack[i], parent, (uint32_t)parent->user_stack[i]);
	}

	strcpy(child->info.cmd, parent->info.cmd);
	return 0;
}

/*static int32_t proc_clone(proc_t* child, proc_t* parent) {
	uint32_t pages = parent->space->heap_size / PAGE_SIZE;
	if((parent->space->heap_size % PAGE_SIZE) != 0)
		pages++;

	uint32_t p;
	for(p=0; p<pages; ++p) {
		uint32_t v_addr = (p * PAGE_SIZE);
		if(proc_expand_mem(child, 1) != 0) {
			printf("Panic: kfork expand memory failed!!(%d)\n", parent->info.pid);
			return -1;
		}
		// copy parent's memory to child's memory
		proc_page_clone(child, v_addr, parent, v_addr);
	}

	//set father
	child->info.father_pid = parent->info.pid;
	// copy parent's stack to child's stack
	int32_t i;
	for(i=0; i<STACK_PAGES; i++) {
		proc_page_clone(child, (uint32_t)child->user_stack[i], parent, (uint32_t)parent->user_stack[i]);
	}

	strcpy(child->info.cmd, parent->info.cmd);
	return 0;
}
*/

static inline void proc_thread_clone(context_t* ctx, proc_t* child, proc_t* parent) {
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
	memcpy(&child->ctx, &parent->ctx, sizeof(context_t));

	if(type == PROC_TYPE_PROC) {
		if(proc_clone(child, parent) != 0) {
			printf("panic: kfork clone failed!!(%d)\n", parent->info.pid);
			return NULL;
		}
	}
	else {
		proc_thread_clone(ctx, child, parent);
	}
	return child;
}

proc_t* kfork(context_t* ctx, int32_t type) {
	proc_t* cproc = get_current_proc();
	proc_t* child = kfork_raw(ctx, type, cproc);
	if(_core_proc_ready && child->info.type == PROC_TYPE_PROC) {
		kev_push(KEV_PROC_CREATED, cproc->info.pid, child->info.pid, 0);
	}
	else
		proc_ready(child);

	core_attach(child);
	return child;
}

static int32_t get_procs_num(void) {
	proc_t* cproc = get_current_proc();
	int32_t res = 0;
	int32_t i;
	for(i=0; i<PROC_MAX; i++) {
		if(_proc_table[i].info.state != UNUSED &&
				(cproc->info.owner == 0 ||
				 _proc_table[i].info.owner == cproc->info.owner)) {
			res++;
		}
	}
	return res;
}

procinfo_t* get_procs(int32_t *num) {
	proc_t* cproc = get_current_proc();
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
				(cproc->info.owner == 0 ||
				 _proc_table[i].info.owner == cproc->info.owner)) {
			memcpy(&procs[j], &_proc_table[i].info, sizeof(procinfo_t));
			j++;
		}
	}

	*num = j;
	return procs;
}

static void renew_sleep_counter(uint64_t usec) {
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

#define IPC_TIMEOUT 300000000u

static void check_ipc_timeout(uint64_t usec) {
	int i;
	for(i=0; i<PROC_MAX; i++) {
		proc_t* proc = &_proc_table[i];
		if(proc->ipc_client != NULL && proc->ipc_client->state != IPC_IDLE) {
			proc->ipc_client->usec += usec;
			if(proc->ipc_client->usec > IPC_TIMEOUT) {
#ifdef KDEBUG
				printf("ipc time out: c:%d s: %d\n", proc->ipc_client->client_pid, proc->ipc_client->server_pid);
#endif
				proc->ipc_client->client_pid = 0; //set proc ipc terminated.
				proc_wakeup(proc->info.pid, 0);
			}
		}
	}
}

void renew_kernel_tic(uint64_t usec) {
	renew_sleep_counter(usec);	
	check_ipc_timeout(usec);
}

proc_t* proc_get_proc(proc_t* proc) {
	while(proc != NULL) {
		if(proc->info.type == PROC_TYPE_PROC)
			return proc;
		proc = proc_get(proc->info.father_pid);
	}
	return NULL;
}

proc_t* kfork_core_halt(uint32_t core) {
	proc_t* cproc = &_proc_table[0];
	proc_t* child = kfork_raw(NULL, PROC_TYPE_PROC, cproc);
	child->info.core = core;
	return child;
}
