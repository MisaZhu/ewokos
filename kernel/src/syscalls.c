#include <lib/syscall.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <dev/uart.h>
#include <proc.h>
#include <kernel.h>
#include <lib/string.h>
#include <pmalloc.h>
#include <kserv.h>

void schedule();

static int syscall_uartPutch(int c)
{
	uartPutch(c);
	return 0;
}

static int syscall_uartGetch()
{
	return uartGetch();
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

	/*pmalloc list*/
	child->mHead = parent->mHead;
	child->mTail = parent->mTail;

	/* set return value of fork in child to 0 */
	child->context[R0] = 0;

	/* child is ready to run */
	child->state = READY;

	/* return pid of child to the parent. */
	return child->pid;
}

static int syscall_exit(int arg0)
{
	int pid;

	(void) arg0;
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

int syscall_wait(int arg0)
{
	ProcessT *proc = procGet(arg0);
	if (proc && proc->state != UNUSED) {
		_currentProcess->waitPid = arg0;
		_currentProcess->state = SLEEPING;
		schedule();
	}
	return 0;
}


static int syscall_yield() {
	schedule();
	return 0;
}

static int syscall_pmalloc(int arg0) {
	char* p = pmalloc(_currentProcess, (uint32_t)arg0);
	return (int)p;
}

static int syscall_pfree(int arg0) {
	char* p = (char*)arg0;
	pfree(_currentProcess, p);
	return 0;
}

static int syscall_kservReg(int arg0) {
	if(kservReg(arg0))
		return 0;
	return -1;
}

static int syscall_kservRequest(int arg0, int arg1, int arg2) {
	return kservRequest(arg0, (char*)arg1, arg2);
}

static int syscall_kservGetResponse(int arg0, int arg1, int arg2) {
	return kservGetResponse(arg0, (char*)arg1, arg2);
}

static int syscall_kservGetRequest(int arg0, int arg1, int arg2, int arg3) {
	return kservGetRequest(arg0, (int*)arg1, (char*)arg2, arg3);
}

static int syscall_kservResponse(int arg0, int arg1, int arg2) {
	return kservResponse(arg0, (char*)arg1, arg2);
}

static int (*const _syscallHandler[])() = {
	[SYSCALL_UART_PUTCH] = syscall_uartPutch,
	[SYSCALL_UART_GETCH] = syscall_uartGetch,
	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_YIELD] = syscall_yield,
	[SYSCALL_EXIT] = syscall_exit,
	[SYSCALL_PMALLOC] = syscall_pmalloc,
	[SYSCALL_PFREE] = syscall_pfree,

	[SYSCALL_KSERV_REG] = syscall_kservReg,
	[SYSCALL_KSERV_REQUEST] = syscall_kservRequest,
	[SYSCALL_KSERV_GET_REQUEST] = syscall_kservGetRequest,
	[SYSCALL_KSERV_RESP] = syscall_kservResponse,
	[SYSCALL_KSERV_GET_RESP] = syscall_kservGetResponse,
};

/* kernel side of system calls. */
int handleSyscall(int code, int arg0, int arg1, int arg2)
{
	return _syscallHandler[code](arg0, arg1, arg2);
}

