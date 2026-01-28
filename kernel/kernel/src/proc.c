#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/schedule.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/kmalloc_vm.h>
#include <mm/dma.h>
#include <mm/shm.h>
#include <kernel/kevqueue.h>
#include <kernel/semaphore.h>
#include <kstring.h>
#include <kprintf.h>
#include <queue.h>
#include <kernel/elf.h>
#include <kernel/hw_info.h>
#include <kernel/core.h>
#include <kernel/irq.h>
#include <stddef.h>
#include <signals.h>

static proc_t **_task_table;//[_kernel_config.max_task_num];

typedef struct {
	page_dir_entry_t pde[PAGE_DIR_NUM];
} proc_vm_t;

static proc_vm_t *_proc_vm = NULL;
static uint8_t  *_proc_vm_mark = NULL;

static queue_t _ready_queue[CPU_MAX_CORES];
static int32_t _current_proc[CPU_MAX_CORES];
static uint32_t _use_core_id = 0;
static uint32_t _proc_uuid = 0;
static int32_t _last_create_pid = 0;

bool _core_proc_ready = false;
int32_t _core_proc_pid = -1;
uint32_t _ipc_uid = 0;

/* proc_init initializes the process sub-system. */
int32_t procs_init(void) {
	_use_core_id = 0;
	_ipc_uid = 0;
	_proc_uuid = 0;
	_core_proc_ready = false;
	int32_t i;

	uint32_t size = PAGE_DIR_SIZE + (_kernel_config.max_proc_num*sizeof(proc_vm_t));
	uint32_t pde = (uint32_t)kmalloc(size);
	if(pde == 0) {
		printf("Panic: VMM process page dir entry table alloc failed (%d MB)!\n", size/1024/1024);
		return -1;
	}

	_proc_vm = (proc_vm_t*)ALIGN_UP(pde, PAGE_DIR_SIZE);

	size = _kernel_config.max_proc_num;
	_proc_vm_mark = (uint8_t*)kmalloc(size);
	for (i = 0; i < _kernel_config.max_proc_num; i++) {
		_proc_vm_mark[i] = 0;
	}

	size = _kernel_config.max_task_num*sizeof(proc_t);
	_task_table = (proc_t**)kmalloc(size);
	for (i = 0; i < _kernel_config.max_task_num; i++) {
		_task_table[i] = NULL;
	}

	_core_proc_pid = -1;

	for (i = 0; i < CPU_MAX_CORES; i++) {
		_current_proc[i] = -1;
		queue_init(&_ready_queue[i]);
	}

	return 0;
}

int32_t  proc_childof(proc_t* proc, proc_t* parent) {
	while(proc != NULL) {
		if(proc == parent)
			return 0;
		proc = proc_get(proc->info.father_pid);
	}
	return -1;
}

inline proc_t* proc_get(int32_t pid) {
	if(pid < 0 || pid >= _kernel_config.max_task_num)
		return NULL;

	proc_t* p = _task_table[pid];
	if(p->info.state == UNUSED || p->info.state == ZOMBIE)
		return NULL;
	return p;
}

inline proc_t* proc_get_by_uuid(uint32_t uuid) {
	proc_t* proc = NULL;
	for (uint32_t i = 0; i < _kernel_config.max_task_num; i++) {
		proc = _task_table[i];
		if(proc != NULL && proc->info.uuid == uuid) {
			break;
		}
	}

	if(proc == NULL || proc->info.state == UNUSED || proc->info.state == ZOMBIE)
		return NULL;
	return proc;
}

inline proc_t* get_current_proc(void) {
	uint32_t core_id = get_core_id();
	proc_t* cproc = proc_get(_current_proc[core_id]);
	if(cproc == NULL || cproc->info.state == UNUSED || cproc->info.state == ZOMBIE)
		return NULL;
	return cproc;
}

inline proc_t* get_current_core_proc(uint32_t core) {
	proc_t* cproc = proc_get(_current_proc[core]);
	if(cproc == NULL || cproc->info.state == UNUSED || cproc->info.state == ZOMBIE)
		return NULL;
	return cproc;
}

inline void set_current_proc(proc_t* proc) {
	uint32_t core_id = get_core_id();
	if(proc == NULL) {
		_current_proc[core_id] = -1;
		return;
	}

	if(proc->info.core == core_id)
		_current_proc[core_id] = proc->info.pid;
}

static inline uint32_t proc_get_user_stack_pages(proc_t* proc) {
	if(proc->info.type == TASK_TYPE_PROC)
		return STACK_PAGES;
	return THREAD_STACK_PAGES;
}

static inline  uint32_t proc_get_user_stack_base(proc_t* proc) {
	if(proc->info.type == TASK_TYPE_PROC)
		return USER_STACK_TOP - STACK_PAGES*PAGE_SIZE;
	return proc->thread_stack_base;
}

static void map_stack(proc_t* proc, uint32_t* stacks, uint32_t base, uint32_t pages) {
	uint32_t i;
	for(i=0; i<pages; i++) {
		stacks[i] = (uint32_t)kalloc4k();
		map_page(proc->space->vm,
			base + PAGE_SIZE*i,
			V2P(stacks[i]),
			AP_RW_RW, PTE_ATTR_WRBACK);
	}
	flush_tlb();
}

