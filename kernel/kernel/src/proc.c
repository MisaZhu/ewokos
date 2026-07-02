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
#include <dev/timer.h>
#include <stddef.h>
#include <signals.h>

static proc_t **_task_table;//[_kernel_config.max_task_num];

typedef struct {
	page_dir_entry_t pde[PAGE_DIR_NUM];
} proc_vm_t;

static proc_vm_t *_proc_vm = NULL;
static uint8_t  *_proc_vm_mark = NULL;

static queue_t _ready_queue[CPU_MAX_CORES];
static queue_t _interrupt_timeout_queue;
static queue_t _ipc_timeout_queue;
static queue_t _priority_update_queue;
static int32_t _current_proc[CPU_MAX_CORES];
static uint32_t _use_core_id = 0;
static uint32_t _proc_uuid = 0;
static int32_t _last_create_pid = 0;
static uint64_t _run_window_start_usec = 0;

static void proc_wakeup_all_state(proc_t* proc);

bool _core_proc_ready = false;
int32_t _core_proc_pid = -1;
uint32_t _ipc_uid = 0;

#ifdef KERNEL_SMP
static int32_t _proc_spin = 0;
static int32_t _proc_lock_owner = -1;
static uint32_t _proc_lock_depth = 0;
#endif

#ifdef __x86_64__
static uint32_t _x86_ap_switch_trace_count = 0;
static uint32_t _x86_core_attach_trace_count = 0;
#endif

static inline uint64_t proc_account_now_usec(void) {
	return irq_accounting_now_usec();
}

void proc_lock_enter(void) {
#ifdef KERNEL_SMP
	int32_t core = (int32_t)get_core_id();
	if(_proc_lock_owner == core) {
		_proc_lock_depth++;
		return;
	}
	mcore_lock(&_proc_spin);
	_proc_lock_owner = core;
	_proc_lock_depth = 1;
#endif
}

void proc_lock_leave(void) {
#ifdef KERNEL_SMP
	if(_proc_lock_owner != (int32_t)get_core_id() || _proc_lock_depth == 0)
		return;
	_proc_lock_depth--;
	if(_proc_lock_depth == 0) {
		_proc_lock_owner = -1;
		mcore_unlock(&_proc_spin);
	}
#endif
}

static inline void proc_account_running_until(proc_t* proc, uint64_t now_usec) {
	if(proc == NULL || !proc->run_accounting_active)
		return;
	if(get_current_core_proc(proc->info.core) != proc)
		return;
	if(now_usec > proc->run_last_start_usec)
		proc->run_usec_counter += now_usec - proc->run_last_start_usec;
	proc->run_last_start_usec = now_usec;
}

static inline void proc_account_switch_out(proc_t* proc, uint64_t now_usec) {
	proc_account_running_until(proc, now_usec);
	if(proc != NULL)
		proc->run_accounting_active = false;
}

static inline void proc_account_switch_in(proc_t* proc, uint64_t now_usec) {
	if(proc == NULL)
		return;
	proc->run_last_start_usec = now_usec;
	proc->run_accounting_active = true;
}

static inline uint32_t proc_run_usec_snapshot_at(proc_t* proc, uint64_t now_usec) {
	uint64_t average;
	uint64_t window_usec;

	if (proc == NULL)
		return 0;

	if(now_usec <= _run_window_start_usec)
		return 0;

	window_usec = now_usec - _run_window_start_usec;
	average = (proc->run_usec_counter * 1000000ULL) / window_usec;
	if (average > 0xffffffffULL)
		average = 0xffffffffULL;
	return (uint32_t)average;
}

static void proc_refresh_runtime_stats_internal(bool refresh_idle, bool refresh_non_idle, uint64_t now_usec) {
	for (uint32_t i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		if (proc == NULL || proc->info.state == UNUSED || proc->info.state == ZOMBIE)
			continue;
		if(proc->is_core_idle_proc && !refresh_idle)
			continue;
		if(!proc->is_core_idle_proc && !refresh_non_idle)
			continue;
		proc_account_running_until(proc, now_usec);
		proc->info.run_usec = proc_run_usec_snapshot_at(proc, now_usec);
	}
}

void proc_refresh_runtime_stats(void) {
	proc_lock_enter();
	proc_refresh_runtime_stats_internal(false, true, proc_account_now_usec());
	proc_lock_leave();
}

void proc_refresh_idle_runtime_stats(void) {
	proc_lock_enter();
	proc_refresh_runtime_stats_internal(true, false, proc_account_now_usec());
	proc_lock_leave();
}

void proc_get_core_runtime_stats(uint32_t* core_procs, uint32_t* core_idles, uint32_t* core_kernels, uint32_t cores) {
	uint64_t now_usec = proc_account_now_usec();

	if(core_procs != NULL)
		memset(core_procs, 0, sizeof(uint32_t)*cores);
	if(core_idles != NULL)
		memset(core_idles, 0, sizeof(uint32_t)*cores);
	if(core_kernels != NULL)
		memset(core_kernels, 0, sizeof(uint32_t)*cores);

	proc_lock_enter();
	proc_refresh_runtime_stats_internal(true, true, now_usec);

	for (uint32_t i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t* proc = _task_table[i];
		uint32_t core;
		uint64_t load;

		if (proc == NULL || proc->info.state == UNUSED || proc->info.state == ZOMBIE)
			continue;
		core = proc->info.core;
		if(core >= cores)
			continue;

		if(proc->is_core_idle_proc) {
			if(core_idles != NULL)
				core_idles[core] = proc->info.run_usec;
			continue;
		}

		if(core_procs == NULL)
			continue;
		load = (uint64_t)core_procs[core] + proc->info.run_usec;
		if(load > 1000000ULL)
			load = 1000000ULL;
		core_procs[core] = (uint32_t)load;
	}

	if(core_kernels != NULL) {
		for(uint32_t core = 0; core < cores; core++) {
			uint32_t idle = core_idles != NULL ? core_idles[core] : 0;
			uint32_t proc = core_procs != NULL ? core_procs[core] : 0;
			uint32_t residual = 0;

			if(idle > 1000000U)
				idle = 1000000U;
			if(proc > 1000000U)
				proc = 1000000U;

			if(idle + proc < 1000000U)
				residual = 1000000U - idle - proc;
			core_kernels[core] = residual;
		}
	}

	proc_lock_leave();
}

