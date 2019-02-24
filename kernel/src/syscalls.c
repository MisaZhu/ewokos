#include <syscallcode.h>
#include <system.h>
#include <syscalls.h>
#include <types.h>
#include <dev/uart.h>
#include <proc.h>
#include <kernel.h>
#include <kstring.h>
#include <mm/trunkmalloc.h>
#include <kipc.h>
#include <vfs/vfs.h>
#include <types.h>
#include <kserv.h>
#include <fsinfo.h>
#include <scheduler.h>

static int32_t syscall_uartPutch(int32_t c) {
	uartPutch(c);
	return 0;
}

static int32_t syscall_uartGetch() {
	int32_t r = uartGetch();
	return r;
}

static int32_t syscall_execElf(int32_t arg0, int32_t arg1) {
	const char*cmd = (const char*)arg0;
	const char*p = (const char*)arg1;

	if(p == NULL || cmd == NULL)
		return -1;
		
	strncpy(_currentProcess->cmd, cmd, CMD_MAX);

	if(!procLoad(_currentProcess, p))
		return -1;

	procStart(_currentProcess);
	return 0;
}

static int32_t syscall_fork(void) {
	return kfork();
}

static int32_t syscall_getpid(void) {
	return _currentProcess->pid;
}

static int32_t syscall_exit(int32_t arg0) {
	(void)arg0;
	procExit();
	return 0;
}

static int32_t syscall_wait(int32_t arg0) {
	ProcessT *proc = procGet(arg0);
	if (proc->state != UNUSED) {
		_currentProcess->waitPid = arg0;
		_currentProcess->state = SLEEPING;
		schedule();
	}
	return 0;
}


static int32_t syscall_yield() {
	schedule();
	return 0;
}

static int32_t syscall_pmalloc(int32_t arg0) {
	char* p = (char*)pmalloc((uint32_t)arg0);
	return (int)p;
}

static int32_t syscall_pfree(int32_t arg0) {
	char* p = (char*)arg0;
	trunkFree(&_currentProcess->mallocMan, p);
	return 0;
}

static int32_t syscall_ipcOpen(int32_t arg0) {
	return ipcOpen(arg0);
}

static int32_t syscall_ipcReady() {
	return ipcReady();
}

static int32_t syscall_ipcClose(int32_t arg0) {
	ipcClose(arg0);
	return 0;
}

static int32_t syscall_ipcWrite(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipcWrite(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipcRead(int32_t arg0, int32_t arg1, int32_t arg2) {
	return ipcRead(arg0, (void*)arg1, arg2);
}

static int32_t syscall_ipc_getpid_read(int32_t arg0) {
	return ipcGetPidR(arg0);
}

static int32_t syscall_ipc_getpid_write(int32_t arg0) {
	return ipcGetPidW(arg0);
}

static int32_t syscall_readInitRD(int32_t arg0, int32_t arg1, int32_t arg2) {
	const char* fname = (const char*)arg0;
	int32_t fsize = 0;
	int32_t rdSize = *(int*)arg2;
	*(int*)arg2 = 0;

	const char*p = ramdiskRead(&_initRamDisk, fname, &fsize);
	if(p == NULL || fsize == 0)
		return 0;

	int32_t restSize = fsize - arg1; /*arg1: seek*/
	if(restSize <= 0) {
		return 0;
	}

	if(rdSize <= 0 || rdSize > restSize)
		rdSize = restSize;


	char* ret = (char*)pmalloc(rdSize);
	if(ret == NULL)
		return 0;
	memcpy(ret, p+arg1, rdSize);
	*((int*)arg2) = rdSize;
	return (int)ret;
}

static int32_t syscall_filesInitRD() {
	RamFileT* f = _initRamDisk.head;
	char* ret = (char*)pmalloc(1024+1);
	if(ret == NULL)
		return 0;

	int32_t i=0;
	while(f != NULL) {
		int32_t j=0;
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

static int32_t syscall_infoInitRD(int32_t arg0, int32_t arg1) {
	const char* fname = (const char*)arg0;
	FSInfoT* info = (FSInfoT*)arg1;
	int32_t fsize = 0;

	const char*p = ramdiskRead(&_initRamDisk, fname, &fsize);
	if(p == NULL)
		return -1;
	
	info->type = FS_TYPE_FILE;
	info->size = fsize;

	return 0;
}

static int32_t syscall_kdb(int32_t arg0) {
	return arg0;
}

static int32_t syscall_pfOpen(int32_t arg0, int32_t arg1, int32_t arg2) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	KFileT* kf = kfOpen((uint32_t)arg1);
	if(kf == NULL)
		return -1;

	kfRef(kf, arg2);

	int32_t i;
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

static int32_t syscall_pfClose(int32_t arg0, int32_t arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int32_t fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	kfUnref(proc->files[fd].kf, proc->files[fd].flags);

	proc->files[fd].kf = NULL;
	proc->files[fd].flags = 0;
	proc->files[fd].seek = 0;
	return  0;
}

static int32_t syscall_pfNode(int32_t arg0, int32_t arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;
	int32_t fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return 0;
	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL)
		return 0;
	return kf->nodeAddr;
}

static int32_t syscall_pfSeek(int32_t arg0, int32_t arg1, int32_t arg2) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int32_t fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	proc->files[fd].seek = arg2;
	return arg2;
}

static int32_t syscall_pfGetSeek(int32_t arg0, int32_t arg1) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;

	int32_t fd = arg1;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	KFileT* kf = proc->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	return proc->files[fd].seek;
}

static int32_t syscall_kservReg(int32_t arg0) {
	return kservReg((const char*)arg0);
}

static int32_t syscall_kservGet(int32_t arg0) {
	return kservGet((const char*)arg0);
}

static int32_t getProcs(bool owner) {
	int32_t res = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX; i++) {
		if(_processTable[i].state != UNUSED && 
				(_currentProcess->owner == 0 || 
				owner != true || 
				_processTable[i].owner == _currentProcess->owner)) {
			res++;
		}
	}
	return res;
}