static void unmap_stack(proc_t* proc, uint32_t* stacks, uint32_t base, uint32_t pages) {
	uint32_t i;
	for(i=0; i<pages; i++) {
		unmap_page(proc->space->vm, base + PAGE_SIZE*i);
		kfree4k((void*)stacks[i]);
	}
	flush_tlb();
}

uint32_t thread_stack_alloc(proc_t* proc) {
	uint32_t i;
	if(proc->space->thread_stacks == NULL) {
		proc->space->thread_stacks = (thread_stack_t*)kmalloc(_kernel_config.max_task_per_proc*sizeof(thread_stack_t));
		memset(proc->space->thread_stacks, 0, _kernel_config.max_task_per_proc*sizeof(thread_stack_t));
	}

	for(i=0; i<_kernel_config.max_task_per_proc; i++) {
		if(proc->space->thread_stacks[i].base == 0)
			break;
	}

	if(i >= _kernel_config.max_task_per_proc) {
		printf("thread stack alloc failed(pid %d)!\n", proc->info.pid);
		return 0;
	}

	uint32_t base = USER_STACK_TOP - STACK_PAGES*PAGE_SIZE - THREAD_STACK_PAGES*PAGE_SIZE*(i+1);
	uint32_t pages = THREAD_STACK_PAGES;
	proc->space->thread_stacks[i].base = base;
	if(proc->space->thread_stacks[i].stacks == NULL) 
		proc->space->thread_stacks[i].stacks = kmalloc(THREAD_STACK_PAGES*sizeof(void*));
	memset(proc->space->thread_stacks[i].stacks, 0, THREAD_STACK_PAGES*sizeof(void*));
	map_stack(proc, (uint32_t*)proc->space->thread_stacks[i].stacks, base, pages);
	return base;
}

void thread_stack_free(proc_t* proc, uint32_t base) {
	uint32_t i;
	for(i=0; i<_kernel_config.max_task_per_proc; i++) {
		if(proc->space->thread_stacks[i].base == base)
			break;
	}
	if(i >= _kernel_config.max_task_per_proc) 
		return;
	unmap_stack(proc, (uint32_t*)proc->space->thread_stacks[i].stacks, base, THREAD_STACK_PAGES);
	proc->space->thread_stacks[i].base = 0;
	if(proc->space->thread_stacks[i].stacks != NULL)  {
		kfree(proc->space->thread_stacks[i].stacks);
		proc->space->thread_stacks[i].stacks = NULL;
	}
}

static inline void* proc_get_mem_tail(void* p) {
	proc_t* proc = (proc_t*)p;
	return (void*)proc->space->heap_size;
}