/* proc_init initializes the process sub-system. */
int32_t procs_init(void) {
	_use_core_id = 0;
	_ipc_uid = 0;
	_proc_uuid = 0;
	_core_proc_ready = false;
	_run_window_start_usec = proc_account_now_usec();
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
#ifdef KERNEL_SMP
	_proc_spin = 0;
	_proc_lock_owner = -1;
	_proc_lock_depth = 0;
#endif

	for (i = 0; i < CPU_MAX_CORES; i++) {
		_current_proc[i] = -1;
		queue_init(&_ready_queue[i]);
	}
	queue_init(&_interrupt_timeout_queue);
	queue_init(&_ipc_timeout_queue);
	queue_init(&_priority_update_queue);

	return 0;
}

static inline void proc_track_timeout_queue(queue_t* q, proc_t* proc) {
	if(q == NULL || proc == NULL)
		return;
	if(queue_in(q, proc) == NULL)
		queue_push(q, proc);
}

static inline void proc_untrack_timeout_queue(queue_t* q, proc_t* proc) {
	if(q == NULL || proc == NULL)
		return;
	queue_item_t* it = queue_in(q, proc);
	if(it != NULL)
		queue_remove(q, it);
}

static inline void proc_track_priority_update(proc_t* proc) {
	if(proc == NULL || proc->is_core_idle_proc)
		return;
	if(proc->info.state != READY && proc->info.state != RUNNING)
		return;
	proc_track_timeout_queue(&_priority_update_queue, proc);
}

static inline void proc_untrack_priority_update(proc_t* proc) {
	proc_untrack_timeout_queue(&_priority_update_queue, proc);
}

void proc_track_interrupt_timeout(proc_t* proc) {
	proc_lock_enter();
	if(proc == NULL || proc->space == NULL ||
			proc->space->interrupt.state == INTR_STATE_IDLE) {
		proc_lock_leave();
		return;
	}
	proc_track_timeout_queue(&_interrupt_timeout_queue, proc);
	proc_lock_leave();
}

void proc_untrack_interrupt_timeout(proc_t* proc) {
	proc_lock_enter();
	proc_untrack_timeout_queue(&_interrupt_timeout_queue, proc);
	proc_lock_leave();
}

void proc_track_ipc_timeout(proc_t* proc) {
	proc_lock_enter();
	if(proc == NULL || proc->space == NULL ||
			proc->space->ipc_server.ctask.state == IPC_IDLE) {
		proc_lock_leave();
		return;
	}
	proc_track_timeout_queue(&_ipc_timeout_queue, proc);
	proc_lock_leave();
}

void proc_untrack_ipc_timeout(proc_t* proc) {
	proc_lock_enter();
	proc_untrack_timeout_queue(&_ipc_timeout_queue, proc);
	proc_lock_leave();
}

int32_t  proc_childof(proc_t* proc, proc_t* parent) {
	while(proc != NULL) {
		if(proc == parent)
			return 0;
		proc = proc_get(proc->info.father_pid);
	}
	return -1;
}

proc_t* proc_get(int32_t pid) {
	if(pid < 0 || pid >= _kernel_config.max_task_num)
		return NULL;

	proc_t* p = _task_table[pid];
	if(p->info.state == UNUSED || p->info.state == ZOMBIE)
		return NULL;
	return p;
}

proc_t* proc_get_by_uuid(uint32_t uuid) {
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

proc_t* get_current_proc(void) {
	uint32_t core_id = get_core_id();
	proc_t* cproc = proc_get(_current_proc[core_id]);
	if(cproc == NULL || cproc->info.state == UNUSED || cproc->info.state == ZOMBIE)
		return NULL;
	return cproc;
}

proc_t* get_current_core_proc(uint32_t core) {
	proc_t* cproc = proc_get(_current_proc[core]);
	if(cproc == NULL || cproc->info.state == UNUSED || cproc->info.state == ZOMBIE)
		return NULL;
	return cproc;
}

void set_current_proc(proc_t* proc) {
	uint32_t core_id = get_core_id();
	proc_t* cproc = proc_get(_current_proc[core_id]);
	uint64_t now_usec = proc_account_now_usec();

	if(cproc != NULL && cproc != proc)
		proc_account_switch_out(cproc, now_usec);

	if(proc == NULL) {
		_current_proc[core_id] = -1;
		return;
	}

	if(proc->info.core == core_id) {
		_current_proc[core_id] = proc->info.pid;
		if(cproc != proc || !proc->run_accounting_active)
			proc_account_switch_in(proc, now_usec);
	}
}

void proc_account_pause_current(void) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;
	proc_account_switch_out(cproc, proc_account_now_usec());
}

void proc_account_resume_current(void) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL || cproc->info.state != RUNNING || cproc->run_accounting_active)
		return;
	proc_account_switch_in(cproc, proc_account_now_usec());
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
		page_table_entry_t* pte;
		stacks[i] = (uint32_t)kalloc4k();
		map_page(proc->space->vm,
			base + PAGE_SIZE*i,
			V2P(stacks[i]),
			AP_RW_RW, PTE_ATTR_WRBACK);
#ifdef __aarch64__
		pte = get_page_table_entry(proc->space->vm, base + PAGE_SIZE*i);
		if(pte != NULL) {
			pte->UXN = 1;
		}
#endif
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

void proc_save_state(proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res) {
	saved_state->state = proc->info.state;
	saved_state->sleep_counter = proc->sleep_counter;
	memcpy(saved_ipc_res, &proc->ipc_res, sizeof(ipc_res_t));
}

void proc_restore_state(context_t* ctx, proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res) {
	proc->info.state = saved_state->state;
	proc->sleep_counter = saved_state->sleep_counter;
	//memcpy(&proc->ipc_res, saved_ipc_res, sizeof(ipc_res_t));
	memcpy(ctx, &saved_state->ctx, sizeof(context_t));
	memset(saved_state, 0, sizeof(saved_state_t));
	memset(saved_ipc_res, 0, sizeof(ipc_res_t));
}

static void proc_interrupt_timeout(proc_t* proc) {
	if(proc == NULL || proc->space == NULL) {
		return;
	}

	proc_interrupt_t* intr = &proc->space->interrupt;
	uint32_t interrupt = intr->interrupt;
	uint32_t intr_state = intr->state;

	intr->counter = 0;
	intr->state = INTR_STATE_IDLE;
	proc_untrack_interrupt_timeout(proc);

	if(proc->info.state != UNUSED && proc->info.state != ZOMBIE) {
		proc->info.state = intr->saved_state.state;
		proc->sleep_counter = intr->saved_state.sleep_counter;
		memcpy(&proc->ctx, &intr->saved_state.ctx, sizeof(context_t));
		intr->restore_pending = (intr_state == INTR_STATE_WORKING) ? 1 : 0;
		if(proc->info.state == READY) {
			proc_ready(proc);
		}
	}
	else {
		intr->restore_pending = 0;
	}

	memset(&intr->saved_state, 0, sizeof(saved_state_t));
	memset(&intr->saved_ipc_res, 0, sizeof(ipc_res_t));
	intr->entry = 0;
	intr->data = 0;
	intr->interrupt = 0;

	if(interrupt != 0 && interrupt != IRQ_SOFT) {
		irq_enable_arch(interrupt);
	}

	proc_interrupt_wakeup(proc);

#ifdef KERNEL_SMP
	if(intr->restore_pending != 0 &&
			_cpu_cores[proc->info.core].actived &&
			proc->info.core != get_core_id()) {
		_cpu_cores[proc->info.core].need_resched = 1;
		ipi_send(proc->info.core);
	}
#endif
}

