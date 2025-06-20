#ifndef PROC_H
#define PROC_H

#include <kernel/ipc.h>
#include <kernel/signal.h>
#include <kernel/interrupt.h>
//#include <kernel/core.h>
#include <mm/mmu.h>
#include <mm/trunkmem.h>
#include <procinfo.h>
#include <stdbool.h>
#include <queue.h>

#define SHM_MAX 128
#define BLOCK_EVT_MAX 16

typedef struct {
	uint32_t event;
	uint32_t refs;
} proc_block_event_t;

#define THREAD_STACK_PAGES 16
#define STACK_PAGES 32

typedef struct {
	uint32_t base;
	void**   stacks;
} thread_stack_t;

typedef struct {
	uint32_t           pde_index;
	page_dir_entry_t*  vm;

	uint32_t           malloc_base;
	uint32_t           rw_heap_base;
	uint32_t           heap_size;
	uint32_t           heap_used;
	int32_t            refs;
	bool               ready_ping;
	
	int32_t            shms[SHM_MAX];
	proc_block_event_t block_events[BLOCK_EVT_MAX];

	ipc_server_t       ipc_server;
	signal_t           signal;
	proc_interrupt_t   interrupt;

	thread_stack_t     *thread_stacks;
	uint32_t           user_stack[STACK_PAGES];
} proc_space_t;

typedef struct st_proc {
	procinfo_t        info;
	bool              is_core_idle_proc;

	uint32_t          block_event;
	uint32_t          ipc_buffered;
	bool              ipc_buffer_clean;
	ipc_res_t         ipc_res;

	int64_t           sleep_counter; //sleep usec
	uint32_t          run_usec_counter; //run time usec

	proc_space_t*     space; //threads share the space from owner proc
	uint32_t          thread_stack_base;

	context_t         ctx;
} proc_t;

extern proc_t* get_current_core_proc(uint32_t core);
extern proc_t* get_current_proc(void);
extern bool _core_proc_ready;
extern int32_t _core_proc_pid;
extern uint32_t _ipc_uid;

extern int32_t procs_init(void);
extern int32_t proc_load_elf(proc_t *proc, const char *proc_image, uint32_t size);
extern int32_t proc_start(proc_t* proc, uint32_t entry);
extern proc_t* proc_get_next_ready(void);
extern proc_t* proc_get_core_ready(uint32_t core_id);
extern void    proc_switch_multi_core(context_t* ctx, proc_t* to, uint32_t core);
extern void    proc_switch(context_t* ctx, proc_t* to, bool quick);
extern void    set_current_proc(proc_t* proc);

extern void    proc_map_page(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions);
extern void    proc_unmap_page(page_dir_entry_t *vm, uint32_t vaddr);

extern void    proc_funeral(proc_t* proc);
extern void    proc_zombie_funeral(void);
extern void    proc_exit(context_t* ctx, proc_t *proc, int32_t res);
extern proc_t *proc_create(int32_t type, proc_t* parent);

extern void*   proc_malloc(proc_t* proc, int32_t size);
extern uint32_t  proc_msize(proc_t* proc);
extern void    proc_free(proc_t* proc);

extern void    proc_block_on(context_t* text, int32_t pid_by, uint32_t event);
extern void    proc_wakeup(int32_t pid_by, int32_t pid, uint32_t event);
extern void    proc_waitpid(context_t* ctx, int32_t pid);
extern proc_t* proc_get(int32_t pid);
extern proc_t* proc_get_by_uuid(uint32_t uuid);
extern proc_t* proc_get_proc(proc_t* proc);
extern int32_t get_proc_pid(int32_t pid);
extern proc_t* kfork_raw(context_t* ctx, int32_t type, proc_t* parent);
extern proc_t* kfork(context_t* ctx, int32_t type);
extern proc_t* kfork_core_halt(uint32_t core);

extern int32_t get_procs_num(void);
extern int32_t get_procs(int32_t num, procinfo_t* procs);
extern int32_t get_proc(int32_t pid, procinfo_t *info);

extern int32_t renew_kernel_tic(uint32_t usec);
extern void    renew_kernel_sec(void);
extern void    proc_usleep(context_t* ctx, uint32_t usec);
extern void    proc_ready(proc_t* proc);
extern int32_t proc_childof(proc_t* proc, proc_t* parent);

extern bool    proc_have_ready_task(uint32_t core);
extern void    proc_save_state(proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res);
extern void    proc_restore_state(context_t* ctx, proc_t* proc, saved_state_t* saved_state, ipc_res_t* saved_ipc_res);
#endif