/* proc_shrink_memory shrinks the heap size of the given process. */
static void proc_shrink_mem(proc_t* proc, int32_t page_num) {
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

/* proc_exapnad_memory expands the heap size of the given process. */
static int32_t proc_expand_mem(proc_t *proc, int32_t page_num) {
	int32_t i;
	int32_t res = 0;

	for (i = 0; i < page_num; i++) {
		void *page = kalloc4k();
		if(page == NULL) {
			printf("proc expand failed!! free mem size: (%x), pid:%d(%s), pages ask:%d\n",
					get_free_mem_size(),
					proc->info.pid,
					proc->info.cmd,
					page_num);
			proc_shrink_mem(proc, i);
			res = -1;
			break;
		}
		memset(page, 0, PAGE_SIZE);
		map_page_ref(proc->space->vm,
				proc->space->heap_size,
				V2P(page),
				AP_RW_RW, PTE_ATTR_WRBACK);
		proc->space->heap_size += PAGE_SIZE;
	}
	flush_tlb();
	return res;
}

static int32_t get_free_pde(void) {
	for(int32_t i=0; i<_kernel_config.max_proc_num; i++) {
		if(_proc_vm_mark[i] == 0) {
			_proc_vm_mark[i] = 1;
			return i;
		}
	}
	return -1;
}

static int32_t proc_init_space(proc_t* proc) {
	int32_t pde_index = get_free_pde();
	if(pde_index < 0) {
		return -1;
	}

	page_dir_entry_t *vm = _proc_vm[pde_index].pde;
	set_vm(vm);
	proc->space = (proc_space_t*)kmalloc(sizeof(proc_space_t));
	memset(proc->space, 0, sizeof(proc_space_t));

	proc->space->pde_index = pde_index;
	proc->space->vm = vm;
	proc->space->heap_size = 0;
	proc->space->heap_used = 0;
	return 0;
}

inline void proc_save_state(proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res) {
	saved_state->state = proc->info.state;
	saved_state->block_by = proc->info.block_by;
	saved_state->block_event = proc->block_event;
	saved_state->sleep_counter = proc->sleep_counter;
	memcpy(saved_ipc_res, &proc->ipc_res, sizeof(ipc_res_t));
}

inline void proc_restore_state(context_t* ctx, proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res) {
	proc->info.state = saved_state->state;
	proc->info.block_by = saved_state->block_by;
	proc->block_event = saved_state->block_event;
	proc->sleep_counter = saved_state->sleep_counter;
	//memcpy(&proc->ipc_res, saved_ipc_res, sizeof(ipc_res_t));
	memcpy(ctx, &saved_state->ctx, sizeof(context_t));
	memset(saved_state, 0, sizeof(saved_state_t));
	memset(saved_ipc_res, 0, sizeof(ipc_res_t));
}

void proc_switch_multi_core(context_t* ctx, proc_t* to, uint32_t core) {
	if(to->info.core == core) {
		to->info.state = RUNNING;
		proc_switch(ctx, to, true);
	}
	else {
#ifdef KERNEL_SMP
		proc_ready(to);
		ipi_send(to->info.core);
#endif
	}
}

void proc_switch(context_t* ctx, proc_t* to, bool quick){
	proc_t* cproc = get_current_proc();
	if(to == NULL)
		return;

	if(cproc != NULL && cproc->info.state != UNUSED)
		memcpy(&cproc->ctx, ctx, sizeof(context_t));

	if(cproc != to) {
		page_dir_entry_t *vm = to->space->vm;
		set_translation_table_base((uint32_t)V2P(vm));
	}

	if(to->info.type == TASK_TYPE_PROC) {
		if (to->space->interrupt.state == INTR_STATE_START) {																				// have irq request to handle
			to->space->interrupt.state = INTR_STATE_WORKING; // clear irq request mask
			memcpy(&to->space->interrupt.saved_state.ctx, &to->ctx, sizeof(context_t)); // save "to" context to irq ctx, will restore after irq done.
			to->ctx.gpr[0] = to->space->interrupt.interrupt;
			to->ctx.gpr[1] = to->space->interrupt.data;
			to->ctx.pc = to->ctx.lr = to->space->interrupt.entry;
			if(to->space->interrupt.stack == 0)
				to->space->interrupt.stack = thread_stack_alloc(to);
			to->ctx.sp = ALIGN_DOWN(to->space->interrupt.stack + THREAD_STACK_PAGES * PAGE_SIZE, 8);
		}
		else if (to->space->signal.do_switch) {																			 // have signal request to handle
			memcpy(&to->space->signal.saved_state.ctx, &to->ctx, sizeof(context_t)); // save "to" context to ipc ctx, will restore after ipc done.
			to->ctx.gpr[0] = to->space->signal.sig_no;
			to->ctx.pc = to->ctx.lr = to->space->signal.entry;
			if(to->space->signal.stack == 0)
				to->space->signal.stack = thread_stack_alloc(to);
			to->ctx.sp = ALIGN_DOWN(to->space->signal.stack + THREAD_STACK_PAGES * PAGE_SIZE, 8);
			to->space->signal.do_switch = false; // clear ipc request mask
		}
		else if (to->space->ipc_server.do_switch) { // have ipc request to handle
			ipc_task_t *ipc = proc_ipc_get_task(to);
			memcpy(&to->space->ipc_server.saved_state.ctx, &to->ctx, sizeof(context_t)); // save "to" context to ipc ctx, will restore after ipc done.
			to->ctx.gpr[0] = ipc->uid;
			to->ctx.gpr[1] = to->space->ipc_server.extra_data;
			to->ctx.pc = to->ctx.lr = to->space->ipc_server.entry;
			if(to->space->ipc_server.stack == 0)
				to->space->ipc_server.stack = thread_stack_alloc(to);
			to->ctx.sp = ALIGN_DOWN(to->space->ipc_server.stack + THREAD_STACK_PAGES * PAGE_SIZE, 8);
			to->space->ipc_server.do_switch = false; // clear ipc request mask
		}
	}

	if(cproc != to && cproc != NULL &&
			cproc->info.state != UNUSED &&
			cproc != _cpu_cores[cproc->info.core].halt_proc) {
			//halt proc can't be pushed into ready queue, can't be scheduled.
		if(cproc->info.state == RUNNING) {
			cproc->info.state = READY;
			if(quick)
				queue_push_head(&_ready_queue[cproc->info.core], cproc);
			else
				queue_push(&_ready_queue[cproc->info.core], cproc);
		}	
	}

	to->info.state = RUNNING;
	to->info.block_by = -1;
	if(cproc != to)
		set_current_proc(to);
	memcpy(ctx, &to->ctx, sizeof(context_t));
}

static inline void proc_unmap_shms(proc_t *proc) {
	int32_t i;
	for(i=0; i<SHM_MAX; i++) {
		int32_t shm = proc->space->shms[i];
		if(shm > 0) {
			shm_proc_unmap_by_id(proc, shm, true);
		}
	}
}

inline void proc_ready(proc_t* proc) {
	if(proc == NULL)
		return;

	proc->info.state = READY;
	proc->block_event = 0;
	proc->info.block_by = -1;

	if(proc->priority_count > 0)
		return;
	proc->priority_count = proc->info.priority;

	if(queue_in(&_ready_queue[proc->info.core], proc) == NULL)
		//queue_push_head(&_ready_queue[proc->info.core], proc);
		queue_push(&_ready_queue[proc->info.core], proc);
}

inline proc_t* proc_get_core_ready(uint32_t core_id) {
	return (proc_t*)_ready_queue[core_id].head;
}

inline bool proc_have_ready_task(uint32_t core) {
	return !queue_is_empty(&_ready_queue[core]) ||
			_current_proc[core] >= 0;
}

proc_t* proc_get_next_ready(void) {
	uint32_t core_id = get_core_id();
	proc_t* next = queue_pop(&_ready_queue[core_id]);
	while(next != NULL && next->info.state != READY && next->info.state != RUNNING)
		next = queue_pop(&_ready_queue[core_id]);

	if(next == NULL) {
		proc_t*	cproc = get_current_proc();
		if(cproc != NULL && cproc->info.state == RUNNING &&
				cproc != _cpu_cores[cproc->info.core].halt_proc)
				//halt proc can't be sheduled.
			next = cproc;
	}
	return next;
}

static inline void proc_unready(proc_t* proc, int32_t state) {
	queue_item_t* it = queue_in(&_ready_queue[proc->info.core], proc);	
	if(it != NULL)
		queue_remove(&_ready_queue[proc->info.core], it);
	proc->info.state = state;
}

static void proc_wakeup_waiting(int32_t pid) {
	int32_t i;
	for (i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t *proc = _task_table[i];
		if (proc != NULL && proc->info.state == WAIT && proc->info.wait_for == pid) {
			proc->info.wait_for = -1;
			proc_ready(proc);
		}
	}
}

static inline void proc_update_vsyscall(proc_t* proc) {
	if(_kernel_vsyscall_info == NULL || proc == NULL || proc->info.pid < 0)
		return;
	_kernel_vsyscall_info->proc_info[proc->info.pid].father_pid = proc->info.father_pid;
	_kernel_vsyscall_info->proc_info[proc->info.pid].uuid = proc->info.uuid;
	_kernel_vsyscall_info->proc_info[proc->info.pid].type = proc->info.type;
}

static void proc_terminate(context_t* ctx, proc_t* proc) {
	if(proc->info.state == ZOMBIE || proc->info.state == UNUSED)
		return;

	proc_unready(proc, ZOMBIE);

	if(proc->info.type == TASK_TYPE_PROC) {
		semaphore_clear(proc->info.pid);

		kev_push(KEV_PROC_EXIT, proc->info.pid, 0, 0);
		int32_t i;
		for (i = 0; i < _kernel_config.max_task_num; i++) {
			proc_t *p = _task_table[i];
			if(p != NULL) {
				/*terminate forked from this proc*/
				if(p->info.father_pid == proc->info.pid) { //terminate forked children, skip reloaded ones
					if(p->info.type == TASK_TYPE_PROC)
						proc_signal_send(ctx, p, SYS_SIG_STOP, false);
					else
						proc_exit(ctx, p, 0);
				}
			}
		}

		/*free all ipc context*/
		proc_ipc_clear(proc);
		if(proc->space->interrupt.state != INTR_STATE_IDLE) {
			if(proc->space->interrupt.interrupt != IRQ_SOFT) {
				irq_enable(proc->space->interrupt.interrupt);
			}
			proc->space->interrupt.state = INTR_STATE_IDLE;
			proc_wakeup(proc->info.pid, -1, (uint32_t)&proc->space->interrupt);
		}
		proc_wakeup_waiting(proc->info.pid);
		proc->info.father_pid = 0;
	}
	else if(proc->info.type == TASK_TYPE_THREAD) { //TODO
		proc->info.father_pid = 0;
	}

	proc->info.uuid = 0;
	proc_update_vsyscall(proc);
}

static inline void proc_init_user_stack(proc_t* proc) {
	if(proc->info.type == TASK_TYPE_THREAD) {
		proc->thread_stack_base = thread_stack_alloc(proc);
	}
	else {
		uint32_t i;
		uint32_t base =  proc_get_user_stack_base(proc);
		uint32_t pages = proc_get_user_stack_pages(proc);
		map_stack(proc, proc->space->user_stack, base, pages);
	}
}

static inline void proc_free_user_stack(proc_t* proc) {
	/*free user_stack*/
	if(proc->info.type == TASK_TYPE_THREAD) {
		if(proc->thread_stack_base != 0) {
			thread_stack_free(proc, proc->thread_stack_base);
		}
	}
	else {
		uint32_t base = proc_get_user_stack_base(proc);
		uint32_t pages = proc_get_user_stack_pages(proc);
		unmap_stack(proc, proc->space->user_stack, base, pages);
	}
}

void proc_funeral(proc_t* proc) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL || cproc == proc || proc->info.state == UNUSED)
		return;
	if(proc->info.type == TASK_TYPE_PROC) {
		if(proc->space->refs > 0) //keep it still got child thread running.
			return;
	}
	else {
		if(proc->space->refs > 0)
			proc->space->refs--;
	}

	set_translation_table_base((uint32_t)V2P(proc->space->vm));
	dma_release(proc->info.pid);
	proc_free_user_stack(proc);

	if(proc->info.type == TASK_TYPE_PROC) {
		//free small_stack
		if (proc->space->interrupt.stack != 0) {
			thread_stack_free(proc, proc->space->interrupt.stack);
		}
		if (proc->space->signal.stack != 0) {
			thread_stack_free(proc, proc->space->signal.stack);
		}
		if (proc->space->ipc_server.stack != 0) {
			thread_stack_free(proc, proc->space->ipc_server.stack);
		}

		/*free proc heap*/
		proc_shrink_mem(proc, proc->space->heap_size / PAGE_SIZE);

		/*unmap share mems*/
		proc_unmap_shms(proc);

		set_translation_table_base(V2P(cproc->space->vm));
		free_page_tables(proc->space->vm);

		_proc_vm_mark[proc->space->pde_index] = 0;
		if(proc->space->thread_stacks != NULL)
			kfree(proc->space->thread_stacks);
		kfree(proc->space);
	}
	else {
		set_translation_table_base(V2P(cproc->space->vm));
	}

	_task_table[proc->info.pid] = NULL;
	kfree(proc);
}