static void proc_ipc_timeout(proc_t* proc) {
	if(proc == NULL || proc->space == NULL) {
		return;
	}

	ipc_server_t* server = &proc->space->ipc_server;
	ipc_task_t* ipc = &server->ctask;
	proc_t* client_proc = NULL;
	uint32_t client_uid = 0;

	if(ipc->state == IPC_IDLE) {
		server->do_switch = false;
		server->restore_pending = 0;
		return;
	}

	if(ipc->client_pid > 0) {
		client_proc = proc_get(ipc->client_pid);
		client_uid = ipc->uid;
	}

	server->do_switch = false;
	proc->info.state = server->saved_state.state;
	proc->sleep_counter = server->saved_state.sleep_counter;
	memcpy(&proc->ctx, &server->saved_state.ctx, sizeof(context_t));
	server->restore_pending = 1;
	if(proc->info.state == READY) {
		proc_ready(proc);
	}

	memset(&server->saved_state, 0, sizeof(saved_state_t));
	memset(&server->saved_ipc_res, 0, sizeof(ipc_res_t));

	if(client_proc != NULL &&
			client_proc->info.state != UNUSED &&
			client_proc->info.state != ZOMBIE &&
			client_proc->ipc_res.uid == client_uid) {
		client_proc->ipc_res.uid = 0;
		client_proc->ipc_res.state = IPC_IDLE;
		proto_clear(&client_proc->ipc_res.data);
	}

	proc_ipc_close(proc, ipc);

	if(client_proc != NULL &&
			client_proc->info.state != UNUSED &&
			client_proc->info.state != ZOMBIE) {
		proc_wakeup(client_proc);
	}

	proc_ipc_wakeup(proc);

#ifdef KERNEL_SMP
	if(server->restore_pending != 0 &&
			_cpu_cores[proc->info.core].actived &&
			proc->info.core != get_core_id()) {
		_cpu_cores[proc->info.core].need_resched = 1;
		ipi_send(proc->info.core);
	}
#endif
}

void proc_switch_multi_core(context_t* ctx, proc_t* to, uint32_t core) {
	if(to->info.core == core) {
		proc_lock_enter();
		if(to->info.state != RUNNING) {
			proc_wakeup_all_state(to);
		}
		proc_lock_leave();
		to->info.state = RUNNING;
		proc_switch(ctx, to, true);
	}
	else {
#ifdef KERNEL_SMP
		/*
		 * IPC dispatch can target a server that is sleeping or blocked on
		 * another core.  Waking it with proc_ready() alone leaves the
		 * nested saved_state copies untouched, so a concurrent restore path
		 * can still drag the server back to SLEEPING/BLOCK and lose the
		 * wakeup.  Reuse the full proc_wakeup() path so both the visible
		 * state and the saved nested states become READY before the remote
		 * core is kicked.
		 */
		proc_wakeup(to);
#endif
	}
}