static int32_t syscall_getProcs(int32_t arg0, int32_t arg1) {
	bool owner = (bool)arg1;
	int32_t num = getProcs(owner);
	if(num == 0)
		return 0;

	/*need to be freed later used!*/
	ProcInfoT* procs = (ProcInfoT*)pmalloc(sizeof(ProcInfoT)*num);
	if(procs == NULL)
		return 0;

	int32_t j = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX && j<num; i++) {
		if(_processTable[i].state != UNUSED && 
				(_currentProcess->owner == 0 ||
				owner != true ||
				 _processTable[i].owner == _currentProcess->owner)) {
			procs[j].pid = _processTable[i].pid;	
			procs[j].fatherPid = _processTable[i].fatherPid;	
			procs[j].owner = _processTable[i].owner;	
			procs[j].heapSize = _processTable[i].heapSize;	
			strcpy(procs[j].cmd, _processTable[i].cmd);
			j++;
		}
	}

	*(int*)arg0 = j;
	return (int)procs;
}

static int32_t syscall_getCWD(int32_t arg0, int32_t arg1) {
	char* pwd = (char*)arg0;
	strncpy(pwd, _currentProcess->pwd,
		arg1 < NAME_MAX ? arg1: NAME_MAX);
	return (int)pwd;
}

static int32_t syscall_setCWD(int32_t arg0) {
	const char* pwd = (const char*)arg0;
	strncpy(_currentProcess->pwd, pwd, NAME_MAX);
	return 0;
}

static int32_t syscall_getCmd(int32_t arg0, int32_t arg1) {
	char* cmd = (char*)arg0;
	strncpy(cmd, _currentProcess->cmd, arg1);
	return arg0;
}

static int32_t syscall_setUID(int32_t arg0, int32_t arg1) {
	if(arg1 < 0 || _currentProcess->owner >= 0) {/*current process not kernel proc*/
		return -1;
	}
	
	ProcessT* proc = procGet(arg0);
	if(proc == NULL) {
		return -1;
	}

	proc->owner = arg1;
	return 0;
}

static int32_t syscall_getUID(int32_t arg0) {
	ProcessT* proc = procGet(arg0);
	if(proc == NULL)
		return -1;
	return proc->owner;
}

static int32_t syscall_vfsMountFile(int32_t arg0, int32_t arg1, int32_t arg2) {
	return vfsMountFile((const char*)arg0, (const char*)arg1, arg2);
}

static int32_t syscall_vfsMountDir(int32_t arg0, int32_t arg1, int32_t arg2) {
	return vfsMountDir((const char*)arg0, (const char*)arg1, arg2);
}

static int32_t syscall_vfsOpen(int32_t arg0, int32_t arg1) {
	return vfsOpen((const char*)arg0, arg1);
}

static int32_t syscall_vfsClose(int32_t arg0) {
	return vfsClose(arg0);
}

static int32_t syscall_vfsRead(int32_t arg0, int32_t arg1, int32_t arg2) {
	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	int32_t seek = 0;
	KFileT* kf = _currentProcess->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	seek = _currentProcess->files[fd].seek;
	return vfsRead(fd, (void*)arg1, arg2, seek);
}

