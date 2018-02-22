#include <lib/syscall.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <console.h>
#include <proc.h>
#include <kernel.h>
#include <lib/string.h>

void schedule();

static int syscall_putch(int c)
{
	kputch(c);
	return 0;
}

static int syscall_fork(void)
{
	ProcessT *child = NULL;
	ProcessT *parent = _currentProcess;
	uint32_t i = 0;

	child = procCreate();
	procExpandMemory(child, parent->heapSize / PAGE_SIZE);

	/* copy parent's memory to child's memory */
	for (i = 0; i < parent->heapSize; i++) {
		int childPAddr = resolvePhyAddress(child->vm, i);
		char *childPtr = (char *) P2V(childPAddr);
		char *parentPtr = (char *) i;

		*childPtr = *parentPtr;
	}

	/* copy parent's stack to child's stack */
	memcpy(child->kernelStack, parent->kernelStack, PAGE_SIZE);
	memcpy(child->userStack, parent->userStack, PAGE_SIZE);

	/* copy parent's context to child's context */
	memcpy(child->context, parent->context, sizeof(child->context));

	/* set return value of fork in child to 0 */
	child->context[R0] = 0;

	/* child is ready to run */
	child->state = READY;

	/* return pid of child to the parent. */
	return child->pid;
}

static int syscall_exit(int arg1)
{
	int pid;

	(void) arg1;
	if (_currentProcess == NULL)
		return -1;

	__setTranslationTableBase((uint32_t) V2P(_kernelVM));
	__asm__ volatile("ldr sp, = _initStack");
	__asm__ volatile("add sp, #4096");

	for (pid = 0; pid < PROCESS_COUNT_MAX; pid++) {
		ProcessT *proc = &_processTable[pid];

		if (proc->state == SLEEPING &&
				proc->waitPid == _currentProcess->pid) {
			proc->waitPid = -1;
			proc->state = READY;
		}
	}

	procFree(_currentProcess);
	_currentProcess = NULL;

	schedule();
	return 0;
}

static int (*const _syscallHandler[])() = {
	[SYSCALL_PUTCH] = syscall_putch,
	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_EXIT] = syscall_exit
};

/* kernel side of system calls. */
int handleSyscall(int code, int arg1, int arg2, int arg3)
{
	return _syscallHandler[code](arg1, arg2, arg3);
}