void proc_switch(context_t* ctx, proc_t* to, bool quick){
	proc_lock_enter();
	proc_t* cproc = get_current_proc();
	if(to == NULL) {
		proc_lock_leave();
		return;
	}

	if(cproc != NULL && cproc->info.state != UNUSED) {
		if(cproc->info.type == TASK_TYPE_PROC &&
				cproc->space != NULL &&
				(cproc->space->interrupt.restore_pending != 0 ||
				 cproc->space->ipc_server.restore_pending != 0)) {
			memcpy(ctx, &cproc->ctx, sizeof(context_t));
			cproc->space->interrupt.restore_pending = 0;
			cproc->space->ipc_server.restore_pending = 0;
		}
		else {
			memcpy(&cproc->ctx, ctx, sizeof(context_t));
		}
	}

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
			to->ctx.sp = ALIGN_DOWN(to->space->interrupt.stack + THREAD_STACK_PAGES * PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
		}
		else if (to->space->signal.do_switch) {																			 // have signal request to handle
			memcpy(&to->space->signal.saved_state.ctx, &to->ctx, sizeof(context_t)); // save "to" context to ipc ctx, will restore after ipc done.
			to->ctx.gpr[0] = to->space->signal.sig_no;
			to->ctx.pc = to->ctx.lr = to->space->signal.entry;
			if(to->space->signal.stack == 0)
				to->space->signal.stack = thread_stack_alloc(to);
			to->ctx.sp = ALIGN_DOWN(to->space->signal.stack + THREAD_STACK_PAGES * PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
			to->space->signal.do_switch = false; // clear ipc request mask
		}
		else if (to->space->ipc_server.do_switch) { // have ipc request to handle
			ipc_task_t *ipc = proc_ipc_get_task(to);
			if(ipc == NULL) {
				to->space->ipc_server.do_switch = false;
				goto proc_switch_done;
			}
			memcpy(&to->space->ipc_server.saved_state.ctx, &to->ctx, sizeof(context_t)); // save "to" context to ipc ctx, will restore after ipc done.
			to->ctx.gpr[0] = ipc->uid;
			to->ctx.gpr[1] = to->space->ipc_server.extra_data;
			to->ctx.pc = to->ctx.lr = to->space->ipc_server.entry;
			if(to->space->ipc_server.stack == 0)
				to->space->ipc_server.stack = thread_stack_alloc(to);
			to->ctx.sp = ALIGN_DOWN(to->space->ipc_server.stack + THREAD_STACK_PAGES * PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
			to->space->ipc_server.do_switch = false; // clear ipc request mask
		}
	}

proc_switch_done:

	if(cproc != to && cproc != NULL &&
			cproc->info.state != UNUSED &&
			cproc != _cpu_cores[cproc->info.core].idle_proc) {
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
	proc_track_priority_update(to);
	if(cproc != to)
		set_current_proc(to);
	memcpy(ctx, &to->ctx, sizeof(context_t));
	proc_lock_leave();
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

static inline void proc_queue_remove_all(proc_t* proc) {
	queue_item_t* it = queue_in(&_ready_queue[proc->info.core], proc);
	while(it != NULL) {
		queue_remove(&_ready_queue[proc->info.core], it);
		it = queue_in(&_ready_queue[proc->info.core], proc);
	}
}

static inline void proc_ready_with_order(proc_t* proc, bool push_head) {
	proc_lock_enter();
	if(proc == NULL) {
		proc_lock_leave();
		return;
	}

	proc->info.state = READY;
	proc_track_priority_update(proc);

#ifdef __x86_64__
	if(proc->priority_count == 0)
		proc->priority_count = proc->info.priority;

	proc_queue_remove_all(proc);
	if(push_head)
		queue_push_head(&_ready_queue[proc->info.core], proc);
	else
		queue_push(&_ready_queue[proc->info.core], proc);
#else
	(void)push_head;
	if(proc->priority_count == 0)
		proc->priority_count = proc->info.priority;

	if(queue_in(&_ready_queue[proc->info.core], proc) == NULL)
		queue_push(&_ready_queue[proc->info.core], proc);
#endif
	proc_lock_leave();
}

void proc_ready(proc_t* proc) {
	proc_ready_with_order(proc, false);
}

proc_t* proc_get_core_ready(uint32_t core_id) {
	proc_lock_enter();
	proc_t* ready = (proc_t*)_ready_queue[core_id].head;
	proc_lock_leave();
	return ready;
}

bool proc_have_ready_task(uint32_t core) {
	proc_lock_enter();
	if(core >= CPU_MAX_CORES || core >= _sys_info.cores) {
		proc_lock_leave();
		return false;
	}
	bool ready = !queue_is_empty(&_ready_queue[core]);
	proc_lock_leave();
	return ready;
}

proc_t* proc_get_next_ready(void) {
	proc_lock_enter();
	uint32_t core_id = get_core_id();
	proc_t* next = queue_pop(&_ready_queue[core_id]);
	while(next != NULL && next->info.state != READY && next->info.state != RUNNING)
		next = queue_pop(&_ready_queue[core_id]);

	if(next == NULL) {
		proc_t*	cproc = get_current_proc();
		if(cproc != NULL && cproc->info.state == RUNNING &&
				cproc != _cpu_cores[cproc->info.core].idle_proc)
				//halt proc can't be sheduled.
			next = cproc;
	}
	proc_lock_leave();
	return next;
}

static inline void proc_unready_locked(proc_t* proc, int32_t state) {
#ifdef __x86_64__
	proc_queue_remove_all(proc);
#else
	queue_item_t* it = queue_in(&_ready_queue[proc->info.core], proc);
	if(it != NULL)
		queue_remove(&_ready_queue[proc->info.core], it);
#endif
	proc_untrack_priority_update(proc);
	proc->info.state = state;
}

static inline void proc_unready(proc_t* proc, int32_t state) {
	proc_lock_enter();
	proc_unready_locked(proc, state);
	proc_lock_leave();
}

static void proc_wakeup_waiting(int32_t pid) {
	int32_t i;
	for (i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t *proc = _task_table[i];
		if (proc != NULL && proc->info.state == WAIT && proc->info.wait_for == pid) {
			proc->info.wait_for = -1;
#ifdef __x86_64__
			proc_ready_with_order(proc, true);
#else
			proc_ready(proc);
#endif
		}
	}
}

static inline void proc_update_vsyscall(proc_t* proc) {
	if(_kernel_info.vsyscall_info == NULL || proc == NULL || proc->info.pid < 0)
		return;

	_kernel_info.vsyscall_info->proc_info[proc->info.pid].father_pid = proc->info.father_pid;
	_kernel_info.vsyscall_info->proc_info[proc->info.pid].uuid = proc->info.uuid;
	_kernel_info.vsyscall_info->proc_info[proc->info.pid].type = proc->info.type;
}

static void proc_terminate(context_t* ctx, proc_t* proc) {
	if(proc->info.state == ZOMBIE || proc->info.state == UNUSED)
		return;
	proc_unready(proc, ZOMBIE);
	proc_untrack_interrupt_timeout(proc);
	proc_untrack_ipc_timeout(proc);
	/*
	 * Every task pid may own a cloned VFS fd table. If a thread is terminated
	 * by the kernel before user-mode cleanup runs, vfsd still needs an exit
	 * event to release those descriptors; otherwise device refs (for example
	 * netd socket tasks) can stay pinned forever.
	 */
	kev_push(KEV_PROC_EXIT, proc->info.pid, 0, 0);

	if(proc->info.type == TASK_TYPE_PROC) {
		semaphore_clear(proc->info.pid);
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
		proc_ipc_wakeup_all(proc);
		proc_interrupt_wakeup_all(proc);
	
		if(proc->space->interrupt.state != INTR_STATE_IDLE) {
			if(proc->space->interrupt.interrupt != IRQ_SOFT) {
				irq_enable_arch(proc->space->interrupt.interrupt);
			}
			proc->space->interrupt.state = INTR_STATE_IDLE;
		}
		proc->info.father_pid = 0;
	}
	else if(proc->info.type == TASK_TYPE_THREAD) { //TODO
		proc->info.father_pid = 0;
	}
	proc_wakeup_waiting(proc->info.pid);

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

static void proc_funeral_dump_target(const char* stage, proc_t* current, proc_t* target) {
	if(target == NULL) {
		printf("proc_funeral[%s]: target=null\n", stage);
		return;
	}

	printf("proc_funeral[%s]: current(pid=%d cmd=%s) target(pid=%d type=%d state=%d cmd=%s)\n",
			stage,
			current ? current->info.pid : -1,
			current ? current->info.cmd : "<none>",
			target->info.pid,
			target->info.type,
			target->info.state,
			target->info.cmd);
}

static void proc_funeral_dump_space(const char* stage, proc_space_t* space) {
	if(space == NULL) {
		printf("proc_funeral[%s]: space=null\n", stage);
		return;
	}

	printf("proc_funeral[%s]: pde_index=%d refs=%d heap=%x malloc=%x shm0=%d\n",
			stage,
			space->pde_index,
			space->refs,
			space->heap_size,
			space->malloc_base,
			space->shms[0]);
}

void proc_funeral(proc_t* proc) {
	proc_t* cproc = get_current_proc();
	proc_space_t* space;
	if(cproc == NULL || cproc == proc || proc->info.state == UNUSED)
		return;
	space = proc->space;
	if(space == NULL) {
		proc_funeral_dump_target("space-null", cproc, proc);
		_task_table[proc->info.pid] = NULL;
		kfree(proc);
		return;
	}
	if(proc->info.type == TASK_TYPE_PROC) {
		if(space->refs > 0) //keep it still got child thread running.
			return;
	}
	else {
		if(space->refs > 0)
			space->refs--;
	}

	if(space->vm == NULL) {
		proc_funeral_dump_target("vm-null", cproc, proc);
		proc_funeral_dump_space("vm-null", space);
		_task_table[proc->info.pid] = NULL;
		kfree(proc);
		return;
	}

	if(proc->info.type == TASK_TYPE_PROC &&
			space->pde_index >= _kernel_config.max_proc_num) {
		proc_funeral_dump_target("pde-bad", cproc, proc);
		proc_funeral_dump_space("pde-bad", space);
		_task_table[proc->info.pid] = NULL;
		kfree(proc);
		return;
	}

	set_translation_table_base((uint32_t)V2P(space->vm));
	dma_release(proc->info.pid);
	proc_free_user_stack(proc);

	if(proc->info.type == TASK_TYPE_PROC) {
		//free small_stack
		if (space->interrupt.stack != 0) {
			thread_stack_free(proc, space->interrupt.stack);
		}
		if (space->signal.stack != 0) {
			thread_stack_free(proc, space->signal.stack);
		}
		if (space->ipc_server.stack != 0) {
			thread_stack_free(proc, space->ipc_server.stack);
		}

		/*free proc heap*/
		proc_shrink_mem(proc, space->heap_size / PAGE_SIZE);

		/*unmap share mems*/
		proc_unmap_shms(proc);

		set_translation_table_base(V2P(cproc->space->vm));
		free_page_tables(space->vm);

		if(_proc_vm_mark == NULL) {
			proc_funeral_dump_target("mark-null", cproc, proc);
		}
		else {
			_proc_vm_mark[space->pde_index] = 0;
		}

		if(space->thread_stacks != NULL)
			kfree(space->thread_stacks);
		kfree(space);
		proc->space = NULL;
	}
	else {
		set_translation_table_base(V2P(cproc->space->vm));
	}

	_task_table[proc->info.pid] = NULL;
	kfree(proc);
}

void proc_zombie_funeral(void) {
	int32_t i;
	for (i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t *p = _task_table[i];
		if(p != NULL && p->info.state == ZOMBIE)
			proc_funeral(p);
	}
}

static inline void proc_kick_ready_core(proc_t* proc) {
#ifdef KERNEL_SMP
	if(proc == NULL)
		return;
	if(_cpu_cores[proc->info.core].actived &&
			proc->info.core != get_core_id()) {
		_cpu_cores[proc->info.core].need_resched = 1;
		ipi_send(proc->info.core);
	}
#else
	(void)proc;
#endif
}

/* proc_free frees all resources allocated by proc. */
void proc_exit(context_t* ctx, proc_t *proc, int32_t res) {
	(void)res;
	if(proc->info.state != UNUSED && proc->info.state != ZOMBIE)
		proc_terminate(ctx, proc);
	schedule(ctx);
}

void* proc_malloc(proc_t* proc, int32_t size) {
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

uint32_t proc_msize(proc_t* proc) {
	return proc->space->heap_size - proc->space->malloc_base;
}

void proc_free(proc_t* proc) {
	uint32_t size = proc_msize(proc);
	uint32_t pages = size / PAGE_SIZE;
	proc_shrink_mem(proc, pages);
}

static inline uint32_t core_task_count_locked(uint32_t core) {
	uint32_t count = 0;

	for(uint32_t i = 0; i < _kernel_config.max_task_num; i++) {
		proc_t* task = _task_table[i];
		if(task == NULL || task->info.state == UNUSED || task->info.state == ZOMBIE)
			continue;
		if(task->is_core_idle_proc || task->info.core != core)
			continue;
		count++;
	}
	return count;
}

static inline uint32_t core_fetch(proc_t* proc) {
	if(_sys_info.cores == 1 || proc->info.uid < 0)
		return 0;

	//fetch the next core.
	/*uint32_t ret = _use_core_id++; 
	if(_use_core_id >= _sys_info.cores) 
		_use_core_id = 1;
	return ret;
	*/

	/*
	 * Pick the least loaded active core first, then use idle time as a
	 * tie-breaker.  Relying on idle time alone can collapse a burst of
	 * thread creations onto the same core because the accounting window has
	 * not yet reflected the freshly attached tasks.
	 */
	uint32_t start = _use_core_id;
	if(start >= _sys_info.cores)
		start = 0;

	uint32_t ret = 0;
	uint32_t best_tasks = 0xffffffffU;
	uint32_t best_idle = 0;
	bool found = false;

	proc_lock_enter();
	proc_refresh_runtime_stats_internal(true, false, proc_account_now_usec());

	for(uint32_t off = 0; off < _sys_info.cores; off++) {
		uint32_t i = (start + off) % _sys_info.cores;
		if(!_cpu_cores[i].actived || _cpu_cores[i].idle_proc == NULL)
			continue;

		if(_cpu_cores[i].idle_proc->info.state == CREATED) {
			_use_core_id = (i + 1) % _sys_info.cores;
			proc_lock_leave();
			return i;
		}

		uint32_t task_count = core_task_count_locked(i);
		uint32_t idle_usec = _cpu_cores[i].idle_proc->info.run_usec;
		if(!found || task_count < best_tasks ||
				(task_count == best_tasks && idle_usec > best_idle)) {
			ret = i;
			best_tasks = task_count;
			best_idle = idle_usec;
			found = true;
		}
	}
	proc_lock_leave();

	if(!found)
		return 0;

	_use_core_id = (ret + 1) % _sys_info.cores;
	return ret;
}

static inline void core_attach(proc_t* proc) {
	proc_t* parent;

	if(proc == NULL) {
		return;
	}

	/*
	* Keep threads with their parent because they share one address space and
	* frequently synchronize with the creator. Real processes can be queued
	* onto another core safely because they are only woken after fork/clone
	* setup is finished.
	*/
	/*parent = proc_get(proc->info.father_pid);
	if(parent != NULL) {
		
		if(proc->info.type != TASK_TYPE_PROC) {
			proc->info.core = parent->info.core;
			return;
		}
	}
	*/

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
	proc->info.start_sec = _kernel_info.uptime_sec;
	CONTEXT_INIT(proc->ctx);

	proc_update_vsyscall(proc);
	return proc;
}

static inline void proc_free_heap(proc_t* proc) {
	proc_shrink_mem(proc, proc->space->heap_size/PAGE_SIZE);
	proc->space->heap_used = 0;
}

static void proc_load_segment(proc_t* proc,
		uint32_t vaddr,
		const uint8_t* src,
		uint32_t filesz,
		uint32_t memsz) {
	uint32_t copied = 0;

	while (copied < filesz) {
		uint32_t cur_vaddr = vaddr + copied;
		uint32_t page_off = cur_vaddr & (PAGE_SIZE - 1);
		uint32_t chunk = PAGE_SIZE - page_off;
		if (chunk > (filesz - copied))
			chunk = filesz - copied;

		ewokos_addr_t kaddr = resolve_kernel_address(proc->space->vm, cur_vaddr);
		memcpy((void*)kaddr, src + copied, chunk);
		copied += chunk;
	}

	while (copied < memsz) {
		uint32_t cur_vaddr = vaddr + copied;
		uint32_t page_off = cur_vaddr & (PAGE_SIZE - 1);
		uint32_t chunk = PAGE_SIZE - page_off;
		if (chunk > (memsz - copied))
			chunk = memsz - copied;

		ewokos_addr_t kaddr = resolve_kernel_address(proc->space->vm, cur_vaddr);
		memset((void*)kaddr, 0, chunk);
		copied += chunk;
	}
}

/* proc_load loads the given ELF process image into the given process. */
int32_t proc_load_elf(proc_t *proc, const char *image, uint32_t size) {
	uint32_t prog_header_offset = 0;
	uint32_t prog_header_count = 0;
	uint32_t i = 0;

	proc->info.uuid = ++_proc_uuid; //load elf means a totally new proc
	int32_t shm_id = shm_get(0, size, 0666);
	if(shm_id <= 0) {
		return -1;
	}
	uint8_t* proc_image = (uint8_t*)shm_proc_map(proc, shm_id);
	if(proc_image == NULL) {
		// Don't unmap if mapping failed
		return -1;
	}
	memcpy(proc_image, image, size);
	proc_free_heap(proc);
	proc->space->rw_heap_base = 0;

	/*read elf format from saved proc image*/
	if (ELF_TYPE(proc_image) != ELFTYPE_EXECUTABLE) {
		shm_proc_unmap(proc, (void*)proc_image);
		return -1;
	}

	prog_header_count = ELF_PHNUM(proc_image);
	uint32_t *debug = 0;
	for (i = 0; i < prog_header_count; i++) {
		/* make enough room for this section */
		uint32_t vaddr = ELF_PVADDR(proc_image, i);
		uint32_t memsz = ELF_PSIZE(proc_image, i);
		uint32_t filesz = ELF_PFILESZ(proc_image, i);
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

		uint32_t old_heap_size = proc->space->heap_size;
		uint32_t need_heap_size = ALIGN_UP(vaddr + memsz, PAGE_SIZE);
		if (proc->space->heap_size < need_heap_size) {
			uint32_t expand_pages = (need_heap_size - proc->space->heap_size) / PAGE_SIZE;
			proc->space->heap_used += expand_pages * PAGE_SIZE;
			if(proc_expand_mem(proc, expand_pages) != 0){ 
				shm_proc_unmap(proc, (void*)proc_image);
				return -1;
			}
		}

		/* Copy initialized bytes in page-sized chunks instead of byte-by-byte. */
		if (offset < size) {
			uint32_t copy_filesz = filesz;
			if (copy_filesz > (size - offset))
				copy_filesz = size - offset;
			proc_load_segment(proc, vaddr, proc_image + offset, copy_filesz, memsz);
		}
		else
			proc_load_segment(proc, vaddr, proc_image, 0, memsz);
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
	proc->ctx.sp = ALIGN_DOWN(user_stack_base + pages*PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
	proc->ctx.pc = ELF_ENTRY(proc_image);
	proc->ctx.lr = ELF_ENTRY(proc_image);
	proc_ready(proc);
	shm_proc_unmap(proc, (void*)proc_image);

	proc_update_vsyscall(proc);
	return 0;
}

void proc_usleep(context_t* ctx, uint32_t count) {
	proc_t* cproc = get_current_proc();
	if(cproc == NULL)
		return;

	proc_lock_enter();
	if(cproc->info.state == READY) {
		/* A cross-core wakeup (e.g. IPC dispatch) already marked us READY;
		 * honour it instead of sleeping. */
		cproc->info.state = RUNNING;
		proc_lock_leave();
		return;
	}
	cproc->sleep_counter = count;
	proc_unready_locked(cproc, SLEEPING);
	proc_lock_leave();
	proc_account_pause_current();
	schedule(ctx);
	proc_account_resume_current();
}
	
void proc_block(context_t* ctx, proc_t* proc) {
	if(proc == NULL)
		return;
	proc_lock_enter();
	if(proc->info.state == READY) {
		/*
		 * A wakeup may arrive between userspace deciding to block and the
		 * actual SYS_BLOCK trap. Preserve that sticky wake instead of
		 * overwriting it with BLOCK and sleeping forever.
		 */
		proc->info.state = RUNNING;
		proc_lock_leave();
		return;
	}
	/*
	 * Hold the lock through proc_unready_locked() to close the race window
	 * where another core's proc_ready() (e.g. from IPC dispatch) could
	 * enqueue us between the READY check above and the state transition
	 * to BLOCK. Without this, a cross-core IPC wakeup can be lost.
	 */
	proc_unready_locked(proc, BLOCK);
	proc_lock_leave();
	if(proc == get_current_proc())
		proc_account_pause_current();
	//proc_block_saved_state(pid_by, event, cproc);
	schedule(ctx);
	if(proc == get_current_proc())
		proc_account_resume_current();
}

void proc_waitpid(context_t* ctx, int32_t pid) {
	proc_t* cproc = get_current_proc();
	proc_t* p;
	if(cproc == NULL || pid < 0 || pid >= _kernel_config.max_task_num)
		return;

	proc_lock_enter();
	p = _task_table[pid];
	if(p == NULL || p->info.state == UNUSED) {
		proc_lock_leave();
		return;
	}
	if(p->info.state == ZOMBIE) {
		proc_lock_leave();
		proc_funeral(p);
		return;
	}

	/*
	 * Publish WAIT while holding the proc lock so a fast child exit cannot
	 * run proc_wakeup_waiting(pid) before the parent becomes visible as a
	 * waiter, otherwise the wake edge is lost and waitpid() can sleep forever.
	 */
	cproc->info.wait_for = pid;
	proc_unready_locked(cproc, WAIT);
	proc_lock_leave();
	proc_account_pause_current();
	schedule(ctx);
	proc_account_resume_current();

	p = _task_table[pid];
	if(p != NULL && p->info.state == ZOMBIE) {
		proc_funeral(p);
	}
}

static void proc_wakeup_all_state(proc_t* proc) {
	if(proc->space == NULL)
		return;

	proc_ready(proc);
	proc->space->ipc_server.saved_state.state = READY;
	proc->space->signal.saved_state.state = READY;
	proc->space->interrupt.saved_state.state = READY;
}

void proc_wakeup(proc_t* proc) {
	proc_lock_enter();
	if(proc == NULL || proc->info.state == UNUSED ||
			proc->info.state == ZOMBIE) {
		proc_lock_leave();
		return;
	}
	proc_wakeup_all_state(proc);
	proc_lock_leave();
#ifdef KERNEL_SMP
	if(_cpu_cores[proc->info.core].actived) {
		_cpu_cores[proc->info.core].need_resched = 1;
		if(proc->info.core != get_core_id()) {
		ipi_send(proc->info.core);
		}
	}
#endif
}

static inline char* proc_clone_addr_to_ptr(proc_t* proc, uint32_t addr) {
	if(addr >= KERNEL_BASE) {
		return (char*)addr;
	}

	ewokos_addr_t phy = resolve_phy_address(proc->space->vm, addr);
	if(phy == 0) {
		return NULL;
	}
	return (char*)P2V(phy);
}

static inline void proc_page_clone(proc_t* to, uint32_t to_addr, proc_t* from, uint32_t from_addr) {
	char *to_ptr = proc_clone_addr_to_ptr(to, to_addr);
	char *from_ptr = proc_clone_addr_to_ptr(from, from_addr);
	if(to_ptr == NULL || from_ptr == NULL) {
		printf("panic: proc_page_clone invalid addr to=0x%x from=0x%x\n", to_addr, from_addr);
		return;
	}
	memcpy(to_ptr, from_ptr, PAGE_SIZE);
}

static int32_t proc_clone(proc_t* child, proc_t* parent) {
	uint32_t pages = parent->space->heap_size / PAGE_SIZE;
	if((parent->space->heap_size % PAGE_SIZE) != 0)
		pages++;

	// Copy On Write
	uint32_t p;
	for(p=0; p<pages; ++p) { 
		uint32_t v_addr = (p * PAGE_SIZE);
		uint32_t phy_page_addr = resolve_phy_address(parent->space->vm, v_addr);

		/*
		 * Some virtual pages below heap_size are reserved but not backed yet.
		 * Skip those holes instead of remapping them to physical 0.
		 */
		if(phy_page_addr == 0)
			continue;

		map_page_ref(child->space->vm,
				v_addr,
				phy_page_addr,
				AP_RW_R, PTE_ATTR_WRBACK); // share page table to child with read only permissions, and ref the page

		map_page(parent->space->vm,
				v_addr,
				phy_page_addr,
				AP_RW_R, PTE_ATTR_WRBACK); // set parent page table with read only permissions
	}
	flush_tlb();
	child->space->heap_size = pages * PAGE_SIZE;
	/*
	 * Preserve the parent's actual heap break. User-space allocators keep
	 * their current break in process memory and continue from there after
	 * fork(), so resetting heap_used to heap_size desynchronizes kernel/user
	 * heap state in the child.
	 */
	child->space->heap_used = parent->space->heap_used;

	// Copy the process user stack via its user virtual addresses. The
	// user_stack[] array stores kernel backing-page pointers, not process
	// virtual addresses, so passing it into resolve_kernel_address() will
	// translate an unmapped address and can collapse to P2V(0).
	int32_t i;
	uint32_t user_stack_base = proc_get_user_stack_base(parent);
	for(i=0; i<STACK_PAGES; i++) {
		uint32_t vaddr = user_stack_base + PAGE_SIZE*i;
		proc_page_clone(child, vaddr, parent, vaddr);
	}
	child->space->malloc_base = parent->space->malloc_base;
	child->space->rw_heap_base = parent->space->rw_heap_base;
	return 0;
}

proc_t* kfork_raw(context_t* ctx, int32_t type, proc_t* parent) {
	proc_t *child = NULL;
	child = proc_create(type, parent);
	if(child == NULL) {
		printf("panic: kfork create proc failed!!(%d)\n", parent->info.pid);
		return NULL;
	}
	if(ctx != NULL)
		memcpy(&child->ctx, ctx, sizeof(context_t));
	else
		memcpy(&child->ctx, &parent->ctx, sizeof(context_t));
	child->ctx.gpr[0] = 0;

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
		child->ctx.sp = ALIGN_DOWN(child->thread_stack_base + THREAD_STACK_PAGES*PAGE_SIZE, EWOK_STACK_ALIGN) - EWOK_STACK_INIT_BIAS;
	}
	return child;
}

proc_t* kfork(context_t* ctx, int32_t type) {
	proc_t* cproc = get_current_proc();
	proc_t* child = kfork_raw(ctx, type, cproc);
	core_attach(child);

	if(_core_proc_ready && child->info.type == TASK_TYPE_PROC) {
		kev_push(KEV_PROC_CREATED, cproc->info.pid, child->info.pid, 0);
	}
	else
		proc_ready(child);
	return child;
}

int32_t get_procs_num(void) {
	int32_t res = 0;
	uint32_t i;
	proc_lock_enter();
	for(i=0; i<_kernel_config.max_task_num; i++) {
		if(_task_table[i] != NULL && _task_table[i]->info.state != UNUSED)  {
			if(!_task_table[i]->is_core_idle_proc)
				res++;
		}
	}
	proc_lock_leave();
	return res;
}

int32_t get_procs(int32_t num, procinfo_t* procs) {
	if(procs == NULL)
		return -1;

	proc_lock_enter();
	proc_refresh_runtime_stats_internal(false, true, proc_account_now_usec());
	int32_t j = 0;
	uint32_t i;
	for(i=0; i<_kernel_config.max_task_num && j<(num); i++) {
		proc_t* p = _task_table[i];
		if(p != NULL && p->info.state != UNUSED && !p->is_core_idle_proc) {
			memcpy(&procs[j], &p->info, sizeof(procinfo_t));
			procs[j].heap_size = p->space->heap_size;
			j++;
		}
	}
	proc_lock_leave();
	return 0;
}

int32_t get_proc(int32_t pid, procinfo_t *info) {
	proc_lock_enter();
	proc_t* proc = proc_get(pid);
	if(proc == NULL) {
		proc_lock_leave();
		return -1;
	}

	proc_refresh_runtime_stats_internal(false, true, proc_account_now_usec());
	memcpy(info, &proc->info, sizeof(procinfo_t));
	info->heap_size = proc->space->heap_size;
	proc_lock_leave();
	return 0;
}

int32_t proc_get_root_pid_visible(int32_t pid) {
	int32_t result = -1;
	proc_lock_enter();
	proc_t* cproc = get_current_proc();
	proc_t* proc = cproc;
	if(cproc == NULL) {
		proc_lock_leave();
		return -1;
	}
	if(pid >= 0)
		proc = proc_get(pid);
	if(proc == NULL) {
		proc_lock_leave();
		return -1;
	}
	if(cproc->info.uid > 0 && cproc->info.uid != proc->info.uid) {
		proc_lock_leave();
		return -1;
	}
	proc_t* root = proc_get_proc(proc);
	if(root != NULL)
		result = root->info.pid;
	proc_lock_leave();
	return result;
}

int32_t proc_get_current_thread_id_safe(void) {
	int32_t pid = -1;
	proc_lock_enter();
	proc_t* cproc = get_current_proc();
	if(cproc != NULL)
		pid = cproc->info.pid;
	proc_lock_leave();
	return pid;
}

int32_t proc_get_current_uid_safe(void) {
	int32_t uid = -1;
	proc_lock_enter();
	proc_t* cproc = get_current_proc();
	if(cproc != NULL)
		uid = cproc->info.uid;
	proc_lock_leave();
	return uid;
}

int32_t proc_get_current_gid_safe(void) {
	int32_t gid = -1;
	proc_lock_enter();
	proc_t* cproc = get_current_proc();
	if(cproc != NULL)
		gid = cproc->info.gid;
	proc_lock_leave();
	return gid;
}

int32_t proc_get_cmd_safe(int32_t pid, char* cmd, int32_t sz) {
	if(cmd == NULL || sz <= 0)
		return -1;
	proc_lock_enter();
	proc_t* proc = proc_get(pid);
	if(proc == NULL) {
		proc_lock_leave();
		return -1;
	}
	sstrncpy(cmd, proc->info.cmd, sz);
	proc_lock_leave();
	return 0;
}

int32_t proc_get_ready_ping_safe(int32_t pid) {
	int32_t ret = -1;
	proc_lock_enter();
	proc_t* proc = proc_get_proc(proc_get(pid));
	if(proc != NULL && proc->space->ready_ping)
		ret = 0;
	proc_lock_leave();
	return ret;
}

int32_t proc_get_core_pid_safe(void) {
	int32_t pid;
	proc_lock_enter();
	pid = _core_proc_pid;
	proc_lock_leave();
	return pid;
}

void proc_set_core_pid_safe(int32_t pid) {
	proc_lock_enter();
	_core_proc_pid = pid;
	proc_lock_leave();
}

static int32_t renew_sleep_counter(uint32_t usec) {
	proc_lock_enter();
	uint32_t i;
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
				proc_kick_ready_core(proc);
				res = 0;
			}
		}
	}
	proc_lock_leave();
	return res;
}

static int32_t renew_interrupt_counter(uint32_t usec) {
	proc_lock_enter();
	int32_t res = -1;
	queue_item_t* it = _interrupt_timeout_queue.head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		proc_t* proc = (proc_t*)it->data;
		if(proc == NULL ||
				proc->info.state == UNUSED || proc->info.state == ZOMBIE ||
				proc->space == NULL ||
				proc->space->interrupt.state == INTR_STATE_IDLE) {
			proc_untrack_interrupt_timeout(proc);
			it = next;
			continue;
		}
		proc->space->interrupt.counter += usec;
		if(proc->space->interrupt.counter >= INTERRUPT_TIMEOUT_USEC) {
			printf("interrupt timeout: %d , %d\n", proc->space->interrupt.interrupt, proc->info.pid);
			proc_interrupt_timeout(proc);
			res = 0;
		}
		it = next;
	}
	proc_lock_leave();
	return res;
}

static int32_t renew_ipc_counter(uint32_t usec) {
	proc_lock_enter();
	int32_t res = -1;
	queue_item_t* it = _ipc_timeout_queue.head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		proc_t* proc = (proc_t*)it->data;
		if(proc == NULL ||
				proc->info.state == UNUSED || proc->info.state == ZOMBIE ||
				proc->space == NULL) {
			proc_untrack_ipc_timeout(proc);
			it = next;
			continue;
		}
		ipc_task_t* ipc = &proc->space->ipc_server.ctask;
		if(ipc->state == IPC_IDLE) {
			proc_untrack_ipc_timeout(proc);
			it = next;
			continue;
		}

		ipc->counter += usec;
		if(ipc->counter >= IPC_TIMEOUT_USEC) {
			proc_t* client = proc_get(ipc->client_pid);
			printf("ipc timeout: uid:%u clt:%d(%s), srv:%d(%s), call:%d/0x%x\n",
					ipc->uid,
					ipc->client_pid,
					client ? client->info.cmd : "?",
					proc->info.pid,
					proc->info.cmd,
					ipc->call_id,
					ipc->call_id);
			proc_ipc_timeout(proc);
			res = 0;
		}
		it = next;
	}
	proc_lock_leave();
	return res;
}

static int32_t renew_priority_counter(uint32_t usec) {
	(void)usec;
	proc_lock_enter();
	queue_item_t* it = _priority_update_queue.head;
	while(it != NULL) {
		queue_item_t* next = it->next;
		proc_t* proc = (proc_t*)it->data;
		if(proc == NULL || proc->info.state == UNUSED || proc->info.state == ZOMBIE ||
				(proc->info.state != RUNNING && proc->info.state != READY)) {
			proc_untrack_priority_update(proc);
			it = next;
			continue;
		}
		if(proc->priority_count > 0)
			proc->priority_count--;

		if(proc->priority_count > 0) {
			it = next;
			continue;
		}

		if(proc->info.state == READY) {
			proc_ready(proc);
		}
		else if(proc->info.state == RUNNING &&
				get_current_core_proc(proc->info.core) == proc) {
			proc->priority_count = proc->info.priority;
		}
		it = next;
	}
	proc_lock_leave();
	return 0;
}

static void renew_vsyscall_info(void) {
	if(_kernel_info.vsyscall_info == NULL)
		return;

	_kernel_info.vsyscall_info->kernel_usec = _kernel_info.uptime_usec;
}

int32_t renew_kernel_tic(uint32_t usec) {
	renew_vsyscall_info();
	renew_priority_counter(usec);
	renew_interrupt_counter(usec);
	renew_ipc_counter(usec);
	return renew_sleep_counter(usec);	
}

void renew_kernel_sec(void) {
	proc_zombie_funeral();
	proc_refresh_runtime_stats();
	uint64_t now_usec = proc_account_now_usec();
	if(now_usec - _run_window_start_usec >=
			((uint64_t)KERNEL_PROC_RUN_RECOUNT_SEC * 1000000ULL)) {
		for(uint32_t i=0; i<_kernel_config.max_task_num; i++) {
			proc_t* proc = _task_table[i];
			if(proc == NULL)
				continue;
			if(proc->info.state != UNUSED && proc->info.state != ZOMBIE) {
				proc->run_usec_counter = 0;
				proc->info.run_usec = 0;
				if(proc->run_accounting_active)
					proc->run_last_start_usec = now_usec;
			}
		}
		_run_window_start_usec = now_usec;
	}
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
	_cpu_cores[core].idle_proc = child;
	child->is_core_idle_proc = true;

	return child;
}
