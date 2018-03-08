#include <syscall.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <dev/uart.h>
#include <proc.h>
#include <kernel.h>
#include <sramdisk.h>
#include <string.h>
#include <pmalloc.h>
#include <kmessage.h>
#include <kfile.h>

void schedule();

static int syscall_uartPutch(int c)
{
	uartPutch(c);
	return 0;
}

static int syscall_uartGetch()
{
	int r = uartGetch();
	return r;
}

static int syscall_exec(int arg0) {
	int size = 0;
	const char*p = ramdiskRead(&_initRamDisk, (const char*)arg0, &size);
	if(p == NULL)
		return -1;
		
	if(!procLoad(_currentProcess, p))
		return -1;
	procStart(_currentProcess);
	return 0;
}

static int syscall_execElf(int arg0) {
	const char*p = (const char*)arg0;
	if(p == NULL)
		return -1;
		
	if(!procLoad(_currentProcess, p))
		return -1;
	procStart(_currentProcess);
	return 0;
}

static int syscall_fork(void)
{
	return kfork();
}

static int syscall_getpid(void)
{
	return _currentProcess->pid;
}

static int syscall_exit(int arg0)
{
	int i;

	(void) arg0;
	if (_currentProcess == NULL)
		return -1;

	__setTranslationTableBase((uint32_t) V2P(_kernelVM));
	__asm__ volatile("ldr sp, = _initStack");
	__asm__ volatile("add sp, #4096");

	for (i = 0; i < PROCESS_COUNT_MAX; i++) {
		ProcessT *proc = &_processTable[i];

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
	if (proc) {
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
	char* p = pmalloc(&_currentProcess->mallocMan, (uint32_t)arg0);
	return (int)p;
}

static int syscall_pfree(int arg0) {
	char* p = (char*)arg0;
	pfree(&_currentProcess->mallocMan, p);
	return 0;
}

static int syscall_sendMessage(int arg0, int arg1, int arg2) {
	PackageT* p = (PackageT*)arg2;
	return ksend(arg0, arg1, p);
}

static int syscall_readMessage(int arg0) {
	PackageT* pkg = krecv(arg0);
	if(pkg == NULL)
		return 0;
	
	return (int)pkg;
}

static int syscall_readInitRD(int arg0, int arg1) {
	const char* fname = (const char*)arg0;
	int size = 0;
	const char*p = ramdiskRead(&_initRamDisk, fname, &size);
	if(p == NULL || size == 0)
		return 0;

	char* ret = pmalloc(&_currentProcess->mallocMan, size);
	if(ret == NULL)
		return 0;
	memcpy(ret, p, size);
	*((int*)arg1) = size;
	return (int)ret;
}

static int syscall_filesInitRD() {
	RamFileT* f = _initRamDisk.head;
	char* ret = pmalloc(&_currentProcess->mallocMan, 1024+1);
	if(ret == NULL)
		return 0;

	int i=0;
	while(f != NULL) {
		int j=0;
		char c = f->name[j];
		while(c != 0) {
			ret[i] = c;
			i++;
			j++;

			if(i >= 1024) {
				ret[i] = 0;
				return (int)ret;
			}
			c = f->name[j];
		}
		f = f->next;
		ret[i++] = '\n';
		j = 0;
	}
	ret[i] = 0;
	return (int)ret;
}

static int syscall_kdb(int arg0) {
	return arg0;
}

static int syscall_pfOpen(int arg0, int arg1) {
	KFileT* kf = kfOpen((uint32_t)arg0);
	if(kf == NULL)
		return -1;

	kfRef(kf, arg1);

	ProcessT* proc = _currentProcess;
	int i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->files[i].kf == NULL) {
			proc->files[i].kf = kf;
			proc->files[i].flags = arg1;
			proc->files[i].seek = 0;
			return i;
		}
	}

	kfUnref(kf, arg1);
	return -1;
}

static int syscall_pfClose(int arg0) {
	if(arg0 < 0 || arg0 >= FILE_MAX)
		return -1;

	ProcessT* proc = _currentProcess;
	kfUnref(proc->files[arg0].kf, proc->files[arg0].flags);

	proc->files[arg0].kf = NULL;
	proc->files[arg0].flags = 0;
	proc->files[arg0].seek = 0;
	return  0;
}

static int syscall_pfNode(int arg0) {
	ProcessT* proc = _currentProcess;
	if(arg0 < 0 || arg0 >= FILE_MAX)
		return 0;
	KFileT* kf = proc->files[arg0].kf;
	if(kf == NULL)
		return 0;
	return kf->fsNodeAddr;
}

static int syscall_pfSeek(int arg0, int arg1) {
	ProcessT* proc = _currentProcess;
	if(arg0 < 0 || arg0 >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[arg0].kf;
	if(kf == NULL || kf->fsNodeAddr == 0)
		return -1;
	proc->files[arg0].seek = arg1;
	return arg1;
}

static int syscall_pfGetSeek(int arg0) {
	ProcessT* proc = _currentProcess;
	if(arg0 < 0 || arg0 >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[arg0].kf;
	if(kf == NULL || kf->fsNodeAddr == 0)
		return -1;
	return proc->files[arg0].seek;
}

static int (*const _syscallHandler[])() = {
	[SYSCALL_KDB] = syscall_kdb,
	[SYSCALL_UART_PUTCH] = syscall_uartPutch,
	[SYSCALL_UART_GETCH] = syscall_uartGetch,

	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_EXEC] = syscall_exec,
	[SYSCALL_EXEC_ELF] = syscall_execElf,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_YIELD] = syscall_yield,
	[SYSCALL_EXIT] = syscall_exit,

	[SYSCALL_PMALLOC] = syscall_pmalloc,
	[SYSCALL_PFREE] = syscall_pfree,

	[SYSCALL_SEND_MSG] = syscall_sendMessage,
	[SYSCALL_READ_MSG] = syscall_readMessage,

	[SYSCALL_INITRD_READ] = syscall_readInitRD,
	[SYSCALL_INITRD_FILES] = syscall_filesInitRD,

	[SYSCALL_PFILE_NODE] = syscall_pfNode,
	[SYSCALL_PFILE_GET_SEEK] = syscall_pfGetSeek,
	[SYSCALL_PFILE_SEEK] = syscall_pfSeek,
	[SYSCALL_PFILE_OPEN] = syscall_pfOpen,
	[SYSCALL_PFILE_CLOSE] = syscall_pfClose,
};

/* kernel side of system calls. */
int handleSyscall(int code, int arg0, int arg1, int arg2)
{
	return _syscallHandler[code](arg0, arg1, arg2);
}

