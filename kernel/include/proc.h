#ifndef PROC_H
#define PROC_H

#ifdef __ASSEMBLER__

#define SAVE_CONTEXT \
	push {r0, r14};\
	mov r0, lr;\
	bl __save_context1;\
	pop {r0, r14};\
	push {r14};\
	bl __save_context2;\
	pop {r14};

#else

#include <types.h>
#include <mm/mmu.h>
#include <mm/trunkmalloc.h>
#include <kipc.h>
#include <kfile.h>
#include <procinfo.h>

typedef void (*entry_function_t)(void);

#define PROCESS_COUNT_MAX 128

enum {
	UNUSED = 0,
	CREATED,
	SLEEPING,
	READY,
	RUNNING,
	TERMINATED
};

enum {
	TYPE_PROC = 0,
	TYPE_THREAD
};

enum {
	CPSR, RESTART_ADDR,
	R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12,
	SP, LR
};

#define FILE_MAX 32

typedef struct {
	k_file_t* kf;	
	uint32_t flags;
	uint32_t seek;
} proc_file_t;

typedef struct {
	page_dir_entry_t *vm;
	uint32_t heap_size;
	/*for malloc*/
	malloc_t malloc_man;
	/*for file*/
	proc_file_t files[FILE_MAX];
} process_space_t; 


typedef struct process {
	uint32_t type;
	uint32_t state;
	int32_t pid; /*auto-increased id*/
	int32_t father_pid; /*father pid*/
	int32_t owner; /*owner for muti-user system*/
	char cmd[CMD_MAX]; /*run command*/
	char pwd[NAME_MAX]; /*working dir*/
	int wait_pid; /*waiting for specific process end*/

	entry_function_t entry;
	char *user_stack;
	//char *kernelStack;
	int32_t context[17];

	process_space_t* space;

	/*proc link*/
	struct process* next;
	struct process* prev;
} process_t;

/* public symbols */
extern process_t *_current_proc;
extern process_t _process_table[PROCESS_COUNT_MAX];

extern void proc_init();
extern process_t *proc_create(uint32_t type);
bool proc_load(process_t *proc, const char *proc_image, uint32_t img_size);
void proc_start(process_t *proc);
void proc_free(process_t *proc);
bool proc_expand_mem(void *proc, int page_num);
void proc_shrink_mem(void *proc, int page_num);
process_t* proc_get(int pid);
void proc_sleep(int pid);
void proc_wake(int pid);

process_t* kfork(uint32_t type);
void proc_exit();
void* pmalloc(uint32_t size);

#endif

#endif