inline void proc_zombie_funeral(void) {
	int32_t i;
	for (i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t *p = _task_table[i];
		if(p != NULL && p->info.state == ZOMBIE)
			proc_funeral(p);
	}
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	if(proc->info.state != UNUSED && proc->info.state != ZOMBIE)
		proc_terminate(ctx, proc);
	schedule(ctx);
}

inline void* proc_malloc(proc_t* proc, int32_t size) {

	proc->space->heap_used += size;
	size = proc->space->heap_used - proc->space->heap_size; 

	if(size == 0)
		return (void*)proc->space->malloc_base;

	uint8_t shrink = 0;
	uint32_t pages;
	if(size < 0) {
		shrink = 1;
		size = -size;
		size = ALIGN_DOWN(size, PAGE_SIZE);
	}
	else {
		size = ALIGN_UP(size, PAGE_SIZE);
	}

	pages = (size / PAGE_SIZE);
	if(pages == 0)
		return (void*)proc->space->malloc_base;

	if(shrink != 0) {
		//printf("kproc shrink pages: %d, size: %d\n", pages, size);
		proc_shrink_mem(proc, pages);
	}
	else {
		//printf("kproc expand pages: %d, size: %d\n", pages, size);
		if(proc_expand_mem(proc, pages) != 0)
			return NULL;
	}
	return (void*)proc->space->malloc_base;
}

