#ifndef PROC_H
#define PROC_H

#include <kernel/context.h>
#include <mm/mmu.h>
#include <mm/trunkmem.h>
#include <procinfo.h>
#include <proto.h>
#include <stdbool.h>

enum {
	UNUSED = 0,
	CREATED,
	SLEEPING,
	WAIT,
	BLOCK,
	READY,
	RUNNING,
	ZOMBIE
};

enum {
	IPC_IDLE = 0,
	IPC_BUSY,
	IPC_RETURN
};

#define PROC_FILE_MAX 128
#define SHM_MAX 128
#define LOCK_MAX 64

typedef struct {
	page_dir_entry_t *vm;
	malloc_t malloc_man;
	uint32_t heap_size;
	bool ready_ping;
	
	int32_t shms[SHM_MAX];
	uint32_t locks[LOCK_MAX];

	struct {
		int32_t  ipc_pid;
		uint32_t sp;
		uint32_t entry;
		proto_t* data;
		uint32_t extra_data;
		uint32_t state;
		int32_t from_pid;
	} ipc;

} proc_space_t;

#define STACK_PAGES 32
#define THREAD_STACK_PAGES 4

#define CRITICAL_MAX 32 //critical zone keep just for 32 timer schedules
typedef struct st_proc {
	int32_t type;
	int32_t pid;
	int32_t father_pid;
	int32_t state;
	int32_t owner;
	uint32_t start_sec;

	int32_t critical_counter;
	uint32_t block_event;
	int32_t block_pid;
	int64_t sleep_counter; //sleep usec
	int32_t wait_pid;

	char cmd[PROC_INFO_CMD_MAX];

	proc_space_t* space;
	void* user_stack[STACK_PAGES];

	context_t ctx;
} proc_t;

extern context_t* _current_ctx;
extern proc_t* _current_proc;
extern proc_t* _ready_proc;
extern bool _core_ready;
extern int32_t _core_pid;

extern void    procs_init(void);
extern int32_t proc_load_elf(proc_t *proc, const char *proc_image, uint32_t size);
extern int32_t proc_start(proc_t* proc, uint32_t entry);
extern proc_t* proc_get_next_ready(void);
extern void    proc_switch(context_t* ctx, proc_t* to, bool quick);
extern int32_t proc_expand_mem(proc_t *proc, int32_t page_num);
extern void    proc_shrink_mem(proc_t* proc, int32_t page_num);
extern void    proc_exit(context_t* ctx, proc_t *proc, int32_t res);
extern proc_t *proc_create(int32_t type, proc_t* parent);

extern void*   proc_malloc(uint32_t size);
extern void*   proc_realloc(void* p, uint32_t size);
extern void    proc_free(void* p);

extern void    proc_block_on(context_t* ctx, uint32_t event);
extern void    proc_wakeup(int32_t pid, uint32_t event);
extern void    proc_waitpid(context_t* ctx, int32_t pid);
extern proc_t* proc_get(int32_t pid);
extern proc_t* proc_get_proc(proc_t* proc);
extern proc_t* kfork_raw(int32_t type, proc_t* parent);
extern proc_t* kfork(int32_t type);

extern procinfo_t* get_procs(int32_t* num);

extern void    renew_sleep_counter(uint32_t usec);
extern void    proc_usleep(context_t* ctx, uint32_t usec);
extern void    proc_ready(proc_t* proc);

extern int32_t     proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra, bool prefork);
extern int32_t     proc_ipc_call(context_t* ctx, proc_t* proc, int32_t call_id);

#endif
