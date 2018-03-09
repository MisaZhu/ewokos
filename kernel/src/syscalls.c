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
#include <types.h>
#include <kserv.h>
#include <kserv/fs.h>

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

static int syscall_readInitRD(int arg0, int arg1, int arg2) {
	const char* fname = (const char*)arg0;
	int fsize = 0;
	int rdSize = *(int*)arg2;
	*(int*)arg2 = 0;

	const char*p = ramdiskRead(&_initRamDisk, fname, &fsize);
	if(p == NULL || fsize == 0)
		return 0;

	int restSize = fsize - arg1; /*arg1: seek*/
	if(restSize <= 0) {
		return 0;
	}

	if(rdSize <= 0 || rdSize > restSize)
		rdSize = restSize;


	char* ret = pmalloc(&_currentProcess->mallocMan, rdSize);
	if(ret == NULL)
		return 0;
	memcpy(ret, p+arg1, rdSize);
	*((int*)arg2) = rdSize;
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

static int syscall_infoInitRD(int arg0, int arg1) {
	const char* fname = (const char*)arg0;
	FSInfoT* info = (FSInfoT*)arg1;
	int fsize = 0;

	const char*p = ramdiskRead(&_initRamDisk, fname, &fsize);
	if(p == NULL)
		return -1;
	
	info->type = FS_TYPE_FILE;
	info->size = fsize;

	return 0;
}

static int syscall_kdb(int arg0) {
	return arg0;
}

static int syscall_pfOpen(int arg0, int arg1, int arg2) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	KFileT* kf = kfOpen((uint32_t)arg1);
	if(kf == NULL)
		return -1;

	kfRef(kf, arg2);

	int i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->files[i].kf == NULL) {
			proc->files[i].kf = kf;
			proc->files[i].flags = arg2;
			proc->files[i].seek = 0;
			return i;
		}
	}

	kfUnref(kf, arg2);
	return -1;
}

static int syscall_pfClose(int arg0, int arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kfUnref(proc->files[fd].kf, proc->files[fd].flags);

	proc->files[fd].kf = NULL;
	proc->files[fd].flags = 0;
	proc->files[fd].seek = 0;
	return  0;
}

static int syscall_pfNode(int arg0, int arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;
	int fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return 0;
	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL)
		return 0;
	return kf->fsNodeAddr;
}

static int syscall_pfSeek(int arg0, int arg1, int arg2) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->fsNodeAddr == 0)
		return -1;
	proc->files[fd].seek = arg2;
	return arg2;
}

static int syscall_pfGetSeek(int arg0, int arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->fsNodeAddr == 0)
		return -1;
	return proc->files[fd].seek;
}

static int syscall_kservReg(int arg0) {
	return kservReg((const char*)arg0);
}

static int syscall_kservGet(int arg0) {
	return kservGet((const char*)arg0);
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
	[SYSCALL_INITRD_INFO] = syscall_infoInitRD,

	[SYSCALL_PFILE_NODE] = syscall_pfNode,
	[SYSCALL_PFILE_GET_SEEK] = syscall_pfGetSeek,
	[SYSCALL_PFILE_SEEK] = syscall_pfSeek,
	[SYSCALL_PFILE_OPEN] = syscall_pfOpen,
	[SYSCALL_PFILE_CLOSE] = syscall_pfClose,

	[SYSCALL_KSERV_REG] = syscall_kservReg,
	[SYSCALL_KSERV_GET] = syscall_kservGet,
};

/* kernel side of system calls. */
int handleSyscall(int code, int arg0, int arg1, int arg2)
{
	return _syscallHandler[code](arg0, arg1, arg2);
}