inline uint32_t proc_msize(proc_t* proc) {
	return proc->space->heap_size - proc->space->malloc_base;
}

inline void proc_free(proc_t* proc) {
	uint32_t size = proc_msize(proc);
	uint32_t pages = size / PAGE_SIZE;
	proc_shrink_mem(proc, pages);
}

static inline uint32_t core_fetch(proc_t* proc) {
	(void)proc;
	if(_sys_info.cores == 1 || proc->info.uid < 0)
		return 0;

	//fetch the next core.
	/*uint32_t ret = _use_core_id++; 
	if(_use_core_id >= _sys_info.cores) 
		_use_core_id = 1;
	return ret;
	*/

	//fetch the most idle core.
	uint32_t ret = 0;
	for(uint32_t i = 0; i < _sys_info.cores; i++) {
		if(_cpu_cores[i].halt_proc->info.state == CREATED)
			return i;
		if(_cpu_cores[i].halt_proc->info.run_usec >
				_cpu_cores[ret].halt_proc->info.run_usec)
			ret = i;
	}
	return ret;
}

static inline void core_attach(proc_t* proc) {
	//proc->info.core = 0;
	proc->info.core = core_fetch(proc);
}


/* proc_creates allocates a new process and returns it. */
proc_t *proc_create(int32_t type, proc_t* parent) {
	int32_t index = -1;
	uint32_t i;

	for (i = 0; i < _kernel_config.max_task_num; i++) {
		int32_t at = i + _last_create_pid;
		if(at >= _kernel_config.max_task_num)
			at = at % _kernel_config.max_task_num;
		if (_task_table[at] == NULL) {
			_task_table[at] = (proc_t*)kmalloc(sizeof(proc_t));
			index = at;
			break;
		}
	}
	if (index < 0) {
		return NULL;
	}
	_last_create_pid = index+1;

	proc_t *proc = _task_table[index];
	memset(proc, 0, sizeof(proc_t));
	proc->info.wait_for = -1;
	proc->info.uuid = ++_proc_uuid;
	proc->info.pid = index;
	proc->info.type = type;
	proc->info.father_pid = -1;
	proc->info.state = CREATED;
	proto_init(&proc->ipc_res.data);

	if(type == TASK_TYPE_PROC) {
		proc_init_space(proc);
	}
	else {
		proc->space = parent->space;
		proc->space->refs++;
		if(proc->space->thread_stacks == NULL) {
			proc->space->thread_stacks = (thread_stack_t*)kmalloc(_kernel_config.max_task_per_proc*sizeof(thread_stack_t));
			memset(proc->space->thread_stacks, 0, _kernel_config.max_task_per_proc*sizeof(thread_stack_t));
		}
	}

	if(parent != NULL) {
		proc->info.father_pid = parent->info.pid;
		proc->info.uid = parent->info.uid;
		proc->info.gid = parent->info.gid;
		strcpy(proc->info.cmd, parent->info.cmd);
	}

	proc_init_user_stack(proc);
	proc->info.start_sec = _kernel_sec;
	CONTEXT_INIT(proc->ctx);

	proc_update_vsyscall(proc);
	return proc;
}

