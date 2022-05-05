#ifndef PROC_H
#define PROC_H

#include <kernel/ipc.h>
//#include <kernel/core.h>
#include <mm/mmu.h>
#include <mm/trunkmem.h>
#include <procinfo.h>
#include <stdbool.h>
#include <queue.h>

#define PROC_FILE_MAX 128
#define SHM_MAX 128
#define LOCK_MAX 64
#define IPC_CTX_MAX 8
#define PROC_KPAGE_MAX 8

enum {
	SIG_STATE_IDLE = 0,
	SIG_STATE_BUSY
};

typedef struct {
	uint32_t  uid;
	uint32_t  state;
	proto_t   data;
} ipc_res_t;

typedef struct {
	bool          disabled;
	uint32_t      entry;
	uint32_t      flags;
	uint32_t      extra_data;
	queue_t       tasks;
	ipc_task_t*   ctask; //current_task

    bool          do_switch;
	uint32_t      stack; //mapped stack page
	saved_state_t saved_state;
} ipc_server_t;

typedef struct {
    uint32_t      entry;
    uint32_t      sig_no;

    bool          do_switch;
	uint32_t      stack; //mapped stack page
	saved_state_t saved_state;
} signal_t;

typedef struct {
    uint32_t      entry;
    uint32_t      interrupt;

    bool          do_switch;
	uint32_t      stack; //mapped stack page
	saved_state_t saved_state;
} proc_interrupt_t;

typedef struct {
	page_dir_entry_t* vm;
	malloc_t          malloc_man;
	uint32_t          heap_size;
	uint32_t          kpages[PROC_KPAGE_MAX]; //mapped pages , share same address with kernel
	bool              ready_ping;
	
	int32_t           shms[SHM_MAX];

	ipc_server_t      ipc_server;
	signal_t          signal;
	proc_interrupt_t  interrupt;
} proc_space_t;

#define STACK_PAGES 32 
#define THREAD_STACK_PAGES  4

typedef struct st_proc {
	procinfo_t        info;
	uint32_t          block_event;
	int64_t           sleep_counter; //sleep usec
	uint32_t          run_usec_counter; //run time usec
	proc_space_t*     space;

	union {
		void*             user_stack[STACK_PAGES];
		uint32_t          thread_stack;
	} stack;

	context_t         ctx;
	ipc_res_t         ipc_res;
} proc_t;

extern proc_t* get_current_proc(void);
extern bool _core_proc_ready;
extern int32_t _core_proc_pid;
extern uint32_t _ipc_uid;

extern void    procs_init(void);
extern int32_t proc_load_elf(proc_t *proc, const char *proc_image, uint32_t size);
extern int32_t proc_start(proc_t* proc, uint32_t entry);
extern proc_t* proc_get_next_ready(void);
extern proc_t* proc_get_core_ready(uint32_t core_id);
extern void    proc_switch(context_t* ctx, proc_t* to, bool quick);
extern void    set_current_proc(proc_t* proc);

extern void    proc_map_page(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions);
extern void    proc_unmap_page(page_dir_entry_t *vm, uint32_t vaddr);

extern int32_t proc_zombie_funeral(void);
extern void    proc_exit(context_t* ctx, proc_t *proc, int32_t res);
extern proc_t *proc_create(int32_t type, proc_t* parent);

extern void*   proc_malloc(proc_t* proc, uint32_t size);
extern void*   proc_realloc(proc_t* proc, void* p, uint32_t size);
extern void    proc_free(proc_t* proc, void* p);

extern void    proc_block_on(int32_t pid_by, uint32_t event);
extern void    proc_wakeup(int32_t pid, uint32_t event);
extern void    proc_waitpid(context_t* ctx, int32_t pid);
extern proc_t* proc_get(int32_t pid);
extern proc_t* proc_get_proc(proc_t* proc);
extern proc_t* kfork_raw(context_t* ctx, int32_t type, proc_t* parent);
extern proc_t* kfork(context_t* ctx, int32_t type);
extern proc_t* kfork_core_halt(uint32_t core);

extern procinfo_t* get_procs(int32_t* num);

extern void    renew_kernel_tic(uint64_t usec);
extern void    renew_kernel_sec(void);
extern void    proc_usleep(context_t* ctx, uint32_t usec);
extern void    proc_ready(proc_t* proc);

extern uint32_t proc_stack_alloc(proc_t* proc);
extern void     proc_stack_free(proc_t* proc, uint32_t stack);
extern bool    proc_have_ready_task(uint32_t core);
extern void    proc_save_state(proc_t* proc, saved_state_t* saved_state);
extern void    proc_restore_state(context_t* ctx, proc_t* proc, saved_state_t* saved_state);
#endif
