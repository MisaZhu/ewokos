#include <mmu.h>
#include <proc.h>
#include <kernel.h>
#include <kalloc.h>
#include <lib/string.h>
#include <system.h>
#include <types.h>

ProcessT _processTable[PROCESS_COUNT_MAX];

__attribute__((__aligned__(PAGE_DIR_SIZE)))
PageDirEntryT _processVM[PROCESS_COUNT_MAX][PAGE_DIR_NUM];

ProcessT* _currentProcess = NULL;

/* proc_init initializes the process sub-system. */
void procInit(void)
{
	for (int i = 0; i < PROCESS_COUNT_MAX; i++)
		_processTable[i].state = UNUSED;
	_currentProcess = NULL;
}

/* proc_creates allocates a new process and returns it. */
ProcessT *procCreate(void)
{
	ProcessT *proc = NULL;
	int pid = -1;
	PageDirEntryT *vm = NULL;
	char *kernelStack = NULL;
	char *userStack = NULL;

	for (int i = 0; i < PROCESS_COUNT_MAX; i++)
		if (_processTable[i].state == UNUSED) {
			pid = i + 1;
			break;
		}

	if (pid == -1)
		return NULL;

	kernelStack = kalloc();
	userStack = kalloc();

	vm = _processVM[pid - 1];
	setKernelVM(vm);

	mapPages(vm, (MemoryMapT){
		KERNEL_STACK_BOTTOM,
		V2P(kernelStack),
		V2P(kernelStack) + PAGE_SIZE,
		AP_RW_R
	});

	mapPages(vm, (MemoryMapT){
		USER_STACK_BOTTOM,
		V2P(userStack),
		V2P(userStack) + PAGE_SIZE,
		AP_RW_RW
	});


	proc = &_processTable[pid - 1];
	proc->pid = pid;
	proc->state = CREATED;
	proc->vm = vm;
	proc->heapSize = 0;
	proc->kernelStack = kernelStack;
	proc->userStack = userStack;
	proc->waitPid = -1;

	return proc;
}

int *getCurrentContext(void)
{
	return _currentProcess->context;
}