static inline void proc_free_heap(proc_t* proc) {
	proc_shrink_mem(proc, proc->space->heap_size/PAGE_SIZE);
}

/* proc_load loads the given ELF process image into the given process. */
int32_t proc_load_elf(proc_t *proc, const char *image, uint32_t size) {
	uint32_t prog_header_offset = 0;
	uint32_t prog_header_count = 0;
	uint32_t i = 0;

	proc->info.uuid = ++_proc_uuid; //load elf means a totally new proc
	uint8_t* proc_image = (uint8_t*)kmalloc_vm(proc->space->vm, size);
	memcpy(proc_image, image, size);
	proc_free_heap(proc);

	/*read elf format from saved proc image*/
	if (ELF_TYPE(proc_image) != ELFTYPE_EXECUTABLE)
		return -1;

	prog_header_count = ELF_PHNUM(proc_image);
	uint32_t *debug = 0;
	for (i = 0; i < prog_header_count; i++) {
		uint32_t j = 0;
		/* make enough room for this section */
		uint32_t vaddr = ELF_PVADDR(proc_image, i);
		uint32_t memsz = ELF_PSIZE(proc_image, i);
		uint32_t offset = ELF_POFFSET(proc_image, i);
		uint32_t flags = ELF_PFLAGS(proc_image, i);

		uint8_t rdonly = 0;	
		if((flags & 0x2) == 0) {
			rdonly = 1;
		}
		else {
			if(proc->space->rw_heap_base == 0)
				proc->space->rw_heap_base = vaddr;
			else if(vaddr < proc->space->rw_heap_base)
				proc->space->rw_heap_base = vaddr;
		}

		uint32_t old_heap_size = ALIGN_UP(proc->space->heap_size, PAGE_SIZE);
		while (proc->space->heap_size < (vaddr + memsz)) {
			proc->space->heap_used += PAGE_SIZE;
			if(proc_expand_mem(proc, 1) != 0){ 
				kfree(proc_image);
				return -1;
			}
		}

		/* copy the section from kernel to proc mem space*/
		uint32_t hvaddr = vaddr;
		uint32_t hoff = offset;
		for (j = 0; j < memsz; j++) {
			vaddr = hvaddr + j; /*vaddr in elf (proc vaddr)*/
			uint32_t vkaddr = resolve_kernel_address(proc->space->vm, vaddr); /*trans to phyaddr by proc's page dir*/
			/*copy from elf to vaddrKernel(=phyaddr=vaddrProc=vaddrElf)*/
			uint32_t image_off = hoff + j;
			*(uint8_t*)vkaddr = proc_image[image_off];
			if(image_off >= size)
				break;
		}
		prog_header_offset += sizeof(struct elf_program_header);

		if(rdonly) {
			while (old_heap_size < proc->space->heap_size) {
				ewokos_addr_t phy_page = (ewokos_addr_t)resolve_phy_address(proc->space->vm, old_heap_size);
				map_page(proc->space->vm,
					old_heap_size,
					phy_page,
					AP_RW_R, PTE_ATTR_WRBACK);
				old_heap_size += PAGE_SIZE;
			}
		}
	}

	if(proc->space->rw_heap_base < 0x400)
			proc->space->rw_heap_base = 0x400; //1024
	proc->space->malloc_base = proc->space->heap_size;
	ewokos_addr_t user_stack_base =  proc_get_user_stack_base(proc);
	uint32_t pages = proc_get_user_stack_pages(proc);
	proc->ctx.sp = user_stack_base + pages*PAGE_SIZE;
	proc->ctx.pc = ELF_ENTRY(proc_image);
	proc->ctx.lr = ELF_ENTRY(proc_image);
	proc_ready(proc);
	kfree_vm(proc->space->vm, proc_image);

	proc_update_vsyscall(proc);
	return 0;
}

inline void proc_usleep(context_t* ctx, uint32_t count) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	cproc->sleep_counter = count;
	proc_unready(cproc, SLEEPING);
	schedule(ctx);
}
	
inline void proc_block_on(context_t* ctx, int32_t pid_by, uint32_t event) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	cproc->block_event = event;
	cproc->info.block_by = pid_by;
	proc_unready(cproc, BLOCK);
	//proc_block_saved_state(pid_by, event, cproc);
	schedule(ctx);
}

inline void proc_waitpid(context_t* ctx, int32_t pid) {
	proc_t* cproc = get_current_proc();
	proc_t* p = _task_table[pid];
	if(cproc == NULL || p == NULL)
		return;

	cproc->info.wait_for = pid;
	proc_unready(cproc, WAIT);
	schedule(ctx);
}