static int32_t syscall_vfsWrite(int32_t arg0, int32_t arg1, int32_t arg2) {
	int32_t fd = arg0;
	if(fd < 0 || fd >= FILE_MAX)
		return -1;

	int32_t seek = 0;
	KFileT* kf = _currentProcess->files[fd].kf;
	if(kf == NULL || kf->nodeAddr == 0)
		return -1;
	seek = _currentProcess->files[fd].seek;
	return vfsWrite(fd, (void*)arg1, arg2, seek);
}

static int32_t syscall_vfsFInfo(int32_t arg0, int32_t arg1) {
	return vfsFInfo((const char*)arg0, (FSInfoT*)arg1);
}

static int32_t syscall_vfsInfo(int32_t arg0, int32_t arg1) {
	return vfsInfo(arg0, (FSInfoT*)arg1);
}

static int32_t syscall_vfsIoctl(int32_t arg0, int32_t arg1, int32_t arg2) {
	return vfsIoctl(arg0, arg1, arg2);
}

static int32_t syscall_vfsAdd(int32_t arg0, int32_t arg1) {
	return vfsAdd(arg0, (const char*)arg1);
}

static int32_t syscall_vfsDel(int32_t arg0, int32_t arg1) {
	return vfsDel(arg0, (const char*)arg1);
}

static int32_t syscall_vfsFKids(int32_t arg0, int32_t arg1) {
	FSInfoT* ret = vfsFKids((const  char*)arg0, (int32_t*)arg1);
	return (int32_t)ret;
}

static int32_t syscall_vfsKids(int32_t arg0, int32_t arg1) {
	FSInfoT* ret = vfsKids(arg0, (int32_t*)arg1);
	return (int32_t)ret;
}

static int32_t (*const _syscallHandler[])() = {
	[SYSCALL_KDB] = syscall_kdb,
	[SYSCALL_UART_PUTCH] = syscall_uartPutch,
	[SYSCALL_UART_GETCH] = syscall_uartGetch,

	[SYSCALL_FORK] = syscall_fork,
	[SYSCALL_GETPID] = syscall_getpid,
	[SYSCALL_EXEC_ELF] = syscall_execElf,
	[SYSCALL_WAIT] = syscall_wait,
	[SYSCALL_YIELD] = syscall_yield,
	[SYSCALL_EXIT] = syscall_exit,

	[SYSCALL_PMALLOC] = syscall_pmalloc,
	[SYSCALL_PFREE] = syscall_pfree,

	[SYSCALL_GET_CMD] = syscall_getCmd,

	[SYSCALL_KOPEN] = syscall_ipcOpen,
	[SYSCALL_KCLOSE] = syscall_ipcClose,
	[SYSCALL_KWRITE] = syscall_ipcWrite,
	[SYSCALL_KREADY] = syscall_ipcReady,
	[SYSCALL_KREAD] = syscall_ipcRead,
	[SYSCALL_KGETPID_R] = syscall_ipc_getpid_read,
	[SYSCALL_KGETPID_W] = syscall_ipc_getpid_write,

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

	[SYSCALL_GET_PROCS] = syscall_getProcs,
	[SYSCALL_GET_CWD] = syscall_getCWD,
	[SYSCALL_SET_CWD] = syscall_setCWD,

	[SYSCALL_SET_UID] = syscall_setUID,
	[SYSCALL_GET_UID] = syscall_getUID,

	[SYSCALL_VFS_MOUNT_DIR] = syscall_vfsMountDir,
	[SYSCALL_VFS_MOUNT_FILE] = syscall_vfsMountFile,

	[SYSCALL_VFS_OPEN] = syscall_vfsOpen,
	[SYSCALL_VFS_CLOSE] = syscall_vfsClose,
	[SYSCALL_VFS_READ] = syscall_vfsRead,
	[SYSCALL_VFS_WRITE] = syscall_vfsWrite,
	[SYSCALL_VFS_FINFO] = syscall_vfsFInfo,
	[SYSCALL_VFS_INFO] = syscall_vfsInfo,
	[SYSCALL_VFS_IOCTL] = syscall_vfsIoctl,
	[SYSCALL_VFS_ADD] = syscall_vfsAdd,
	[SYSCALL_VFS_DEL] = syscall_vfsDel,
	[SYSCALL_VFS_FKIDS] = syscall_vfsFKids,
	[SYSCALL_VFS_KIDS] = syscall_vfsKids,
};

/* kernel side of system calls. */
int32_t handleSyscall(int32_t code, int32_t arg0, int32_t arg1, int32_t arg2) {
	return _syscallHandler[code](arg0, arg1, arg2);
}

