#ifndef PROC_H
#define PROC_H

#ifdef __ASSEMBLER__

#define SAVE_CONTEXT \
	push {r0, r14};\
	mov r0, lr;\
	bl __saveContext1;\
	pop {r0, r14};\
	push {r14};\
	bl __saveContext2;\
	pop {r14};

#else

#include <types.h>
#include <mmu.h>
#include <pmalloc.h>
#include <kmessage.h>

typedef void (*EntryFunctionT)(void);

#define PROCESS_COUNT_MAX 128

enum ProcessState {
	UNUSED,
	CREATED,
	SLEEPING,
	READY,
	RUNNING,
	TERMINATED
};

enum ContextItem {
	CPSR, RESTART_ADDR,
	R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12,
	SP, LR
};

typedef struct {
	enum ProcessState state;
	int32_t pid; /*auto-increased id*/
	EntryFunctionT entry;
	PageDirEntryT *vm;
	uint32_t heapSize;
	char *userStack;
	char *kernelStack;
	int context[17];

	int waitPid;
	int childReturnValue;

	/*for malloc*/
	MallocT mallocMan;

	/*for process communication*/
	MessageQueueT messageQueue;
} ProcessT;

/* public symbols */
extern ProcessT *_currentProcess;
extern ProcessT _processTable[PROCESS_COUNT_MAX];

extern void procInit();
extern ProcessT *procCreate(void);
bool procLoad(ProcessT *proc, const char *procImage);
void procStart(ProcessT *proc);
void procFree(ProcessT *proc);
bool procExpandMemory(void *proc, int pageCount);
void procShrinkMemory(void *proc, int pageCount);
ProcessT* procGet(int pid);

#endif

#endif