static void proc_wakeup_all_state(int32_t pid_by, uint32_t event, proc_t* proc) {
	if((event == 0 || proc->block_event == event) && 
			(pid_by < 0 || proc->info.block_by == pid_by )) {
		proc_ready(proc);
	}

	if((pid_by < 0 || proc->space->ipc_server.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->ipc_server.saved_state.block_event == event)) {
		proc->space->ipc_server.saved_state.state = READY;
		proc->space->ipc_server.saved_state.block_by = -1;
		proc->space->ipc_server.saved_state.block_event = 0;
	}	

	if((pid_by < 0 || proc->space->signal.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->signal.saved_state.block_event == event)) {
		proc->space->signal.saved_state.state = READY;
		proc->space->signal.saved_state.block_by = -1;
		proc->space->signal.saved_state.block_event = 0;
	}

	if((pid_by < 0 || proc->space->interrupt.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->interrupt.saved_state.block_event == event)) {
		proc->space->interrupt.saved_state.state = READY;
		proc->space->interrupt.saved_state.block_by = -1;
		proc->space->interrupt.saved_state.block_event = 0;
	}

/*
	if(proc->info.state == BLOCK && (event == 0 || proc->block_event == event) && 
			(pid_by < 0 || proc->info.block_by == pid_by )) {
		proc_ready(proc);
	}

	if(proc->space->ipc_server.saved_state.state == BLOCK && 
			(pid_by < 0 || proc->space->ipc_server.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->ipc_server.saved_state.block_event == event)) {
		proc->space->ipc_server.saved_state.state = READY;
		proc->space->ipc_server.saved_state.block_by = -1;
		proc->space->ipc_server.saved_state.block_event = 0;
	}	

	if(proc->space->signal.saved_state.state == BLOCK && 
			(pid_by < 0 || proc->space->signal.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->signal.saved_state.block_event == event)) {
		proc->space->signal.saved_state.state = READY;
		proc->space->signal.saved_state.block_by = -1;
		proc->space->signal.saved_state.block_event = 0;
	}

	if(proc->space->interrupt.saved_state.state == BLOCK && 
			(pid_by < 0 || proc->space->interrupt.saved_state.block_by == pid_by) &&
			(event == 0 || proc->space->interrupt.saved_state.block_event == event)) {
		proc->space->interrupt.saved_state.state = READY;
		proc->space->interrupt.saved_state.block_by = -1;
		proc->space->interrupt.saved_state.block_event = 0;
	}
	*/
}

void proc_wakeup(int32_t pid_by, int32_t pid, uint32_t event) {
	if(pid >= 0) {
		if(pid >= _kernel_config.max_task_num)
			return;

		proc_t* proc = _task_table[pid];	
		if(proc == NULL || proc->info.state == UNUSED ||
				proc->info.state == ZOMBIE)
			return;

		if(proc == proc_get_proc(get_current_proc())) //self force wakeup
			proc_wakeup_all_state(-1, 0, proc);
		else
			proc_wakeup_all_state(pid_by, event, proc);
		return;
	} 

	int32_t i = 0;	
	while(1) {
		if(i >= _kernel_config.max_task_num)
			break;
		proc_t* proc = _task_table[i];	
		i++;
		if(proc == NULL || proc->info.state == UNUSED ||
				proc->info.state == ZOMBIE)
			continue;
		proc_wakeup_all_state(pid_by, event, proc);
	}
}

static inline void proc_page_clone(proc_t* to, uint32_t to_addr, proc_t* from, uint32_t from_addr) {
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
				AP_RW_R, PTE_ATTR_WRBACK); //share page table to child with read only permissions, and ref the page

		map_page(parent->space->vm, 
				v_addr,
				phy_page_addr,
				AP_RW_R, PTE_ATTR_WRBACK); // set parent page table with read only permissions
	}
	flush_tlb();
	child->space->heap_size = pages * PAGE_SIZE;
	child->space->heap_used = pages * PAGE_SIZE;

	// copy parent's stack to child's stack
	int32_t i;
	for(i=0; i<STACK_PAGES; i++) {
		proc_page_clone(child, 
			child->space->user_stack[i],
			parent,
			parent->space->user_stack[i]);
	}
	child->space->malloc_base = parent->space->malloc_base;
	child->space->rw_heap_base = parent->space->rw_heap_base;
	return 0;
}

proc_t* kfork_raw(context_t* ctx, int32_t type, proc_t* parent) {
	(void)ctx;
	proc_t *child = NULL;
	child = proc_create(type, parent);
	if(child == NULL) {
		printf("panic: kfork create proc failed!!(%d)\n", parent->info.pid);
		return NULL;
	}
	memcpy(&child->ctx, &parent->ctx, sizeof(context_t));

	if(type == TASK_TYPE_PROC) {
		if(proc_clone(child, parent) != 0) {
			printf("panic: kfork clone failed!!(%d)\n", parent->info.pid);
			return NULL;
		}
		/*printf("clone: \n\tfather: 0x%x->0x%x\n\tchild:  0x%x->0x%x\n",
				parent->space->malloc_base, parent->space->heap_size,
				child->space->malloc_base, child->space->heap_size);
				*/
	}
	else {
		child->ctx.sp = ALIGN_DOWN(child->thread_stack_base + THREAD_STACK_PAGES*PAGE_SIZE, 8);
	}
	return child;
}

proc_t* kfork(context_t* ctx, int32_t type) {
	proc_t* cproc = get_current_proc();
	proc_t* child = kfork_raw(ctx, type, cproc);
	core_attach(child);

	if(_core_proc_ready && child->info.type == TASK_TYPE_PROC)
		kev_push(KEV_PROC_CREATED, cproc->info.pid, child->info.pid, 0);
	else
		proc_ready(child);
	return child;
}

int32_t get_procs_num(void) {
	proc_t* cproc = get_current_proc();
	int32_t res = 0;
	int32_t i;
	for(i=0; i<_kernel_config.max_task_num; i++) {
		if(_task_table[i] != NULL && _task_table[i]->info.state != UNUSED)  {
			if(!_task_table[i]->is_core_idle_proc)
				res++;
		}
	}
	return res;
}

int32_t get_procs(int32_t num, procinfo_t* procs) {
	if(procs == NULL)
		return -1;

	int32_t j = 0;
	int32_t i;
	for(i=0; i<_kernel_config.max_task_num && j<(num); i++) {
		proc_t* p = _task_table[i];
		if(p != NULL && p->info.state != UNUSED && !p->is_core_idle_proc) {
			memcpy(&procs[j], &p->info, sizeof(procinfo_t));
			procs[j].heap_size = p->space->heap_size;
			j++;
		}
	}
	return 0;
}

int32_t get_proc(int32_t pid, procinfo_t *info) {
	proc_t* proc = proc_get(pid);
	if(proc == NULL)
		return -1;

	memcpy(info, &proc->info, sizeof(procinfo_t));
	info->heap_size = proc->space->heap_size;
	return 0;
}

static int32_t renew_sleep_counter(uint32_t usec) {
	int i;
	int32_t res = -1;
	for(i=0; i<_kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		if(proc == NULL)
			continue;

		if(proc->info.state == SLEEPING) {
			proc->sleep_counter -= usec;
			if(proc->sleep_counter <= 0) {
				proc->sleep_counter = 0;
				proc_ready(proc);
				res = 0;
			}
		}
	}
	return res;
}

static int32_t renew_interrupt_counter(uint32_t usec) {
	int i;
	int32_t res = -1;
	for(i=0; i<_kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		if(proc == NULL ||
				proc->info.state == UNUSED || proc->info.state == ZOMBIE ||
				proc->space->interrupt.state == INTR_STATE_IDLE)
			continue;

		proc->space->interrupt.counter += usec;
		if(proc->space->interrupt.counter >= INTERRUPT_TIMEOUT_USEC) {
			printf("interrupt timeout: %d\n", proc->space->interrupt.interrupt);
			proc->space->interrupt.counter = 0;
			proc->space->interrupt.state = INTR_STATE_IDLE;
			if(proc->space->interrupt.interrupt != IRQ_SOFT)
				irq_enable(proc->space->interrupt.interrupt);
		}
	}
	return res;
}

static int32_t renew_priority_counter(uint32_t usec) {
	for(int i=0; i<_kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		if(proc == NULL || proc->info.state == UNUSED || proc->info.state == ZOMBIE)
			continue;

		if(proc->priority_count > 0)
			proc->priority_count--;
		else if(proc->info.state == RUNNING)
			proc->run_usec_counter += usec;
	
		if(proc->info.state == RUNNING || proc->info.state == READY) {
			proc_ready(proc);
		}
	}
}

static void renew_vsyscall_info(void) {
	if(_kernel_vsyscall_info == NULL)
		return;
	_kernel_vsyscall_info->kernel_usec = _kernel_usec;
}

inline int32_t renew_kernel_tic(uint32_t usec) {
	renew_vsyscall_info();
	renew_priority_counter(usec);
	return renew_sleep_counter(usec);	
}

static uint32_t _k_sec_counter = 0;
inline void renew_kernel_sec(void) {
	int i;
	_k_sec_counter++;
	for(i=0; i<_kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		if(proc == NULL)
			continue;
		if(proc->info.state != UNUSED && 
				proc->info.state != ZOMBIE) {

			proc->info.run_usec = proc->run_usec_counter/_k_sec_counter;
			if(_k_sec_counter >= KERNEL_PROC_RUN_RECOUNT_SEC)
				proc->run_usec_counter = 0;
		}
	}

	if(_k_sec_counter >= KERNEL_PROC_RUN_RECOUNT_SEC)
		_k_sec_counter = 0;
}

proc_t* proc_get_proc(proc_t* proc) {
	while(proc != NULL) {
		if(proc->info.type == TASK_TYPE_PROC)
			return proc;
		proc = proc_get(proc->info.father_pid);
	}
	return NULL;
}

int32_t get_proc_pid(int32_t pid) {
	proc_t* p = proc_get_proc(proc_get(pid));
	if(p == NULL)
		return pid;
	return p->info.pid;
}

proc_t* kfork_core_halt(uint32_t core) {
	proc_t* cproc = _task_table[0];
	proc_t* child = kfork_raw(NULL, TASK_TYPE_PROC, cproc);
	child->info.core = core;
	_cpu_cores[core].halt_proc = child;
	child->is_core_idle_proc = true;

	return child;
}